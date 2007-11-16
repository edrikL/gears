// Copyright 2007, Google Inc.
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

#ifndef GEARS_HTTPREQUEST_FIREFOX_HTTPREQUEST_FF_H__
#define GEARS_HTTPREQUEST_FIREFOX_HTTPREQUEST_FF_H__

#ifdef WIN32
#include <windows.h> // must manually #include before nsIEventQueue.h
#endif
#include <vector>
#include "ff/genfiles/httprequest.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/http_utils.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/string16.h"
#include "gears/localserver/common/http_request.h"
#include "gears/third_party/gecko_internal/nsIEventQueue.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// Object identifiers
extern const char *kGearsHttpRequestClassName;
extern const nsCID kGearsHttpRequestClassId;

class nsIEventQueue;
class HTTPHeaders;

//-----------------------------------------------------------------------------
// GearsHttpRequest
//-----------------------------------------------------------------------------
class GearsHttpRequest
    : public ModuleImplBaseClass,
      public GearsHttpRequestInterface,
      public HttpRequest::ReadyStateListener,
      public JsEventHandlerInterface {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  GearsHttpRequest();
  NS_DECL_GEARSHTTPREQUESTINTERFACE

  void HandleEvent(JsEventType event_type);

 protected:
  // need an accessible destructor to use with nsCOMPtr<GearsHttpRequest>
  ~GearsHttpRequest();

 private:
  struct RequestInfo {
    std::string16 method;
    std::string16 full_url;
    std::vector< std::pair<std::string16, std::string16> > headers;
    bool has_post_data;
    std::string16 post_data;
    RequestInfo() : has_post_data(false) {}
  };

  struct ResponseInfo {
    HttpRequest::ReadyState pending_ready_state;
    HttpRequest::ReadyState ready_state;
    int status;
    std::string16 status_text;
    std::string16 headers;
    scoped_ptr<HTTPHeaders> parsed_headers;
    std::string16 response_text;
    ResponseInfo() : pending_ready_state(HttpRequest::UNINITIALIZED),
                     ready_state(HttpRequest::UNINITIALIZED), status(0) {}
  };

  Mutex lock_;
  scoped_ptr<RequestInfo> request_info_;
  scoped_ptr<ResponseInfo> response_info_;
  HttpRequest *request_;
  scoped_ptr<JsRootedCallback> onreadystatechange_;
  PRThread *apartment_thread_;
  nsCOMPtr<nsIEventQueue> apartment_event_queue_;
  nsCOMPtr<nsIEventQueue> ui_event_queue_;
  bool content_type_header_was_set_;
  bool page_is_unloaded_;
  scoped_ptr<JsEventMonitor> unload_monitor_;

  bool ResolveUrl(const char16 *url, std::string16 *resolved_url,
                  std::string16 *exception_message);
  virtual void DataAvailable(HttpRequest *source);
  virtual void ReadyStateChanged(HttpRequest *source);
  void CreateRequest();
  void RemoveRequest();
  bool IsUninitialized();  // ready_state 0
  bool IsOpen();           // ready_state 1
  bool IsSent();           // ready_state 2
  bool IsInteractive();    // ready_state 3
  bool IsComplete();       // ready_state 4
  bool IsValidResponse();

  // Initiating HTTP requests in Firefox can only be performed from the
  // main ui thread. When an instance of this class is created on
  // a worker thread, we have to play ping/pong with the ui thread to
  // actually do the request. Terminology: the 'apartment thread' is the
  // thread of control an instance of Gears.HttpRequest is created on.

  // Returns true if the currently executing thread is our apartment thread
  bool IsApartmentThread() { 
    return apartment_thread_ == PR_GetCurrentThread();
  }

  enum AsyncCallType {
    kSend,  // invoked from apartment to execute on ui thread
    kAbort, // invoked from apartment to execute on ui thread
    kReadyStateChanged,  // invoked from ui to execute on apartment thread
    kDataAvailable  // invoked from ui to execute on apartment thread
  };
  bool CallAbortOnUiThread();
  bool CallSendOnUiThread();
  bool CallReadyStateChangedOnApartmentThread();
  bool CallDataAvailableOnApartmentThread();
  void OnAbortCall();
  void OnSendCall();
  void OnReadyStateChangedCall();
  void OnDataAvailableCall();

  nsresult CallAsync(nsIEventQueue *event_queue, AsyncCallType call_type);
  void OnAsyncCall(AsyncCallType call_type);
  bool InitEventQueues();
  void InitUnloadMonitor();

  struct AsyncCallEvent;
  static void *PR_CALLBACK AsyncCall_EventHandlerFunc(AsyncCallEvent*);
  static void  PR_CALLBACK AsyncCall_EventCleanupFunc(AsyncCallEvent*);

  DISALLOW_EVIL_CONSTRUCTORS(GearsHttpRequest);
};


#endif // GEARS_HTTPREQUEST_FIREFOX_HTTPREQUEST_FF_H__
