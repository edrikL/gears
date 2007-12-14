// Copyright 2006, Google Inc.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// JavaScript threading in Firefox.
//
// In hindsight, the approach used here makes sense.  But reading the Firefox
// code suggests many alternate (and stray) paths.  Here Be Dragons.
//
// Quick notes:
// * jsapi is used to instantiate the JavaScript engine (SpiderMonkey) for
//   each worker thread.
// * Cross-thread communication happens using thread event queues, which are
//   analogous to the per-thread message queues on Windows.
//
// Open questions:
// * Should we be using something higher-level than jsapi?  It's unclear
//   whether nsIScriptContext is powerful enough, thread-safe, and DOM-free.
//   (Note: Darin Fisher says XPConnect is not thread-safe.)
//
// Useful references:
// * /mozilla/js/src/jsapi.h -- the JavaScript engine's interface
// * /mozilla/js/src/readme.html -- overview of using the JavaScript engine
// * /mozilla/xpcom/threads/plevent.h -- the Firefox event model
//     Despite warnings in plevent.h about entering the queue monitor, it seems
//     plevent.c has since been updated to handle the monitor automatically.
//
// PoolThreadsManager is ref-counted. The following objects each AddRef() when
// they are created and ReleaseRef() when they are destroyed. Thus PTM dies when
// the last of these objects dies.
// - GearsWorkerPool instances. The one that created the PTM, along with one for
//   each thread the PTM creates.
// - Created threads. Each created thread AddRef()'s on entry and Release()'s on
//   exit.
// - ThreadEvents. ThreadEvents bound for the main UI thread sometimes outlive
//   all other objects referencing the PTM and can cause code on the main thread
//   to execute after the PTM has otherwise shutdown. Therefore, the PTM
//   lifetime must extend until all ThreadEvents have been processed.

#include <assert.h> // TODO(cprince): use DCHECK() when have google3 logging
#include <queue>
#ifdef WIN32
#include <windows.h> // must manually #include before nsIEventQueueService.h
#endif
#include <nsCOMPtr.h>
#include <nspr.h> // for PR_*
#include <nsServiceManagerUtils.h> // for NS_IMPL_* and NS_INTERFACE_*
#include "gears/third_party/gecko_internal/jsapi.h"
struct JSContext; // must declare this before including nsIJSContextStack.h
#include "gears/third_party/gecko_internal/nsIJSContextStack.h"
#include "gears/third_party/gecko_internal/nsIJSRuntimeService.h"
#include "gears/third_party/gecko_internal/nsIDOMClassInfo.h" // for *_DOM_CLASSINFO
#include "gears/third_party/gecko_internal/nsIEventQueueService.h" // for event loop
#include "gears/third_party/gecko_internal/nsIScriptContext.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "gears/workerpool/firefox/workerpool.h"

#include "ff/genfiles/database.h"
#include "ff/genfiles/localserver.h"
#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/exception_handler_win32.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/js_runner_utils.h"
#include "gears/base/common/permissions_db.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/factory/firefox/factory.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/http_request.h"
#include "gears/workerpool/common/workerpool_utils.h"


// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_ADDREF(GearsWorkerPool)
NS_IMPL_RELEASE(GearsWorkerPool)
NS_INTERFACE_MAP_BEGIN(GearsWorkerPool)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsWorkerPoolInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsWorkerPoolInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsWorkerPool)
NS_INTERFACE_MAP_END


// Object identifiers
const char *kGearsWorkerPoolClassName = "GearsWorkerPool";
const nsCID kGearsWorkerPoolClassId = {0x0a15a787, 0x9aef, 0x4a5e, {0x92, 0x7d,
                                       0xdb, 0x55, 0x5b, 0x48, 0x49, 0x09}};
                                      // {0A15A787-9AEF-4a5e-927D-DB555B484909}


//
// Message container.
//

struct Message {
  std::string16 text;
  int sender;
  SecurityOrigin origin;

  Message(const std::string16 &t, int s, const SecurityOrigin &o)
      : text(t), sender(s), origin(o) { }
  Message() : sender(-1) { }
};


//
// JavaScriptWorkerInfo -- contains the state of each JavaScript worker.
//

struct JavaScriptWorkerInfo {
  // Our code assumes some items begin cleared. Zero all members w/o ctors.
  JavaScriptWorkerInfo()
      : threads_manager(NULL), js_runner(NULL),
        is_invoking_error_handler(false),
        thread_init_signalled(false), thread_init_ok(false),
        script_signalled(false), script_ok(false), http_request(NULL),
        is_factory_suspended(false), thread_pointer(NULL) {}

  //
  // These fields are used for all workers in pool (parent + children).
  //
  PoolThreadsManager *threads_manager;
  JsRunnerInterface *js_runner;
  scoped_ptr<JsRootedCallback> onmessage_handler;
  scoped_ptr<JsRootedCallback> onerror_handler;

  nsCOMPtr<nsIEventQueue> thread_event_queue;
  std::queue<Message> message_queue;

  bool is_invoking_error_handler;  // prevents recursive onerror

  //
  // These fields are used only for created workers (children).
  //
  Mutex thread_init_mutex;  // Protects: thread_init_signalled
  bool thread_init_signalled;
  bool thread_init_ok;  // Owner: child before signal, parent after signal

