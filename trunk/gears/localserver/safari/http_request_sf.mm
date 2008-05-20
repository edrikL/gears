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

// TODO(playmobil): The getters in this version of http_request test their
// input parameters and return false in the case of NULLs.  We should look at
// either bubbling this behavior up to other platforms, or changing this version
// to be consistent with the others.

// Safari implementation of HttpRequest using an NSURLConnection.

#include <iostream>

#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/http_utils.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/safari/cf_string_utils.h"
#ifndef OFFICIAL_BUILD
#include "gears/blob/blob_input_stream_sf.h"
#include "gears/blob/blob_interface.h"
#endif  // !OFFICIAL_BUILD
#include "gears/localserver/common/http_request.h"
#include "gears/localserver/safari/http_request_delegate.h"

// PIMPL store for Objective-C delegate.
struct SFHttpRequest::HttpRequestData {
  HttpRequestDelegate *delegate;
  
  HttpRequestData() : delegate(NULL) {}
  
  ~HttpRequestData() {
    [delegate abort];
    [delegate release];
    delegate = NULL;
  }
};

//------------------------------------------------------------------------------
// Create
//------------------------------------------------------------------------------

// static
bool HttpRequest::Create(scoped_refptr<HttpRequest>* request) {
  request->reset(new SFHttpRequest);
  return true;
}

// static
bool HttpRequest::CreateSafeRequest(scoped_refptr<HttpRequest>* request) {
  return HttpRequest::Create(request);
}

//------------------------------------------------------------------------------
// Constructor / Destructor
//------------------------------------------------------------------------------
SFHttpRequest::SFHttpRequest()
  : listener_(NULL),
    listener_data_available_enabled_(false), 
    ready_state_(UNINITIALIZED), 
    caching_behavior_(USE_ALL_CACHES), 
    redirect_behavior_(FOLLOW_ALL),
    was_sent_(false), was_aborted_(false),
    was_redirected_(false), async_(false) {

  delegate_holder_ = new HttpRequestData;
}

SFHttpRequest::~SFHttpRequest() {
  delete delegate_holder_;
}

void SFHttpRequest::Ref() {
  count_.Ref();
}
  
void SFHttpRequest::Unref() {
  if (count_.Unref()) {
    delete this;
  }
}

//------------------------------------------------------------------------------
// GetReadyState
//------------------------------------------------------------------------------
bool SFHttpRequest::GetReadyState(ReadyState *state) {
  if (!state) {
    return false;
  }

  *state = ready_state_;
  return true;
}

//------------------------------------------------------------------------------
// GetResponseBodyAsText
//------------------------------------------------------------------------------
bool SFHttpRequest::GetResponseBodyAsText(std::string16 *text) {
  if (!IsInteractiveOrComplete() || was_aborted_) {
    return false;
  }
    
  if (!text) {
    return false;
  }

  return [delegate_holder_->delegate responseAsString:text];
}

//------------------------------------------------------------------------------
// GetResponseBody
//------------------------------------------------------------------------------
bool SFHttpRequest::GetResponseBody(std::vector<uint8> *body) {
  if (!(IsComplete() && !was_aborted_)) {
    return false;
  }
  
  if (!body) {
    return false;
  }
  
  [delegate_holder_->delegate responseBytes:body];
  return true;
}

//------------------------------------------------------------------------------
// GetResponseBody
//------------------------------------------------------------------------------
std::vector<uint8> *SFHttpRequest::GetResponseBody() {
  scoped_ptr< std::vector<uint8> > body(new std::vector<uint8>);
  if (!GetResponseBody(body.get())) {
    return NULL;
  } else {
    return body.release();
  }
}

//------------------------------------------------------------------------------
// GetStatus
//------------------------------------------------------------------------------
bool SFHttpRequest::GetStatus(int *status) {
  if (!(IsInteractiveOrComplete() && !was_aborted_)) {
    return false;
  }
  
  if (!status) {
    return false;
  }
  
  *status = [delegate_holder_->delegate statusCode];
  
  return true;
}

//------------------------------------------------------------------------------
// GetStatusText
//------------------------------------------------------------------------------
bool SFHttpRequest::GetStatusText(std::string16 *status_text) {
  if (!(IsInteractiveOrComplete() && !was_aborted_)) {
    return false;
  }
  
  if (!status_text) {
    return false;
  }

  [delegate_holder_->delegate statusText:status_text];
  return true;
}

//------------------------------------------------------------------------------
// GetStatusLine
//------------------------------------------------------------------------------
bool SFHttpRequest::GetStatusLine(std::string16 *status_line) {
  if (!(IsInteractiveOrComplete() && !was_aborted_)) {
    return false;
  }
  
  if (!status_line) {
    return false;
  }

  int status_code = [delegate_holder_->delegate statusCode];
  std::string16 status_text;
  [delegate_holder_->delegate statusText:&status_text];

  std::string16 ret(STRING16(L"HTTP/1.1 "));
  ret += IntegerToString16(status_code);
  ret += STRING16(L" ");
  ret += status_text;

  *status_line = ret;

  return true;
}

