// Copyright 2008, Google Inc.
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

#ifndef GEARS_HTTPREQUEST_NPAPI_HTTPREQUEST_NP_H__
#define GEARS_HTTPREQUEST_NPAPI_HTTPREQUEST_NP_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/localserver/common/http_request.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

class HttpRequest;

class GearsHttpRequest
    : public ModuleImplBaseClassVirtual,
      public JsEventHandlerInterface,
      public HttpRequest::ReadyStateListener {
 public:
  GearsHttpRequest();
  
  // IN: string method
  // IN: string URL
  void Open(JsCallContext *context);
  
  // IN: string name
  // IN: string value
  void SetRequestHeader(JsCallContext *context);
  
  // OPTIONAL IN: string postData
  void Send(JsCallContext *context);
  
  void Abort(JsCallContext *context);
  
  // IN: string headerName
  // OUT: string
  void GetResponseHeader(JsCallContext *context);

  // OUT: string
  void GetAllResponseHeaders(JsCallContext *context);
  
  // OUT: function
  void GetOnReadyStateChange(JsCallContext *context);

  // IN: function
  void SetOnReadyStateChange(JsCallContext *context);

  // OUT: int
  void GetReadyState(JsCallContext *context);
  
  // OUT: string
  void GetResponseText(JsCallContext *context);
  
  // OUT: int
  void GetStatus(JsCallContext *context);

  // OUT: string
  void GetStatusText(JsCallContext *context);
  
 private:
  scoped_refptr<HttpRequest> request_;
  bool content_type_header_was_set_;
  bool has_fired_completion_event_;
  scoped_ptr<JsRootedCallback> onreadystatechangehandler_;
  scoped_ptr<JsEventMonitor> unload_monitor_;
  
  // From JsEventHandlerInterface.
  virtual void HandleEvent(JsEventType event_type);

  void AbortRequest();
  void CreateRequest();
  void ReleaseRequest();

  HttpRequest::ReadyState GetState();
  bool IsUninitialized() { return GetState() == HttpRequest::UNINITIALIZED; }
  bool IsOpen()          { return GetState() == HttpRequest::OPEN; }
  bool IsSent()          { return GetState() == HttpRequest::SENT; }
  bool IsInteractive()   { return GetState() == HttpRequest::INTERACTIVE; }
  bool IsComplete()      { return GetState() == HttpRequest::COMPLETE; }
  bool IsValidResponse();

  bool ResolveUrl(const std::string16 &url, std::string16 *resolved_url,
                 std::string16 *exception_message);
                  
  // HttpRequest::ReadyStateListener impl
  virtual void DataAvailable(HttpRequest *source);
  virtual void ReadyStateChanged(HttpRequest *source);

  ~GearsHttpRequest();
  DISALLOW_EVIL_CONSTRUCTORS(GearsHttpRequest);
};

#endif // GEARS_HTTPREQUEST_NPAPI_HTTPREQUEST_NP_H__