  Mutex script_mutex;  // Protects: script_signalled
  bool script_signalled;  
  bool script_ok;  // Owner: parent before signal, immutable after
  std::string16 script_text;  // Owner: parent before signal, immutable after
  SecurityOrigin script_origin;  // Owner: parent before signal, immutable after

  ScopedHttpRequestPtr http_request;  // For createWorkerFromUrl()
  scoped_ptr<HttpRequest::ReadyStateListener> http_request_listener;
  nsCOMPtr<GearsFactory> factory_ref;
  bool is_factory_suspended;
  PRThread *thread_pointer;
};


//
// GearsWorkerPool -- handles the browser glue.
//

GearsWorkerPool::GearsWorkerPool()
    : threads_manager_(NULL),
      owns_threads_manager_(false) {
}

GearsWorkerPool::~GearsWorkerPool() {
  if (owns_threads_manager_) {
    assert(threads_manager_);
    threads_manager_->ShutDown();
  }

  if (threads_manager_) {
    threads_manager_->UninitWorkerThread();
    threads_manager_->ReleaseWorkerRef();
  }
}

void GearsWorkerPool::SetThreadsManager(PoolThreadsManager *manager) {
  assert(!threads_manager_);
  threads_manager_ = manager;
  threads_manager_->AddWorkerRef();

  // Leave owns_threads_manager_ set to false.
  assert(!owns_threads_manager_);
}

NS_IMETHODIMP GearsWorkerPool::SetOnmessage(nsIVariant *in_value) {
  Initialize();

  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"Value is required."));
  }

  // Once we aquire the callback, we're responsible for its lifetime until it's
  // passed into SetCurrentThreadMessageHandler.
  JsRootedCallback *onmessage_handler;
  if (!js_params.GetAsNewRootedCallback(0, &onmessage_handler)) {
    RETURN_EXCEPTION(STRING16(L"Invalid value for onmesssage property."));
  }

  if (!threads_manager_->SetCurrentThreadMessageHandler(onmessage_handler)) {
    RETURN_EXCEPTION(STRING16(L"Error setting onmessage handler."));
  }

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::GetOnmessage(nsIVariant **out_value) {
  *out_value = NULL; // TODO(cprince): implement this (requires some changes)
                     // See code in nsXMLHttpRequest for NS_IF_ADDREF usage.
                     // May be able to return a raw JsToken for main and worker?
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::SetOnerror(nsIVariant *in_value) {
  Initialize();

  JsRootedCallback *onerror_handler;
  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"Value is required."));
  }

  // Once we aquire the callback, we're responsible for its lifetime until it's
  // passed into SetCurrentThreadErrorHandler.
  if (!js_params.GetAsNewRootedCallback(0, &onerror_handler)) {
    RETURN_EXCEPTION(STRING16(L"Invalid value for onerror property."));
  }

  if (!threads_manager_->SetCurrentThreadErrorHandler(onerror_handler)) {
    // Currently, the only reason this can fail is because of this one
    // particular error.
    // TODO(aa): We need a system throughout Gears for being able to handle
    // exceptions from deep inside the stack better.
    RETURN_EXCEPTION(STRING16(L"The onerror property cannot be set on a "
                              L"parent worker"));
  }

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::GetOnerror(nsIVariant **out_value) {
  // TODO(cprince): Implement this. See comment in GetOnmessage.
  *out_value = NULL; 
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::CreateWorker(//const nsAString &full_script
                                            PRInt32 *retval) {
  Initialize();
  
  JsParamFetcher js_params(this);
  std::string16 full_script;

  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"The full_script parameter is required."));
  } else if (!js_params.GetAsString(0, &full_script)) {
    RETURN_EXCEPTION(STRING16(L"The full_script parameter must be a string."));
  }

  int worker_id_temp;  // protects against modifying output param on failure
  bool succeeded = threads_manager_->CreateThread(full_script.c_str(),
                                                  true,  // is_param_script
                                                  &worker_id_temp);
  if (!succeeded) {
    RETURN_EXCEPTION(STRING16(L"Internal error."));
  }

  *retval = worker_id_temp;
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::CreateWorkerFromUrl(//const nsAString &url
                                                   PRInt32 *retval) {
  Initialize();

  // Make sure URLs are only fetched from the main thread.
  // TODO(michaeln): Remove this limitation of Firefox HttpRequest someday.
  if (EnvIsWorker()) {
    RETURN_EXCEPTION(STRING16(L"createWorkerFromUrl() cannot be called from a"
                              L" worker."));
  }
  
  JsParamFetcher js_params(this);
  std::string16 url;

  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"The url parameter is required."));
  } else if (!js_params.GetAsString(0, &url)) {
    RETURN_EXCEPTION(STRING16(L"The url parameter must be a string."));
  }

  std::string16 absolute_url;
  ResolveAndNormalize(EnvPageLocationUrl().c_str(), url.c_str(), &absolute_url);

  SecurityOrigin script_origin;
  if (!script_origin.InitFromUrl(absolute_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Internal error."));
  }

  // We do not currently support file:// URLs. See bug:
  // http://code.google.com/p/google-gears/issues/detail?id=239.
  if (!HttpRequest::IsSchemeSupported(script_origin.scheme().c_str())) {
    std::string16 message(STRING16(L"URL scheme '"));
    message += script_origin.scheme();
    message += STRING16(L"' is not supported.");
    RETURN_EXCEPTION(message.c_str());
  }
  
  // Enable the worker's origin for gears access if it isn't explicitly
  // disabled.
  // NOTE: It is OK to do this here, even though there is a race with starting
  // the background thread. Even if permission is revoked before after this
  // happens that is no different than what happens if permission is revoked
  // after the thread starts.
  if (!script_origin.IsSameOrigin(EnvPageSecurityOrigin())) {
    PermissionsDB *db = PermissionsDB::GetDB();
    if (!db) {
      RETURN_EXCEPTION(STRING16(L"Internal error."));
    }

    if (!db->EnableGearsForWorker(script_origin)) {
      std::string16 message(STRING16(L"Gears access is denied for url: "));
      message += absolute_url;
      message += STRING16(L".");
      RETURN_EXCEPTION(message.c_str());
    }
  }

  int worker_id_temp;  // protects against modifying output param on failure
  bool succeeded = threads_manager_->CreateThread(absolute_url.c_str(),
                                                  false,  // is_param_script
                                                  &worker_id_temp);
  if (!succeeded) {
    RETURN_EXCEPTION(STRING16(L"Internal error."));
  }

  *retval = worker_id_temp;
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::AllowCrossOrigin() {
  Initialize();

  if (owns_threads_manager_) {
    RETURN_EXCEPTION(STRING16(L"Method is only used by child workers."));
  }

  threads_manager_->AllowCrossOrigin();
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::SendMessage(//const nsAString &message,
                                           //PRInt32 dest_worker_id
                                          ) {
  Initialize();

  JsParamFetcher js_params(this);
  std::string16 text;
  int dest_worker_id;

  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"The message parameter is required."));
  } else if (!js_params.GetAsString(0, &text)) {
    RETURN_EXCEPTION(STRING16(L"The message parameter must be a string."));
  }

  if (js_params.GetCount(false) < 2) {
    RETURN_EXCEPTION(STRING16(L"The destId parameter is required."));
  } else if (!js_params.GetAsInt(1, &dest_worker_id)) {
    RETURN_EXCEPTION(STRING16(L"The destId parameter must be an int."));
  }

  bool succeeded = threads_manager_->PutPoolMessage(text.c_str(),
                                                    dest_worker_id,
                                                    EnvPageSecurityOrigin());
  if (!succeeded) {
    std::string16 error(STRING16(L"Worker "));
    error += IntegerToString16(dest_worker_id);
    error += STRING16(L" does not exist.");

    RETURN_EXCEPTION(error.c_str());
  }
  RETURN_NORMAL();
}