//------------------------------------------------------------------------------
// Open
//------------------------------------------------------------------------------
bool SFHttpRequest::Open(const char16 *method, const char16 *url, bool async) {
  assert(!IsRelativeUrl(url));
  // TODO(michaeln): Add some of the sanity checks the IE implementation has.

  // Do not allow reuse of this object for multiple requests.
  assert(!IsComplete());
  
  if (!IsUninitialized()) {
    return false;
  }

  method_ = method;
  url_ = url;
  async_ = async;
  if (!origin_.InitFromUrl(url)) {
    return false;
  }
  
  delegate_holder_->delegate = [[HttpRequestDelegate alloc] initWithOwner:this];
  if (!delegate_holder_->delegate) {
    return false;
  }
  if (![delegate_holder_->delegate open:url method:method]) {
    return false;
  }

  SetReadyState(OPEN);
  return true;
}

//------------------------------------------------------------------------------
// SetRequestHeader
//------------------------------------------------------------------------------
bool SFHttpRequest::SetRequestHeader(const char16* name, const char16* value) {
  if (was_sent_) {
    return false;
  }
  
  headers_to_send_.push_back(HttpHeader(name, value));
  return true;
}

//------------------------------------------------------------------------------
// RedirectBehavior
//------------------------------------------------------------------------------

bool SFHttpRequest::WasRedirected() {
  return IsInteractiveOrComplete() && !was_aborted_ && was_redirected_;
}

bool SFHttpRequest::GetFinalUrl(std::string16 *full_url) {
  if (!IsInteractiveOrComplete() || was_aborted_) {
    return false;
  }

  if (WasRedirected()) {
    *full_url = redirect_url_;
  } else {
    *full_url = url_;
  }
  return true;
}

bool SFHttpRequest::GetInitialUrl(std::string16 *full_url) {
  *full_url = url_;  // may be empty if request has not occurred
  return true;
}

//------------------------------------------------------------------------------
// Send, SendString, SendBlob, SendImpl
//------------------------------------------------------------------------------

bool SFHttpRequest::Send() {
  if (IsPostOrPut()) {
    return SendString(STRING16(L""));
  } else {
    return SendImpl(nil);
  }
}


bool SFHttpRequest::SendString(const char16 *data) {
  if (was_sent_ || !data) {
    return false;
  }
  if (!IsPostOrPut()) {
    return false;
  }

  NSString *post_data_string = [NSString stringWithString16:data];
  NSData *post_data = [post_data_string dataUsingEncoding:NSUTF8StringEncoding];
  NSInputStream *post_data_stream = [NSInputStream
                                     inputStreamWithData:post_data];
  if (!post_data_stream) {
    return false;
  }
  // It appears that the OS defaults to sending out the request with
  // chunked encoding if we assign it an inputstream.  To solve this we
  // need to explicitly set the Content-length header.
  std::string16 content_length_as_string(
      IntegerToString16([post_data_string length]));
  headers_to_send_.push_back(HttpHeader(HttpConstants::kContentLengthHeader,
                                        content_length_as_string));
  return SendImpl(post_data_stream);
}

#ifndef OFFICIAL_BUILD

bool SFHttpRequest::SendBlob(BlobInterface *blob) {
  if (was_sent_) {
    return false;
  }
  NSInputStream *post_data_stream =
      [[[BlobInputStream alloc] initFromBlob:blob] autorelease];
  if (!post_data_stream) {
    return false;
  }
  // It appears that the OS defaults to sending out the request with
  // chunked encoding if we assign it an inputstream.  To solve this we
  // need to explicitly set the Content-length header.
  std::string16 content_length_as_string(Integer64ToString16(blob->Length()));
  headers_to_send_.push_back(HttpHeader(HttpConstants::kContentLengthHeader,
                                        content_length_as_string));
  bool ok = SendImpl(post_data_stream);
  return ok;
}
#endif

bool SFHttpRequest::SendImpl(NSInputStream *post_data_stream) {
  if (!IsOpen()) {
    return false;
  }
  
  if (was_sent_) {
    return false;
  }
  was_sent_ = true;
  
  std::string16 user_agent;
  if (!BrowserUtils::GetUserAgentString(&user_agent)) {
    return false;
  }
  
  // TODO(playmobil): Handle case of ShouldBypassLocalServer().
  if (![delegate_holder_->delegate send:post_data_stream
                              userAgent:user_agent
                                headers:headers_to_send_
                     bypassBrowserCache:ShouldBypassBrowserCache()]) {
     Reset();
     return false;
  }
  
  scoped_refptr<SFHttpRequest> reference(this);
  SetReadyState(SENT);
  
  // Block until completion on synchronous requests.
  if (!async_) {
    // NSURLConnection has a default idle timeout of 60 seconds after which the
    // connection should be terminated, we rely on this to ensure that the
    // loop exits.
    const CFTimeInterval kTenMilliseconds = 10*10e-3;
    
    
    // TODO(playmobil): Reexamine this code when implementing Worker pools, and
    // consider logging cases in which NSURLConnection blocks for longer than
    // we expect.
    
    // Make sure the object isn't deleted while waiting for the request to
    // finish.
    scoped_refptr<HttpRequest> hold(this);
    while (ready_state_ != HttpRequest::COMPLETE && !was_aborted_) {
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, kTenMilliseconds, false);
    }
  }
  
  return true;
}

