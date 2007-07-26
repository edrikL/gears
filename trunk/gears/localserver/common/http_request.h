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

#ifndef GEARS_LOCALSERVER_COMMON_HTTP_REQUEST_H__
#define GEARS_LOCALSERVER_COMMON_HTTP_REQUEST_H__

#include <assert.h>
#include <vector>
#include "gears/base/common/int_types.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/string16.h"

//------------------------------------------------------------------------------
// A cross-platform interface for sending HTTP requests.  Implementations
// use the underlying capabilities of different browsers.
//
// IMPORTANT: this class bypasses all caches, including Gears localserver!
//------------------------------------------------------------------------------
class HttpRequest {
 public:
  // factory
  static HttpRequest *Create();

  // refcounting
  virtual int AddReference() = 0;
  virtual int ReleaseReference() = 0;

  // properties
  virtual bool GetReadyState(long *state) = 0;
  virtual bool GetResponseBody(std::vector<uint8> *body) = 0;
  virtual std::vector<uint8> *GetResponseBody() = 0;
  virtual bool GetStatus(long *status) = 0;
  virtual bool GetStatusText(std::string16 *status_text) = 0;
  virtual bool GetStatusLine(std::string16 *status_line) = 0;

  // Set whether or not to follow HTTP redirection, the default is to
  // follow redirects. To disable redirection, call this method after open
  // has been called and prior to calling send.
  virtual bool SetFollowRedirects(bool follow) = 0;

  // Whether or not this request has followed a redirect
  virtual bool WasRedirected() = 0;

  // Returns true and the full url to the final location if this request
  // has followed any redirects
  virtual bool GetRedirectUrl(std::string16 *full_redirect_url) = 0;

  // methods
  virtual bool Open(const char16 *method, const char16* url, bool async) = 0;
  virtual bool SetRequestHeader(const char16 *name, const char16 *value) = 0;
  virtual bool Send() = 0;
  virtual bool GetAllResponseHeaders(std::string16 *headers) = 0;
  virtual bool GetResponseHeader(const char16* name, std::string16 *header) = 0;
  virtual bool Abort() = 0;

  // events and listeners
  class ReadyStateListener {
   public:
    virtual void ReadyStateChanged(HttpRequest *source) = 0;
  };
  virtual bool SetOnReadyStateChange(ReadyStateListener *listener) = 0;

 protected:
  HttpRequest() {}
  virtual ~HttpRequest() {}
};

//------------------------------------------------------------------------------
// A scoped ptr class that automatically calls ReleaseReference on its
// contained HttpRequest pointer.
//------------------------------------------------------------------------------

class ReleaseHttpRequestFunctor {
 public:
  void operator()(HttpRequest *request) const {
    if (request) { request->ReleaseReference(); }
  }
};

typedef scoped_token<HttpRequest*, ReleaseHttpRequestFunctor>
    ScopedHttpRequestPtr;

#endif  // GEARS_LOCALSERVER_COMMON_HTTP_REQUEST_H__
