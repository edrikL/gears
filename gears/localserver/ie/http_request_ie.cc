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
#include "gears/base/ie/atl_headers.h"
#include "gears/localserver/ie/http_handler_ie.h"
#include "gears/localserver/ie/http_request_ie.h"
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
HttpRequest *HttpRequest::Create() {
  CComObject<IEHttpRequest> *request;
  HRESULT hr = CComObject<IEHttpRequest>::CreateInstance(&request);
  if (FAILED(hr)) {
    ATLTRACE(_T("HttpRequest::Create - CreateInstance failed - %d\n"), hr);
    return NULL;
  }
  request->AddReference();
  return request;
}

//------------------------------------------------------------------------------
// Construction, destruction and refcounting
//------------------------------------------------------------------------------

IEHttpRequest::IEHttpRequest() :
    was_sent_(false), is_complete_(false), follow_redirects_(true),
    was_redirected_(false), was_aborted_(false), listener_(NULL),
    ready_state_(0), actual_data_size_(0) {
}

HRESULT IEHttpRequest::FinalConstruct() {
  return S_OK;
}

void IEHttpRequest::FinalRelease() {
}

int IEHttpRequest::AddReference() {
  return AddRef();
}

int IEHttpRequest::ReleaseReference() {
  return Release();
}

//------------------------------------------------------------------------------
// getReadyState
//------------------------------------------------------------------------------
bool IEHttpRequest::getReadyState(long *state) {
  *state = ready_state_;
  return true;
}

//------------------------------------------------------------------------------
// getResponseBody
//------------------------------------------------------------------------------
std::vector<unsigned char> *IEHttpRequest::getResponseBody() {
  if (!is_complete_ || was_aborted_)
    return NULL;
  return response_payload_.data.release();
}

//------------------------------------------------------------------------------
// getResponseBody
// TODO(michaeln): remove one or the other of these getResponseBody methods from
// the interface.
//------------------------------------------------------------------------------
bool IEHttpRequest::getResponseBody(std::vector<unsigned char> *body) {
  if (!is_complete_ || was_aborted_)
    return false;
  if (response_payload_.data.get()) {
    body->swap(*response_payload_.data.get());
  } else {
    body->clear();
  }
  return false;
}

//------------------------------------------------------------------------------
// getStatus
//------------------------------------------------------------------------------
bool IEHttpRequest::getStatus(long *status) {
  if (!is_complete_ || was_aborted_)
    return false;
  *status = response_payload_.status_code;
  return true;
}

//------------------------------------------------------------------------------
// getStatusText
// TODO(michaeln): remove this method from the interface, prefer getStatusLine
//------------------------------------------------------------------------------
bool IEHttpRequest::getStatusText(std::string16 *status_text) {
  if (!is_complete_ || was_aborted_)
    return false;
  *status_text = response_payload_.status_line;
  return true;
}

//------------------------------------------------------------------------------
// getStatusLine
//------------------------------------------------------------------------------
bool IEHttpRequest::getStatusLine(std::string16 *status_line) {
  if (!is_complete_ || was_aborted_)
    return false;
  *status_line = response_payload_.status_line;
  return true;
}

//------------------------------------------------------------------------------
// getAllResponseHeaders
//------------------------------------------------------------------------------
bool IEHttpRequest::getAllResponseHeaders(std::string16 *headers) {
  if (!is_complete_ || was_aborted_)
    return false;
  headers->assign(response_payload_.headers);
  return true;
}

//------------------------------------------------------------------------------
// getResponseHeader
//------------------------------------------------------------------------------
bool IEHttpRequest::getResponseHeader(const char16* name,
                                      std::string16 *value) {
  if (!is_complete_ || was_aborted_)
    return false;
  return response_payload_.GetHeader(name, value);
}

//------------------------------------------------------------------------------
// open
// This class only knows how to make async GET requests at this time, although
// the interface promises to do more (sync POSTS etc). We assert that we're
// being asked to do an async GET, which is all we need to do for now.
//------------------------------------------------------------------------------
bool IEHttpRequest::open(const char16 *method, const char16* url, bool async) {
  if (was_sent_)
    return false;
  assert(async);
  assert(std::string16(L"GET") == method);
  url_ = url;
  SetReadyState(1);
  return true;
}