//------------------------------------------------------------------------------
// GetAllResponseHeaders
//------------------------------------------------------------------------------
bool SFHttpRequest::GetAllResponseHeaders(std::string16 *headers) {
  if (!(IsInteractiveOrComplete() && !was_aborted_)) {
    return false;
  }
  
  const char16 *kNameValueSeperator = STRING16(L": "); 
  
  HttpHeaderVector response_headers;
  [delegate_holder_->delegate headers:&response_headers];
  std::string16 header_str;
  for (HttpHeaderVectorConstIterator hdr = response_headers.begin();
       hdr != response_headers.end();
       ++hdr) {
    header_str += hdr->first;
    header_str += kNameValueSeperator;
    header_str += hdr->second;
    header_str += HttpConstants::kCrLf;
  }
  
  // Stash the MIMEType as a custom header.
  // TODO(playmobil): This leaks back to the scriptable object layer,
  // we may want to fix that at a later date.
  std::string16 mime_type;
  [delegate_holder_->delegate mimeType:&mime_type];
  header_str += HttpConstants::kXGearsSafariCapturedMimeType;
  header_str += kNameValueSeperator;
  header_str += mime_type;
  header_str += HttpConstants::kCrLf;
  
  header_str += HttpConstants::kCrLf;  // blank line at the end
  *headers = header_str;
  
  return true;
}

//------------------------------------------------------------------------------
// GetResponseHeader
//------------------------------------------------------------------------------
bool SFHttpRequest::GetResponseHeader(const char16 *name,
                                      std::string16 *value) {
  if (!(IsInteractiveOrComplete() && !was_aborted_)) {
    return false;
  }
  
  [delegate_holder_->delegate headerByName:name value:value];
    
  return true;
}

//------------------------------------------------------------------------------
// Abort
//------------------------------------------------------------------------------
bool SFHttpRequest::Abort() {
  [delegate_holder_->delegate abort];
  
  was_aborted_ = true;
  SetReadyState(COMPLETE);
  return true;
}

//------------------------------------------------------------------------------
// SetListener
//------------------------------------------------------------------------------
bool SFHttpRequest::SetListener(HttpListener *listener, 
                                bool enable_data_available) {
  listener_ = listener;
  listener_data_available_enabled_ = enable_data_available;
  return true;
}

//------------------------------------------------------------------------------
// SetReadyState
//------------------------------------------------------------------------------
void SFHttpRequest::SetReadyState(ReadyState state) {
  if (state > ready_state_) {
    ready_state_ = state;
    if (listener_) {
      listener_->ReadyStateChanged(this);
    }
  }
}

//------------------------------------------------------------------------------
// OnDataAvailable
//------------------------------------------------------------------------------
void SFHttpRequest::OnDataAvailable() {
  scoped_refptr<SFHttpRequest> reference(this);
  SetReadyState(HttpRequest::INTERACTIVE);
  
  if (was_aborted_) {
    return;
  }
  
  if (listener_ && listener_data_available_enabled_) {
    listener_->DataAvailable(this);
  }
}

//------------------------------------------------------------------------------
// AllowRedirect
//------------------------------------------------------------------------------
bool SFHttpRequest::AllowRedirect(const std::string16 &redirect_url) {
  bool follow = false;
  switch (redirect_behavior_) {
    case FOLLOW_ALL:
      follow = true;
      break;

    case FOLLOW_NONE:
      follow = false;
      break;

    case FOLLOW_WITHIN_ORIGIN:
      follow = origin_.IsSameOriginAsUrl(redirect_url.c_str());
      break;
  }

  if (follow) {
    was_redirected_ = true;
    redirect_url_ = redirect_url;
  }

  return follow;
}

//------------------------------------------------------------------------------
// Reset
//------------------------------------------------------------------------------
void SFHttpRequest::Reset() {
  [delegate_holder_->delegate abort];
  [delegate_holder_->delegate release];
  delegate_holder_->delegate = NULL;
  listener_ = NULL;
  listener_data_available_enabled_ = false;
  method_.clear();
  url_.clear();
  caching_behavior_ = USE_ALL_CACHES;
  redirect_behavior_ = FOLLOW_ALL;
  was_sent_ = false;
  was_aborted_ = false;
  was_redirected_ = false;
  async_ = false;
  redirect_url_.clear();
  ready_state_ = UNINITIALIZED;
}
