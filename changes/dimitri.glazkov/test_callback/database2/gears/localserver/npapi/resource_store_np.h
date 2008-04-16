// Copyright 2005, Google Inc.
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

#ifndef GEARS_LOCALSERVER_NPAPI_RESOURCE_STORE_NP_H__
#define GEARS_LOCALSERVER_NPAPI_RESOURCE_STORE_NP_H__

#include <deque>
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/string16.h"
#include "gears/localserver/common/resource_store.h"
#include "gears/localserver/npapi/capture_task_np.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

//-----------------------------------------------------------------------------
// GearsResourceStore
//-----------------------------------------------------------------------------
class GearsResourceStore
    : public ModuleImplBaseClassVirtual,
      public AsyncTask::Listener,
      public JsEventHandlerInterface {
 public:
  GearsResourceStore() : next_capture_id_(0), page_is_unloaded_(false) {}

  // OUT: string name
  void GetName(JsCallContext *context);

  // OUT: string cookie
  void GetRequiredCookie(JsCallContext *context);

  // OUT: bool enabled
  void GetEnabled(JsCallContext *context);
  // IN: bool enabled
  void SetEnabled(JsCallContext *context);

  // IN: string url | string[] url_array
  // IN: function completion_callback
  // OUT: int capture_id
  void Capture(JsCallContext *context);

  // IN: int capture_id
  void AbortCapture(JsCallContext *context);

  // IN: string url
  // OUT: bool is_captured
  void IsCaptured(JsCallContext *context);

  // IN: string url
  void Remove(JsCallContext *context);

  // IN: string src_url
  // IN: string dst_url
  void Rename(JsCallContext *context);

  // IN: string src_url
  // IN: string dst_url
  void Copy(JsCallContext *context);

  // IN: string url
  // IN: string header_name
  // OUT: string header
  void GetHeader(JsCallContext *context);

  // IN: string url
  // OUT: string headers
  void GetAllHeaders(JsCallContext *context);

  // IN: HtmlElement file_input_element
  // IN: string url
  void CaptureFile(JsCallContext *context);

  // IN: string url
  // OUT: string filename
  void GetCapturedFileName(JsCallContext *context);

  // OUT: GearsFileSubmitter *retval
  void CreateFileSubmitter(JsCallContext *context);

 protected:
  ~GearsResourceStore();

 private:
  bool ResolveAndAppendUrl(const std::string16 &url, NPCaptureRequest *request);
  bool ResolveUrl(const std::string16 &url, std::string16 *resolved_url);
  bool StartCaptureTaskIfNeeded(bool fire_events_on_failure);
  void FireFailedEvents(NPCaptureRequest *request);
  void InvokeCompletionCallback(NPCaptureRequest *request,
                                const std::string16 &capture_url,
                                int capture_id,
                                bool succeeded);
  // JsEventHandlerInterface
  virtual void HandleEvent(JsEventType event_type);
  // AsyncTask::Listener
  virtual void HandleEvent(int code, int param, AsyncTask *source);
  void OnCaptureUrlComplete(int index, bool success);
  void OnCaptureTaskComplete();

  void AbortAllRequests();

  scoped_ptr<JsEventMonitor> unload_monitor_;
  int next_capture_id_;
  std::deque<NPCaptureRequest*> pending_requests_;
  scoped_ptr<NPCaptureRequest> current_request_;
  scoped_ptr<CaptureTask> capture_task_;
  bool page_is_unloaded_;
  std::string16 exception_message_;
  ResourceStore store_;
  // This flag is set to true until the first OnCaptureUrlComplete is called by 
  // the capture_task_. If current_request_ is aborted before it gets a chance
  // to begin the capture, OnCaptureUrlComplete will not be called for any of
  // its urls, which implies that none of the corresponding capture callbacks
  // will be called, either. To prevent this, we inspect this flag in
  // OnCaptureTaskComplete and, if set, we call FireFailedEvents for
  // current_request_.
  bool need_to_fire_failed_events_;

  friend class GearsLocalServer;

  DISALLOW_EVIL_CONSTRUCTORS(GearsResourceStore);
};

#endif // GEARS_LOCALSERVER_NPAPI_RESOURCE_STORE_NP_H__