#ifdef DEBUG
NS_IMETHODIMP GearsWorkerPool::ForceGC() {
  threads_manager_->ForceGCCurrentThread();
  RETURN_NORMAL();
}
#endif // DEBUG

void GearsWorkerPool::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);

  if (owns_threads_manager_ && threads_manager_) {
    threads_manager_->ShutDown();
  }
}

void GearsWorkerPool::Initialize() {
  if (!threads_manager_) {
    assert(EnvPageSecurityOrigin().full_url() == EnvPageLocationUrl());
    SetThreadsManager(new PoolThreadsManager(EnvPageSecurityOrigin(),
                                             GetJsRunner()));
    owns_threads_manager_ = true;
  }

  // Monitor 'onunload' to shutdown threads when the page goes away.
  //
  // A thread that keeps running after the page changes can cause odd problems,
  // if it continues to send messages. (This can happen if it busy-loops.)  On
  // Firefox, such a thread triggered the Print dialog after the page changed!
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD,
                                             this));
  }
}


//
// ThreadsEvent -- used for Firefox cross-thread communication.
//

enum ThreadsEventType {
  EVENT_TYPE_MESSAGE = 0,
  EVENT_TYPE_ERROR = 1
};

struct ThreadsEvent {
  ThreadsEvent(JavaScriptWorkerInfo *worker_info, ThreadsEventType event_type)
      : wi(worker_info), type(event_type) {
    wi->threads_manager->AddWorkerRef();
  }

  ~ThreadsEvent() {
    wi->threads_manager->ReleaseWorkerRef();
  }

  PLEvent e; // PLEvent must be first field
  JavaScriptWorkerInfo *wi;
  ThreadsEventType type;
};


// Called when the event is received.
void* PoolThreadsManager::OnReceiveThreadsEvent(ThreadsEvent *event) {
  JavaScriptWorkerInfo *wi = event->wi;

  // This is necessary because some events may still be in the queue and get
  // processed after shutdown. Also, we post a final "fake" event to wake
  // workers which should not get processed.
  if (wi->threads_manager->is_shutting_down_) {
    return NULL;
  }

  // Retrieve message information.
  Message msg;
  if (!wi->threads_manager->GetPoolMessage(&msg)) {
    return NULL;
  }

  if (event->type == EVENT_TYPE_MESSAGE) {
    wi->threads_manager->ProcessMessage(wi, msg);
  } else {
    assert(event->type == EVENT_TYPE_ERROR);
    wi->threads_manager->ProcessError(wi, msg);
  }

  return NULL; // retval only matters for PostSynchronousEvent()
}


// Called when the event has been processed.
static void OnDestroyThreadsEvent(ThreadsEvent *event) {
  delete event;
}


