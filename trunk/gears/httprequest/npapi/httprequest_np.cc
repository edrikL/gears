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

#include "gears/httprequest/npapi/httprequest_np.h"

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/url_utils.h"

// Error messages.
static const char16 *kRequestFailedError = STRING16(L"The request failed.");
static const char16 *kInternalError = STRING16(L"Internal error.");
static const char16 *kAlreadyOpenError =  STRING16(L"Request is already open.");
static const char16 *kFailedURLResolveError =  
                         STRING16(L"Failed to resolve URL.");
static const char16 *kNotOpenError = STRING16(L"Request is not open.");
static const char16 *kNotInteractiveError =
                        STRING16(L"Request is not loading or done.");
static const char16 *kURLNotFromSameOriginError = 
                         STRING16(L"URL is not from the same origin.");

DECLARE_DISPATCHER(GearsHttpRequest);

// static
template<>
void Dispatcher<GearsHttpRequest>::Init() {
  RegisterMethod("open", &GearsHttpRequest::Open);
  RegisterMethod("setRequestHeader", &GearsHttpRequest::SetRequestHeader);
  RegisterMethod("send", &GearsHttpRequest::Send);
  RegisterMethod("abort", &GearsHttpRequest::Abort);
  RegisterMethod("getResponseHeader", &GearsHttpRequest::GetResponseHeader);
  RegisterMethod("getAllResponseHeaders", 
                 &GearsHttpRequest::GetAllResponseHeaders);

  RegisterProperty("onreadystatechange", 
                   &GearsHttpRequest::GetOnReadyStateChange, 
                   &GearsHttpRequest::SetOnReadyStateChange);

  RegisterProperty("readyState", &GearsHttpRequest::GetReadyState, NULL);
  RegisterProperty("responseText", &GearsHttpRequest::GetResponseText, NULL);
  RegisterProperty("status", &GearsHttpRequest::GetStatus, NULL);
  RegisterProperty("statusText", &GearsHttpRequest::GetStatusText, NULL);
}

GearsHttpRequest::GearsHttpRequest() : request_(NULL),
                                       content_type_header_was_set_(false),
                                       has_fired_completion_event_(false) {
}

GearsHttpRequest::~GearsHttpRequest() {
  AbortRequest();
}

void GearsHttpRequest::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
  onreadystatechangehandler_.release();
  AbortRequest();
}

void GearsHttpRequest::Open(JsCallContext *context) {
  if (IsComplete()) {
    ReleaseRequest();
  }
  if (!IsUninitialized()) {
    context->SetException(kAlreadyOpenError);
    return;
  }
  
  std::string16 method;
  std::string16 url;
  
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &method },
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &url },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set())
    return;
    
  if (method.empty()) {
    context->SetException(STRING16(L"The method parameter is required."));
    return;
  }
  
  if (url.empty()) {
    context->SetException(STRING16(L"The url parameter is required."));
    return;
  }
  
  std::string16 full_url;
  std::string16 exception_message;
  if (!ResolveUrl(url, &full_url, &exception_message)) {
    context->SetException(exception_message);
    return;
  }

  CreateRequest();
  
  // Create an event monitor to alert us when the page unloads.
  if (unload_monitor_ == NULL) { 
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD,
                                             this));
  }

  content_type_header_was_set_ = false;
  has_fired_completion_event_ = false;

  if (!request_->Open(method.c_str(), full_url.c_str(), true)) {
    context->SetException(kInternalError);
    return;
  }

  return;
}

static bool IsDisallowedHeader(const char16 *header) {
  // Headers which cannot be set according to the w3c spec.
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
  for (int i = 0; i < static_cast<int>(ARRAYSIZE(kDisallowedHeaders)); ++i) {
    if (StringCompareIgnoreCase(header, kDisallowedHeaders[i]) == 0)
      return true;
  }
  return false;
}

void GearsHttpRequest::SetRequestHeader(JsCallContext *context) {
  std::string16 name;
  std::string16 value;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &name },
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &value }
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) {
    return;
  }
  
  if (!IsOpen()) {
    context->SetException(kNotOpenError);
    return;
  }
  if (IsDisallowedHeader(name.c_str())) {
    context->SetException(STRING16(L"This header may not be set."));
    return;
  }
  if (!request_->SetRequestHeader(name.c_str(), value.c_str())) {
    context->SetException(kInternalError);
    return;
  }
  if (StringCompareIgnoreCase(name.c_str(), 
      HttpConstants::kContentTypeHeader) == 0) {
    content_type_header_was_set_ = true;
  }
}

