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

#include "ff/genfiles/database.h"
#include "ff/genfiles/localserver.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/firefox/factory.h"
#include "gears/workerpool/common/workerpool_utils.h"
#include "gears/workerpool/firefox/js_wrapper.h"
#include "gears/workerpool/firefox/workerpool.h"


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

struct OnmessageHandler {
  JsToken function;
  JsContextPtr context;
};

struct JavaScriptWorkerInfo {
  // Our code assumes some items begin cleared. Zero all members w/o ctors.
  JavaScriptWorkerInfo()
      : threads_manager(NULL), // onmessage_handler(),
        js_init_mutex(NULL), js_init_signalled(false), js_init_ok(false),
        thread_pointer(NULL), js_engine_context(NULL),
        alloc_workerpool(NULL), alloc_factory(NULL), alloc_js_wrapper(NULL) {}

  //
  // These fields are used by all workers (main + children)
  // and should therefore be initialized when a message handler is set.
  //
  PoolThreadsManager *threads_manager; // NULL means struct is invalidated
  OnmessageHandler onmessage_handler;
  nsCOMPtr<nsIEventQueue> thread_event_queue;
  std::queue< std::pair<std::string16, int> > message_queue;

  // 
  // These fields are used only with descendants (i.e., workers).
  //
  nsString full_script;            // store the script code and error in this
  std::string16 last_script_error; // struct to cross the OS thread boundary
  Mutex  *js_init_mutex; // protects: js_init_signalled
  bool    js_init_signalled;
  bool    js_init_ok; // owner: thread before signal, caller after signal
  PRThread  *thread_pointer;
  JSContext *js_engine_context;
  GearsWorkerPool *alloc_workerpool; // (created in thread) to access PoolThreadsManager
  GearsFactory *alloc_factory; // (created in thread) to create new objects
  JsContextWrapper *alloc_js_wrapper; // (created in thread)
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
    delete threads_manager_;
  }
}

void GearsWorkerPool::UseExternalThreadsManager(PoolThreadsManager *manager) {
  assert(!threads_manager_);
  threads_manager_ = manager;
  // leave owns_threads_manager_ set to false
  assert(!owns_threads_manager_);
}