void PoolThreadsManager::ProcessMessage(JavaScriptWorkerInfo *wi,
                                        const Message &msg) {
  assert(wi);
  if (wi->onmessage_handler.get() &&
      !wi->onmessage_handler->IsNullOrUndefined()) {

    // TODO(zork): Remove this with dump_on_error.  It is declared as volatile
    // to ensure that it exists on the stack even in opt builds.
    volatile bool is_shutting_down = wi->threads_manager->is_shutting_down_;
    is_shutting_down;

    // Setup the onmessage parameter (type: Object).
    assert(wi->js_runner);
    scoped_ptr<JsRootedToken> onmessage_param(
                                  wi->js_runner->NewObject(NULL, true));
    // TODO(zork): Checking this return value is temporary, as callers are not
    // supposed to have to worry about NewObject() failing.
    if (!onmessage_param.get()) {
      JsErrorInfo error_info = {
        0,
        STRING16(L"Internal error. (Could not create message object.)")
      };
      HandleError(error_info);
      return;
    }

    wi->js_runner->SetPropertyString(onmessage_param->token(),
                                     STRING16(L"text"),
                                     msg.text.c_str());
    wi->js_runner->SetPropertyInt(onmessage_param->token(),
                                  STRING16(L"sender"),
                                  msg.sender);
    wi->js_runner->SetPropertyString(onmessage_param->token(),
                                     STRING16(L"origin"),
                                     msg.origin.url().c_str());

    const int argc = 3;
    JsParamToSend argv[argc] = {
      { JSPARAM_STRING16, &msg.text },
      { JSPARAM_INT, &msg.sender },
      { JSPARAM_OBJECT_TOKEN, onmessage_param.get() }
    };
    wi->js_runner->InvokeCallback(wi->onmessage_handler.get(), argc, argv,
                                  NULL);
  } else {
    JsErrorInfo error_info = {
      0, // line number -- What we really want is the line number in the
         // sending worker, but that would be hard to get.
      STRING16(L"Could not process message because worker does not have an "
               L"onmessage handler.")
    };

    // We go through the message queue even in the case where this happens on
    // the owning worker, just so that things are consistent for all cases.
    HandleError(error_info);
  }
}


void PoolThreadsManager::ProcessError(JavaScriptWorkerInfo *wi,
                                      const Message &msg) {
#ifdef DEBUG
  {
    // We only expect to be receive errors on the owning worker, all workers
    // forward their errors here (via HandleError).
    MutexLock lock(&mutex_);
    assert(kOwningWorkerId == GetCurrentPoolWorkerId());
  }
#endif

  // Bubble the error up to the owning worker's script context. If that
  // worker is also nested, this will cause PoolThreadsManager::HandleError
  // to get called again on that context.
  ThrowGlobalError(wi->js_runner, msg.text);
}


//
// PoolThreadsManager -- handles threading and JS engine setup.
//

PoolThreadsManager::PoolThreadsManager(
                        const SecurityOrigin &page_security_origin,
                        JsRunnerInterface *root_js_runner)
    : num_workers_(0), 
      is_shutting_down_(false),
      page_security_origin_(page_security_origin) {

  // Add a JavaScriptWorkerInfo entry for the owning worker.
  JavaScriptWorkerInfo *wi = new JavaScriptWorkerInfo;
  wi->threads_manager = this;
  wi->js_runner = root_js_runner;
  InitWorkerThread(wi);
  worker_info_.push_back(wi);
}


PoolThreadsManager::~PoolThreadsManager() {
  for (size_t i = 0; i < worker_info_.size(); ++i) {
    delete worker_info_[i];
  }
}


int PoolThreadsManager::GetCurrentPoolWorkerId() {
  // no MutexLock here because this function is private, and callers are
  // responsible for acquiring the exclusive lock
  PRThread *os_thread_id = PR_GetCurrentThread();

  // lookup OS-defined id in list of known mappings
  // (linear scan is fine because number of threads per pool will be small)
  int count = static_cast<int>(worker_id_to_os_thread_id_.size());

  for (int i = 0; i < count; ++i) {
    if (worker_id_to_os_thread_id_[i] == os_thread_id) {
      assert(i < static_cast<int>(worker_info_.size()));
      assert(worker_info_[i]);
      return i;
    }
  }

  assert(false);
  return kInvalidWorkerId;
}


void PoolThreadsManager::AllowCrossOrigin() {
  MutexLock lock(&mutex_);

  int current_worker_id = GetCurrentPoolWorkerId();
  JavaScriptWorkerInfo *wi = worker_info_[current_worker_id];

  // is_factory_suspended ensures ...UpdatePermissions() happens at most once,
  // and only for cross-origin workers.
  if (wi->is_factory_suspended) {
    wi->is_factory_suspended = false;
    wi->factory_ref->ResumeObjectCreationAndUpdatePermissions();
  }
}


