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
// Adds worker threads to JavaScript. Each thread runs in its own context; no
// state is shared between threads. Threads can only send strings to each other.
// By using this model, we give developers parallel execution without exposing
// them to the normal pitfalls of synchronizing data access.
//
// TODO(cprince): in the IE and FF code
// - [P2] Measure performance of JS engine instancing; consider caching.

#ifndef GEARS_WORKERPOOL_IE_WORKERPOOL_H__
#define GEARS_WORKERPOOL_IE_WORKERPOOL_H__


#include <vector>
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "ie/genfiles/interfaces.h" // from OUTDIR

const UINT WM_WORKERPOOL_ONMESSAGE = (WM_USER + 0);
const UINT WM_WORKERPOOL_ONERROR = (WM_USER + 1);

class PoolThreadsManager;
struct Message;
struct JavaScriptWorkerInfo;


class ATL_NO_VTABLE GearsWorkerPool
    : public ModuleImplBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsWorkerPool>,
      public IDispatchImpl<GearsWorkerPoolInterface>,
      public JsEventHandlerInterface {
 public:
  BEGIN_COM_MAP(GearsWorkerPool)
    COM_INTERFACE_ENTRY(GearsWorkerPoolInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsWorkerPool)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  // End boilerplate code. Begin interface.

  // Need a default constructor to CreateInstance objects in IE.
  GearsWorkerPool();
  ~GearsWorkerPool();

  // Creates a new worker.
  // 'full_script' is the entire body of JavaScript the worker will know about.
  // 'url' is a file to fetch to use as the full body of JavaScript.
  // Returns, with the ID of the created worker, as soon as the worker finishes
  // message queue initialization, possibly before any script has been executed.
  STDMETHOD(createWorker)(const BSTR *full_script, int *retval);
  STDMETHOD(createWorkerFromUrl)(const BSTR *url, int *retval);

  // Lets a worker opt-in to being created from another origin.
  STDMETHOD(allowCrossOrigin)();

  // Sends message_string to a given worker_id.
  STDMETHOD(sendMessage)(const BSTR *message, int dest_worker_id);

  // Sets the onmessage handler for the current worker.
  // The handler has the prototype Handler(message_string, src_worker_id).
  STDMETHOD(put_onmessage)(const VARIANT *in_value);

  // Sets the onerror handler for the current worker. This can only be set on
  // the owning worker (the one that created the PoolThreadsManager). Calls
  // from other workers will fail.
  // The handler has the prototype Handler(error_message, src_worker_id).
  STDMETHOD(put_onerror)(const VARIANT *in_value);

  void HandleEvent(JsEventType event_type);

#ifdef DEBUG
  STDMETHOD(forceGC)();
#endif

 private:
  friend class PoolThreadsManager; // for SetThreadsManager

  void Initialize(); // okay to call this multiple times
  void SetThreadsManager(PoolThreadsManager *manager);

  PoolThreadsManager *threads_manager_;
  bool owns_threads_manager_;
  scoped_ptr<JsEventMonitor> unload_monitor_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsWorkerPool);
};


class PoolThreadsManager
    : JsErrorHandlerInterface {
 public:
  PoolThreadsManager(const SecurityOrigin &page_security_origin,
                     JsRunnerInterface *root_js_runner);

  // We handle the lifetime of the PoolThreadsMananger using ref-counting. 
  // When all references go away, the PoolThreadsManager deletes itself.
  // NOTE: each worker will add (and release) multiple references.
  void AddWorkerRef();
  void ReleaseWorkerRef();

  bool SetCurrentThreadMessageHandler(JsRootedCallback *handler);
  bool SetCurrentThreadErrorHandler(JsRootedCallback *handler);
  bool CreateThread(const char16 *url_or_full_script, bool is_param_script,
                    int *worker_id);
  void AllowCrossOrigin();
  void HandleError(const JsErrorInfo &error_info);
  bool PutPoolMessage(const char16 *text, int dest_worker_id,
                      const SecurityOrigin &src_origin);

  // Worker initialization that must be done from the worker's thread.
  bool InitWorkerThread(JavaScriptWorkerInfo *wi);
  void UninitWorkerThread();

  void ShutDown();
#ifdef DEBUG
  void ForceGCCurrentThread();
#endif

  const SecurityOrigin& page_security_origin() { return page_security_origin_; }

 private:
  ~PoolThreadsManager();

  // Gets the id of the worker associated with the current thread. Caller must
  // acquire the mutex.
  int GetCurrentPoolWorkerId();
  bool GetPoolMessage(Message *msg);
  bool InvokeOnErrorHandler(JavaScriptWorkerInfo *wi,
                            const JsErrorInfo &error_info);

  static unsigned __stdcall JavaScriptThreadEntry(void *args);
  static bool SetupJsRunner(JsRunnerInterface *js_runner,
                            JavaScriptWorkerInfo *wi);
  static LRESULT CALLBACK ThreadWndProc(HWND hwnd, UINT message,
                                        WPARAM wparam, LPARAM lparam);

  // Helpers for processing events received from other workers.
  void ProcessMessage(JavaScriptWorkerInfo *wi,
                      const Message &msg);
  void ProcessError(JavaScriptWorkerInfo *wi,
                    const Message &msg);

  // This is used by Add/ReleaseWorkerRef(). Note that it is not equal to the
  // total number of threads, as each worker thread (not the main thread)
  // increments the count twice.
  int ref_count_;
  bool is_shutting_down_;

  std::vector<DWORD> worker_id_to_os_thread_id_;
  // this _must_ be a vector of pointers, since each worker references its
  // JavaScriptWorkerInfo, but STL vector realloc can move its elements.
  std::vector<JavaScriptWorkerInfo*> worker_info_;

  Mutex mutex_;  // for exclusive access to all class methods and data

  SecurityOrigin page_security_origin_;

  DISALLOW_EVIL_CONSTRUCTORS(PoolThreadsManager);
};


#endif  // GEARS_WORKERPOOL_IE_WORKERPOOL_H__
