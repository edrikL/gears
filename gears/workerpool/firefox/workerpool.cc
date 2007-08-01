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
#include "gears/base/common/js_runner.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/base/firefox/factory.h"
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
// JavaScriptWorkerInfo -- contains the state of each JavaScript worker.
//

struct JavaScriptWorkerInfo {
  // Our code assumes some items begin cleared. Zero all members w/o ctors.
  JavaScriptWorkerInfo()
      : threads_manager(NULL),
        js_init_mutex(NULL), js_init_signalled(false), js_init_ok(false),
        thread_pointer(NULL) {}

  //
  // These fields are used by all workers in pool (root + descendants).
  //
  PoolThreadsManager *threads_manager;
  JsCallback onmessage_handler;
  JsCallback onerror_handler;
  nsCOMPtr<nsIEventQueue> thread_event_queue;
  std::queue< std::pair<std::string16, int> > message_queue;

  //
  // These fields are used only by created workers (descendants).
  //
  std::string16 full_script; // Keep copy to cross OS thread broundary
  Mutex  *js_init_mutex; // Protects: js_init_signalled
  bool    js_init_signalled;
  bool    js_init_ok; // Owner: thread before signal, caller after signal
  PRThread  *thread_pointer;
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

  JsCallback onmessage_handler;
  JsParamFetcher js_params(this);
  if (!js_params.GetAsCallback(0, &onmessage_handler)) {
    RETURN_EXCEPTION(STRING16(L"Invalid value for onmesssage property."));
  }

  // "Root" the handler so it cannot get garbage collected.
  // TODO(aa): Since we never undo this, it leaks. We should do something
  // similar to the CComPtr<IDispatch> that the IE version has so that this is
  // handled cleanly. Same thing for onerror handler.
  if (!RootJsToken(onmessage_handler.context, onmessage_handler.function)) {
    RETURN_EXCEPTION(STRING16(L"Error rooting JS value."));
  }