void GearsHttpRequest::Send(JsCallContext *context) {
  if (!IsOpen()) {
    context->SetException(kNotOpenError);
    return;
  }
  
  std::string16 post_data;
  JsArgument argv[] = {
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &post_data }
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set())
    return;

  HttpRequest *request_being_sent = request_;

  bool ok = false;
  if (!post_data.empty()) {
    if (!content_type_header_was_set_) {
      request_->SetRequestHeader(HttpConstants::kContentTypeHeader,
                                 HttpConstants::kMimeTextPlain);
    }
    ok = request_->SendString(post_data.c_str());
  } else {
    ok = request_->Send();
  }

  if (!ok) {
    if (!has_fired_completion_event_) {
      // We only throw here if we haven't surfaced the error through
      // an onreadystatechange callback. Since the JS code for
      // xhr.onreadystatechange might call xhr.open(), check whether 
      // 'request_' has changed, which indicates that happened.
      // Also, we don't trust IsComplete() to indicate that we actually
      // fired the event, the underlying C++ object *may* declare itself
      // complete without having called our callback. We're being defensive.
      if (request_ == request_being_sent) {
        onreadystatechangehandler_.reset();
        context->SetException(kInternalError);
        return;
      }
    }
  }
}

void GearsHttpRequest::Abort(JsCallContext *context) {
  AbortRequest();
}

void GearsHttpRequest::GetResponseHeader(JsCallContext *context) {
  std::string16 header_name;
  JsArgument argv[] = {
     { JSPARAM_REQUIRED, JSPARAM_STRING16, &header_name },
   };
   context->GetArguments(ARRAYSIZE(argv), argv);
   if (context->is_exception_set())
     return;
  
  
  if (!(IsInteractive() || IsComplete())) {
    context->SetException(kNotInteractiveError);
    return;
  }
  if (!IsValidResponse()) {
    std::string16 empty_string;
    context->SetReturnValue(JSPARAM_STRING16, &empty_string);
    return;
  }
  std::string16 header_value;
  if (!request_->GetResponseHeader(header_name.c_str(), &header_value)) {
    context->SetException(kInternalError);
    return;
  }

  context->SetReturnValue(JSPARAM_STRING16, &header_value);
}

void GearsHttpRequest::GetAllResponseHeaders(JsCallContext *context) {
  if (!(IsInteractive() || IsComplete())) {
    context->SetException(kNotInteractiveError);
    return;
  }
  if (!IsValidResponse()) {
    std::string16 empty_string;
    context->SetReturnValue(JSPARAM_STRING16, &empty_string);
    return;
  }

  std::string16 all_headers;
  if (!request_->GetAllResponseHeaders(&all_headers)) {
    context->SetException(kInternalError);
    return;
  }

  context->SetReturnValue(JSPARAM_STRING16, &all_headers);
}

void GearsHttpRequest::GetOnReadyStateChange(JsCallContext *context) {
  if (onreadystatechangehandler_.get() == NULL) {
    context->SetReturnValue(JSPARAM_NULL, NULL);
  } else {
    const JsToken callback = onreadystatechangehandler_->token();
    context->SetReturnValue(JSPARAM_FUNCTION, &callback);
  }
}

void GearsHttpRequest::SetOnReadyStateChange(JsCallContext *context) {
  // Get & sanitize parameters.
  JsRootedCallback *function = NULL;
  JsArgument argv[] = {
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &function },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  scoped_ptr<JsRootedCallback> scoped_function(function);
  
  if (context->is_exception_set())
    return;

  // release onreadystatechangehandler_ at end of scope
  // and retain contents of scoped_function.
  onreadystatechangehandler_.swap(scoped_function);
}

void GearsHttpRequest::GetReadyState(JsCallContext *context) {
  int ready_state = GetState();
  context->SetReturnValue(JSPARAM_INT, &ready_state);
}