void PoolThreadsManager::HandleError(const JsErrorInfo &error_info) {
  // If there is an error handler on this thread, invoke it.
  int src_worker_id = kInvalidWorkerId;
  JavaScriptWorkerInfo *src_wi = NULL;
  bool error_was_handled = false;

  // Must lock conservatively here because InvokeOnErrorHandler() below can end
  // up calling methods on workerpool which also acquire the mutex.
  {
    MutexLock lock(&mutex_);
    src_worker_id = GetCurrentPoolWorkerId();
    src_wi = worker_info_[src_worker_id];
  }

  // TODO(cprince): Add the following lines when ReadyStateChanged doesn't need
  // to be called from the main thread -- i.e. when HttpRequest can fetch URLs
  // from threads other than the main thread.
  //   // We only expect to receive errors from created workers.
  //   assert(src_worker_id != kOwningWorkerId);

  // Guard against the onerror handler itself throwing an error and causing a
  // loop.
  if (!src_wi->is_invoking_error_handler) {
    src_wi->is_invoking_error_handler = true;
    error_was_handled = InvokeOnErrorHandler(src_wi, error_info);
    src_wi->is_invoking_error_handler = false;
  }

  // If the error was not handled, bubble it up to the parent worker.
  if (!error_was_handled) {
    MutexLock lock(&mutex_);

    std::string16 text;
    FormatWorkerPoolErrorMessage(error_info, src_worker_id, &text);

    JavaScriptWorkerInfo *dest_wi = worker_info_[kOwningWorkerId];  // parent

    // Copy the message to an internal queue.
    dest_wi->message_queue.push(Message(text, src_worker_id,
                                        dest_wi->script_origin));
    // Notify the receiving worker.
    ThreadsEvent *event = new ThreadsEvent(dest_wi, EVENT_TYPE_ERROR);
    dest_wi->thread_event_queue->InitEvent(
        &event->e, nsnull, // event, owner
        reinterpret_cast<PLHandleEventProc>(OnReceiveThreadsEvent),
        reinterpret_cast<PLDestroyEventProc>(OnDestroyThreadsEvent));
    dest_wi->thread_event_queue->PostEvent(&event->e);
  }
}


bool PoolThreadsManager::InvokeOnErrorHandler(JavaScriptWorkerInfo *wi,
                                              const JsErrorInfo &error_info) {
  assert(wi);
  if (!wi->onerror_handler.get() || wi->onerror_handler->IsNullOrUndefined()) {
    return false;
  }

  // TODO(zork): Remove this with dump_on_error.  It is declared as volatile to
  // ensure that it exists on the stack even in opt builds.
  volatile bool is_shutting_down = wi->threads_manager->is_shutting_down_;
  is_shutting_down;

  // Setup the onerror parameter (type: Error).
  assert(wi->js_runner);
  scoped_ptr<JsRootedToken> onerror_param(
      wi->js_runner->NewObject(STRING16(L"Error"), true));
  if (!onerror_param.get()) {
    return false;
  }

  wi->js_runner->SetPropertyString(onerror_param->token(),
                                   STRING16(L"message"),
                                   error_info.message.c_str());
  wi->js_runner->SetPropertyInt(onerror_param->token(),
                                STRING16(L"lineNumber"),
                                error_info.line);
  // TODO(aa): Additional information, like fragment of code where the error
  // occurred, stack?

  const int argc = 1;
  JsParamToSend argv[argc] = {
    { JSPARAM_OBJECT_TOKEN, onerror_param.get() }
  };

  JsRootedToken *alloc_js_retval = NULL;
  bool js_retval = false;

  if (wi->js_runner->InvokeCallback(wi->onerror_handler.get(), argc, argv,
                                    &alloc_js_retval)) {
    alloc_js_retval->GetAsBool(&js_retval);
    delete alloc_js_retval;
  }

  return js_retval;
}


bool PoolThreadsManager::PutPoolMessage(const char16 *text,
                                        int dest_worker_id,
                                        const SecurityOrigin &src_origin) {
  MutexLock lock(&mutex_);

  int src_worker_id = GetCurrentPoolWorkerId();

  // check for valid dest_worker_id
  if (dest_worker_id < 0 ||
      dest_worker_id >= static_cast<int>(worker_info_.size())) {
    return false;
  }
  JavaScriptWorkerInfo *dest_wi = worker_info_[dest_worker_id];
  if (NULL == dest_wi || NULL == dest_wi->threads_manager ||
      NULL == dest_wi->thread_event_queue) {
    return false;
  }

  // Copy the message to an internal queue.
  dest_wi->message_queue.push(Message(text, src_worker_id, src_origin));
  // Notify the receiving worker.
  ThreadsEvent *event = new ThreadsEvent(dest_wi, EVENT_TYPE_MESSAGE);
  dest_wi->thread_event_queue->InitEvent(
      &event->e, nsnull, // event, owner
      reinterpret_cast<PLHandleEventProc>(OnReceiveThreadsEvent),
      reinterpret_cast<PLDestroyEventProc>(OnDestroyThreadsEvent));
  dest_wi->thread_event_queue->PostEvent(&event->e);

  return true; // succeeded
}


bool PoolThreadsManager::GetPoolMessage(Message *msg) {
  MutexLock lock(&mutex_);

  int current_worker_id = GetCurrentPoolWorkerId();
  JavaScriptWorkerInfo *wi = worker_info_[current_worker_id];

  assert(!wi->message_queue.empty());

  *msg = wi->message_queue.front();
  wi->message_queue.pop();
  return true;
}


