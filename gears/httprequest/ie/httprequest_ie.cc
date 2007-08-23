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

#include <assert.h>
#include <dispex.h>

#include "gears/httprequest/ie/httprequest_ie.h"

#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/localserver/common/http_constants.h"


// Error messages
static const char16 *kInternalError = STRING16(L"Internal error.");
static const char16 *kAlreadyOpenError =  STRING16(L"Request is already open.");
static const char16 *kNotOpenError = STRING16(L"Request is not open.");
static const char16 *kNotCompleteError = STRING16(L"Request is not done.");
static const char16 *kNotInteractiveError =
                        STRING16(L"Request is not loading or done.");


GearsHttpRequest::GearsHttpRequest()
  : request_(NULL), content_type_header_was_set_(false) {
}

GearsHttpRequest::~GearsHttpRequest() {
  abort();
}


STDMETHODIMP GearsHttpRequest::put_onreadystatechange(
    /* [in] */ IDispatch *handler) {
  onreadystatechangehandler_ = handler;
  RETURN_NORMAL();
}

  
STDMETHODIMP GearsHttpRequest::get_onreadystatechange( 
      /* [retval][out] */ IDispatch **handler){
  return onreadystatechangehandler_.CopyTo(handler);
}


STDMETHODIMP GearsHttpRequest::get_readyState( 
    /* [retval][out] */ int *state) {
  if (!state) return E_POINTER;
  *state = GetState();
  RETURN_NORMAL();
}
  

STDMETHODIMP GearsHttpRequest::open( 
      /* [in] */ const BSTR method,
      /* [in] */ const BSTR url,
      /* [optional][in] */ const VARIANT *async) {
  if (!method || !url) return E_POINTER;

  if (IsComplete()) {
    ReleaseRequest();
  }
  if (!IsUninitialized()) {
    RETURN_EXCEPTION(kAlreadyOpenError);
  }
 
  if (!method[0]) {
    RETURN_EXCEPTION(STRING16(L"The method parameter must be a string."));
  }
  if (!url[0]) {
    RETURN_EXCEPTION(STRING16(L"The url parameter must be a string."));
  }

  std::string16 full_url;
  std::string16 exception_message;
  if (!ResolveUrl(url, &full_url, &exception_message))
    RETURN_EXCEPTION(exception_message.c_str());

  CreateRequest();

  if (!request_->Open(method, full_url.c_str(), true))
    RETURN_EXCEPTION(kInternalError);

  content_type_header_was_set_ = false;

  RETURN_NORMAL();
}


static bool IsDisallowedHeader(const char16 *header) {
  // Headers which cannot be set according to the w3c spec
  static const char16* kDisallowedHeaders[] = {
      STRING16(L"Accept-Charset"), 
      STRING16(L"Accept-Encoding"),
      STRING16(L"Connection"),
      STRING16(L"Content-Length"),
      STRING16(L"Content-Transfer-Encoding"),
      STRING16(L"Date"),
      STRING16(L"Expect"),
      STRING16(L"Host"),
      STRING16(L"Keep-Alive"),
      STRING16(L"Referer"),
      STRING16(L"TE"),
      STRING16(L"Trailer"),
      STRING16(L"Transfer-Encoding"),
      STRING16(L"Upgrade"),
      STRING16(L"Via") };
  for (int i = 0; i < ARRAYSIZE(kDisallowedHeaders); ++i) {
    if (StringCompareIgnoreCase(header, kDisallowedHeaders[i]) == 0)
      return true;
  }
  return false;
}


STDMETHODIMP GearsHttpRequest::setRequestHeader( 
      /* [in] */ const BSTR name,
      /* [in] */ const BSTR value) {
  if (!name) return E_POINTER;
  if (!IsOpen()) {
    RETURN_EXCEPTION(kNotOpenError);
  }
  if (IsDisallowedHeader(name)) {
    RETURN_EXCEPTION(STRING16(L"This header may not be set."));
  }
  if (!request_->SetRequestHeader(name, value ? value : L"")) {
    RETURN_EXCEPTION(kInternalError);
  }
  if (StringCompareIgnoreCase(name, HttpConstants::kContentTypeHeader) == 0) {
    content_type_header_was_set_ = true;
  }
  RETURN_NORMAL();
}