void GearsHttpRequest::GetResponseText(JsCallContext *context) {
  if (!(IsInteractive() || IsComplete())) {
    context->SetException(kNotInteractiveError);
    return;
  }
  
  std::string16 body;
  
  if (IsValidResponse()) {
    if (!request_->GetResponseBodyAsText(&body)) {
      context->SetException(kInternalError);
      return;
    }
  }

  LOG(("GearsHttpRequest::get_responseText - %d chars\n",
         body.length()));

  context->SetReturnValue(JSPARAM_STRING16, &body);
}

void GearsHttpRequest::GetStatus(JsCallContext *context) {
  if (!(IsInteractive() || IsComplete())) {
    context->SetException(kNotInteractiveError);
    return;
  }
  int status_code;
  if (!request_->GetStatus(&status_code)) {
    context->SetException(kInternalError);
    return;
  }
    
  if (!IsValidResponseCode(status_code)) {
    context->SetException(kRequestFailedError);
    return;
  }
  context->SetReturnValue(JSPARAM_INT, &status_code);
}

void GearsHttpRequest::GetStatusText(JsCallContext *context) {
  if (!(IsInteractive() || IsComplete())) {
    context->SetException(kNotInteractiveError);
    return;
  }
  if (!IsValidResponse()) {
    context->SetException(kRequestFailedError);
    return;
  }

  std::string16 status_str;
  if (!request_->GetStatusText(&status_str)) {
    context->SetException(kInternalError);
    return;
  }
  
  context->SetReturnValue(JSPARAM_STRING16, &status_str);
}

void GearsHttpRequest::AbortRequest() {
  if (request_) {
    request_->SetOnReadyStateChange(NULL);
    request_->Abort();
    ReleaseRequest();
  }
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

HttpRequest::ReadyState GearsHttpRequest::GetState() {
  HttpRequest::ReadyState state = HttpRequest::UNINITIALIZED;
  if (request_) {
    request_->GetReadyState(&state);
  }
  return state;
}

bool GearsHttpRequest::IsValidResponse() {
  assert(IsInteractive() || IsComplete());
  int status_code = -1;
  if (!request_->GetStatus(&status_code))
    return false;
  return ::IsValidResponseCode(status_code);
}

//------------------------------------------------------------------------------
// This helper does several things:
// - resolve relative urls based on the page location, the 'url' may also
//   be an absolute url to start with, if so this step does not modify it.
// - normalizes the resulting absolute url, ie. removes path navigation.
// - removes the fragment part of the url, ie. truncates at the '#' character.
// - ensures the the resulting url is from the same-origin.
// - ensures the requested url is HTTP or HTTPS.
//------------------------------------------------------------------------------
bool GearsHttpRequest::ResolveUrl(const std::string16 &url, 
                                  std::string16 *resolved_url,
                                  std::string16 *exception_message) {
  assert(resolved_url && exception_message);
  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url.c_str(), 
                           resolved_url)) {
    *exception_message = kFailedURLResolveError;
    return false;
  }

  SecurityOrigin url_origin;
  if (!url_origin.InitFromUrl(resolved_url->c_str()) ||
      !url_origin.IsSameOrigin(EnvPageSecurityOrigin())) {
    *exception_message = kURLNotFromSameOriginError;
    return false;
  }

  if (!HttpRequest::IsSchemeSupported(url_origin.scheme().c_str())) {
    *exception_message = STRING16(L"URL scheme '");
    *exception_message += url_origin.scheme();
    *exception_message += STRING16(L"' is not supported.");
    return false;
  }

  return true;
                    
}

void GearsHttpRequest::DataAvailable(HttpRequest *source) {
  assert(source == request_);
  LOG(("GearsHttpRequest::DataAvailable\n"));
  ReadyStateChanged(source);
}

void GearsHttpRequest::ReadyStateChanged(HttpRequest *source) {
  assert(source == request_);
  
  // To remove cyclic dependencies we drop our reference to the callback when 
  // the request is complete.
  bool is_complete = IsComplete();
  JsRootedCallback *handler = is_complete ?
                                  onreadystatechangehandler_.release() :
                                  onreadystatechangehandler_.get();
  
  if (handler) {
    JsRunnerInterface *runner = GetJsRunner();
    assert(runner);
    
    if (runner) {
      runner->InvokeCallback(handler, 0, NULL, NULL);
    }
    
    if (is_complete) {
      delete handler;
    }
  }
}