bool PoolThreadsManager::InitWorkerThread(JavaScriptWorkerInfo *wi) {
  MutexLock lock(&mutex_);

  // Sanity-check that we're not calling this twice. Doing so would mean we
  // created multiple hwnds for the same worker, which would be bad.
  assert(!wi->thread_event_queue);

  // Register this worker so that it can be looked up by OS thread ID.
  PRThread *os_thread_id = PR_GetCurrentThread();
  worker_id_to_os_thread_id_.push_back(os_thread_id);

  // Also get the event queue for this worker.
  // This is how we service JS worker messages synchronously relative to other
  // JS execution.
  //
  // Firefox has a single event queue per thread, and messages are sent to this
  // shared queue.  (Compare this to the Win32 model where events are sent to
  // multiple HWNDs, which share an event queue internally.)  So first check to
  // see if a thread event queue exists. The main worker will already have one,
  // but child workers will not.
  nsresult nr;

  nsCOMPtr<nsIEventQueueService> event_queue_service =
      do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &nr);
  if (NS_FAILED(nr)) {
    return false;
  }

  nsCOMPtr<nsIEventQueue> event_queue;
  nr = event_queue_service->GetThreadEventQueue(NS_CURRENT_THREAD,
                                                getter_AddRefs(event_queue));
  if (NS_FAILED(nr)) {
    // no thread event queue yet, so create one
    nr = event_queue_service->CreateMonitoredThreadEventQueue();
    if (NS_FAILED(nr)) {
      return false;
    }
    nr = event_queue_service->GetThreadEventQueue(NS_CURRENT_THREAD,
                                                  getter_AddRefs(event_queue));
    if (NS_FAILED(nr)) {
      return false;
    }
  }

  wi->thread_event_queue = event_queue;
  return true;
}


void PoolThreadsManager::UninitWorkerThread() {
  // Nothing to do here yet. Only included for parallelism with IE
  // implementation.
}


bool PoolThreadsManager::SetCurrentThreadMessageHandler(
                             JsRootedCallback *handler) {
  MutexLock lock(&mutex_);

  int worker_id = GetCurrentPoolWorkerId();
  JavaScriptWorkerInfo *wi = worker_info_[worker_id];

  wi->onmessage_handler.reset(handler);
  return true;
}


bool PoolThreadsManager::SetCurrentThreadErrorHandler(
                             JsRootedCallback *handler) {
  MutexLock lock(&mutex_);

  int worker_id = GetCurrentPoolWorkerId();
  if (kOwningWorkerId == worker_id) {
    // TODO(aa): Change this error to an assert when we remove the ability to
    // set 'onerror' on parent workers.
    return false;
  }

  JavaScriptWorkerInfo *wi = worker_info_[worker_id];
  wi->onerror_handler.reset(handler);

  return true;
}


class CreateWorkerUrlFetchListener : public HttpRequest::ReadyStateListener {
 public:
  explicit CreateWorkerUrlFetchListener(JavaScriptWorkerInfo *wi) : wi_(wi) {}

  virtual void ReadyStateChanged(HttpRequest *source) {
    HttpRequest::ReadyState ready_state = HttpRequest::UNINITIALIZED;
    source->GetReadyState(&ready_state);
    if (ready_state == HttpRequest::COMPLETE) {
      // Fetch completed.  First, unregister this listener.
      source->SetOnReadyStateChange(NULL);

      int status_code;
      std::string16 body;
      std::string16 final_url;
      if (source->GetStatus(&status_code) &&
          status_code == HttpConstants::HTTP_OK &&
          source->GetResponseBodyAsText(&body) &&
          source->GetFinalUrl(&final_url)) {
        // These are purposely set before locking mutex, because they are still
        // owned by the parent thread at this point.
        wi_->script_ok = true;
        wi_->script_text += body;
        // Must use security origin of final url, in case there were redirects.
        wi_->script_origin.InitFromUrl(final_url.c_str());
      } else {
        // Throw an error, but don't return!  Continue and set script_signalled
        // so the worker doesn't spin forever in Mutex::Await().
        std::string16 message(STRING16(L"Failed to load script."));
        std::string16 status_line;
        if (source->GetStatusLine(&status_line)) {
          message += STRING16(L" Status: ");
          message += status_line;
        }
        std::string16 requested_url;
        if (source->GetInitialUrl(&requested_url)) {
          message += STRING16(L" URL: ");
          message += requested_url;
        }
        JsErrorInfo error_info = { 0, message };  // line, message
        wi_->threads_manager->HandleError(error_info);
      }

      wi_->script_mutex.Lock();
      assert(!wi_->script_signalled);
      wi_->script_signalled = true;
      wi_->script_mutex.Unlock();
    }
  }
 private:
  JavaScriptWorkerInfo *wi_;
};