NS_IMETHODIMP GearsWorkerPool::SetOnmessage(
                                   WorkerOnmessageHandler *in_value) {
  if (!threads_manager_) {
    // We are the first worker on this page, so create a manager.
    // This only happens from the main JS thread. Worker threads call
    // UseExternalThreadsManager to setup threads_manager_.
    threads_manager_ = new PoolThreadsManager(this->EnvPageSecurityOrigin());
    owns_threads_manager_ = true;
  }

  OnmessageHandler onmessage_handler = {0};
  JsParamFetcher js_params(this);
  if (!js_params.GetAsToken(0, &onmessage_handler.function)) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter"));
  }
  onmessage_handler.context = js_params.GetContextPtr();

  // "Root" the handler so it cannot get garbage collected.
  if (!RootJsToken(onmessage_handler.context, onmessage_handler.function)) {
    RETURN_EXCEPTION(STRING16(L"Error rooting JS value."));
  }

  if (!threads_manager_->SetCurrentThreadMessageHandler(&onmessage_handler)) {
    RETURN_EXCEPTION(STRING16(L"Error setting onmessage handler."));
  }

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::GetOnmessage(
                                   WorkerOnmessageHandler **out_value) {
  *out_value = NULL; // TODO(cprince): implement this (requires some changes)
                     // See code in nsXMLHttpRequest for NS_IF_ADDREF usage.
                     // May be able to return a raw JsToken for main and worker?
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::CreateWorker(const nsAString &full_script,
                                            PRInt32 *retval) {
  if (!threads_manager_) {
    // if this happens on main worker, maybe user didn't set onmessage first
    RETURN_EXCEPTION(STRING16(L"Set onmessage before calling createWorker."));
  }

  int worker_id_temp; // protects against modifying output param on failure
  std::string16 script_error;
  bool succeeded = threads_manager_->CreateThread(full_script,
                                                  &worker_id_temp,
                                                  &script_error);
  if (!succeeded) {
    std::string16 msg = STRING16(L"Worker creation failed.");
    msg += STRING16(L" ERROR: ");
    msg += script_error;
    RETURN_EXCEPTION(msg.c_str());
  }

  *retval = worker_id_temp;
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsWorkerPool::SendMessage(const nsAString &message_string,
                                           PRInt32 dest_worker_id) {
  if (!threads_manager_) {
    // if this happens on main worker, maybe user didn't set onmessage first
    RETURN_EXCEPTION(STRING16(L"GearsWorkerPool has not been setup correctly."));
  }

  bool succeeded = threads_manager_->PutPoolMessage(message_string,
                                                    dest_worker_id);
  if (!succeeded) {
    RETURN_EXCEPTION(STRING16(L"SendMessage failed."));
  }
  RETURN_NORMAL();
}


#ifdef DEBUG
NS_IMETHODIMP GearsWorkerPool::ForceGC() {
  threads_manager_->ForceGCCurrentThread();
  RETURN_NORMAL();
}
#endif // DEBUG



//
// ThreadsEvent -- used for Firefox cross-thread communication.
//

struct ThreadsEvent {
  PLEvent e; // PLEvent must be first field
  JavaScriptWorkerInfo *wi;
};


// Called when the event is received.
void* OnReceiveThreadsEvent(ThreadsEvent *event) {
  JavaScriptWorkerInfo *wi = event->wi;

  // See if the workerpool has been invalidated (as on termination).
  if (wi->threads_manager->is_pool_stopping_) { return NULL; }

  // Retrieve message information
  std::string16 message_string;
  int src_worker_id;
  //bool succeeded =  // comment out until we use it, to avoid compiler warning
  wi->threads_manager->GetPoolMessage(&message_string, &src_worker_id);

  // Invoke JavaScript onmessage handler
  // (different paths because each child JS worker gives us a jsval,
  // while the main JS worker gives us a WorkerOnmessageHandler*)
  JSString *message_arg_jsstring = JS_NewUCStringCopyZ(
      wi->onmessage_handler.context,
      message_string.c_str()); // TODO(cprince): ensure freeing memory
  uintN argc = 2;
  jsval argv[] = { STRING_TO_JSVAL(message_arg_jsstring),
                   INT_TO_JSVAL(src_worker_id) };
  jsval js_retval;
  //JSBool js_ok =  // comment out until we use it, to avoid compiler warning
  JS_CallFunctionValue( // goes to js_InternalInvoke()
      wi->onmessage_handler.context,
      JS_GetGlobalObject(wi->onmessage_handler.context),
      wi->onmessage_handler.function, argc, argv, &js_retval);

  return NULL; // retval only matters for PostSynchronousEvent()
}

// Called when the event has been processed.
static void OnDestroyThreadsEvent(ThreadsEvent *event) {
  delete event;
}



//
// PoolThreadsManager -- handles threading and JS engine setup.
//

PoolThreadsManager::PoolThreadsManager(
                        const SecurityOrigin &page_security_origin)
    : is_pool_stopping_(false),
      page_security_origin_(page_security_origin) {

  // Add a JavaScriptWorkerInfo entry for the main worker.
  worker_info_.push_back(new JavaScriptWorkerInfo);

  // Monitor 'onunload' to shutdown threads when the page goes away.
  //
  // A thread that keeps running after the page changes can cause odd problems,
  // if it continues to send messages. (This can happen if it busy-loops.)  On
  // Firefox, such a thread triggered the Print dialog after the page changed!
  //
  // TODO(cprince): Expose HTML event notifications to threads other than
  // the main thread.  If a worker thread creates a _new_ workerpool + thread,
  // the latter thread will not get destroyed. (TBD: do we need to make sure
  // thread hierarchies get cleaned up in a particular order?)
  unload_monitor_.reset(new HtmlEventMonitor(kEventUnload,
                                             HandleEventUnload, this));
  nsCOMPtr<nsIDOMEventTarget> event_source;
  if (NS_SUCCEEDED(DOMUtils::GetWindowEventTarget(
                                 getter_AddRefs(event_source)))) {
    unload_monitor_->Start(event_source);
  }
}

PoolThreadsManager::~PoolThreadsManager() {
  StopAllCreatedThreads();
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
  int size = static_cast<int>(worker_id_to_os_thread_id_.size());
  for (int i = 0; i < size; ++i) {
    if (worker_id_to_os_thread_id_[i] == os_thread_id) {
      return i;
    }
  }
  // not found, so add a new entry
  worker_id_to_os_thread_id_.push_back(os_thread_id);
  return size;
}


bool PoolThreadsManager::PutPoolMessage(const nsAString &message_string,
                                        int dest_worker_id) {
  MutexLock lock(&mutex_);
  if (is_pool_stopping_) { return true; } // fail w/o triggering exceptions

  int src_worker_id = GetCurrentPoolWorkerId();

  // check for valid dest_worker_id
  if (dest_worker_id < 0 ||
      dest_worker_id >= static_cast<int>(worker_info_.size())) {
    return false;
  }
  JavaScriptWorkerInfo *dest_wi = worker_info_[dest_worker_id];
  if (NULL == dest_wi || NULL == dest_wi->threads_manager) {
    return false;
  }

  // put the message in our internal queue
  const PRUnichar *msg_chars;
  NS_StringGetData(message_string, &msg_chars);
  std::pair<std::string16, int> msg(static_cast<const char16*>(msg_chars),
                                    src_worker_id);  // copies the string
  dest_wi->message_queue.push(msg);

  // notify the receiving thread
  ThreadsEvent *event = new ThreadsEvent();
  event->wi = dest_wi;
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
  if (is_pool_stopping_) { return true; } // fail w/o triggering exceptions

  int current_worker_id = GetCurrentPoolWorkerId();
  JavaScriptWorkerInfo *wi = worker_info_[current_worker_id];

  if (wi->message_queue.empty()) {
    return false;
  }
  *message_string = wi->message_queue.front().first;
  *src_worker_id = wi->message_queue.front().second;
  wi->message_queue.pop();
  return true;
}


// TODO(cprince): consider returning success/failure here (and in IE code)
bool PoolThreadsManager::SetCurrentThreadMessageHandler(
    OnmessageHandler *handler) {

  MutexLock lock(&mutex_);
  if (is_pool_stopping_) { return true; } // fail w/o triggering exceptions

  int worker_id = GetCurrentPoolWorkerId();

  // validate our internal data structure
  if (static_cast<int>(worker_info_.size()) < (worker_id + 1)) {
    return false;
  }

  JavaScriptWorkerInfo *wi = worker_info_[worker_id];

  wi->onmessage_handler = *handler;
  wi->threads_manager = this;

  // Get the event queue for this worker.
  // This is how we service JS worker messages synchronously relative to other
  // JS execution.
  //
  // Firefox has a single event queue per thread, and messages are sent to this
  // shared queue.  (Compare this to the Win32 model where events are sent to
  // multiple HWNDs, which share an event queue internally.)  So first check to
  // see if a thread event queue exists. The main worker will already have one,
  // but child workers will not (unless they already called this function).

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


bool PoolThreadsManager::CreateThread(const nsAString &full_script,
                                      int *worker_id,
                                      std::string16 *script_error) {
  int new_worker_id = -1;
  JavaScriptWorkerInfo *wi = NULL;
  {
    MutexLock lock(&mutex_);
    if (is_pool_stopping_) { return true; } // fail w/o triggering exceptions

    // Add a JavaScriptWorkerInfo entry.
    // Is okay not to undo this if code below fails. Behavior will be correct.
    worker_info_.push_back(new JavaScriptWorkerInfo);
    new_worker_id = static_cast<int>(worker_info_.size()) - 1;
    wi = worker_info_.back();
  }

  wi->full_script = kWorkerInsertedPreamble; // calls NS_StringCopy
  wi->full_script.Append(full_script);
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
    // thread needs to set onessage handler before we continue
    wi->js_init_mutex->Await(Condition(&wi->js_init_signalled));
  }

  // cleanup notifier
  wi->js_init_mutex->Unlock();
  wi->js_init_mutex = NULL;

  if (wi->thread_pointer == NULL || !wi->js_init_ok) {
    if (script_error) {
      *script_error = wi->last_script_error;
    }
    return false; // failed
  }

  *worker_id = new_worker_id;
  return true; // succeeded
}


#ifdef DEBUG
void PoolThreadsManager::ForceGCCurrentThread() {
  MutexLock lock(&mutex_);
  if (is_pool_stopping_) { return; }

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
  // NOTE: things seem to work without calling InitXPCOM() here,
  // though code in /mozilla/netwerk/test/... [sic] did call it.
  bool js_init_succeeded = InitJavaScriptEngine(wi);

  //
  // Tell caller we finished initializing, and indicate status
  //
  wi->js_init_ok = js_init_succeeded;
  wi->js_init_mutex->Lock();
  wi->js_init_signalled = true;
  wi->js_init_mutex->Unlock();

  //
  // Pump messages
  //
  if (js_init_succeeded) {
    assert(wi->thread_event_queue);
    while (1) {
      // (based on sample code in /mozilla/netwerk/test/... [sic])
      PLEvent *event;
      wi->thread_event_queue->WaitForEvent(&event);
      // check flag after waiting, before handling (see StopAllCreatedThreads)
      if (wi->threads_manager->is_pool_stopping_) { break; } 
      wi->thread_event_queue->HandleEvent(event);
    }
  }

  //
  // Cleanup
  //
  if (js_init_succeeded) {
    JS_DestroyContext(wi->js_engine_context);
    wi->js_engine_context = NULL;
  }

  // PRThread functions don't return a value
}


void JS_DLL_CALLBACK PoolThreadsManager::JsErrorHandler(JSContext *cx,
                                                        const char *message,
                                                        JSErrorReport *report) {
  JavaScriptWorkerInfo *wi = static_cast<JavaScriptWorkerInfo*>(
                                 JS_GetContextPrivate(cx));
  if (wi && report && report->ucmessage) { // string16 faults on NULL assignment
    wi->last_script_error = report->ucmessage;
  }
}

class JS_DestroyContextFunctor {
 public:
  inline void operator()(JSContext* x) const {
    if (x != NULL) { JS_DestroyContext(x); }
  }
};
typedef scoped_token<JSContext*, JS_DestroyContextFunctor> scoped_jscontext_ptr;

bool PoolThreadsManager::InitJavaScriptEngine(JavaScriptWorkerInfo *wi) {
  JSBool js_ok;
  bool succeeded;

  // To cleanup after failures we use scoped objects to manage everything that
  // should be destroyed.  On success we take ownership to avoid cleanup.

  // These structs are static because they must live for duration of JS engine.
  // SpiderMonkey README also suggests using static for one-off objects.
  static JSClass global_class = {
      "Global", 0, // name, flags
      JS_PropertyStub, JS_PropertyStub,  // defineProperty, deleteProperty
      JS_PropertyStub, JS_PropertyStub, // getProperty, setProperty
      JS_EnumerateStub, JS_ResolveStub, // enum, resolve
      JS_ConvertStub, JS_FinalizeStub // convert, finalize
  };


  //
  // Instantiate a JavaScript engine
  //

  // Create a new runtime.  If we instead use xpc/RuntimeService to get a
  // runtime, strange things break (like eval).
  const int kRuntimeMaxBytes = 64 * 1024 * 1024; // mozilla/.../js.c uses 64 MB
  JSRuntime *rt = JS_NewRuntime(kRuntimeMaxBytes);
  if (!rt) { return false; }

  const int kContextStackChunkSize = 1024; // Firefox often uses 1024;
                                           // also see js/src/readme.html

  scoped_jscontext_ptr cx(JS_NewContext(rt, kContextStackChunkSize));
  if (!cx.get()) { return false; }
  // VAROBJFIX is recommended in /mozilla/js/src/jsapi.h
  JS_SetOptions(cx.get(), JS_GetOptions(cx.get()) | JSOPTION_VAROBJFIX);

  // JS_SetErrorReporter takes a static callback, so we need
  // JS_SetContextPrivate to later save the error in a per-worker location
  JS_SetErrorReporter(cx.get(), JsErrorHandler);
  JS_SetContextPrivate(cx.get(), static_cast<void*>(wi));
#ifdef DEBUG
  // must set this here to allow workerPool.forceGC() during child init
  wi->js_engine_context = cx.get();
#endif

  JSObject *global_obj = JS_NewObject(cx.get(), &global_class, 0, 0);

  if (!global_obj) { return false; }
  js_ok = JS_InitStandardClasses(cx.get(), global_obj);
  if (!js_ok) { return false; }
  // Note: an alternative is to lazily define the "standard classes" (which
  // include things like eval).  To do that, change JS_InitStandardClasses
  // to JS_SetGlobalObject, and add handlers for Enumerate and Resolve in
  // global_class.  See /mozilla/js/src/js.c for sample code.


  //
  // Define classes in the JSContext
  //

  // first need to create a JsWrapperManager for this thread
  scoped_ptr<JsContextWrapper> js_wrapper(new JsContextWrapper(cx.get(),
                                                               global_obj));

  struct {
    const nsIID iface_id;
    JSObject *proto_obj; // gets set by code below
  } classes[] = {
    // TODO(cprince): Unify the interface lists here and in GearsFactory.
    // Could share code, or could query GearsFactory.
    {GEARSFACTORYINTERFACE_IID, NULL},
    // workerpool
    {GEARSWORKERPOOLINTERFACE_IID, NULL},
    // database
    {GEARSDATABASEINTERFACE_IID, NULL},
    {GEARSRESULTSETINTERFACE_IID, NULL},
    // localserver
    {GEARSLOCALSERVERINTERFACE_IID, NULL},
    {GEARSMANAGEDRESOURCESTOREINTERFACE_IID, NULL},
    {GEARSRESOURCESTOREINTERFACE_IID, NULL},
    // GEARSFILESUBMITTERINTERFACE_IID can never be created in a child worker
  };
  const int num_classes = sizeof(classes) / sizeof(classes[0]);

  for (int i = 0; i < num_classes; ++i) {
    // passing NULL for class_id and class_name prevents child workers
    // from using "new CLASSNAME"
    succeeded = js_wrapper->DefineClass(&classes[i].iface_id,
                                        NULL, // class_id
                                        NULL, // class_name
                                        &classes[i].proto_obj);
    if (!succeeded) { return false; }
  }


  //
  // Add global Factory and WorkerPool objects into the namespace.
  // 
  // The factory alone is not enough; GearsFactory.create(GearsWorkerPool)
  // would return a NEW PoolThreadsManager instance, but we want access to
  // the one that was previously created for the current page.
  //

  scoped_ptr<GearsFactory> factory(new GearsFactory()); // will AddRef below
  factory->InitBaseManually(true, // is_worker
                            cx.get(),
                            wi->threads_manager->page_security_origin());
  // the worker's Factory inherits the capabilities from
  // the main thread that created this WorkerPool
  factory->is_permission_granted_ = true;
  factory->is_permission_value_from_user_ = true;

  // the worker needs to have a _new_ GearsWorkerPool object (to avoid issues
  // with XPConnect on main worker, vs JsContextWrapper on child worker)
  scoped_ptr<GearsWorkerPool> workerpool(new GearsWorkerPool()); // will AddRef below
  workerpool->InitBaseManually(true, // is_worker
                               cx.get(),
                               wi->threads_manager->page_security_origin());
  // but it must have the same underlying PoolThreadsManager
  workerpool->UseExternalThreadsManager(wi->threads_manager);




  const int workerpool_class_index = 1;
#ifdef DEBUG
  const nsIID workerpool_iface_id = GEARSWORKERPOOLINTERFACE_IID;
  assert(classes[workerpool_class_index].iface_id.Equals(workerpool_iface_id));
#endif
  succeeded = js_wrapper->DefineGlobal(classes[workerpool_class_index].proto_obj,
                                       workerpool.get(), // nsISupports
                                       kWorkerInsertedWorkerPoolNameAscii);
  if (!succeeded) { return false; }

  const int factory_class_index = 0;
#ifdef DEBUG
  const nsIID factory_iface_id = GEARSFACTORYINTERFACE_IID;
  assert(classes[factory_class_index].iface_id.Equals(factory_iface_id));
#endif
  succeeded = js_wrapper->DefineGlobal(classes[factory_class_index].proto_obj,
                                       factory.get(), // nsISupports
                                       kWorkerInsertedFactoryNameAscii);
  if (!succeeded) { return false; }


  //
  // Add script code to the engine instance
  //

  uintN line_number_start = 0;
  JSScript *compiled_script = JS_CompileUCScript(cx.get(), global_obj,
                                                 wi->full_script.get(),
                                                 wi->full_script.Length(),
                                                 "script", line_number_start);
  if (!compiled_script) { return false; }

  // we must root any script returned by JS_Compile* (see jsapi.h)
  JSObject *compiled_script_obj = JS_NewScriptObject(cx.get(), compiled_script);
  if (!compiled_script_obj) { return false; }
  if (!RootJsToken(cx.get(), OBJECT_TO_JSVAL(compiled_script_obj))) {
    return false;
  }


  //
  // Start the engine running
  //

  jsval return_string;
  js_ok = JS_ExecuteScript(cx.get(), global_obj, compiled_script,
                           &return_string);
  if (!js_ok) { return false; }

  // NOTE: at this point, the JS code has returned, and it should have set
  // an onmessage handler.  (If it didn't, the worker is most likely cut off
  // from other workers.  There are ways to prevent this, but they are poor.)

  // Success! Tell the code not to release our objects,
  // and save values for cleanup later.
  wi->js_engine_context = cx.release();
  wi->alloc_js_wrapper = js_wrapper.release();
  wi->alloc_workerpool = workerpool.release();
  NS_ADDREF(wi->alloc_workerpool);
  wi->alloc_factory = factory.release();
  NS_ADDREF(wi->alloc_factory);

  // Worker init should have set an onmessage handler, and the JS context value
  // should match the context we created for this child worker.
  assert(wi->js_engine_context == wi->onmessage_handler.context);

#ifdef DEBUG
  // After JS_ExecuteScript() should always be a "safe" time to garbage collect.
  // Do it here to trigger potential GC bugs in our code.
  JS_GC(wi->js_engine_context); 
#endif

  return true; // succeeded
}

void PoolThreadsManager::HandleEventUnload(void *user_param) {
  reinterpret_cast<PoolThreadsManager*>(user_param)->StopAllCreatedThreads();
}

void PoolThreadsManager::StopAllCreatedThreads() {
  MutexLock lock(&mutex_);
  if (is_pool_stopping_) { return; }

  is_pool_stopping_ = true;

  for (size_t i = 0; i < worker_info_.size(); ++i) {
    JavaScriptWorkerInfo *wi = worker_info_[i];

    // If the worker is a created thread...
    if (wi->thread_pointer) {
      // Send a dummy message in case the thread is waiting.
      ThreadsEvent *event = new ThreadsEvent();
      event->wi = wi;
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
