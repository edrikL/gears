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
// JavaScript worker threads in Internet Explorer.
//
// Implementation details:
//
// CROSS-THREAD COMMUNICATION: Each thread has a (non-visible) window.  For
// every Gears message sent to that thread, a Win32 message is posted to the
// thread's window.
// This is necessary so we can wake the "root" JS thread on a page. The browser
// owns that thread, but we need a way to tell it to run our JS
// callback (onmessage) -- and only when the JS engine exits to the top level.
// Window messages give just the right behavior, because senders identify
// a window by its handle, but multiple windows can be associated with a
// thread, and messages sent to a window get queued at the thread level.
// BTW, I'm told IE6 XmlHttpRequest also uses window messages.
// We didn't need to use the same scheme for the non-root (created) threads,
// but it makes sense to be consistent.

#include <assert.h> // TODO(cprince): use DCHECK() when have google3 logging
#include <queue>
#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/scoped_win32_handles.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#include "gears/base/ie/factory.h"
#include "gears/workerpool/common/workerpool_utils.h"
#include "gears/workerpool/ie/workerpool.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"


//
// JavaScriptWorkerInfo -- contains the state of each JavaScript worker.
//

struct JavaScriptWorkerInfo {
  // Our code assumes some items begin cleared. Zero all members w/o ctors.
  JavaScriptWorkerInfo()
      : threads_manager(NULL), onmessage_handler(NULL), message_hwnd(0),
        js_init_mutex(NULL), js_init_signalled(false), js_init_ok(false),
        thread_handle(INVALID_HANDLE_VALUE) {}

  //
  // These fields are used by all workers (main + children)
  // and should therefore be initialized when a message handler is set.
  //
  PoolThreadsManager *threads_manager;
  IDispatch *onmessage_handler; // set by thread, not by thread creator
  HWND message_hwnd;
  std::queue< std::pair<std::string16, int> > message_queue;

  // 
  // These fields are used only with descendants (i.e., workers).
  //
  CComBSTR full_script; // store the script code and error in this
  std::string16 last_script_error; // struct to cross the OS thread boundary
  Mutex *js_init_mutex; // protects: js_init_signalled
  bool js_init_signalled;
  bool js_init_ok; // owner: thread before signal, caller after signal
  SAFE_HANDLE thread_handle;
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
    JavaScriptWorkerInfo *wi = threads_manager_->GetCurrentThreadWorkerInfo();
    assert(wi);

    // message_hwnd can be null if initialization never occurred or if it
    // failed.
    if (wi->message_hwnd) {
      BOOL success = DestroyWindow(wi->message_hwnd);
      assert(success);
    }

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

// NOTE: get_onmessage() is not yet implemented (low priority)

STDMETHODIMP GearsWorkerPool::createWorker(const BSTR *full_script,
                                           int *worker_id) {
  Initialize();

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

  *worker_id = worker_id_temp;
  RETURN_NORMAL();
}

STDMETHODIMP GearsWorkerPool::sendMessage(const BSTR *message_string,
                                          int dest_worker_id) {
  Initialize();

  bool succeeded = threads_manager_->PutPoolMessage(message_string,
                                                    dest_worker_id);
  if (!succeeded) {
    RETURN_EXCEPTION(STRING16(L"SendMessage failed."));
  }
  RETURN_NORMAL();
}

STDMETHODIMP GearsWorkerPool::put_onmessage(IDispatch *in_value) {
  Initialize();

  if (!threads_manager_->SetCurrentThreadMessageHandler(in_value)) {
    RETURN_EXCEPTION(STRING16(L"Error setting onmessage handler."));
  }

  RETURN_NORMAL();
}

#ifdef DEBUG
STDMETHODIMP GearsWorkerPool::forceGC() {
  // TODO(cprince): Maybe implement this someday, for completeness.
  // forceGC() was added for finding garbage collection bugs on Firefox,
  // where we control the JS engine more manually.  There are rumors of
  // an undocumented CollectGarbage() method in jscript.dll.
  RETURN_NORMAL();
}
#endif

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
  //
  // TODO(cprince): Expose HTML event notifications to threads other than
  // the main thread.  If a worker thread creates a _new_ workerpool + thread,
  // the latter thread will not get destroyed. (TBD: do we need to make sure
  // thread hierarchies get cleaned up in a particular order?)
  if (!EnvIsWorker() && unload_monitor_ == NULL) {
    unload_monitor_.reset(new HtmlEventMonitor(kEventUnload,
                                               HandleEventUnload, this));
    CComPtr<IHTMLWindow3> event_source;
    IUnknown *site = this->EnvPageIUnknownSite();
    assert(site);
    HRESULT hr = ActiveXUtils::GetHtmlWindow3(site, &event_source);
    assert(SUCCEEDED(hr));
    unload_monitor_->Start(event_source);
  }
}