bool PoolThreadsManager::CreateThread(const char16 *url_or_full_script,
                                      bool is_param_script, int *worker_id) {
  int new_worker_id = -1;
  JavaScriptWorkerInfo *wi = NULL;
  {
    MutexLock lock(&mutex_);

    // If the creating thread didn't intialize properly it doesn't have a
    // message queue, so there's no point in letting it start a new thread.
    if (!worker_info_[GetCurrentPoolWorkerId()]->thread_event_queue) {
      return false;
    }

    // Add a JavaScriptWorkerInfo entry.
    // Is okay not to undo this if code below fails. Behavior will be correct.
    worker_info_.push_back(new JavaScriptWorkerInfo);
    new_worker_id = static_cast<int>(worker_info_.size()) - 1;
    wi = worker_info_.back();
  }

  // The code below should not access shared data structures. We
  // only synchronize the block above, which modifies the shared 
  // worker_info_ vector.

  wi->threads_manager = this;

  wi->script_text = kWorkerInsertedPreamble;
  if (is_param_script) {
    wi->script_ok = true;
    wi->script_text += url_or_full_script;
    wi->script_origin = page_security_origin_;

    wi->script_mutex.Lock();
    wi->script_signalled = true;
    wi->script_mutex.Unlock();
  } else {
    // For URL params we start an async fetch here.  The created thread will
    // setup an incoming message queue, then Mutex::Await for the script to be
    // fetched, before finally pumping messages.

    wi->http_request.reset(HttpRequest::Create());
    if (!wi->http_request.get()) { return false; }
    
    wi->http_request_listener.reset(new CreateWorkerUrlFetchListener(wi));
    if (!wi->http_request_listener.get()) { return false; }

    HttpRequest *request = wi->http_request.get();  // shorthand

    request->SetOnReadyStateChange(wi->http_request_listener.get());
    request->SetCachingBehavior(HttpRequest::USE_ALL_CACHES);
    request->SetRedirectBehavior(HttpRequest::FOLLOW_ALL);

    std::string16 url;
    ResolveAndNormalize(page_security_origin_.full_url().c_str(),
                  url_or_full_script, &url);

    bool is_async = true;
    if (!request->Open(HttpConstants::kHttpGET, url.c_str(), is_async) ||
        !request->Send()) {
      request->SetOnReadyStateChange(NULL);
      request->Abort();
      return false;
    }

    // 'script_signalled' will be set when async fetch completes.
  }

  // Setup notifier to know when thread init has finished.
  // Then create thread and wait for signal.
  wi->thread_init_mutex.Lock();
  wi->thread_init_signalled = false;

  wi->thread_pointer = PR_CreateThread(
                           PR_USER_THREAD, JavaScriptThreadEntry, // type, func
                           wi, PR_PRIORITY_NORMAL,   // arg, priority
                           PR_LOCAL_THREAD,          // scheduled by whom?
                           PR_UNJOINABLE_THREAD, 0); // joinable?, stack bytes
  if (wi->thread_pointer != NULL) {
    // thread needs to message queue init before we continue
    wi->thread_init_mutex.Await(Condition(&wi->thread_init_signalled));
  }

  // cleanup notifier
  wi->thread_init_mutex.Unlock();

  if (wi->thread_pointer == NULL || !wi->thread_init_ok) {
    return false; // failed
  }

  *worker_id = new_worker_id;
  return true; // succeeded
}


// Creates the JS engine, then pumps messages for the thread.
void PoolThreadsManager::JavaScriptThreadEntry(void *args) {
  assert(args);
  JavaScriptWorkerInfo *wi = static_cast<JavaScriptWorkerInfo*>(args);
  wi->threads_manager->AddWorkerRef();

  // Setup worker thread.
  // Then signal that initialization is done, and indicate success/failure.
  //
  // WARNING: must fire thread_init_signalled even on failure, or caller won't
  // continue.  So fire it from a non-nested location, before any early exits.
  scoped_ptr<JsRunnerInterface> js_runner(NewJsRunner());
  assert(NULL == wi->js_runner);
  wi->js_runner = js_runner.get();

  bool thread_init_succeeded = (NULL != js_runner.get()) &&
                               wi->threads_manager->InitWorkerThread(wi);

  wi->thread_init_ok = thread_init_succeeded;
  wi->thread_init_mutex.Lock();
  wi->thread_init_signalled = true;
  wi->thread_init_mutex.Unlock();

  if (thread_init_succeeded) {
    // Block until 'script_signalled' (i.e. wait for URL fetch, if being used).
    // Thread shutdown will set this flag as well.
    wi->script_mutex.Lock();
    wi->script_mutex.Await(Condition(&wi->script_signalled));
    wi->script_mutex.Unlock();

    if (wi->script_ok) {
      if (SetupJsRunner(js_runner.get(), wi)) {
        // Add JS code to engine.  Any script errors trigger HandleError().
        js_runner->Start(wi->script_text);
      }
    }

    // Pump messages. We do this whether or not the initial script evaluation
    // succeeded (just like in browsers).
    assert(wi->thread_event_queue);
    while (1) {
      // (based on sample code in /mozilla/netwerk/test/... [sic])
      PLEvent *event;
      wi->thread_event_queue->WaitForEvent(&event);
      wi->thread_event_queue->HandleEvent(event);
      // Check flag after handling, otherwise last event never gets deleted.
      if (wi->threads_manager->is_shutting_down_) {
        break;
      }
    }
  }

  // Remove the message handlers here, since we're destroying the context they
  // belong to.
  wi->onmessage_handler.reset(NULL);
  wi->onerror_handler.reset(NULL);

  // nsCOMPtr is not threadsafe, must release from creation thread.
  wi->factory_ref = NULL;

  // TODO(aa): Consider deleting wi here and setting PTM.worker_info_[i] to
  // NULL. This allows us to free up these thread resources sooner, and it
  // seems a little cleaner too.
  wi->js_runner = NULL;  // scoped_ptr is about to delete the underlying object
  wi->threads_manager->ReleaseWorkerRef();

  // PRThread functions don't return a value
}