//------------------------------------------------------------------------------
// setRequestHeader
// Here we gather additional request headers to be sent. They are plumbed
// into URLMON in our IHttpNegotiate::BeginningTransaction method.
//------------------------------------------------------------------------------
bool IEHttpRequest::setRequestHeader(const char16* name, const char16* value) {
  if (was_sent_)
    return false;
  additional_headers_ += name;
  additional_headers_ += L": ";
  additional_headers_ += value;
  additional_headers_ += HttpConstants::kCrLf;
  return true;
}

bool IEHttpRequest::setFollowRedirects(bool follow) {
  if (was_sent_)
    return false;
  follow_redirects_ = follow;
  return true;
}

bool IEHttpRequest::wasRedirected() {
  return follow_redirects_ && is_complete_ && was_redirected_ && !was_aborted_;
}

bool IEHttpRequest::getRedirectUrl(std::string16 *full_redirect_url) {
  if (!wasRedirected())
    return false;
  *full_redirect_url = redirect_url_;
  return true;
}

//------------------------------------------------------------------------------
// send
//------------------------------------------------------------------------------
bool IEHttpRequest::send() {
  if (was_sent_ || url_.empty())
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

  if (was_aborted_) {
    return false;
  }
  SetReadyState(1);
  return !was_aborted_;;
}

//------------------------------------------------------------------------------
// abort
//------------------------------------------------------------------------------
bool IEHttpRequest::abort() {
  if (!binding_)
    return true;
  HRESULT hr = binding_->Abort();
  was_aborted_ = true;
  return SUCCEEDED(hr);
}

//------------------------------------------------------------------------------
// setOnReadyStateChange
//------------------------------------------------------------------------------
bool IEHttpRequest::setOnReadyStateChange(ReadyStateListener *listener) {
  listener_ = listener;
  return true;
}

