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

struct OnmessageHandler {
  JsToken function;
  JsContextPtr context;
};

struct JavaScriptWorkerInfo {
  // Our code assumes some items begin cleared. Zero all members w/o ctors.
  JavaScriptWorkerInfo()
      : threads_manager(NULL), // onmessage_handler(),
        js_init_mutex(NULL), js_init_signalled(false), js_init_ok(false),
        thread_pointer(NULL) {}

  //
  // These fields are used by all workers in pool (root + descendants).
  //
  PoolThreadsManager *threads_manager;
  OnmessageHandler onmessage_handler;
  nsCOMPtr<nsIEventQueue> thread_event_queue;
  std::queue< std::pair<std::string16, int> > message_queue;

  //
  // These fields are used only by created workers (descendants).
  //
  std::string16 full_script;        // Store the script code and error in this
  std::string16 last_script_error;  // struct to cross the OS thread boundary.
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

NS_IMETHODIMP GearsWorkerPool::SetOnmessage(
                                   WorkerOnmessageHandler *in_value) {
  Initialize();

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
  std::string16 script_error;
  bool succeeded = threads_manager_->CreateThread(full_script.c_str(),
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
  Initialize();

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

void GearsWorkerPool::HandleEventUnload(void *user_param) {
  GearsWorkerPool *wp = static_cast<GearsWorkerPool*>(user_param);
  if (wp->threads_manager_) {
    wp->threads_manager_->ShutDown();
  }
}

void GearsWorkerPool::Initialize() {
  if (!threads_manager_) {
    SetThreadsManager(new PoolThreadsManager(this->EnvPageSecurityOrigin()));
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

struct ThreadsEvent {
  ThreadsEvent(JavaScriptWorkerInfo *worker_info) : wi(worker_info) {
    wi->threads_manager->AddWorkerRef();
  }

  ~ThreadsEvent() {
    wi->threads_manager->ReleaseWorkerRef();
  }

  PLEvent e; // PLEvent must be first field
  JavaScriptWorkerInfo *wi;
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

  // Invoke JavaScript onmessage handler
  // (different paths because each child JS worker gives us a jsval,
  // while the main JS worker gives us a WorkerOnmessageHandler*)
  JSString *message_arg_jsstring = JS_NewUCStringCopyZ(
      wi->onmessage_handler.context,
      reinterpret_cast<const jschar *>(
          message_string.c_str())); // TODO(cprince): ensure freeing memory
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
    : num_workers_(0), 
      is_shutting_down_(false),
      page_security_origin_(page_security_origin) {

  // Add a JavaScriptWorkerInfo entry for the owning worker.
  worker_info_.push_back(new JavaScriptWorkerInfo);
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
  ThreadsEvent *event = new ThreadsEvent(dest_wi);
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


// TODO(cprince): consider returning success/failure here (and in IE code)
bool PoolThreadsManager::SetCurrentThreadMessageHandler(
    OnmessageHandler *handler) {

  MutexLock lock(&mutex_);

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


bool PoolThreadsManager::CreateThread(const char16 *full_script,
                                      int *worker_id,
                                      std::string16 *script_error) {
  int new_worker_id = -1;
  JavaScriptWorkerInfo *wi = NULL;
  {
    MutexLock lock(&mutex_);

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

  JsContextPtr cx;

  bool js_init_succeeded = js_runner->GetContext(&cx);

  if (js_init_succeeded) {
    // Add global Factory and WorkerPool objects into the namespace.
    //
    // The factory alone is not enough; GearsFactory.create(GearsWorkerPool)
    // would return a NEW PoolThreadsManager instance, but we want access to
    // the one that was previously created for the current page.

    scoped_ptr<GearsFactory> factory(new GearsFactory()); // will AddRef below
    factory->InitBaseManually(true, // is_worker
                              cx,
                              wi->threads_manager->page_security_origin(),
                              js_runner.get());
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
                                 js_runner.get());
    // but it must have the same underlying PoolThreadsManager
    workerpool->SetThreadsManager(wi->threads_manager);

    const nsIID workerpool_iface_id = GEARSWORKERPOOLINTERFACE_IID;
    if (!js_runner->AddGlobal(kWorkerInsertedWorkerPoolName,
                              workerpool.release(), // nsISupports
                              workerpool_iface_id)) {
      js_init_succeeded = false;
    }

    const nsIID factory_iface_id = GEARSFACTORYINTERFACE_IID;
    if (!js_runner->AddGlobal(kWorkerInsertedFactoryName,
                              factory.release(), // nsISupports
                              factory_iface_id)) {
      js_init_succeeded = false;
    }

    // Run the script
    if (!js_runner->Start(wi->full_script)) {
      js_init_succeeded = false;
    }
  }

  wi->last_script_error = js_runner->GetLastScriptError();

  // NOTE: things seem to work without calling InitXPCOM() here,
  // though code in /mozilla/netwerk/test/... [sic] did call it.

  //
  // Tell caller we finished initializing, and indicate status
  //
  wi->js_init_ok = js_init_succeeded;
  wi->js_init_mutex->Lock();
  wi->js_init_signalled = true;
  wi->js_init_mutex->Unlock();

  // Pump messages
  if (js_init_succeeded) {
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

  //
  // Cleanup
  //
  // TODO(aa): Alas, none of this results in Gears modules created in workers
  // getting deleted. Therefore nested workers never stop or get deleted. I
  // don't understand this!

  wi->threads_manager->ReleaseWorkerRef();

  // TODO(aa): Consider deleting wi here and setting PTM.worker_info_[i] to
  // NULL. This allows us to free up these thread resources sooner, and it
  // seems a little cleaner too.

  // PRThread functions don't return a value
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
      ThreadsEvent *event = new ThreadsEvent(wi);
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
