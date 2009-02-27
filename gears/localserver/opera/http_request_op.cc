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

#include "gears/localserver/opera/http_request_op.h"

#include <algorithm>
#include <vector>

#include "gears/base/common/byte_store.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/thread_locals.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/opera/opera_utils.h"
#include "gears/base/opera/browsing_context_op.h"
#include "gears/blob/blob_interface.h"
#include "gears/blob/blob_stream_ie.h"
#include "gears/blob/buffer_blob.h"
#include "gears/localserver/common/safe_http_request.h"

//------------------------------------------------------------------------------
// Create
//------------------------------------------------------------------------------
// static
bool HttpRequest::Create(scoped_refptr<HttpRequest> *request) {
  if (OperaUtils::GetBrowserThreadId() ==
      ThreadMessageQueue::GetInstance()->GetCurrentThreadId()) {
    OPHttpRequest *op_request = new OPHttpRequest();
    if (!op_request) {
      LOG16((L"HttpRequest::Create - Create failed\n"));
      return false;
    }
    request->reset(op_request);
    return true;
  } else {
    return HttpRequest::CreateSafeRequest(request);
  }
}

// static
bool HttpRequest::CreateSafeRequest(scoped_refptr<HttpRequest> *request) {
  request->reset(new SafeHttpRequest(OperaUtils::GetBrowserThreadId()));
  return true;
}

//------------------------------------------------------------------------------
// Construction, destruction and refcounting
//------------------------------------------------------------------------------

OPHttpRequest::OPHttpRequest()
    : caching_behavior_(USE_ALL_CACHES),
      redirect_behavior_(FOLLOW_ALL),
      was_redirected_(false),
      url_data_(NULL),
      was_aborted_(false),
      listener_(NULL),
      ready_state_(UNINITIALIZED),
      actual_data_size_(0),
      async_(false) {
}

OPHttpRequest::~OPHttpRequest() {
  if (url_data_) {
    url_data_->OnCompleted();
    url_data_ = NULL;
  }
}

void OPHttpRequest::Ref() {
  refcount_.Ref();
}

void OPHttpRequest::Unref() {
  if (refcount_.Unref())
    delete this;
}

//------------------------------------------------------------------------------
// GetReadyState
//------------------------------------------------------------------------------
bool OPHttpRequest::GetReadyState(ReadyState *state) {
  *state = ready_state_;
  return true;
}

//------------------------------------------------------------------------------
// GetResponseBody
//------------------------------------------------------------------------------
bool OPHttpRequest::GetResponseBody(scoped_refptr<BlobInterface> *blob) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;

  response_body_->CreateBlob(blob);
  return true;
}

//------------------------------------------------------------------------------
// GetStatus
//------------------------------------------------------------------------------
bool OPHttpRequest::GetStatus(int *status) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  *status = response_payload_.status_code;
  return true;
}

//------------------------------------------------------------------------------
// GetStatusText
// TODO(michaeln): remove this method from the interface, prefer getStatusLine
//------------------------------------------------------------------------------
bool OPHttpRequest::GetStatusText(std::string16 *status_text) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  return ParseHttpStatusLine(response_payload_.status_line,
                             NULL, NULL, status_text);
}

//------------------------------------------------------------------------------
// GetStatusLine
//------------------------------------------------------------------------------
bool OPHttpRequest::GetStatusLine(std::string16 *status_line) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  *status_line = response_payload_.status_line;
  return true;
}

//------------------------------------------------------------------------------
// WasRedirected
//------------------------------------------------------------------------------
bool OPHttpRequest::WasRedirected() {
  return IsInteractiveOrComplete() && !was_aborted_ && was_redirected_;
}

//------------------------------------------------------------------------------
// GetFinalUrl
//------------------------------------------------------------------------------
bool OPHttpRequest::GetFinalUrl(std::string16 *full_url) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;

  if (WasRedirected())
    *full_url = redirect_url_;
  else
    *full_url = url_;
  return true;
}

//------------------------------------------------------------------------------
// GetInitialUrl
//------------------------------------------------------------------------------
bool OPHttpRequest::GetInitialUrl(std::string16 *full_url) {
  *full_url = url_;  // may be empty if request has not occurred
  return true;
}

