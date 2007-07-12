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

#ifndef GEARS_WORKERPOOL_FIREFOX_WORKERPOOL_H__
#define GEARS_WORKERPOOL_FIREFOX_WORKERPOOL_H__

#include <vector>
#include <nsComponentManagerUtils.h>
#include "gears/third_party/gecko_internal/nsIVariant.h"

#include "ff/genfiles/workerpool.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/html_event_monitor.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/common.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"

// Object identifiers
extern const char *kGearsWorkerPoolClassName;
extern const nsCID kGearsWorkerPoolClassId;


class PoolThreadsManager;


class GearsWorkerPool
    : public GearsBaseClass,
      public GearsWorkerPoolInterface {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  // Need a default constructor to instance objects from the Factory.
  GearsWorkerPool();
  ~GearsWorkerPool();

  NS_IMETHOD CreateWorker(const nsAString &full_script, PRInt32 *retval);
  NS_IMETHOD SendMessage(const nsAString &message_string,
                         PRInt32 dest_worker_id);
  NS_IMETHOD SetOnmessage(WorkerOnmessageHandler *in_value);
  NS_IMETHOD GetOnmessage(WorkerOnmessageHandler **out_value);
#ifdef DEBUG
  NS_IMETHOD ForceGC();
#endif

  friend class PoolThreadsManager; // for SetThreadsManager()

 private:
  void Initialize(); // okay to call this multiple times
  void SetThreadsManager(PoolThreadsManager *manager);

  static void HandleEventUnload(void *user_param);  // callback for 'onunload'

  PoolThreadsManager *threads_manager_;
  bool owns_threads_manager_;
  scoped_ptr<HtmlEventMonitor> unload_monitor_;  // for 'onunload' notifications

  DISALLOW_EVIL_CONSTRUCTORS(GearsWorkerPool);
};


struct OnmessageHandler;
struct JavaScriptWorkerInfo;
struct ThreadsEvent;

class PoolThreadsManager {
 public:
  PoolThreadsManager(const SecurityOrigin &page_security_origin);

  // We handle the lifetime of PoolThreadsManager using ref-counting. Each of
  // the GearsWorkerPool instances associated with a PoolThreadsManager has a
  // reference. When they all go away, the PoolThreadsManager deletes itself.
  void AddWorkerRef();
  void ReleaseWorkerRef();

  bool SetCurrentThreadMessageHandler(OnmessageHandler *handler);
  bool CreateThread(const nsAString &full_script, int *worker_id,
                    std::string16 *script_error); // script_error can be NULL
  bool PutPoolMessage(const nsAString &message_string, int dest_worker_id);
  void ShutDown();
#ifdef DEBUG
  void ForceGCCurrentThread();
#endif

  const SecurityOrigin& page_security_origin() { return page_security_origin_; }

 private:
  ~PoolThreadsManager();

  bool GetPoolMessage(std::string16 *message_string, int *src_worker_id);
  int GetCurrentPoolWorkerId();

  static void JavaScriptThreadEntry(void *args);
  static bool InitJavaScriptEngine(JavaScriptWorkerInfo *ti);
  static void JS_DLL_CALLBACK JsErrorHandler(JSContext *cx, const char *message,
                                             JSErrorReport *report);
  static void *OnReceiveThreadsEvent(ThreadsEvent *event);

  int num_workers_; // used by Add/ReleaseWorkerRef()
  bool is_shutting_down_;

  std::vector<PRThread*> worker_id_to_os_thread_id_;
  // this _must_ be a vector of pointers, since each worker references its
  // JavaScriptWorkerInfo, but STL vector realloc can move its elements.
  std::vector<JavaScriptWorkerInfo*> worker_info_;

  Mutex mutex_;  // for exclusive access to all class methods and data

  SecurityOrigin page_security_origin_;

  DISALLOW_EVIL_CONSTRUCTORS(PoolThreadsManager);
};



#endif // GEARS_WORKERPOOL_FIREFOX_WORKERPOOL_H__
