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
#include <windows.h>  // must manually #include before nsIEventQueue.h
#endif
#include <utility>
#include <vector>
#include "genfiles/httprequest.h"
#include "gears/base/common/message_queue.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/http_utils.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/string16.h"
#ifndef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#include "gears/base/common/scoped_refptr.h"
#include "gears/blob/blob.h"
class BlobInterface;
#endif
#include "gears/localserver/common/http_request.h"
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

 protected:
  // need an accessible destructor to use with nsCOMPtr<GearsHttpRequest>
  ~GearsHttpRequest();

 private:
  struct ResponseInfo {
    int status;
    std::string16 status_text;
    std::string16 headers;
    scoped_ptr<HTTPHeaders> parsed_headers;

    // valid when IsInteractive or IsComplete
    std::string16 text;

    // only valid when IsComplete and prior to having transfered ownership to
    // response_blob
    scoped_ptr< std::vector<uint8> > body;

#ifndef OFFICIAL_BUILD
    // The blob API has not been finalized for official builds
    // only valid when IsComplete and after ownership of the body has been
    // transfered
    scoped_refptr<GearsBlob> blob;
#endif

    ResponseInfo() : status(0) {}
  };

  struct RequestInfo {
    std::string16 method;
    std::string16 full_url;
    std::vector< std::pair<std::string16, std::string16> > headers;
    bool has_post_data;
    std::string16 post_data;
    bool content_type_header_was_set;
    HttpRequest::ReadyState ready_state;
    HttpRequest::ReadyState upcoming_ready_state;
#ifndef OFFICIAL_BUILD
    scoped_refptr<BlobInterface> blob_;
#endif
    ResponseInfo response;
    RequestInfo() : has_post_data(false), content_type_header_was_set(false),
                    ready_state(HttpRequest::UNINITIALIZED),
                    upcoming_ready_state(HttpRequest::UNINITIALIZED) {}
  };

  // Notes
  //
  // 1. Gears.HttpRequest can be reused by script clients to make many
  // distinct requests, not concurrently but in series. Each in the
  // series is given a unique request_id. For each distinct request
  // a new RequestInfo structure is allocated.
  //
  // 2. Initiating native HTTP requests in Firefox can only be performed from 
  // the main ui thread. When an instance of this class is created on
  // a worker thread, we have to play ping/pong with the ui thread to
  // actually do the request. Terminology: the 'apartment thread' is the
  // thread of control an instance of Gears.HttpRequest is created on.
  //
  // 3. When messaging between threads, we include the request_id the
  // message is relevant to. The receiving side of the message uses
  // this value to determine if the distinct request being handled has
  // changed since the message was sent.

  // The following members represent the state of the current request.
  // Methods of this class running in both the apartment and ui threads
  // access this shared state. The apartment thread manages the life-cycle
  // of the RequestInfo ptr.
  Mutex lock_;
  int current_request_id_;
  scoped_ptr<RequestInfo> current_request_info_;

  // Used only on the ui thread, the native http request object and the
  // request_id corresponding to the request_info it was initialized for. 
  scoped_refptr<HttpRequest> native_request_;
  int native_request_id_;

  scoped_ptr<JsRootedCallback> onreadystatechange_;
  ThreadId apartment_thread_id_;
  scoped_ptr<JsEventMonitor> unload_monitor_;
  bool page_is_unloaded_;

  bool ResolveUrl(const char16 *url, std::string16 *resolved_url,
                  std::string16 *exception_message);

  // JsEventHandlerInterface for page unload monitoring
  virtual void HandleEvent(JsEventType event_type);
  void InitUnloadMonitor();

  // Native HttpRequest::ReadyStateListener interface
  virtual void DataAvailable(HttpRequest *source);
  virtual void ReadyStateChanged(HttpRequest *source);

  void SetCurrentRequestInfo(RequestInfo *info);
  void CreateNativeRequest(int request_id);
  void RemoveNativeRequest();

  HttpRequest::ReadyState GetState();
  bool IsUninitialized() { return GetState() == HttpRequest::UNINITIALIZED; }
  bool IsOpen()          { return GetState() == HttpRequest::OPEN; }
  bool IsSent()          { return GetState() == HttpRequest::SENT; }
  bool IsInteractive()   { return GetState() == HttpRequest::INTERACTIVE; }
  bool IsComplete()      { return GetState() == HttpRequest::COMPLETE; }
  bool IsValidResponse();

  // Returns true if the currently executing thread is our apartment thread
  bool IsApartmentThread() {
    return apartment_thread_id_ ==
        ThreadMessageQueue::GetInstance()->GetCurrentThreadId();
  }

  enum AsyncCallType {
    kSend,   // invoked from apartment to execute on ui thread
    kAbort,  // invoked from apartment to execute on ui thread
    kReadyStateChanged,  // invoked from ui to execute on apartment thread
    kDataAvailable  // invoked from ui to execute on apartment thread
  };
  bool CallAbortOnUiThread();
  bool CallSendOnUiThread();
  bool CallReadyStateChangedOnApartmentThread();
  bool CallDataAvailableOnApartmentThread();
  void OnAbortCall(int request_id);
  void OnSendCall(int request_id);
  void OnReadyStateChangedCall(int request_id);
  void OnDataAvailableCall(int request_id);

  nsresult CallAsync(ThreadId thread_id, AsyncCallType call_type);
  void OnAsyncCall(AsyncCallType call_type, int request_id);

  class AsyncCallEvent;
  friend class AsyncCallEvent;

  DISALLOW_EVIL_CONSTRUCTORS(GearsHttpRequest);
};


#endif  // GEARS_HTTPREQUEST_FIREFOX_HTTPREQUEST_FF_H__
