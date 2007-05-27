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

#include <nsIHttpChannel.h>
#include <nsIHttpHeaderVisitor.h>
#include <nsIInputStream.h>
#include <nsIIOService.h>
#include <nsIURI.h>
#include "gears/third_party/gecko_internal/nsIEncodedChannel.h"
#include "gears/base/common/http_utils.h"
#include "gears/base/common/string_utils.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/firefox/http_request_ff.h"

// From nsNetError.h:
#define NS_BINDING_ABORTED 0x804b0002

NS_IMPL_ISUPPORTS5(FFHttpRequest,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIChannelEventSink,
                   nsIInterfaceRequestor,
                   SpecialHttpRequestInterface)

//------------------------------------------------------------------------------
// HttpRequest::Create
//------------------------------------------------------------------------------
HttpRequest *HttpRequest::Create() {
  FFHttpRequest *request = new FFHttpRequest();
  request->AddReference();
  return request;
}

//------------------------------------------------------------------------------
// Constructor / Destructor / Refcounting
//------------------------------------------------------------------------------
FFHttpRequest::FFHttpRequest()
  : state_(0), was_sent_(false), is_complete_(false), was_aborted_(false), 
    follow_redirects_(true), was_redirected_(false), listener_(NULL) {
}

FFHttpRequest::~FFHttpRequest() {
}

int FFHttpRequest::AddReference() {
  return AddRef();
}

int FFHttpRequest::ReleaseReference() {
  return Release();
}

//------------------------------------------------------------------------------
// getReadyState
//------------------------------------------------------------------------------
bool FFHttpRequest::getReadyState(long *state) {
  *state = state_;
  return true;
}

//------------------------------------------------------------------------------
// getResponseBody
//------------------------------------------------------------------------------
bool FFHttpRequest::getResponseBody(std::vector<unsigned char> *body) {
  NS_ENSURE_TRUE(is_complete_ && !was_aborted_, false);
  if (!response_body_.get()) {
    return false;
  }
  body->resize(response_body_->size());
  if (body->size() != response_body_->size()) {
    return false;
  }
  memcpy(&(*body)[0], &(*response_body_)[0], response_body_->size());
  return true;
}

//------------------------------------------------------------------------------
// getResponseBody
//------------------------------------------------------------------------------
std::vector<unsigned char> *FFHttpRequest::getResponseBody() {
  NS_ENSURE_TRUE(is_complete_ && !was_aborted_, NULL);
  return response_body_.release();
}

//------------------------------------------------------------------------------
// getStatus
//------------------------------------------------------------------------------
bool FFHttpRequest::getStatus(long *status) {
  NS_ENSURE_TRUE(is_complete_ && !was_aborted_, false);
  nsCOMPtr<nsIHttpChannel> http_channel = GetCurrentHttpChannel();
  if (!http_channel) {
    return false;
  }
  PRUint32 pr_status;
  nsresult rv = http_channel->GetResponseStatus(&pr_status);
  NS_ENSURE_SUCCESS(rv, false);
  *status = static_cast<long>(pr_status);
  return true;
}

//------------------------------------------------------------------------------
// getStatusText
//------------------------------------------------------------------------------
bool FFHttpRequest::getStatusText(std::string16 *status_text) {
  NS_ENSURE_TRUE(is_complete_ && !was_aborted_, false);
  nsCOMPtr<nsIHttpChannel> http_channel = GetCurrentHttpChannel();
  if (!http_channel) {
    return false;
  }
  nsCString status_str;
  nsresult rv = http_channel->GetResponseStatusText(status_str);
  NS_ENSURE_SUCCESS(rv, false);
  return UTF8ToString16(status_str.get(), status_str.Length(), status_text);
}

//------------------------------------------------------------------------------
// getStatusLine
//------------------------------------------------------------------------------
bool FFHttpRequest::getStatusLine(std::string16 *status_line) {
  NS_ENSURE_TRUE(is_complete_ && !was_aborted_, false);
  // TODO(michaeln): get the actual status line instead of synthesizing one
  nsCOMPtr<nsIHttpChannel> http_channel = GetCurrentHttpChannel();
  if (!http_channel) {
    return false;
  }

  nsCString status_text;
  nsresult rv = http_channel->GetResponseStatusText(status_text);
  NS_ENSURE_SUCCESS(rv, false);

  long status_code;
  if (!getStatus(&status_code))
    return false;
  char status_code_str[32];
  sprintf(status_code_str, "%d", static_cast<int>(status_code));

  nsCString status_line8;
  status_line8.Assign("HTTP/1.1 ");
  status_line8.Append(status_code_str);
  status_line8.Append(" ");
  status_line8.Append(status_text);

  return UTF8ToString16(status_line8.get(), status_line8.Length(), status_line);
}

