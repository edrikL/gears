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

#include <msxml2.h>
#include <algorithm>
#include <vector>

#include "gears/localserver/ie/http_request_ie.h"

#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/ie/atl_headers.h"
#ifdef OFFICIAL_BUILD
#include "gears/base/ie/stream_buffer.h"
#else  // !OFFICIAL_BUILD
#include "gears/blob/blob_interface.h"
#include "gears/blob/blob_stream_ie.h"
#include "gears/blob/buffer_blob.h"
#endif  // !OFFICIAL_BUILD
#include "gears/localserver/ie/http_handler_ie.h"
#include "gears/localserver/ie/urlmon_utils.h"

// We use URLMON's pull-data model which requires making stream read calls
// beyond the amount of data currently available to force the underlying
// stream implmentation to read more data from the network. This constant
// determines how much data beyond the end we'll request for this purpose.
static const int kReadAheadAmount = 16 * 1024;

//------------------------------------------------------------------------------
// Create
//------------------------------------------------------------------------------
// static
bool HttpRequest::Create(scoped_refptr<HttpRequest>* request) {
  CComObject<IEHttpRequest> *ie_request;
  HRESULT hr = CComObject<IEHttpRequest>::CreateInstance(&ie_request);
  if (FAILED(hr)) {
    LOG16((L"HttpRequest::Create - CreateInstance failed - %d\n", hr));
    return false;
  }
  request->reset(ie_request);
  return true;
}

//------------------------------------------------------------------------------
// Construction, destruction and refcounting
//------------------------------------------------------------------------------

IEHttpRequest::IEHttpRequest()
    : caching_behavior_(USE_ALL_CACHES), redirect_behavior_(FOLLOW_ALL),
      was_redirected_(false), was_aborted_(false), listener_(NULL),
      ready_state_(UNINITIALIZED), has_synthesized_response_payload_(false),
      actual_data_size_(0), async_(false) {
}

HRESULT IEHttpRequest::FinalConstruct() {
  return S_OK;
}

void IEHttpRequest::FinalRelease() {
}

void IEHttpRequest::Ref() {
  AddRef();
}

void IEHttpRequest::Unref() {
  Release();
}

//------------------------------------------------------------------------------
// GetReadyState
//------------------------------------------------------------------------------
bool IEHttpRequest::GetReadyState(ReadyState *state) {
  *state = ready_state_;
  return true;
}

//------------------------------------------------------------------------------
// GetResponseBodyAsText
//------------------------------------------------------------------------------
bool IEHttpRequest::GetResponseBodyAsText(std::string16 *text) {
  assert(text);
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;

  std::vector<uint8> *data = response_payload_.data.get();
  if (!data || data->empty() || !actual_data_size_) {
    text->clear();
    return true;
  }

  // TODO(michaeln): detect charset and decode using MLang
  return UTF8ToString16(reinterpret_cast<const char*>(&(*data)[0]),
                        actual_data_size_, text);
}

//------------------------------------------------------------------------------
// GetResponseBody
//------------------------------------------------------------------------------
std::vector<uint8> *IEHttpRequest::GetResponseBody() {
  if (!IsComplete() || was_aborted_)
    return NULL;
  return response_payload_.data.release();
}

//------------------------------------------------------------------------------
// GetResponseBody
// TODO(michaeln): remove one or the other of these getResponseBody methods from
// the interface.
//------------------------------------------------------------------------------
bool IEHttpRequest::GetResponseBody(std::vector<uint8> *body) {
  if (!IsComplete() || was_aborted_)
    return false;
  if (response_payload_.data.get()) {
    body->swap(*response_payload_.data.get());
  } else {
    body->clear();
  }
  return true;
}

//------------------------------------------------------------------------------
// GetStatus
//------------------------------------------------------------------------------
bool IEHttpRequest::GetStatus(int *status) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  *status = response_payload_.status_code;
  return true;
}

//------------------------------------------------------------------------------
// GetStatusText
// TODO(michaeln): remove this method from the interface, prefer getStatusLine
//------------------------------------------------------------------------------
bool IEHttpRequest::GetStatusText(std::string16 *status_text) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  return ParseHttpStatusLine(response_payload_.status_line,
                             NULL, NULL, status_text);
}

