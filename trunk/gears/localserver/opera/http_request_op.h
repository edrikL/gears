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

#ifndef GEARS_LOCALSERVER_OPERA_HTTP_REQUEST_OP_H__
#define GEARS_LOCALSERVER_OPERA_HTTP_REQUEST_OP_H__

#include "gears/localserver/common/http_request.h"


class BlobInterface;

class OPHttpRequest : public HttpRequest {
 public:
  OPHttpRequest();
  virtual ~OPHttpRequest();

  // HttpRequest interface

  // refcounting
  virtual void Ref();
  virtual void Unref();

  // Get or set whether to use or bypass caches, the default is USE_ALL_CACHES
  // May only be set prior to calling Send.
  virtual CachingBehavior GetCachingBehavior() {
    return caching_behavior_;
  }

  virtual bool SetCachingBehavior(CachingBehavior behavior) {
    if (!(IsUninitialized() || IsOpen())) return false;
    caching_behavior_ = behavior;
    return true;
  }

  // Get or set the redirect behavior, the default is FOLLOW_ALL
  // May only be set prior to calling Send.
  virtual RedirectBehavior GetRedirectBehavior() {
    return redirect_behavior_;
  }

  virtual bool SetRedirectBehavior(RedirectBehavior behavior) {
    if (!(IsUninitialized() || IsOpen())) return false;
    redirect_behavior_ = behavior;
    return true;
  }

  virtual bool SetCookieBehavior(CookieBehavior behavior) {
    if (!(IsUninitialized() || IsOpen())) return false;
    cookie_behavior_ = behavior;
    return true;
  }

  // properties
  virtual bool GetReadyState(ReadyState *state);
  virtual bool GetResponseBody(scoped_refptr<BlobInterface>* blob);
  virtual bool GetStatus(int *status);
  virtual bool GetStatusText(std::string16 *status_text);
  virtual bool GetStatusLine(std::string16 *status_line);

  virtual bool WasRedirected();
  virtual bool GetFinalUrl(std::string16 *full_url);
  virtual bool GetInitialUrl(std::string16 *full_url);

  // methods
  virtual bool Open(const char16 *method, const char16* url, bool async,
                    BrowsingContext *browsing_context);
  virtual bool SetRequestHeader(const char16* name, const char16* value);
  virtual bool Send(BlobInterface* blob);
  virtual bool GetAllResponseHeaders(std::string16 *headers);
  virtual std::string16 GetResponseCharset();
  virtual bool GetResponseHeader(const char16* name, std::string16 *header);
  virtual bool Abort();

  // events
  virtual bool SetListener(HttpListener *listener, bool enable_data_available);

 private:
  bool IsUninitialized() { return ready_state_ == HttpRequest::UNINITIALIZED; }
  bool IsOpen() { return ready_state_ == HttpRequest::OPEN; }

  // Whether to bypass caches
  CachingBehavior caching_behavior_;

  // Whether to follow redirects
  RedirectBehavior redirect_behavior_;

  // Whether to send cookies or not
  CookieBehavior cookie_behavior_;

  // Our XmlHttpRequest like ready state, 0 thru 4
  ReadyState ready_state_;
};

#endif  // GEARS_LOCALSERVER_OPERA_HTTP_REQUEST_OP_H__