//------------------------------------------------------------------------------
// open
//------------------------------------------------------------------------------
bool FFHttpRequest::open(const char16 *method, const char16 *url, bool async) {
  NS_ENSURE_TRUE(state_ == 0, false);

  nsCOMPtr<nsIIOService> ios =
    do_GetService("@mozilla.org/network/io-service;1");
  NS_ENSURE_TRUE(ios, false);

  std::string url_utf8;
  if (!String16ToUTF8(url, &url_utf8)) {
    return false;
  }
  nsCString url_str(url_utf8.c_str());
  nsresult rv = ios->NewChannel(url_str, nsnull, nsnull,
                                getter_AddRefs(channel_));
  NS_ENSURE_SUCCESS(rv, false);
  NS_ENSURE_TRUE(channel_, false);

  rv = channel_->SetLoadFlags(nsIRequest::LOAD_BYPASS_CACHE |
                              nsIRequest::INHIBIT_CACHING);
  NS_ENSURE_SUCCESS(rv, false);

  std::string method_utf8;
  if (!String16ToUTF8(method, &method_utf8)) {
    return false;
  }
  nsCString method_str(method_utf8.c_str());
  nsCOMPtr<nsIHttpChannel> http_channel = GetCurrentHttpChannel();
  NS_ENSURE_TRUE(http_channel, false);
  rv = http_channel->SetRequestMethod(method_str);
  NS_ENSURE_SUCCESS(rv, false);

  SetReadyState(1);
  return true;
}

//------------------------------------------------------------------------------
// setRequestHeader
//------------------------------------------------------------------------------
bool FFHttpRequest::setRequestHeader(const char16* name, const char16* value) {
  if (was_sent_)
    return false;
  nsCOMPtr<nsIHttpChannel> http_channel = GetCurrentHttpChannel();
  NS_ENSURE_TRUE(http_channel, false);

  std::string name_utf8;
  if (!String16ToUTF8(name, &name_utf8)) {
    return false;
  }
  std::string value_utf8;
  if (!String16ToUTF8(value, &value_utf8)) {
    return false;
  }
  nsCString name_str(name_utf8.c_str());
  nsCString value_str(value_utf8.c_str());
  nsresult rv = http_channel->SetRequestHeader(name_str, value_str, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, false);
  return true;
}

//------------------------------------------------------------------------------
// setFollowRedirects
//------------------------------------------------------------------------------
bool FFHttpRequest::setFollowRedirects(bool follow) {
  assert(channel_);
  if (was_sent_)
    return false;
  follow_redirects_ = follow;
  return true;
}

bool FFHttpRequest::wasRedirected() {
  return follow_redirects_ && is_complete_ && was_redirected_ && !was_aborted_;
}

bool FFHttpRequest::getRedirectUrl(std::string16 *full_redirect_url) {
  if (!wasRedirected())
    return false;
  *full_redirect_url = redirect_url_;
  return true;
}


//------------------------------------------------------------------------------
// send
//------------------------------------------------------------------------------
bool FFHttpRequest::send() {
  NS_ENSURE_TRUE(channel_ && !was_sent_, false);
  if (follow_redirects_) {
    channel_->SetNotificationCallbacks(this);
  } else {
    nsCOMPtr<nsIHttpChannel> http_channel = GetCurrentHttpChannel();
    NS_ENSURE_TRUE(http_channel, false);
    nsresult rv = http_channel->SetRedirectionLimit(0);
    NS_ENSURE_SUCCESS(rv, false);
  }
  nsresult rv = channel_->AsyncOpen(this, nsnull);
  NS_ENSURE_SUCCESS(rv, false);
  was_sent_ = true;
  SetReadyState(1);
  return true;
}

//------------------------------------------------------------------------------
// HeaderVisitor, used to implement getAllResponseHeaders()
//------------------------------------------------------------------------------
class HeaderVisitor : public nsIHttpHeaderVisitor {
public:
  // Stack based
  NS_IMETHODIMP_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHODIMP_(nsrefcnt) Release() { return 1; }