//------------------------------------------------------------------------------
// GetStatusLine
//------------------------------------------------------------------------------
bool IEHttpRequest::GetStatusLine(std::string16 *status_line) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  *status_line = response_payload_.status_line;
  return true;
}

//------------------------------------------------------------------------------
// GetAllResponseHeaders
//------------------------------------------------------------------------------
bool IEHttpRequest::GetAllResponseHeaders(std::string16 *headers) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  headers->assign(response_payload_.headers);
  return true;
}

//------------------------------------------------------------------------------
// GetResponseHeader
//------------------------------------------------------------------------------
bool IEHttpRequest::GetResponseHeader(const char16* name,
                                      std::string16 *value) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  return response_payload_.GetHeader(name, value);
}

//------------------------------------------------------------------------------
// Open
//------------------------------------------------------------------------------
bool IEHttpRequest::Open(const char16 *method, const char16* url, bool async) {
  assert(!IsRelativeUrl(url));
  if (!IsUninitialized())
    return false;

  async_ = async;
  url_ = url;
  if (!origin_.InitFromUrl(url)) {
    return false;
  }
  method_ = method;
  UpperString(method_);
  if (method_ == HttpConstants::kHttpGET)
    bind_verb_ = BINDVERB_GET;
  else if (method_ == HttpConstants::kHttpPOST)
    bind_verb_ = BINDVERB_POST;
  else if (method == HttpConstants::kHttpPUT)
    bind_verb_ = BINDVERB_PUT;
  else
    bind_verb_ = BINDVERB_CUSTOM;
  SetReadyState(HttpRequest::OPEN);
  return true;
}

//------------------------------------------------------------------------------
// SetRequestHeader
// Here we gather additional request headers to be sent. They are plumbed
// into URLMON in our IHttpNegotiate::BeginningTransaction method.
//------------------------------------------------------------------------------
bool IEHttpRequest::SetRequestHeader(const char16* name, const char16* value) {
  if (!IsOpen())
    return false;
  additional_headers_ += name;
  additional_headers_ += L": ";
  additional_headers_ += value;
  additional_headers_ += HttpConstants::kCrLf;
  return true;
}

bool IEHttpRequest::WasRedirected() {
  return IsInteractiveOrComplete() && !was_aborted_ && was_redirected_;
}

bool IEHttpRequest::GetFinalUrl(std::string16 *full_url) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;

  if (WasRedirected())
    *full_url = redirect_url_;
  else
    *full_url = url_;
  return true;
}

bool IEHttpRequest::GetInitialUrl(std::string16 *full_url) {
  *full_url = url_;  // may be empty if request has not occurred
  return true;
}

//------------------------------------------------------------------------------
// Send, SendString, SendImp
//------------------------------------------------------------------------------

#ifdef OFFICIAL_BUILD
bool IEHttpRequest::Send() {
  if (IsPostOrPut()) {
    return SendString(L"");
  } else {
    return SendImpl();
  }
}

bool IEHttpRequest::SendString(const char16 *data) {
  if (!IsOpen() || !data) return false;
  if (!IsPostOrPut())
    return false;

  String16ToUTF8(data, &post_data_string_);

  // TODO(michaeln): do we have to set this or will URLMON do so based
  // on the size of our stream?
  std::string16 size_str = IntegerToString16(post_data_string_.size());
  SetRequestHeader(HttpConstants::kContentLengthHeader, size_str.c_str());
  return SendImpl();
}
#else  // !OFFICIAL_BUILD
bool IEHttpRequest::Send() {
  if (!IsOpen())
    return false;

  if (!IsPostOrPut())
    return SendImpl(NULL);

  scoped_refptr<BlobInterface> blob(new EmptyBlob);
  return SendImpl(blob.get());
}

bool IEHttpRequest::SendString(const char16 *data) {
  if (!IsOpen() || !IsPostOrPut() || !data)
    return false;

  std::string data8;
  String16ToUTF8(data, &data8);
  scoped_refptr<BufferBlob> blob(new BufferBlob(data8.data(), data8.size()));

  return SendImpl(blob.get());
}

bool IEHttpRequest::SendBlob(BlobInterface *data) {
  if (!IsOpen() || !IsPostOrPut() || !data)
    return false;

  return SendImpl(data);
}
#endif  // !OFFICIAL_BUILD