bool PoolThreadsManager::SetupJsRunner(JsRunnerInterface *js_runner,
                                       JavaScriptWorkerInfo *wi) {
  if (!js_runner) { return false; }

  JsContextPtr cx = js_runner->GetContext();
  if (!cx) { return false; }

  // Add global Factory and WorkerPool objects into the namespace.
  //
  // The factory alone is not enough; GearsFactory.create(GearsWorkerPool)
  // would return a NEW PoolThreadsManager instance, but we want access to
  // the one that was previously created for the current page.
  //
  // js_runner manages the lifetime of these allocated objects.

  scoped_ptr<GearsFactory> factory(new GearsFactory());
  if (!factory.get()) { return false; }

  if (!factory->InitBaseManually(true, // is_worker
                                 cx,
                                 wi->script_origin,
                                 js_runner)) {
    return false;
  }

  scoped_ptr<GearsWorkerPool> workerpool(new GearsWorkerPool());
  if (!workerpool.get()) { return false; }

  if (!workerpool->InitBaseManually(true, // is_worker
                                    cx,
                                    wi->script_origin,
                                    js_runner)) {
    return false;
  }


  // This Factory always inherits opt-in permissions.
  factory->is_permission_granted_ = true;
  factory->is_permission_value_from_user_ = true;
  // But for cross-origin workers, object creation is suspended until the
  // callee invokes allowCrossOrigin().
  if (!wi->threads_manager->page_security_origin().IsSameOrigin(
                                                       wi->script_origin)) {
    factory->SuspendObjectCreation();
    wi->is_factory_suspended = true;
  }

  // This WorkerPool needs the same underlying PoolThreadsManager as its parent.
  workerpool->SetThreadsManager(wi->threads_manager);


  // Save an AddRef'd pointer to the factory so we can access it later.
  wi->factory_ref = factory.get();


  // Expose created objects as globals in the JS engine.
  const nsIID factory_iface_id = GEARSFACTORYINTERFACE_IID;
  if (!js_runner->AddGlobal(kWorkerInsertedFactoryName,
                            factory.release(), // nsISupports
                            factory_iface_id)) {
    return false;
  }

  const nsIID workerpool_iface_id = GEARSWORKERPOOLINTERFACE_IID;
  if (!js_runner->AddGlobal(kWorkerInsertedWorkerPoolName,
                            workerpool.release(), // nsISupports
                            workerpool_iface_id)) {
    return false;
  }


  // Register the PoolThreadsManager as the error handler for this JS engine.
  js_runner->SetErrorHandler(wi->threads_manager);

  return true;
}


void PoolThreadsManager::ShutDown() {
  MutexLock lock(&mutex_);

  assert(GetCurrentPoolWorkerId() == kOwningWorkerId);

  if (is_shutting_down_) { return; }
  is_shutting_down_ = true;

  // Releasing callbacks in Firefox requires a valid pointer to the js engine.
  // So release the callbacks for the owning worker now, while there is still
  // an engine for the owning worker's thread.
  worker_info_[kOwningWorkerId]->onmessage_handler.reset(NULL);
  worker_info_[kOwningWorkerId]->onerror_handler.reset(NULL);

  for (size_t i = 0; i < worker_info_.size(); ++i) {
    JavaScriptWorkerInfo *wi = worker_info_[i];

    // Cancel any createWorkerFromUrl() network requests that might be pending.
    HttpRequest *request = wi->http_request.get();
    if (request) {
      request->SetOnReadyStateChange(NULL);
      request->Abort();
      // HttpRequest is not threadsafe, must destroy from same thread that
      // created it (which is always the owning thread for now, since we cannot
      // yet make requests from background threads).
      wi->http_request.reset(NULL);
    }

    // If the worker is a created thread...
    if (wi->thread_pointer && wi->thread_event_queue) {
      // Ensure the thread isn't waiting on 'script_signalled'.
      wi->script_mutex.Lock();
      wi->script_signalled = true;
      wi->script_mutex.Unlock();

      // Ensure the thread sees 'is_shutting_down_' by sending a dummy message,
      // in case it is blocked waiting for messages.
      ThreadsEvent *event = new ThreadsEvent(wi, EVENT_TYPE_MESSAGE);
      wi->thread_event_queue->InitEvent(
          &event->e, nsnull, // event, owner
          reinterpret_cast<PLHandleEventProc>(OnReceiveThreadsEvent),
          reinterpret_cast<PLDestroyEventProc>(OnDestroyThreadsEvent));
      wi->thread_event_queue->PostEvent(&event->e);

      // TODO(cprince): Improve handling of a worker spinning in a JS busy loop.
      // Ideas: (1) set it to the lowest thread priority level, or (2) interrupt
      // the JS engine externally (see IActiveScript::InterruptScriptThread
      // on IE, JS_THREADSAFE for Firefox).  We cannot simply terminate the
      // thread; that can leave us in a bad state (e.g. mutexes locked forever).
    }
  }
}


#ifdef DEBUG
void PoolThreadsManager::ForceGCCurrentThread() {
  MutexLock lock(&mutex_);

  int worker_id = GetCurrentPoolWorkerId();

  JavaScriptWorkerInfo *wi = worker_info_[worker_id];
  assert(wi->js_runner);
  wi->js_runner->ForceGC();
}
#endif // DEBUG


void PoolThreadsManager::AddWorkerRef() {
  AtomicIncrement(&num_workers_, 1);
}

void PoolThreadsManager::ReleaseWorkerRef() {
  if (AtomicIncrement(&num_workers_, -1) == 0) {
    delete this;
  }
}
