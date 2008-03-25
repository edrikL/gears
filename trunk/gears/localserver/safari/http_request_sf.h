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

#ifndef GEARS_LOCALSERVER_SAFARI_HTTP_REQUEST_SF_H__
#define GEARS_LOCALSERVER_SAFARI_HTTP_REQUEST_SF_H__

#include <vector>

#include "gears/base/common/common.h"
#include "gears/base/common/security_model.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/http_request.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

//------------------------------------------------------------------------------
// SFHttpRequest
//------------------------------------------------------------------------------
class SFHttpRequest : public HttpRequest {
 public:
 
  // RefCounting
  virtual int AddReference();
  virtual int ReleaseReference();

  // Get or set whether to use or bypass caches, the default is USE_ALL_CACHES
  // May only be set prior to calling Send.
  virtual CachingBehavior GetCachingBehavior() {
    return caching_behavior_;
  }

  virtual bool SetCachingBehavior(CachingBehavior behavior) {
    if (was_sent_) return false;
    caching_behavior_ = behavior;
    return true;
  }

  // Get or set the redirect behavior, the default is FOLLOW_ALL
  // May only be set prior to calling Send.
  virtual RedirectBehavior GetRedirectBehavior() { 
    return redirect_behavior_;
  }

  virtual bool SetRedirectBehavior(RedirectBehavior behavior) {
    if (was_sent_) return false;
    redirect_behavior_ = behavior;
    return true;
  }

  // properties
  virtual bool GetReadyState(ReadyState *state);
  virtual bool GetResponseBodyAsText(std::string16 *text);
  virtual bool GetResponseBody(std::vector<uint8> *body);
  virtual std::vector<uint8> *GetResponseBody();
  virtual bool GetStatus(int *status);
  virtual bool GetStatusText(std::string16 *status_text);
  virtual bool GetStatusLine(std::string16 *status_line);

  virtual bool WasRedirected();
  virtual bool GetFinalUrl(std::string16 *full_url);
  virtual bool GetInitialUrl(std::string16 *full_url);

  // methods
  virtual bool Open(const char16 *method, const char16 *url, bool async);
  virtual bool SetRequestHeader(const char16 *name, const char16 *value);
  virtual bool Send();
  virtual bool SendString(const char16 *data);
  virtual bool GetAllResponseHeaders(std::string16 *headers);
  virtual bool GetResponseHeader(const char16 *name, std::string16 *header);
  virtual bool Abort();

  // events
  virtual bool SetOnReadyStateChange(ReadyStateListener *listener);

 // Methods used to communicate between Obj C delegate and C++ class.
 // You can't make an objc-c selector a friend of a C++ class, so these
 // need to be declared public.
 public:
 
  // Called by the delegate if a redirect is requested by the server
  // returns: true - allow redirect, false - deny redirect.
  bool AllowRedirect(const std::string16 &redirect_url);
  void SetReadyState(ReadyState state);
  
  // Holders for http headers.
  typedef std::pair<std::string16, std::string16> HttpHeader;
  typedef std::vector<HttpHeader> HttpHeaderVector;
  typedef HttpHeaderVector::const_iterator HttpHeaderVectorConstIterator;

 private:
  friend HttpRequest *HttpRequest::Create();
 
  SFHttpRequest();
  ~SFHttpRequest();

  bool IsUninitialized() { return ready_state_ == UNINITIALIZED; }
  bool IsOpen() { return ready_state_ == OPEN; }
  bool IsSent() { return ready_state_ == SENT; }
  bool IsInteractive() { return ready_state_ == INTERACTIVE; }
  bool IsComplete() { return ready_state_ == COMPLETE; }
  bool IsInteractiveOrComplete() { return IsInteractive() || IsComplete(); }

  bool IsPostOrPut() {
    return method_ == HttpConstants::kHttpPOST || 
           method_ == HttpConstants::kHttpPUT;
  }
  
  bool SendImpl(const std::string &post_data);
  void Reset();

  ReadyStateListener *listener_;
  int ref_count_;
  ReadyState ready_state_;
  std::string16 method_;
  std::string16 url_;
  SecurityOrigin origin_;
  CachingBehavior caching_behavior_;
  RedirectBehavior redirect_behavior_;
  bool was_sent_;
  bool was_aborted_;
  bool was_redirected_;
  bool async_;
  std::string16 redirect_url_;
  
  HttpHeaderVector headers_to_send_;
  
  // Use PIMPL idiom to mask obj-c object, so we can include
  // this header in C++ code.
  struct HttpRequestData;
  HttpRequestData *delegate_holder_;
};

#endif  // GEARS_LOCALSERVER_SAFARI_HTTP_REQUEST_SF_H__