  NS_IMETHODIMP QueryInterface(const nsIID &iid, void **result) {
    if (iid.Equals(NS_GET_IID(nsIHttpHeaderVisitor)) ||
        iid.Equals(NS_GET_IID(nsISupports))) {
      *result = this;
      return NS_OK;
    }
    return NS_ERROR_NO_INTERFACE;
  }

  NS_IMETHODIMP VisitHeader(const nsACString &header, const nsACString &value) {
    std::string header_str(header.BeginReading(), header.Length());
    std::string value_str(value.BeginReading(), value.Length());
    headers_.SetHeader(header_str.c_str(),
                       value_str.c_str(),
                       HTTPHeaders::APPEND);
    return NS_OK;
  }

  HeaderVisitor() { }
  virtual ~HeaderVisitor() {}
  HTTPHeaders headers_;
};

//------------------------------------------------------------------------------
// getAllResponseHeaders
//------------------------------------------------------------------------------
bool FFHttpRequest::getAllResponseHeaders(std::string16 *headers) {
  NS_ENSURE_TRUE(is_complete_ && !was_aborted_, false);
  nsCOMPtr<nsIHttpChannel> http_channel = GetCurrentHttpChannel();
  NS_ENSURE_TRUE(http_channel, false);

  HeaderVisitor visitor;
  nsresult rv = http_channel->VisitResponseHeaders(&visitor);
  NS_ENSURE_SUCCESS(rv, false);

  // We fix up the "Content-Length" header and remove any "Content-Encoding"
  // headers since the response body we have has already been decoded.
  // Otherwise when replaying a cached entry, the browser would try to
  // decode an already decoded response body and fail.
  if (response_body_.get()) {
    std::string data_len_str;
    IntegerToString(static_cast<int>(response_body_.get()->size()),
                    &data_len_str);
    visitor.headers_.SetHeader(HTTPHeaders::CONTENT_LENGTH,
                               data_len_str.c_str(),
                               HTTPHeaders::OVERWRITE);
    visitor.headers_.ClearHeader(HTTPHeaders::CONTENT_ENCODING);
  }

  std::string header_str;
  for (HTTPHeaders::const_iterator hdr = visitor.headers_.begin();
       hdr != visitor.headers_.end();
       ++hdr) {
    if (hdr->second != NULL) {  // NULL means do not output
      header_str += hdr->first;
      header_str += ": ";
      header_str += hdr->second;
      header_str += HttpConstants::kCrLfAscii;
    }
  }
  header_str += HttpConstants::kCrLfAscii;  // blank line at the end
  return UTF8ToString16(header_str.c_str(), header_str.length(), headers);
}

//------------------------------------------------------------------------------
// getResponseHeader
//------------------------------------------------------------------------------
bool FFHttpRequest::getResponseHeader(const char16* name,
                                      std::string16 *value) {
  NS_ENSURE_TRUE(is_complete_ && !was_aborted_, false);
  nsCOMPtr<nsIHttpChannel> http_channel = GetCurrentHttpChannel();
  NS_ENSURE_TRUE(http_channel, false);

  std::string name_utf8;
  if (!String16ToUTF8(name, &name_utf8)) {
    return false;
  }
  nsCString name_str(name_utf8.c_str());
  nsCString value_str;
  nsresult rv = http_channel->GetResponseHeader(name_str, value_str);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    value->clear();
    return true;
  }
  return UTF8ToString16(value_str.get(), value_str.Length(), value);
}

//------------------------------------------------------------------------------
// abort
//------------------------------------------------------------------------------
bool FFHttpRequest::abort() {
  // NS_BINDING_ABORTED is a special error code reserved for this purpose that
  // should not be generated for any other reason.
  if (channel_) {
    channel_->Cancel(NS_BINDING_ABORTED);
    channel_ = NULL;
  }
  return true;
}

//------------------------------------------------------------------------------
// setOnReadyStateChange
//------------------------------------------------------------------------------
bool FFHttpRequest::setOnReadyStateChange(ReadyStateListener *listener) {
  listener_ = listener;
  return true;
}

//------------------------------------------------------------------------------
// SetReadyState
//------------------------------------------------------------------------------
void FFHttpRequest::SetReadyState(int state) {
  if (state != state_) {
    state_ = state;
    is_complete_ = (state == 4);
    if (listener_) {
      listener_->ReadyStateChanged(this);
    }
  }
}