//------------------------------------------------------------------------------
// Open
//------------------------------------------------------------------------------
bool OPHttpRequest::Open(const char16 *method,
                         const char16 *url,
                         bool async,
                         BrowsingContext *browsing_context) {
  assert(!IsRelativeUrl(url));
  if (!IsUninitialized())
    return false;

  async_ = async;
  url_ = url;
  if (!origin_.InitFromUrl(url)) {
    return false;
  }

  browsing_context_.reset(static_cast<OPBrowsingContext*>(browsing_context));

  added_request_headers_iter_ = added_request_headers_.end();

  method_ = MakeUpperString(std::string16(method));

  SetReadyState(HttpRequest::OPEN);
  return true;
}

//------------------------------------------------------------------------------
// SetRequestHeader
// Here we gather additional request headers to be sent.
//------------------------------------------------------------------------------
bool OPHttpRequest::SetRequestHeader(const char16 *name, const char16 *value) {
  if (!IsOpen())
    return false;

  HeaderEntry new_header = HeaderEntry(name, value);
  added_request_headers_.push_back(new_header);
  added_request_headers_iter_ = added_request_headers_.begin();

  return true;
}

//------------------------------------------------------------------------------
// Send
//------------------------------------------------------------------------------
bool OPHttpRequest::Send(BlobInterface *blob) {
  post_data_.reset(blob ? blob : new EmptyBlob);
  post_data_offset_ = 0;
  if (IsPostOrPut()) {
    std::string16 size_str;
    if (post_data_)
      size_str = Integer64ToString16(post_data_->Length());
    else
      size_str = Integer64ToString16(0);

    SetRequestHeader(HttpConstants::kContentLengthHeader, size_str.c_str());
  }

  return SendImpl();
}

//------------------------------------------------------------------------------
// GetAllResponseHeaders
//------------------------------------------------------------------------------
bool OPHttpRequest::GetAllResponseHeaders(std::string16 *headers) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  headers->assign(response_payload_.headers);
  return true;
}

//------------------------------------------------------------------------------
// GetResponseCharset
//------------------------------------------------------------------------------
std::string16 OPHttpRequest::GetResponseCharset() {
  if (url_data_) {
    const char *charset = NULL;
    url_data_->GetResponseCharset(&charset);

    std::string16 charset16;
    if (charset)
      UTF8ToString16(charset, &charset16);
    return charset16;
  }

  return std::string16();
}

//------------------------------------------------------------------------------
// GetResponseHeader
//------------------------------------------------------------------------------
bool OPHttpRequest::GetResponseHeader(const char16 *name,
                                      std::string16 *value) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  return response_payload_.GetHeader(name, value);
}

//------------------------------------------------------------------------------
// Abort
//------------------------------------------------------------------------------
bool OPHttpRequest::Abort() {
  SetComplete();

  was_aborted_ = true;
  return true;
}

//------------------------------------------------------------------------------
// SetListener
//------------------------------------------------------------------------------
bool OPHttpRequest::SetListener(HttpListener *listener,
                                bool enable_data_available) {
  listener_ = listener;
  return true;
}

//------------------------------------------------------------------------------
// GetNextAddedReqHeader
//
// This function will only by called from the browser while we are in the
// call to RequestUrl() in SendImpl() so there will be no calls to
// SetRequestHeader() that can disrupt the sequence of the iterator.
//------------------------------------------------------------------------------
bool OPHttpRequest::GetNextAddedReqHeader(const unsigned short **name,
                                          const unsigned short **value) {
  if (added_request_headers_iter_ == added_request_headers_.end())
    return false;

  *name = added_request_headers_iter_->first.c_str();
  *value = added_request_headers_iter_->second.c_str();

  added_request_headers_iter_++;

  return true;
}

const unsigned short *OPHttpRequest::GetUrl() {
  assert(url_.c_str());
  return url_.c_str();
}

const unsigned short *OPHttpRequest::GetMethod() {
  return method_.c_str();
}