STDMETHODIMP GearsHttpRequest::send( 
      /* [optional][in] */ const VARIANT *data) {
  if (!IsOpen())
    RETURN_EXCEPTION(kNotOpenError);

  const char16 *post_data_str = NULL;
  if (ActiveXUtils::OptionalVariantIsPresent(data)) {
    if (data->vt != VT_BSTR) {
      RETURN_EXCEPTION(STRING16(L"Data parameter must be a string."));
    }
    post_data_str = data->bstrVal;
  }

  bool ok = false;
  if (post_data_str && post_data_str[0]) {
    if (!content_type_header_was_set_) {
      request_->SetRequestHeader(HttpConstants::kContentTypeHeader,
                                 HttpConstants::kMimeTextPlain);
    }
    ok = request_->SendString(post_data_str);
  } else {
    ok = request_->Send();
  }

  if (!ok) {
    onreadystatechangehandler_.Release();
    RETURN_EXCEPTION(kInternalError);
  }
  RETURN_NORMAL();
}


STDMETHODIMP GearsHttpRequest::abort() {
  if (request_) {
    request_->Abort();
    ReleaseRequest();
  }
  RETURN_NORMAL();
}


STDMETHODIMP GearsHttpRequest::getAllResponseHeaders( 
      /* [retval][out] */ BSTR *headers) {
  if (!headers) return E_POINTER;

  if (!(IsInteractive() || IsComplete()))
    RETURN_EXCEPTION(kNotInteractiveError);

  std::string16 headers_str;
  if (!request_->GetAllResponseHeaders(&headers_str))
    RETURN_EXCEPTION(kInternalError);

  CComBSTR headers_bstr(headers_str.c_str());
  *headers = headers_bstr.Detach();
  RETURN_NORMAL();
}


STDMETHODIMP GearsHttpRequest::getResponseHeader( 
      /* [in] */ const BSTR header_name,
      /* [retval][out] */ BSTR *header_value) {
  if (!header_name || !header_value) return E_POINTER;

  if (!(IsInteractive() || IsComplete()))
    RETURN_EXCEPTION(kNotInteractiveError);

  std::string16 value_str;
  if (!request_->GetResponseHeader(header_name, &value_str))
    RETURN_EXCEPTION(kInternalError);

  CComBSTR value_bstr(value_str.c_str());
  *header_value = value_bstr.Detach();
  RETURN_NORMAL();
}


STDMETHODIMP GearsHttpRequest::get_responseText( 
      /* [retval][out] */ BSTR *body) {
  if (!body) return E_POINTER;
  
  if (!IsComplete())
    RETURN_EXCEPTION(kNotCompleteError);
 
  std::string16 body_str;
  if (!request_->GetResponseBodyAsText(&body_str))
    RETURN_EXCEPTION(kInternalError);

  CComBSTR body_bstr(body_str.c_str());
  *body = body_bstr.Detach();
  RETURN_NORMAL();
}


STDMETHODIMP GearsHttpRequest::get_status( 
    /* [retval][out] */ int *status_code) {
  if (!status_code) return E_POINTER;

  if (!(IsInteractive() || IsComplete()))
    RETURN_EXCEPTION(kNotInteractiveError);

  if (!request_->GetStatus(status_code))
    RETURN_EXCEPTION(kInternalError);
  RETURN_NORMAL();
}