void IEHttpRequest::SetReadyState(int state) {
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

  // Our HttpHandler (see ie_http_handler.cc h) provides a mechanism to
  // bypass our webcache that involves querying for a particular service
  // id that does not exists. Here we detect that query and set our handler
  // in bypass mode.
  if (InlineIsEqualGUID(guidService, HttpHandler::SID_QueryBypassCache) &&
      InlineIsEqualGUID(riid, HttpHandler::IID_QueryBypassCache)) {
    HttpHandler::SetBypassCache();
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
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::OnStartBinding(DWORD reserved,IBinding *binding) {
  ATLTRACE(_T("IEHttpRequest::OnStartBinding\n"));
  if (!binding) {
    return E_POINTER;
  }
  assert(!binding_);
  binding_ = binding;
  SetReadyState(1);
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
                                       ULONG status_code, LPCWSTR status_text){
#ifdef DEBUG
  ATLTRACE(_T("IEHttpRequest::OnProgress(%s (%d), %s)\n"),
           GetBindStatusLabel(status_code), status_code,
           status_text ? status_text : L"NULL");
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
  ATLTRACE(_T("IEHttpRequest::OnRedirect( %s )\n"), redirect_url);
  if (!follow_redirects_) {
    if (!binding_)
      return E_UNEXPECTED;
    // When we're not supposed to follow redirects, we abort the request when
    // a redirect is reported to us. This causes the bind to fail w/o ever
    // having seen the actual response data. Our HttpRequest interface dictates
    // that we return a valid reponse payload containing the redirect in this
    // case. Here we synthesize a valid redirect response. This is done prior
    // to calling Abort so our response_payload is setup in advance of
    // OnStopBinding being called. I've observed that OnStopBinding gets called
    // prior to Abort returning, but don't have any confidence that behavior can
    // be depended on. We also set the was_redirected_ flag in this case so
    // our other URLMON callbacks know not to overwrite our synthesized
    // redirect response.
    // TODO(michaeln): we 'should' get the actual response data
    was_redirected_ = true;
    response_payload_.SynthesizeHttpRedirect(NULL, redirect_url);
    binding_->Abort();
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
  ATLTRACE(_T("IEHttpRequest::OnStopBinding\n"));
  binding_.Release();
  bind_ctx_.Release();
  url_moniker_.Release();
  is_complete_ = true;
  SetReadyState(4);
  return S_OK;
}

//------------------------------------------------------------------------------
// IBindStatusCallback::GetBindInfo
// Called by URLMON to determine how the 'bind' should be conducted
//------------------------------------------------------------------------------
STDMETHODIMP IEHttpRequest::GetBindInfo(DWORD *flags, BINDINFO *info) {
  ATLTRACE(_T("IEHttpRequest::GetBindInfo\n"));
  if (!info || !flags)
    return E_POINTER;
  if (!info->cbSize)
    return E_INVALIDARG;

  // We setup bind flags such that we send a request all the way through
  // to the server, bypassing caching proxies between here and there;
  // and such that we don't write the response to the browser's cache.
  // We also use the pull-data model as push is unreliable.
  // TODO(michaeln): is there a flag that turns off auto-follow of redirects?
  // INTERNET_FLAG_NO_AUTO_REDIRECT?
  const int kBindFlags = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE |
                         BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE |
                         BINDF_PRAGMA_NO_CACHE | BINDF_FROMURLMON |
                         BINDF_PULLDATA | BINDF_NO_UI;
  *flags = kBindFlags;
  int save_size = static_cast<int>(info->cbSize);
  memset(info, 0, save_size);
  info->cbSize = save_size;
  info->dwBindVerb = BINDVERB_GET;
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
  ATLTRACE(_T("IEHttpRequest::OnDataAvailable( 0x%x, %d )\n"), 
           flags, unreliable_stream_size);
  HRESULT hr = S_OK;

  SetReadyState(3);

  if (!stgmed || !formatetc) {
    abort();
    return E_POINTER;
  }
  if ((stgmed->tymed != TYMED_ISTREAM) || !stgmed->pstm) {
    abort();
    return E_UNEXPECTED;
  }

  // Be careful not to overwrite a synthesized redirect response 
  if (!follow_redirects_ && was_redirected_) {
    do {
      // We don't expect to get here. If we do for some reason, just read
      // data and drop it on the floor, otherwise the bind will stall
      unsigned char buf[kReadAheadAmount];
      DWORD amount_read = 0;
      hr = stgmed->pstm->Read(buf, kReadAheadAmount, &amount_read);
    } while (!(hr == E_PENDING || hr == S_FALSE) && SUCCEEDED(hr));
    return hr;
  }

  std::vector<unsigned char> *data = response_payload_.data.get();
  if (!data) {
    assert(data);
    abort();
    return E_UNEXPECTED;
  }

  if (flags & BSCF_FIRSTDATANOTIFICATION) {
    assert(actual_data_size_ == 0);
  }

  // We use the data-pull model as the push model doesn't work
  // in some circumstances. In the pull model we have to read
  // beyond then end of what's currently available to encourage
  // the stream to read from the wire, otherwise the bind will stall
  // http://msdn2.microsoft.com/en-us/library/aa380034.aspx
  do {
    // Read in big gulps to spin as little as possible
    DWORD amount_to_read = max(data->size() - actual_data_size_,
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
  } while (!(hr == E_PENDING || hr == S_FALSE) && SUCCEEDED(hr));


  if (flags & BSCF_LASTDATANOTIFICATION) {
    data->resize(actual_data_size_);
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
  ATLTRACE(_T("IEHttpRequest::BeginningTransaction\n"));
  if (!additional_headers_.empty()) {
    *additional_headers = static_cast<LPWSTR>
        (CoTaskMemAlloc((additional_headers_.size() + 1) * sizeof(char16)));
    if (!(*additional_headers)) {
      abort();
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
  ATLTRACE(_T("IEHttpRequest::OnResponse (%d)\n"), status_code);
  // Be careful not to overwrite a redirect response synthesized in OnRedirect
  if (!follow_redirects_ && was_redirected_) {
    return E_ABORT;
  }
  
  response_payload_.status_code = status_code;
  
  // 'response_headers' contains the status line followed by the headers,
  // we split them apart at the CRLF that seperates them
  const char16 *crlf = wcsstr(response_headers, HttpConstants::kCrLf);
  if (!crlf) {
    assert(false);
    abort();
    return E_UNEXPECTED;
  }
  response_payload_.status_line.assign(response_headers,
                                       crlf - response_headers);
  response_payload_.headers = (crlf + 2); // skip over the LF
  response_payload_.data.reset(new std::vector<unsigned char>);
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

