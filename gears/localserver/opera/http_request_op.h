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

#include <string>
#include <vector>

#include "gears/base/common/security_model.h"
#include "gears/base/common/scoped_refptr.h"
#include "gears/localserver/common/http_request.h"
#include "gears/localserver/common/localserver_db.h"
#include "third_party/opera/opera_callback_api.h"

class BlobInterface;
class ByteStore;
class OPBrowsingContext;

class OPHttpRequest
    : public HttpRequest,
      public OperaHttpRequestDataProviderInterface,
      public OperaHttpListenerInterface {
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
  virtual bool Open(const char16 *method,
                    const char16 *url,
                    bool async,
                    BrowsingContext *browsing_context);
  virtual bool SetRequestHeader(const char16 *name, const char16 *value);
  virtual bool Send(BlobInterface* blob);
  virtual bool GetAllResponseHeaders(std::string16 *headers);
  virtual std::string16 GetResponseCharset();
  virtual bool GetResponseHeader(const char16 *name, std::string16 *header);
  virtual bool Abort();

  // events
  virtual bool SetListener(HttpListener *listener,
                           bool enable_data_availbable);

  // OperaHttpRequestDataProviderInterface interface
  virtual bool IsAsync() { return async_; }
  virtual bool GetNextAddedReqHeader(const unsigned short **name,
                                     const unsigned short **value);
  virtual const unsigned short *GetUrl();
  virtual const unsigned short *GetMethod();
  virtual long GetPostData(unsigned char *buffer, long max_size);

  // OperaHttpListenerInterface interface
  virtual void OnDataReceived();
  virtual RedirectStatus OnRedirected(const unsigned short *redirect_url);
  virtual void OnError(LoadingError err);
  virtual void OnRequestCreated(OperaUrlDataInterface *data);
  virtual void OnRequestDestroyed();

 private:
  typedef std::pair<std::string16, std::string16> HeaderEntry;

  bool SendImpl();
  void SetReadyState(ReadyState state);
  bool IsUninitialized() { return ready_state_ == HttpRequest::UNINITIALIZED; }
  bool IsOpen() { return ready_state_ == HttpRequest::OPEN; }
  bool IsSent() { return ready_state_ == HttpRequest::SENT; }
  bool IsInteractive() { return ready_state_ == HttpRequest::INTERACTIVE; }
  bool IsComplete() { return ready_state_ == HttpRequest::COMPLETE; }
  bool IsInteractiveOrComplete() { return IsInteractive() || IsComplete(); }
  bool IsPostOrPut() { return method_ == L"POST" || method_ == L"PUT"; }
  void SetComplete();

  RefCount refcount_;

  // The (non-relative) request url
  std::string16 url_;
  SecurityOrigin origin_;

  // Whether the request should be performed asynchronously
  bool async_;

  // Whether to bypass caches
  CachingBehavior caching_behavior_;

  // Whether to follow redirects
  RedirectBehavior redirect_behavior_;

  // Whether to send cookies or not
  CookieBehavior cookie_behavior_;

  // The request method
  std::string16 method_;

  // The POST data
  scoped_refptr<BlobInterface> post_data_;
  std::string post_data_string_;
  int64 post_data_offset_;

  // Additional request headers we've been asked to send with the request
  std::vector<HeaderEntry> added_request_headers_;
  std::vector<HeaderEntry>::const_iterator added_request_headers_iter_;

  // Our XmlHttpRequest like ready state, 0 thru 4
  ReadyState ready_state_;

  // Whether this request was aborted
  bool was_aborted_;

  // Whether or not we have been redirected
  bool was_redirected_;

  // If we've been redirected, the location of the redirect. If we experience
  // a chain of redirects, this will be the last in the chain upon completion.
  std::string16 redirect_url_;

  // Holds a pointer to the active external URL data from Opera.
  OperaUrlDataInterface *url_data_;

  // Our listener
  HttpRequest::HttpListener *listener_;

  // We populate this structure with various pieces of response data:
  // status code, status line, headers, data
  WebCacheDB::PayloadInfo response_payload_;
  scoped_refptr<ByteStore> response_body_;

  // The amount of data we've read into the response_payload_.data
  // Initially the stl vector is allocated to a large size. We keep
  // track of how much of that allocated space is actually used here.
  size_t actual_data_size_;

  // Used internally to get the document context in which to
  // do the HTTP request.
  scoped_refptr<OPBrowsingContext> browsing_context_;
};

#endif  // GEARS_LOCALSERVER_OPERA_HTTP_REQUEST_OP_H__