STDMETHODIMP GearsHttpRequest::get_statusText( 
      /* [retval][out] */ BSTR *status_text) {
  if (!status_text) return E_POINTER;

  if (!(IsInteractive() || IsComplete()))
    RETURN_EXCEPTION(kNotInteractiveError);

  std::string16 status_str;
  if (!request_->GetStatusText(&status_str))
    RETURN_EXCEPTION(kInternalError);
  CComBSTR status_bstr(status_str.c_str());
  *status_text = status_bstr.Detach();
  RETURN_NORMAL();
}


void GearsHttpRequest::ReadyStateChanged(HttpRequest *source) {
  assert(source == request_);
  if (onreadystatechangehandler_) {
    HRESULT hr = 0;
    DISPPARAMS dispparams = {0};

    // To remove cyclic dependencies we drop our reference to the
    // callback when the request is complete.
    CComPtr<IDispatch> dispatch = onreadystatechangehandler_;
    if (IsComplete()) {
      onreadystatechangehandler_.Release();
    }

    // TODO(michaeln): The JavaScript 'this' pointer should be a reference
    // to the HttpRequest object. This is not currently the case.

    // Note: strangely, the parameters passed thru IDispatch(Ex) are in
    // reverse order as compared to the method signature being invoked
    CComQIPtr<IDispatchEx> dispatchex = dispatch;
    if (dispatchex) {
      // We prefer to call things thru IDispatchEx in order to use DISPID_THIS
      // such that the closure is scoped properly.
      DISPID disp_this = DISPID_THIS;
      VARIANT var[1];
      var[0].vt = VT_DISPATCH;
      var[0].pdispVal = dispatchex;
      dispparams.rgvarg = var;
      dispparams.rgdispidNamedArgs = &disp_this;
      dispparams.cNamedArgs = 1;
      dispparams.cArgs = 1;

      hr = dispatchex->InvokeEx(
          DISPID_VALUE, LOCALE_USER_DEFAULT,
          DISPATCH_METHOD, &dispparams,
          NULL, NULL, NULL);

    } else {
      // Fallback on IDispatch if needed.
      UINT arg_err = 0;
      VARIANT var[1];
      dispparams.rgvarg = var;
      dispparams.cArgs = 0;

      hr = dispatch->Invoke(
          DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT,
          DISPATCH_METHOD, &dispparams,
          NULL, NULL, &arg_err);
    }
  }
}


HttpRequest::ReadyState GearsHttpRequest::GetState() {
  HttpRequest::ReadyState state = HttpRequest::UNINITIALIZED;
  if (request_) {
    request_->GetReadyState(&state);
  }
  return state;
}


void GearsHttpRequest::CreateRequest() {
  ReleaseRequest();
  request_ = HttpRequest::Create();
  request_->SetOnReadyStateChange(this);
  request_->SetCachingBehavior(HttpRequest::USE_ALL_CACHES);
  request_->SetRedirectBehavior(HttpRequest::FOLLOW_WITHIN_ORIGIN);
}


void GearsHttpRequest::ReleaseRequest() {
  if (request_) {
    request_->SetOnReadyStateChange(NULL);
    request_->ReleaseReference();
    request_ = NULL;
  }
}


//------------------------------------------------------------------------------
// This helper does several things:
// - resolve relative urls based on the page location, the 'url' may also
//   be an absolute url to start with, if so this step does not modify it
// - normalizes the resulting absolute url, ie. removes path navigation
// - removes the fragment part of the url, ie. truncates at the '#' character
// - ensures the the resulting url is from the same-origin
//------------------------------------------------------------------------------
bool GearsHttpRequest::ResolveUrl(const char16 *url,
                                  std::string16 *resolved_url,
                                  std::string16 *exception_message) {
  assert(url && resolved_url && exception_message);
  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url, resolved_url)) {
    *exception_message = STRING16(L"Failed to resolve url.");
    return false;
  }
  if (!EnvPageSecurityOrigin().IsSameOriginAsUrl(resolved_url->c_str())) {
    *exception_message = STRING16(L"Url is not from the same origin.");
    return false;
  }
  return true;
}