//------------------------------------------------------------------------------
// OnStartRequest
//------------------------------------------------------------------------------
NS_IMETHODIMP FFHttpRequest::OnStartRequest(nsIRequest *request,
                                            nsISupports *context) {
  NS_ENSURE_TRUE(channel_, NS_ERROR_UNEXPECTED);
  response_body_.reset(new std::vector<unsigned char>);
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(request));
  if (chan) {
    PRInt32 length = -1;
    chan->GetContentLength(&length);
    if (length >= 0) {
      response_body_->reserve(length);
    }
  }
  SetReadyState(1);
  return NS_OK;
}

//------------------------------------------------------------------------------
// StreamReaderFunc
//------------------------------------------------------------------------------
NS_METHOD FFHttpRequest::StreamReaderFunc(nsIInputStream *stream,
                                          void *closure,
                                          const char *from_segment,
                                          PRUint32 to_offset,
                                          PRUint32 count,
                                          PRUint32 *write_count) {
  FFHttpRequest *self = reinterpret_cast<FFHttpRequest*>(closure);
  std::vector<unsigned char> *body = self->response_body_.get();
  if (!body) {
    return NS_ERROR_UNEXPECTED;
  }
  size_t cur_size = body->size();
  size_t needed_size = cur_size + count;
  if (body->capacity() < needed_size) {
    body->reserve(needed_size * 2);
  }
  body->resize(needed_size);
  if (body->size() != cur_size + count) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  const unsigned char *p = reinterpret_cast<const unsigned char*>(from_segment);
  memcpy(&(*body)[cur_size], p, count);
  *write_count = count;
  return NS_OK;
}

//------------------------------------------------------------------------------
// OnDataAvailable
//------------------------------------------------------------------------------
NS_IMETHODIMP FFHttpRequest::OnDataAvailable(nsIRequest *request,
                                             nsISupports *context,
                                             nsIInputStream *stream,
                                             PRUint32 offset,
                                             PRUint32 count) {
  NS_ENSURE_TRUE(channel_, NS_ERROR_UNEXPECTED);
  SetReadyState(3);
  PRUint32 n;
  return stream->ReadSegments(StreamReaderFunc, this, count, &n);
}

//------------------------------------------------------------------------------
// OnStopRequest
//------------------------------------------------------------------------------
NS_IMETHODIMP FFHttpRequest::OnStopRequest(nsIRequest *request,
                                           nsISupports *context,
                                           nsresult status) {
  NS_ENSURE_TRUE(channel_, NS_ERROR_UNEXPECTED);
  if (follow_redirects_) {
    channel_->SetNotificationCallbacks(NULL);
  }
  SetReadyState(4);
  return NS_OK;
}

//------------------------------------------------------------------------------
// GetCurrentHttpChannel
//------------------------------------------------------------------------------
already_AddRefed<nsIHttpChannel> FFHttpRequest::GetCurrentHttpChannel() {
  nsIHttpChannel *http_channel = nsnull;
  if (channel_) {
    CallQueryInterface(channel_, &http_channel);
  }
  return http_channel;
}

//------------------------------------------------------------------------------
// nsIChannelEventSink::OnChannelRedirect
//------------------------------------------------------------------------------
NS_IMETHODIMP FFHttpRequest::OnChannelRedirect(nsIChannel *old_channel,
                                               nsIChannel *new_channel,
                                               PRUint32 flags) {
  NS_PRECONDITION(new_channel, "Redirect without a channel?");

  redirect_url_.clear();

  // Get the redirect url
  nsCOMPtr<nsIURI> url;
  nsresult rv = new_channel->GetURI(getter_AddRefs(url));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString url_utf8;
  rv = url->GetSpec(url_utf8);
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert to string16 and remember where we've been redirected to
  nsString url_utf16;
  rv = NS_CStringToUTF16(url_utf8, NS_CSTRING_ENCODING_UTF8, url_utf16);
  NS_ENSURE_SUCCESS(rv, rv);
  redirect_url_ = url_utf16.get();
  was_redirected_ = true;

  // We now have a new channel
  channel_ = new_channel;
  return NS_OK;
}

//-----------------------------------------------------------------------------
// nsIInterfaceRequestor::GetInterface
//-----------------------------------------------------------------------------
NS_IMETHODIMP FFHttpRequest::GetInterface(const nsIID &iid, void **result) {
  return QueryInterface(iid, result);
}