long OPHttpRequest::GetPostData(unsigned char *buffer, long max_size) {
  int64 read;
  // max_size of -1 is interpreted as a query for the size of the data
  if (max_size == -1) {
    read = post_data_.get()->Length();
    if (read > LONG_MAX)
      return -1;  // -1 returned means too long for long
  } else {
    read = post_data_.get()->Read(buffer, post_data_offset_, max_size);
    post_data_offset_ += read;
  }

  // Safe cast since max_size is a long value the return value cannot be larger
  // than long and there is a check in the Length() calculation already.
  return static_cast<long>(read);
}

// This method will only be called by the browser after a call to
// RequestUrl() in SendImpl() and a call to OnRequestCreated() from
// the browser.
void OPHttpRequest::OnDataReceived() {
  assert(ready_state_ >= HttpRequest::SENT);

  if (ready_state_ == HttpRequest::SENT) {
    DWORD status_code = url_data_->GetStatus();
    // Initialize the payload
    LOG16((L"OPHttpRequest::OnDataReceived (%d)\n", status_code));

    response_payload_.status_code = status_code;

    const char *status_line;
    url_data_->GetStatusText(&status_line);
    if (status_line) {
      UTF8ToString16(status_line, strlen(status_line),
                     &response_payload_.status_line);
    }

    const char *response_headers;
    url_data_->GetResponseHeaders(&response_headers);
    if (response_headers) {
      UTF8ToString16(response_headers, strlen(response_headers),
                     &response_payload_.headers);
    }

    actual_data_size_ = 0;
    response_body_.reset(new ByteStore);

    SetReadyState(HttpRequest::INTERACTIVE);
  }

  if (url_data_) {
    const char *content;
    unsigned long content_length;
    url_data_->GetBodyText(&content, &content_length);

    if (content_length && content) {
      // Append the new data to the payload
      response_body_->AddData(content, content_length);

      if (listener_)
        listener_->DataAvailable(this, response_body_->Length());
    }
  }

  if (!url_data_ || url_data_->IsFinished()) {
    SetReadyState(HttpRequest::COMPLETE);

    // reset data source so that we avoid more calls to this
    // object after we have finised.
    SetComplete();
  }
}

OperaHttpListenerInterface::RedirectStatus OPHttpRequest::OnRedirected(
    const unsigned short *redirect_url) {
  bool follow = false;
  switch (redirect_behavior_) {
  case FOLLOW_ALL:
    follow = true;
    break;

  case FOLLOW_NONE:
    follow = false;
    break;

  case FOLLOW_WITHIN_ORIGIN:
    follow = origin_.IsSameOriginAsUrl(redirect_url);
    break;
  }

  if (!follow)
    return OperaHttpListenerInterface::REDIRECT_STATUS_CANCEL;
  was_redirected_ = true;
  redirect_url_ = redirect_url;
  return OperaHttpListenerInterface::REDIRECT_STATUS_OK;
}

void OPHttpRequest::OnError(OperaHttpListenerInterface::LoadingError err) {
  Abort();
}

// This method will only be called as a result of a call to
// RequestUrl() in SendImpl(), when the browser has created its
// internal representation of the request. url_data_ will only
// be valid after this call.
void OPHttpRequest::OnRequestCreated(OperaUrlDataInterface *data) {
  assert(data);
  Ref();  // Will be Unref()'ed in SetComplete()
  url_data_ = data;
  SetReadyState(HttpRequest::SENT);
}

void OPHttpRequest::OnRequestDestroyed() {
  SetComplete();
}

void OPHttpRequest::SetComplete() {
  if (url_data_) {
    url_data_->OnCompleted();
    url_data_ = NULL;

    Unref();  // Unrefing the reference set by OnRequestCreated()
  }
}

bool OPHttpRequest::SendImpl() {
  if (!IsOpen() || url_.empty() || !browsing_context_.get())
    return false;

  OperaGearsApiInterface *opera_api = OperaUtils::GetBrowserApiForGears();
  opera_api->RequestUrl(browsing_context_->GetJsContext(), this, this);

  return !was_aborted_;
}

void OPHttpRequest::SetReadyState(ReadyState state) {
  if (state > ready_state_) {
    ready_state_ = state;
    if (listener_) {
      listener_->ReadyStateChanged(this);
    }
  }
}
