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

#ifndef GEARS_LOCALSERVER_SAFARI_HTTP_REQUEST_SF_H__
#define GEARS_LOCALSERVER_SAFARI_HTTP_REQUEST_SF_H__

#include <CoreServices/CoreServices.h>

#include "gears/base/safari/scoped_cf.h"
#include "gears/localserver/common/http_request.h"

// TODO(playmobil): Make httprequest use the browser's cache & cookies?

class SFHttpRequest : public HttpRequest {
 public:
  // Gears's HttpRequest interface
  virtual int AddReference();
  virtual int ReleaseReference();
  
  virtual CachingBehavior GetCachingBehavior();
  virtual bool SetCachingBehavior(CachingBehavior behavior);
  virtual RedirectBehavior GetRedirectBehavior();
  virtual bool SetRedirectBehavior(RedirectBehavior behavior);

  // properties
  virtual bool GetReadyState(ReadyState *state);
  virtual bool GetResponseBodyAsText(std::string16 *text);
  virtual bool GetResponseBody(std::vector<uint8> *body);
  virtual std::vector<uint8> *GetResponseBody();
  virtual bool GetStatus(int *status);
  virtual bool GetStatusText(std::string16 *status_text);
  virtual bool GetStatusLine(std::string16 *status_line);

  // Whether or not this request has followed a redirect
  virtual bool WasRedirected();
  
  virtual bool GetFinalUrl(std::string16 *full_url);
  virtual bool GetInitialUrl(std::string16 *full_url);

  // methods
  virtual bool Open(const char16 *method, const char16* url, bool async);
  virtual bool SetRequestHeader(const char16* name, const char16* value);
  virtual bool Send();
  virtual bool SendString(const char16 *name);
  virtual bool GetAllResponseHeaders(std::string16 *headers);
  virtual bool GetResponseHeader(const char16* name, std::string16 *header);
  virtual bool Abort();

  // events
  // TODO(playmobil): check that SENT and INTERACTIVE states are used.
  virtual bool SetOnReadyStateChange(ReadyStateListener *listener);

 private:
  friend HttpRequest *HttpRequest::Create();

  SFHttpRequest();
  ~SFHttpRequest();

  static void ConcatenateResponseHeadersToString(const void *key,
                                                 const void *value,
                                                 void *context);
  static void StreamReaderFunc(CFReadStreamRef stream,
                               CFStreamEventType type,
                               void *clientCallBackInfo);
  void SetReadyState(ReadyState ready_state);
  
  bool IsUninitialized() { return ready_state_ == HttpRequest::UNINITIALIZED; }
  bool IsOpen() { return ready_state_ == HttpRequest::OPEN; }
  bool IsSent() { return ready_state_ == HttpRequest::SENT; }
  bool IsInteractive() { return ready_state_ == HttpRequest::INTERACTIVE; }
  bool IsComplete() { return ready_state_ == HttpRequest::COMPLETE; }
  bool IsInteractiveOrComplete() { return IsInteractive() || IsComplete(); }
  
  void TerminateStreamReader();
  void Reset();
  void CopyHeadersIfAvailable();

  ReadyStateListener *listener_;
  int ref_count_;
  ReadyState ready_state_;
  bool follow_redirect_;
  scoped_CFHTTPMessage request_;
  scoped_CFReadStream read_stream_;
  scoped_CFHTTPMessage response_;
  scoped_CFDictionary response_header_;
  scoped_CFMutableData body_;
  std::string16 url_;
  
  // Whether this request was aborted
  bool was_aborted_;
};

#endif  // GEARS_LOCALSERVER_SAFARI_HTTP_REQUEST_SF_H__
