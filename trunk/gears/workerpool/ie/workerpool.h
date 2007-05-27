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
// - [P1] Kill threads when page changes. (Browser thread stays the same.)
// - [P2] Measure performance of JS engine instancing; consider caching.

#ifndef GEARS_WORKERPOOL_IE_WORKERPOOL_H__
#define GEARS_WORKERPOOL_IE_WORKERPOOL_H__


#include <vector>
#include "ie/genfiles/interfaces.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/html_event_monitor.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/common.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"

const UINT WM_WORKERPOOL_ONMESSAGE = (WM_USER+0); // LPARAM = JavaScriptWorkerInfo*

class GearsFactory;
class PoolThreadsManager;
struct JavaScriptWorkerInfo;


class ATL_NO_VTABLE GearsWorkerPool
    : public GearsBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsWorkerPool>,
      public IDispatchImpl<GearsWorkerPoolInterface> {
 public:
  BEGIN_COM_MAP(GearsWorkerPool)
    COM_INTERFACE_ENTRY(GearsWorkerPoolInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsWorkerPool)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  // End boilerplate code. Begin interface.

  // need a default constructor to CreateInstance objects in IE
  GearsWorkerPool();
  ~GearsWorkerPool();

  // Creates a new worker.
  // 'full_script' is the entire body of JavaScript the worker will know about.
  // Any top-level code gets executed once before createWorker() returns, and
  // this is how all required initialization (which includes setting onmessage)
  // should be done.  Returns the worker_id of the created worker.
  STDMETHOD(createWorker)(const BSTR *full_script, int *worker_id);

  // Sends message_string to a given worker_id.
  STDMETHOD(sendMessage)(const BSTR *message_string, int dest_worker_id);

  // Sets the onmessage handler for the current worker.
  // The handler has the prototype Handler(message_string, src_worker_id).
  STDMETHOD(put_onmessage)(IDispatch *in_value);

#ifdef DEBUG
  STDMETHOD(forceGC)();
#endif

 private:
  friend class PoolThreadsManager;
  void UseExternalThreadsManager(PoolThreadsManager *manager);

  PoolThreadsManager *threads_manager_;
  bool owns_threads_manager_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsWorkerPool);
};


class PoolThreadsManager {
 public:
  PoolThreadsManager(IUnknown *page_site,
                     const SecurityOrigin &page_security_origin);
  ~PoolThreadsManager();

  bool SetCurrentThreadMessageHandler(IDispatch *handler);
  bool CreateThread(const BSTR *full_script,
                    int *worker_id,
                    std::string16 *script_error);
  bool PutPoolMessage(const BSTR *message_string, int dest_worker_id);

  const SecurityOrigin& page_security_origin() { return page_security_origin_; }

 private:
  int GetCurrentPoolWorkerId();
  bool GetPoolMessage(std::string16 *message_string, int *src_worker_id);
  static unsigned __stdcall JavaScriptThreadEntry(void *args);
  static LRESULT CALLBACK ThreadWndProc(HWND hwnd, UINT message,
                                        WPARAM wparam, LPARAM lparam);

  static void HandleEventUnload(void *user_param);  // callback for 'onunload'
  void StopAllCreatedThreads();
  bool is_pool_stopping_;  // indicates all threads and messages should stop
  scoped_ptr<HtmlEventMonitor> unload_monitor_;  // for 'onunload' notifications

  std::vector<DWORD> worker_id_to_os_thread_id_;
  // this _must_ be a vector of pointers, since each worker references its
  // JavaScriptWorkerInfo, but STL vector realloc can move its elements.
  std::vector<JavaScriptWorkerInfo*> worker_info_;

  Mutex mutex_;  // for exclusive access to all class methods and data

  CComPtr<IUnknown> page_site_;  // NULL for pools not created from main thread
  SecurityOrigin page_security_origin_;

  DISALLOW_EVIL_CONSTRUCTORS(PoolThreadsManager);
};


#endif  // GEARS_WORKERPOOL_IE_WORKERPOOL_H__