  if (!threads_manager_->SetCurrentThreadMessageHandler(&onmessage_handler)) {
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

  JsCallback onerror_handler;
  JsParamFetcher js_params(this);
  if (!js_params.GetAsCallback(0, &onerror_handler)) {
    RETURN_EXCEPTION(STRING16(L"Invalid value for onerror property."));
  }

  // "Root" the handler so it cannot get garbage collected.
  if (!RootJsToken(onerror_handler.context, onerror_handler.function)) {
    RETURN_EXCEPTION(STRING16(L"Error rooting JS value."));
  }

  if (!threads_manager_->SetCurrentThreadErrorHandler(&onerror_handler)) {
    // Currently, the only reason this can fail is because of this one
    // particular error.
    // TODO(aa): We need a system throughout Gears for being able to handle
    // exceptions from deep inside the stack better.
    RETURN_EXCEPTION(STRING16(L"The onerror property cannot be set from "
                              L"inside a worker"));
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
  
  // Get parameters.
  std::string16 full_script;

  JsParamFetcher js_params(this);

  if (js_params.GetCount() < 1) {
    RETURN_EXCEPTION(STRING16(L"Not enough paramenters."));
  } else if (!js_params.GetAsString(0, &full_script)) {
    RETURN_EXCEPTION(STRING16(L"Script must be a string."));
  }


  int worker_id_temp; // protects against modifying output param on failure
  bool succeeded = threads_manager_->CreateThread(full_script.c_str(),
                                                  &worker_id_temp);
  if (!succeeded) {
    RETURN_EXCEPTION(STRING16(L"Worker creation failed."));
  }

  *retval = worker_id_temp;
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::SendMessage(const nsAString &message_string,
                                           PRInt32 dest_worker_id) {
  Initialize();

  bool succeeded = threads_manager_->PutPoolMessage(message_string,
                                                    dest_worker_id);
  if (!succeeded) {
    std::string16 worker_id_string;
    IntegerToString(dest_worker_id, &worker_id_string);

    std::string16 error(STRING16(L"Worker ID does not exist: "));
    error += worker_id_string;
    error += STRING16(L".");

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

void GearsWorkerPool::HandleEventUnload(void *user_param) {
  GearsWorkerPool *wp = static_cast<GearsWorkerPool*>(user_param);
  if (wp->threads_manager_) {
    wp->threads_manager_->ShutDown();
  }
}

void GearsWorkerPool::Initialize() {
  if (!threads_manager_) {
    SetThreadsManager(
        new PoolThreadsManager(this->EnvPageSecurityOrigin(), GetJsRunner()));
    owns_threads_manager_ = true;
  }

  // Monitor 'onunload' to shutdown threads when the page goes away.
  //
  // A thread that keeps running after the page changes can cause odd problems,
  // if it continues to send messages. (This can happen if it busy-loops.)  On
  // Firefox, such a thread triggered the Print dialog after the page changed!
  if (!EnvIsWorker() && unload_monitor_ == NULL) {
    unload_monitor_.reset(new HtmlEventMonitor(kEventUnload,
                                               HandleEventUnload, this));
    nsCOMPtr<nsIDOMEventTarget> event_source;
#ifdef DEBUG
    nsresult nr = DOMUtils::GetWindowEventTarget(getter_AddRefs(event_source));
    assert(NS_SUCCEEDED(nr));
#endif
    unload_monitor_->Start(event_source);
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

  // Retrieve message information
  std::string16 message_string;
  int src_worker_id;
  
  if (!wi->threads_manager->GetPoolMessage(&message_string, &src_worker_id)) {
    return NULL;
  }

  if (event->type == EVENT_TYPE_MESSAGE) {
    wi->threads_manager->ProcessMessage(wi, message_string, src_worker_id);
  } else {
    assert(event->type == EVENT_TYPE_ERROR);
    wi->threads_manager->ProcessError(wi, message_string, src_worker_id);
  }

  return NULL; // retval only matters for PostSynchronousEvent()
}


void PoolThreadsManager::ProcessMessage(JavaScriptWorkerInfo *wi,
                                        const std::string16 &message_string,
                                        int src_worker_id) {
  if (wi->onmessage_handler.function) {
    FireHandler(wi->onmessage_handler, message_string, src_worker_id);
  } else {
    // It is an error to send a message to a worker that does not have an
    // onmessage handler.
    int worker_id = kInvalidWorkerId;

    // Synchronize only this section because HandleError() below also acquires
    // the lock and locks are not reentrant.
    {
      MutexLock lock(&mutex_);
      worker_id = GetCurrentPoolWorkerId();
    }

    std::string16 worker_id_string;
    IntegerToString(worker_id, &worker_id_string);

    std::string16 error = STRING16(L"Destination worker ");
    error += worker_id_string;
    error += STRING16(L" does not have an onmessage handler");

    // We go through the message queue even in the case where this happens on
    // the owning worker, just so that things are consistent for all cases.
    HandleError(error);
  }
}


void PoolThreadsManager::ProcessError(JavaScriptWorkerInfo *wi,
                                      const std::string16 &error,
                                      int src_worker_id) {
#ifdef DEBUG
  {
    // We only expect to be receive errors on the owning worker, all workers
    // forward their errors here (via HandleError).
    MutexLock lock(&mutex_);
    assert(kOwningWorkerId == GetCurrentPoolWorkerId());
  }
#endif

  if (wi->onerror_handler.function) {
    FireHandler(wi->onerror_handler, error, src_worker_id);
  } else {
    // If there's no onerror handler, we bubble the error up to the owning
    // worker's script context. If that worker is also nested, this will cause
    // PoolThreadsManager::HandleError to get called again on that context.
    std::string16 worker_string;
    IntegerToString(src_worker_id, &worker_string);

    std::string16 message(STRING16(L"Unhandled exception in worker "));
    message += worker_string;
    message += std::string16(STRING16(L":\n"));
    message += error;

    root_js_runner_->ThrowGlobalError(message);
  }
}


void PoolThreadsManager::FireHandler(const JsCallback &handler,
                                     const std::string16 &message,
                                     int src_worker_id) {
  // Invoke JavaScript onmessage handler
  JSString *message_arg_jsstring = JS_NewUCStringCopyZ(
      handler.context,
      reinterpret_cast<const jschar *>(
          message.c_str())); // TODO(cprince): ensure freeing memory
  uintN argc = 2;
  jsval argv[] = { STRING_TO_JSVAL(message_arg_jsstring),
                   INT_TO_JSVAL(src_worker_id) };
  jsval js_retval;
  //JSBool js_ok =  // comment out until we use it, to avoid compiler warning
  JS_CallFunctionValue( // goes to js_InternalInvoke()
      handler.context,
      JS_GetGlobalObject(handler.context),
      handler.function, argc, argv, &js_retval);
}


// Called when the event has been processed.
static void OnDestroyThreadsEvent(ThreadsEvent *event) {
  delete event;
}



//
// PoolThreadsManager -- handles threading and JS engine setup.
//

PoolThreadsManager::PoolThreadsManager(
                        const SecurityOrigin &page_security_origin,
                        JsRunnerInterface *root_js_runner)
    : num_workers_(0), 
      is_shutting_down_(false),
      page_security_origin_(page_security_origin),
      root_js_runner_(root_js_runner) {

  // Add a JavaScriptWorkerInfo entry for the owning worker.
  JavaScriptWorkerInfo *wi = new JavaScriptWorkerInfo;
  wi->threads_manager = this;
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


void PoolThreadsManager::HandleError(const std::string16 &message_string) {
  MutexLock lock(&mutex_);

  int src_worker_id = GetCurrentPoolWorkerId();
  // We only expect to receive errors from created workers.
  assert(src_worker_id != kOwningWorkerId);

  JavaScriptWorkerInfo *dest_wi = worker_info_[kOwningWorkerId];

  // put the message in our internal queue
  std::pair<std::string16, int> msg(message_string,
                                    src_worker_id);  // copies the string
  dest_wi->message_queue.push(msg);

  // notify the receiving thread
  ThreadsEvent *event = new ThreadsEvent(dest_wi, EVENT_TYPE_ERROR);
  dest_wi->thread_event_queue->InitEvent(
      &event->e, nsnull, // event, owner
      reinterpret_cast<PLHandleEventProc>(OnReceiveThreadsEvent),
      reinterpret_cast<PLDestroyEventProc>(OnDestroyThreadsEvent));
  dest_wi->thread_event_queue->PostEvent(&event->e);
}


bool PoolThreadsManager::PutPoolMessage(const nsAString &message_string,
                                        int dest_worker_id) {
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

  // put the message in our internal queue
  const PRUnichar *msg_chars;
  NS_StringGetData(message_string, &msg_chars);
  std::pair<std::string16, int> msg(static_cast<const char16*>(msg_chars),
                                    src_worker_id);  // copies the string
  dest_wi->message_queue.push(msg);

  // notify the receiving thread
  ThreadsEvent *event = new ThreadsEvent(dest_wi, EVENT_TYPE_MESSAGE);
  dest_wi->thread_event_queue->InitEvent(
      &event->e, nsnull, // event, owner
      reinterpret_cast<PLHandleEventProc>(OnReceiveThreadsEvent),
      reinterpret_cast<PLDestroyEventProc>(OnDestroyThreadsEvent));
  dest_wi->thread_event_queue->PostEvent(&event->e);

  return true; // succeeded
}


bool PoolThreadsManager::GetPoolMessage(std::string16 *message_string,
                                        int *src_worker_id) {
  MutexLock lock(&mutex_);

  int current_worker_id = GetCurrentPoolWorkerId();
  JavaScriptWorkerInfo *wi = worker_info_[current_worker_id];

  assert(!wi->message_queue.empty());

  *message_string = wi->message_queue.front().first;
  *src_worker_id = wi->message_queue.front().second;
  wi->message_queue.pop();
  return true;
}


bool PoolThreadsManager::InitWorkerThread(JavaScriptWorkerInfo *wi) {
  MutexLock lock(&mutex_);

  // Sanity-check that we're not calling this twice. Doing so would mean we
  // created multiple hwnds for the same worker, which would have undefined
  // behavior.
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


bool PoolThreadsManager::SetCurrentThreadMessageHandler(JsCallback *handler) {
  MutexLock lock(&mutex_);

  int worker_id = GetCurrentPoolWorkerId();
  JavaScriptWorkerInfo *wi = worker_info_[worker_id];

  wi->onmessage_handler = *handler;
  return true;
}


bool PoolThreadsManager::SetCurrentThreadErrorHandler(JsCallback *handler) {
  MutexLock lock(&mutex_);

  int worker_id = GetCurrentPoolWorkerId();
  if (kOwningWorkerId != worker_id) {
    // TODO(aa): Change this error to an assert when we remove the ability to
    // set 'onerror' from created workers.
    return false;
  }

  JavaScriptWorkerInfo *wi = worker_info_[worker_id];
  wi->onerror_handler = *handler;
  return true;
}


bool PoolThreadsManager::CreateThread(const char16 *full_script,
                                      int *worker_id) {
  int new_worker_id = -1;
  JavaScriptWorkerInfo *wi = NULL;
  {
    MutexLock lock(&mutex_);

    // If this worker didn't intialize properly there won't be an hwnd, so
    // there's no point in starting a thread.
    if (!worker_info_[GetCurrentPoolWorkerId()]->thread_event_queue) {
      return false;
    }

    // Add a JavaScriptWorkerInfo entry.
    // Is okay not to undo this if code below fails. Behavior will be correct.
    worker_info_.push_back(new JavaScriptWorkerInfo);
    new_worker_id = static_cast<int>(worker_info_.size()) - 1;
    wi = worker_info_.back();
  }

  wi->full_script = kWorkerInsertedPreamble;
  wi->full_script += full_script;
  wi->threads_manager = this;

  // Setup notifier to know when thread init has finished.
  // Then create thread and wait for signal.
  Mutex mu;
  wi->js_init_mutex = &mu;
  wi->js_init_mutex->Lock();
  wi->js_init_signalled = false;

  wi->thread_pointer = PR_CreateThread(
                           PR_USER_THREAD, JavaScriptThreadEntry, // type, func
                           wi, PR_PRIORITY_NORMAL,   // arg, priority
                           PR_LOCAL_THREAD,          // scheduled by whom?
                           PR_UNJOINABLE_THREAD, 0); // joinable?, stack bytes
  if (wi->thread_pointer != NULL) {
    // thread needs to set onmessage handler before we continue
    wi->js_init_mutex->Await(Condition(&wi->js_init_signalled));
  }

  // cleanup notifier
  wi->js_init_mutex->Unlock();
  wi->js_init_mutex = NULL;

  if (wi->thread_pointer == NULL || !wi->js_init_ok) {
    return false; // failed
  }

  *worker_id = new_worker_id;
  return true; // succeeded
}


#ifdef DEBUG
void PoolThreadsManager::ForceGCCurrentThread() {
  MutexLock lock(&mutex_);

  int worker_id = GetCurrentPoolWorkerId();

  JavaScriptWorkerInfo *wi = worker_info_[worker_id];

  JS_GC(wi->onmessage_handler.context);
}
#endif // DEBUG


// Creates the JS engine, then pumps messages for the thread.
void PoolThreadsManager::JavaScriptThreadEntry(void *args) {
  // WARNING: must fire js_init_signalled even on failure, or the caller won't
  // continue.  So must fire event from a non-nested location (after JS init).

  //
  // Setup JS engine
  //
  assert(args);
  JavaScriptWorkerInfo *wi = static_cast<JavaScriptWorkerInfo*>(args);
  wi->threads_manager->AddWorkerRef();

  scoped_ptr<JsRunnerInterface> js_runner(NewJsRunner());

  bool js_init_succeeded = wi->threads_manager->InitWorkerThread(wi);
  if (js_init_succeeded) {
    js_init_succeeded = SetupJsRunner(js_runner.get(), wi);
  }

  // NOTE: things seem to work without calling InitXPCOM() here,
  // though code in /mozilla/netwerk/test/... [sic] did call it.

  // Tell caller we finished initializing, and indicate status
  wi->js_init_ok = js_init_succeeded;
  wi->js_init_mutex->Lock();
  wi->js_init_signalled = true;
  wi->js_init_mutex->Unlock();


  if (js_init_succeeded) {
    // Add JS code. If this fails, it will be reported via HandleError().
    js_runner->Start(wi->full_script);

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

  wi->threads_manager->ReleaseWorkerRef();

  // TODO(aa): Consider deleting wi here and setting PTM.worker_info_[i] to
  // NULL. This allows us to free up these thread resources sooner, and it
  // seems a little cleaner too.

  // PRThread functions don't return a value
}

bool PoolThreadsManager::SetupJsRunner(JsRunnerInterface *js_runner,
                                       JavaScriptWorkerInfo *wi) {
  if (!js_runner) { return false; }

  JsContextPtr cx;
  if (!js_runner->GetContext(&cx)) { return false; }

  // Add global Factory and WorkerPool objects into the namespace.
  //
  // The factory alone is not enough; GearsFactory.create(GearsWorkerPool)
  // would return a NEW PoolThreadsManager instance, but we want access to
  // the one that was previously created for the current page.

  scoped_ptr<GearsFactory> factory(new GearsFactory()); // will AddRef below
  factory->InitBaseManually(true, // is_worker
                            cx,
                            wi->threads_manager->page_security_origin(),
                            js_runner);
  // the worker's Factory inherits the capabilities from
  // the main thread that created this WorkerPool
  factory->is_permission_granted_ = true;
  factory->is_permission_value_from_user_ = true;

  // the worker needs to have a _new_ GearsWorkerPool object (to avoid issues
  // with XPConnect on main worker, vs JsContextWrapper on child worker)
  scoped_ptr<GearsWorkerPool>
      workerpool(new GearsWorkerPool()); // will AddRef below
  workerpool->InitBaseManually(true, // is_worker
                                cx,
                                wi->threads_manager->page_security_origin(),
                                js_runner);
  // but it must have the same underlying PoolThreadsManager
  workerpool->SetThreadsManager(wi->threads_manager);

  const nsIID workerpool_iface_id = GEARSWORKERPOOLINTERFACE_IID;
  if (!js_runner->AddGlobal(kWorkerInsertedWorkerPoolName,
                            workerpool.release(), // nsISupports
                            workerpool_iface_id)) {
    return false;
  }

  const nsIID factory_iface_id = GEARSFACTORYINTERFACE_IID;
  if (!js_runner->AddGlobal(kWorkerInsertedFactoryName,
                            factory.release(), // nsISupports
                            factory_iface_id)) {
    return false;
  }


  // Hook ourselves up to receive error messages
  js_runner->SetErrorHandler(wi->threads_manager);
  return true;
}

void PoolThreadsManager::ShutDown() {
  MutexLock lock(&mutex_);
  if (is_shutting_down_) {
    return;
  }

  is_shutting_down_ = true;

  // Tell all the running threads to stop.
  for (size_t i = 0; i < worker_info_.size(); ++i) {
    JavaScriptWorkerInfo *wi = worker_info_[i];

    if (wi->thread_pointer && wi->thread_event_queue) {
      // Send a dummy message in case the thread is waiting.
      ThreadsEvent *event = new ThreadsEvent(wi, EVENT_TYPE_MESSAGE);
      wi->thread_event_queue->InitEvent(
          &event->e, nsnull, // event, owner
          reinterpret_cast<PLHandleEventProc>(OnReceiveThreadsEvent),
          reinterpret_cast<PLDestroyEventProc>(OnDestroyThreadsEvent));
      wi->thread_event_queue->PostEvent(&event->e);

      // TODO(cprince): Better handle any thread spinning in a busy loop.
      // Ideas: (1) force it to the lowest priority level here, or (2) interrupt
      // the JS engine externally (see IActiveScript::InterruptScriptThread
      // on IE, JS_THREADSAFE for Firefox).  We cannot simply terminate it;
      // that can leave us in a bad state (e.g. mutexes locked forever).
    }
  }
}

void PoolThreadsManager::AddWorkerRef() {
  AtomicIncrement(&num_workers_, 1);
}

void PoolThreadsManager::ReleaseWorkerRef() {
  if (AtomicIncrement(&num_workers_, -1) == 0) {
    delete this;
  }
}
