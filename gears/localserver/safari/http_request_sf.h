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

class SFHttpRequest : public HttpRequest {
 public:
  // Gears's HttpRequest interface

  virtual int AddReference();
  virtual int ReleaseReference();

  // properties
  virtual bool GetReadyState(long *state);
  virtual bool GetResponseBody(std::vector<uint8> *body);
  virtual std::vector<uint8> *GetResponseBody();
  virtual bool GetStatus(long *status);
  virtual bool GetStatusText(std::string16 *status_text);
  virtual bool GetStatusLine(std::string16 *status_line);

  // Set whether or not to follow HTTP redirection, the default is to
  // follow redirects. To disable redirection, call this method after open
  // has been called and prior to calling send.
  virtual bool SetFollowRedirects(bool follow);

  // Whether or not this request has followed a redirect
  virtual bool WasRedirected();

  // Returns true and the full url to the final location if this request
  // has followed any redirects
  virtual bool GetRedirectUrl(std::string16 *full_redirect_url);

  // methods
  virtual bool Open(const char16 *method, const char16* url, bool async);
  virtual bool SetRequestHeader(const char16* name, const char16* value);
  virtual bool Send();
  virtual bool GetAllResponseHeaders(std::string16 *headers);
  virtual bool GetResponseHeader(const char16* name, std::string16 *header);
  virtual bool Abort();

  // events
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
  void SetReadyState(long ready_state);
  void TerminateStreamReader();
  void Reset();
  void CopyHeadersIfAvailable();

  ReadyStateListener *listener_;
  int ref_count_;
  long ready_state_;
  bool follow_redirect_;
  scoped_CFHTTPMessage request_;
  scoped_CFReadStream read_stream_;
  scoped_CFHTTPMessage response_;
  scoped_CFDictionary response_header_;
  scoped_CFMutableData body_;
};

#endif  // GEARS_LOCALSERVER_SAFARI_HTTP_REQUEST_SF_H__