#ifdef OFFICIAL_BUILD
bool IEHttpRequest::SendImpl() {
#else  // !OFFICIAL_BUILD
bool IEHttpRequest::SendImpl(BlobInterface *data) {
  if (data) {
    post_data_ = data;

    // TODO(bpm): do we have to set this or will URLMON do so based
    // on the size of our stream?
    std::string16 size_str = Integer64ToString16(post_data_->Length());
    SetRequestHeader(HttpConstants::kContentLengthHeader, size_str.c_str());
  }
#endif  // !OFFICIAL_BUILD

  // The request can complete prior to Send returning depending on whether
  // the response is retrieved from the cache. We guard against a caller's
  // listener removing the last reference prior to return by adding our own
  // reference here.
  CComPtr<IUnknown> reference(_GetRawUnknown());

  if (!IsOpen() || url_.empty())
    return false;

  HRESULT hr = CreateURLMonikerEx(NULL, url_.c_str(), &url_moniker_,
                                  URL_MK_UNIFORM);
  if (FAILED(hr)) {
    return false;
  }
  hr = CreateBindCtx(0, &bind_ctx_);
  if (FAILED(hr)) {
    return false;
  }
  hr = RegisterBindStatusCallback(bind_ctx_,
                                  static_cast<IBindStatusCallback*>(this),
                                  0, 0L);
  if (FAILED(hr)) {
    return false;
  }
  CComPtr<IStream> stream;
  hr = url_moniker_->BindToStorage(bind_ctx_, 0,
                                   __uuidof(IStream),
                                   reinterpret_cast<void**>(&stream));
  if (FAILED(hr)) {
    return false;
  }

  return !was_aborted_;
}

//------------------------------------------------------------------------------
// Abort
//------------------------------------------------------------------------------
bool IEHttpRequest::Abort() {
  if (!binding_)
    return true;
  HRESULT hr = binding_->Abort();
  was_aborted_ = true;
  return SUCCEEDED(hr);
}

//------------------------------------------------------------------------------
// SetOnReadyStateChange
//------------------------------------------------------------------------------
bool IEHttpRequest::SetOnReadyStateChange(ReadyStateListener *listener) {
  listener_ = listener;
  return true;
}

void IEHttpRequest::SetReadyState(ReadyState state) {
  if (state > ready_state_) {
    ready_state_ = state;
    if (listener_) {
      listener_->ReadyStateChanged(this);
    }
  }
}

//------------------------------------------------------------------------------
// IServiceProvider::QueryService
// Implemented to return an interface pointer for IHttpNegotiate and
// to conduct hand-shaking with our HttpHandler to bypass our webcache.
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::QueryService(REFGUID guidService, REFIID riid,
                                         void** ppvObject) {
  ATLASSERT(ppvObject != NULL);
  if (ppvObject == NULL)
    return E_POINTER;
  *ppvObject = NULL;

  // Our HttpHandler (see http_handler_ie.cc h) provides a mechanism to
  // bypass the LocalServer that involves querying for a particular service
  // id that does not exists. Here we detect that query and set our handler
  // in bypass mode.
  if (InlineIsEqualGUID(guidService, HttpHandler::SID_QueryBypassCache) &&
      InlineIsEqualGUID(riid, HttpHandler::IID_QueryBypassCache)) {
    if (ShouldBypassLocalServer()) {
      HttpHandler::SetBypassCache();
    }
    return E_NOINTERFACE;
  }

  // Our superclass will return our IHttpNegotiate interface pointer for us
  return IServiceProviderImpl<IEHttpRequest>::QueryService(guidService, riid,
                                                           ppvObject);
}

//------------------------------------------------------------------------------
// IBindStatusCallback::OnStartBinding
// This method is called by URLMON once per bind operation, even if the bind
// involves a chain of redirects, its only called once at the start.
// Note: binding->Abort() should not be called within this callback, instead
// return E_FAIL to cancel the bind process
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::OnStartBinding(DWORD reserved, IBinding *binding) {
  LOG16((L"IEHttpRequest::OnStartBinding\n"));
  if (!binding) {
    return E_POINTER;
  }
  assert(!binding_);
  binding_ = binding;
  SetReadyState(HttpRequest::SENT);
  return S_OK;
}

// IBindStatusCallback
STDMETHODIMP IEHttpRequest::GetPriority(LONG *priority) {
  if (!priority)
    return E_POINTER;
  *priority = THREAD_PRIORITY_NORMAL;
  return S_OK;
}

// IBindStatusCallback
STDMETHODIMP IEHttpRequest::OnLowResource(DWORD reserved) {
  return S_OK;
}

//------------------------------------------------------------------------------
// IBindStatusCallback::OnProgress
// Implemented to receive redirect notifications
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::OnProgress(ULONG progress, ULONG progress_max,
                                       ULONG status_code, LPCWSTR status_text) {
#ifdef DEBUG
  LOG16((L"IEHttpRequest::OnProgress(%s (%d), %s)\n",
         GetBindStatusLabel(status_code), status_code,
         status_text ? status_text : L"NULL"));
#endif
  if (status_code == BINDSTATUS_REDIRECTING) {
    return OnRedirect(status_text);
  }
  return S_OK;
}

//------------------------------------------------------------------------------
// OnRedirect
// Depending on whether or not we're set up to follow redirects, this
// method either aborts the current bind operation, or remembers the location
// of the redirect and allows it to continue.
//------------------------------------------------------------------------------
HRESULT IEHttpRequest::OnRedirect(const char16 *redirect_url) {
  LOG16((L"IEHttpRequest::OnRedirect( %s )\n", redirect_url));

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

  if (!follow) {
    if (!binding_)
      return E_UNEXPECTED;
    // When we're not supposed to follow redirects, we abort the request when
    // a redirect is reported to us. This causes the bind to fail w/o ever
    // having seen the actual response data. Our HttpRequest interface dictates
    // that we return a valid reponse payload containing the redirect in this
    // case. Here we synthesize a valid redirect response for that purpose.
    // TODO(michaeln): we 'should' get the actual response data
    response_payload_.SynthesizeHttpRedirect(NULL, redirect_url);
    has_synthesized_response_payload_ = true;
    return E_ABORT;
  } else {
    was_redirected_ = true;
    redirect_url_ = redirect_url;
  }
  return S_OK;
}


//------------------------------------------------------------------------------
// IBindStatusCallback::OnStopBinding
// This is called once per bind operation in both success and failure cases.
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::OnStopBinding(HRESULT hresult, LPCWSTR error_text) {
  LOG16((L"IEHttpRequest::OnStopBinding\n"));
  binding_.Release();
  bind_ctx_.Release();
  url_moniker_.Release();
  SetReadyState(HttpRequest::COMPLETE);
  return S_OK;
}

//------------------------------------------------------------------------------
// IBindStatusCallback::GetBindInfo
// Called by URLMON to determine how the 'bind' should be conducted
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::GetBindInfo(DWORD *flags, BINDINFO *info) {
  LOG16((L"IEHttpRequest::GetBindInfo\n"));
  if (!info || !flags)
    return E_POINTER;
  if (!info->cbSize)
    return E_INVALIDARG;

  // When a POST results in a redirect, we GET the new url. The POSTMON
  // sample app does this; although, using IE7 I have not seen a second
  // call to GetBindInfo in this case. BeginningTransaction can get called
  // twice in specific circumstances (see that method for comments).
  // TODO(michaeln): run this in IE6 and see what happens?
  int bind_verb = bind_verb_;
  bool is_post_or_put = IsPostOrPut();
  if (is_post_or_put && was_redirected_) {
    bind_verb = BINDVERB_GET;
    is_post_or_put = false;
  }

  *flags = 0;

  // We use the pull-data model as push is unreliable.
  *flags |= BINDF_FROMURLMON | BINDF_PULLDATA | BINDF_NO_UI;

  if (async_) {
    *flags |= BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE;
  }

  if (ShouldBypassBrowserCache() || is_post_or_put) {
    // Setup bind flags such that we send a request all the way through
    // to the server, bypassing caching proxies between here and there;
    // and such that we don't write the response to the browser's cache.
    *flags |= BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE |
              BINDF_PRAGMA_NO_CACHE;
  }

  int save_size = static_cast<int>(info->cbSize);
  memset(info, 0, save_size);
  info->cbSize = save_size;
  info->dwBindVerb = bind_verb;
  if (bind_verb == BINDVERB_CUSTOM) {
    info->szCustomVerb = static_cast<LPWSTR>
        (CoTaskMemAlloc((method_.size() + 1) * sizeof(char16)));
    wcscpy(info->szCustomVerb, method_.c_str());
  }

#ifdef OFFICIAL_BUILD
  if (is_post_or_put && !post_data_string_.empty()) {
    CComObject<StreamBuffer> *buf = NULL;
    HRESULT hr = CComObject<StreamBuffer>::CreateInstance(&buf);
    if (FAILED(hr))
      return hr;
    buf->Initialize(post_data_string_.data(), post_data_string_.size());
    info->stgmedData.tymed = TYMED_ISTREAM;
    info->stgmedData.pstm = static_cast<IStream*>(buf);
    // buf has a 0 reference count at this point.  The caller of GetBindInfo
    // will immediately do an AddRef on buf.
  }
#else  // !OFFICIAL_BUILD
  if (is_post_or_put && post_data_.get()) {
    CComObject<BlobStream> *stream = NULL;
    HRESULT hr = CComObject<BlobStream>::CreateInstance(&stream);
    if (FAILED(hr))
      return hr;
    stream->Initialize(post_data_.get(), 0);
    info->stgmedData.tymed = TYMED_ISTREAM;
    info->stgmedData.pstm = static_cast<IStream*>(stream);
    // stream has a 0 reference count at this point.  The caller of GetBindInfo
    // will immediately do an AddRef on stream.
  }
#endif  // !OFFICIAL_BUILD

  return S_OK;
}

//------------------------------------------------------------------------------
// IBindStatusCallback::OnDataAvailable
// Called by URLMON to inform us of data being available for reading
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::OnDataAvailable(
    DWORD flags,
    DWORD unreliable_stream_size,  // With IE6, this value is not reliable
    FORMATETC *formatetc,
    STGMEDIUM *stgmed) {
  LOG16((L"IEHttpRequest::OnDataAvailable( 0x%x, %d )\n",
         flags, unreliable_stream_size));
  HRESULT hr = S_OK;

  if (!stgmed || !formatetc) {
    Abort();
    return E_POINTER;
  }
  if ((stgmed->tymed != TYMED_ISTREAM) || !stgmed->pstm) {
    Abort();
    return E_UNEXPECTED;
  }

  // Be careful not to overwrite a synthesized redirect response
  if (has_synthesized_response_payload_) {
    do {
      // We don't expect to get here. If we do for some reason, just read
      // data and drop it on the floor, otherwise the bind will stall
      uint8 buf[kReadAheadAmount];
      DWORD amount_read = 0;
      hr = stgmed->pstm->Read(buf, kReadAheadAmount, &amount_read);
    } while (!(hr == E_PENDING || hr == S_FALSE) && SUCCEEDED(hr));
    return hr;
  }

  std::vector<uint8> *data = response_payload_.data.get();
  if (!data) {
    assert(data);
    Abort();
    return E_UNEXPECTED;
  }

  if (flags & BSCF_FIRSTDATANOTIFICATION) {
    assert(actual_data_size_ == 0);
  }

  bool is_new_data_available = false;

  // We use the data-pull model as the push model doesn't work
  // in some circumstances. In the pull model we have to read
  // beyond then end of what's currently available to encourage
  // the stream to read from the wire, otherwise the bind will stall
  // http://msdn2.microsoft.com/en-us/library/aa380034.aspx
  do {
    // Read in big gulps to spin as little as possible
    DWORD amount_to_read = std::max<DWORD>(data->size() - actual_data_size_,
                                           kReadAheadAmount);

    // Ensure our data buffer is large enough
    size_t needed_size = actual_data_size_ + amount_to_read;
    if (data->size() < needed_size) {
      data->resize(needed_size * 2);
    }

    // Read into our data buffer
    DWORD amount_read = 0;
    hr = stgmed->pstm->Read(&(*data)[actual_data_size_],
                            amount_to_read, &amount_read);
    actual_data_size_ += amount_read;
    is_new_data_available |= (amount_read != 0);
  } while (!(hr == E_PENDING || hr == S_FALSE) && SUCCEEDED(hr));

  if (flags & BSCF_LASTDATANOTIFICATION) {
    data->resize(actual_data_size_);
  }

  if (is_new_data_available && listener_) {
    listener_->DataAvailable(this);
  }

  return hr;
}

//------------------------------------------------------------------------------
// IBindStatusCallback::OnObjectAvailable
// Since we call BindToStorage rather than BindToObject, we should never
// get here.
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::OnObjectAvailable(REFIID riid, IUnknown *punk) {
  assert(false);
  return E_UNEXPECTED;
}

//------------------------------------------------------------------------------
// IHttpNegotiate::BeginningTransaction
// Called by URLMON to determine HTTP specific infomation about how to
// conduct the 'bind'. This is where we inform URLMON of the additional
// headers we would like sent with the HTTP request.
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::BeginningTransaction(LPCWSTR url,
                                                 LPCWSTR headers,
                                                 DWORD reserved,
                                                 LPWSTR *additional_headers) {
  LOG16((L"IEHttpRequest::BeginningTransaction\n"));
  // In the case of a POST with a body which results in a redirect, this
  // method is called more than once. We don't set the additional headers
  // in this case. Those headers include content-length and all user specified
  // headers set by our SetRequestHeader method. The addition of the content
  // length header in this case would result in a malformed request being
  // sent over the network.
  if (!was_redirected_ && !additional_headers_.empty()) {
    *additional_headers = static_cast<LPWSTR>
        (CoTaskMemAlloc((additional_headers_.size() + 1) * sizeof(char16)));
    if (!(*additional_headers)) {
      Abort();
      return E_OUTOFMEMORY;
    }
    wcscpy(*additional_headers, additional_headers_.c_str());
  }
  return S_OK;
}

//------------------------------------------------------------------------------
// IHttpNegotiate::OnResponse
// Called by URLMON when a response is received with the caveat that redirect
// responses are not seen here, only the last response at the end of a chain.
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::OnResponse(DWORD status_code,
                                       LPCWSTR response_headers,
                                       LPCWSTR request_headers,
                                       LPWSTR *additional_request_headers) {
  LOG16((L"IEHttpRequest::OnResponse (%d)\n", status_code));
  // Be careful not to overwrite a redirect response synthesized in OnRedirect
  if (has_synthesized_response_payload_) {
    return E_ABORT;
  }

  response_payload_.status_code = status_code;

  // 'response_headers' contains the status line followed by the headers,
  // we split them apart at the CRLF that seperates them
  const char16 *crlf = wcsstr(response_headers, HttpConstants::kCrLf);
  if (!crlf) {
    assert(false);
    Abort();
    return E_UNEXPECTED;
  }
  response_payload_.status_line.assign(response_headers,
                                       crlf - response_headers);
  response_payload_.headers = (crlf + 2);  // skip over the LF
  response_payload_.data.reset(new std::vector<uint8>);
  actual_data_size_ = 0;

  // We only gather the body for good 200 OK responses
  if (status_code == HttpConstants::HTTP_OK) {
    // Allocate a data buffer based on the content-length header.
    // If this isn't sufficent large, it will be resized as needed in
    // our OnDataAvailable method.
    int content_length = 0;
    std::string16 header_value;
    response_payload_.GetHeader(HttpConstants::kContentLengthHeader,
                                &header_value);
    if (!header_value.empty()) {
      content_length = _wtoi(header_value.c_str());
      if (content_length < 0)
        content_length = 0;
    }
    response_payload_.data->resize(content_length + kReadAheadAmount);
  }

  SetReadyState(HttpRequest::INTERACTIVE);

  // According to http://msdn2.microsoft.com/en-us/library/ms775055.aspx
  // Returning S_OK when the response_code indicates an error implies resending
  // the request with additional_request_headers appended. Presumably to
  // negotiate challenge/response type of authentication with the server.
  // We don't want to resend the request for this or any other purpose.
  // Additionally, on some systems returning S_OK for 304 responses results
  // in an undesireable 60 second delay prior to OnStopBinding happening,
  // so we return E_ABORT to avoid that delay.
  return (status_code == HttpConstants::HTTP_OK) ? S_OK  : E_ABORT;
}