//
// PageTheadsManager -- handles threading and JS engine setup.
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
  DWORD os_thread_id = ::GetCurrentThreadId();

  // lookup OS-defined id in list of known mappings
  // (linear scan is fine because number of threads per pool will be small)
  int size = static_cast<int>(worker_id_to_os_thread_id_.size());
  for (int i = 0; i < size; ++i) {
    if (worker_id_to_os_thread_id_[i] == os_thread_id) {
      return i;
    }
  }
  worker_id_to_os_thread_id_.push_back(os_thread_id);
  return size;
}

bool PoolThreadsManager::PutPoolMessage(const BSTR *message_string,
                                        int dest_worker_id) {
  MutexLock lock(&mutex_);

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
  CComBSTR message_bstr(*message_string);
  std::pair<std::string16, int> msg((char16*)message_bstr, src_worker_id);
  dest_wi->message_queue.push(msg);

  // notify the receiving worker
  PostMessage(dest_wi->message_hwnd, WM_WORKERPOOL_ONMESSAGE, 0,
              reinterpret_cast<LPARAM>(dest_wi));

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



bool PoolThreadsManager::SetCurrentThreadMessageHandler(IDispatch *handler) {
  MutexLock lock(&mutex_);

  int worker_id = GetCurrentPoolWorkerId();

  // validate our internal data structure
  if (static_cast<int>(worker_info_.size()) < (worker_id + 1)) {
    return false;
  }

  handler->AddRef(); // TODO(cprince): Release() later
  JavaScriptWorkerInfo *wi = worker_info_[worker_id];

  wi->onmessage_handler = handler;
  wi->threads_manager = this;

  // Also create a message-only window for every worker (including parent).
  // This is how we service JS worker messages synchronously relative to other
  // JS execution.
  //
  // Can create a message-only window by specifying HWND_MESSAGE as the parent.
  // TODO(cprince): an alternative is to set another message-only window as the
  // parent; see if that helps with cleanup.

  // TODO(cprince): don't run this code again if caller sets handler twice

  static const char16* kWindowClassName = STRING16(L"JsWorkerCommWnd");

  // hinstance should be the Scour DLL's base address, not the process handle
  //   (http://blogs.msdn.com/oldnewthing/archive/2005/04/18/409205.aspx)
  HMODULE hmodule = _AtlBaseModule.GetModuleInstance();

  WNDCLASSEX wc = {0};
  wc.cbSize        = sizeof(wc);
  wc.lpfnWndProc   = ThreadWndProc;
  wc.hInstance     = hmodule;
  wc.lpszClassName = kWindowClassName;
  // use 0 for all other fields
  RegisterClassEx(&wc);

  HWND message_hwnd;
  message_hwnd = CreateWindow(kWindowClassName, // class name
                              kWindowClassName, // window name
                              0 ,               // style
                              0, 0,             // pos
                              0, 0,             // size
                              HWND_MESSAGE,     // parent
                              0,                // ID if a child, else hMenu
                              hmodule,          // module instance
                              NULL);            // fooCREATESTRUCT param
  // TODO(cprince): check for failure
  wi->message_hwnd = message_hwnd;

  return true;
}

bool PoolThreadsManager::CreateThread(const BSTR *full_script,
                                      int *worker_id,
                                      std::string16 *script_error) {
  int new_worker_id = -1;
  JavaScriptWorkerInfo *wi = NULL;
  {
    MutexLock lock(&mutex_);
    if (is_shutting_down_) { return true; } // fail w/o triggering exceptions

    // Add a JavaScriptWorkerInfo entry.
    // Is okay not to undo this if code below fails. Behavior will be correct.
    worker_info_.push_back(new JavaScriptWorkerInfo);
    new_worker_id = static_cast<int>(worker_info_.size()) - 1;
    wi = worker_info_.back();
  }

  // The code below should not access shared data structures, we
  // only synchronize the block above, which modifies the shared 
  // worker_info_ vector.

  wi->full_script = kWorkerInsertedPreamble;
  wi->full_script.Append(*full_script);
  wi->threads_manager = this;

  // Setup notifier to know when thread init has finished.
  // Then create thread and wait for signal.
  Mutex mu;
  wi->js_init_mutex = &mu;
  wi->js_init_mutex->Lock();
  wi->js_init_signalled = false;

  wi->thread_handle.reset((HANDLE)_beginthreadex(
                              NULL, // security descriptor
                              0, // stack size (or 0 for default)
                              JavaScriptThreadEntry, // start address
                              wi, // argument pointer
                              0, // initial state (0 for running)
                              NULL)); // variable to receive thread ID
  if (wi->thread_handle != 0) { // 0 means _beginthreadex() error
    // thread needs to set onessage handler before we continue
    wi->js_init_mutex->Await(Condition(&wi->js_init_signalled));
  }

  // cleanup notifier
  wi->js_init_mutex->Unlock();
  wi->js_init_mutex = NULL;

  if (wi->thread_handle == 0 || !wi->js_init_ok) {
    if (script_error) {
      *script_error = wi->last_script_error;
    }
    return false; // failed
  }

  *worker_id = new_worker_id;
  return true; // succeeded
}

JavaScriptWorkerInfo *PoolThreadsManager::GetCurrentThreadWorkerInfo() {
  MutexLock lock(&mutex_);
  int worker_id = GetCurrentPoolWorkerId();
  return worker_info_[worker_id];
}

// Creates the JS engine, then pumps messages for the thread.
unsigned __stdcall PoolThreadsManager::JavaScriptThreadEntry(void *args) {

  // WARNING: must fire js_init_signalled even on failure, or the caller won't
  // continue.  So must fire event from a non-nested location (after JS init).

  //
  // Setup JS engine
  //
  assert(args);
  JavaScriptWorkerInfo *wi = static_cast<JavaScriptWorkerInfo*>(args);

  wi->threads_manager->AddWorkerRef();

  bool js_init_succeeded = true;  // will set to false on any failure below

  scoped_ptr<JsRunnerInterface> js_runner(NewJsRunner());

  if (NULL == js_runner.get()) {
    js_init_succeeded = false;
  } else {
    // Add global Factory and WorkerPool objects into the namespace.
    //  
    // The factory alone is not enough; GearsFactory.create(GearsWorkerPool)
    // would return a NEW PoolThreadsManager instance, but we want access to
    // the one that was previously created for the current page.

    // ::CreateInstance does not AddRef (see MSDN).  The js_runner instance
    // will handle that, as it will manage the lifetime of these objects.

    CComObject<GearsWorkerPool> *alloc_workerpool;
    HRESULT hr = CComObject<GearsWorkerPool>::CreateInstance(&alloc_workerpool);
    if (FAILED(hr)) {
      js_init_succeeded = false;
    } else {
      if (!alloc_workerpool->InitBaseManually(true, // is_worker
                                NULL, // page_site is NULL in workers
                                wi->threads_manager->page_security_origin(),
                                js_runner.get())) {
        js_init_succeeded = false;
      } else {
        // WorkerPool object needs same underlying PoolThreadsManager
        alloc_workerpool->SetThreadsManager(wi->threads_manager);

        if (!js_runner->AddGlobal(kWorkerInsertedWorkerPoolName,
                                  alloc_workerpool->_GetRawUnknown(),
                                  0)) {
          js_init_succeeded = false;
        }
      }
    }

    CComObject<GearsFactory> *alloc_factory;
    hr = CComObject<GearsFactory>::CreateInstance(&alloc_factory);
    if (FAILED(hr)) {
      js_init_succeeded = false;
    } else {
      if (!alloc_factory->InitBaseManually(true, // is_worker
                              NULL, // page_site is NULL in workers
                              wi->threads_manager->page_security_origin(),
                              js_runner.get())) {
        js_init_succeeded = false;
      } else {
        // The worker's Factory inherits the capabilities from
        // the main thread that created this WorkerPool
        alloc_factory->is_permission_granted_ = true;
        alloc_factory->is_permission_value_from_user_ = true;

        if (!js_runner->AddGlobal(kWorkerInsertedFactoryName,
                                  alloc_factory->_GetRawUnknown(),
                                  0)) {
          js_init_succeeded = false;
        }
      }
    }

    // Add JS code
    if (!js_runner->Start(wi->full_script)) {
      js_init_succeeded = false;
    }
  }

  //
  // Tell caller we finished initializing, and indicate status
  //

  if (js_runner.get()) {
    wi->last_script_error = js_runner->GetLastScriptError();
  }

  wi->js_init_ok = js_init_succeeded;
  wi->js_init_mutex->Lock();
  wi->js_init_signalled = true;
  wi->js_init_mutex->Unlock();

  //
  // Pump messages
  //
  if (js_init_succeeded) {
    MSG msg;
    BOOL ret; // 0 if WM_QUIT, else non-zero (but -1 if error)
    while (ret = GetMessage(&msg, NULL, 0, 0)) {
      // check flag after waiting, before handling (see ShutDown)
      if (wi->threads_manager->is_shutting_down_) { break; } 
      if (ret != -1) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  // TODO(aa): Consider deleting wi here and setting PTM.worker_info_[i] to
  // NULL. This allows us to free up these thread resources sooner, and it
  // seems a little cleaner too.
  wi->threads_manager->ReleaseWorkerRef();

  // All cleanup is handled by the JsRunner scoped_ptr.

  return 0; // value is currently unused
}


LRESULT CALLBACK PoolThreadsManager::ThreadWndProc(HWND hwnd, UINT message,
                                                   WPARAM wparam,
                                                   LPARAM lparam) {
  switch (message) {
    case WM_WORKERPOOL_ONMESSAGE: {
      // Dequeue the message and dispatch it

      JavaScriptWorkerInfo *wi = reinterpret_cast<JavaScriptWorkerInfo*>(lparam);
      assert(wi->message_hwnd == hwnd);

      // See if the workerpool has been invalidated (as on termination).
      if (wi->threads_manager->is_shutting_down_) { return NULL; }

      // Retrieve message information
      std::string16 message_string;
      int src_worker_id;
      if (!wi->threads_manager->GetPoolMessage(&message_string,
                                               &src_worker_id)) {
        return NULL;
      }

      // Invoke JavaScript onmessage handler
      VARIANTARG invoke_argv[2];
      invoke_argv[1].vt = VT_BSTR; // args are expected in reverse order!!
      invoke_argv[0].vt = VT_INT;

      DISPPARAMS invoke_params = {0};
      invoke_params.cArgs = 2;
      invoke_params.rgvarg = invoke_argv;

      CComBSTR message_bstr(message_string.c_str());
      message_bstr.CopyTo(&invoke_argv[1].bstrVal);
      invoke_argv[0].intVal = src_worker_id;

      HRESULT hr = wi->onmessage_handler->Invoke(
          DISPID_VALUE, IID_NULL, // DISPID_VALUE = default action
          LOCALE_SYSTEM_DEFAULT,  // TODO(cprince): should this be user default?
          DISPATCH_METHOD,        // dispatch/invoke as...
          &invoke_params,         // parameters
          NULL,                   // receives result (NULL okay)
          NULL,                   // receives exception (NULL okay)
          NULL);                  // receives badarg index (NULL okay)
      return 0; // anything will do; retval "depends on the message"
    }
  }
  return DefWindowProc(hwnd, message, wparam, lparam);
}

void PoolThreadsManager::ShutDown() {
  MutexLock lock(&mutex_);
  if (is_shutting_down_) { return; }

  is_shutting_down_ = true;

  for (size_t i = 0; i < worker_info_.size(); ++i) {
    JavaScriptWorkerInfo *wi = worker_info_[i];

    // If the worker is a created thread...
    if (wi->thread_handle != INVALID_HANDLE_VALUE) {
      // Send a dummy message in case the thread is waiting.
      PostMessage(wi->message_hwnd, WM_WORKERPOOL_ONMESSAGE, 0, 0);

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
