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

#include "gears/httprequest/httprequest.h"

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/url_utils.h"
#ifndef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#include "gears/blob/blob.h"
#include "gears/blob/buffer_blob.h"
#endif  // not OFFICIAL_BUILD

// Error messages.
static const char16 *kRequestFailedError = STRING16(L"The request failed.");
static const char16 *kInternalError = STRING16(L"Internal error.");
static const char16 *kAlreadyOpenError =  STRING16(L"Request is already open.");
static const char16 *kFailedURLResolveError =
                         STRING16(L"Failed to resolve URL.");
#ifndef OFFICIAL_BUILD
static const char16 *kNotCompleteError = STRING16(L"Request is not done.");
#endif
static const char16 *kNotOpenError = STRING16(L"Request is not open.");
static const char16 *kNotInteractiveError =
                        STRING16(L"Request is not loading or done.");
static const char16 *kURLNotFromSameOriginError =
                         STRING16(L"URL is not from the same origin.");

DECLARE_DISPATCHER(GearsHttpRequest);

// static
template<>
void Dispatcher<GearsHttpRequest>::Init() {
  RegisterMethod("abort", &GearsHttpRequest::Abort);
  RegisterMethod("getResponseHeader", &GearsHttpRequest::GetResponseHeader);
  RegisterMethod("getAllResponseHeaders",
                 &GearsHttpRequest::GetAllResponseHeaders);
  RegisterMethod("open", &GearsHttpRequest::Open);
  RegisterMethod("setRequestHeader", &GearsHttpRequest::SetRequestHeader);
  RegisterMethod("send", &GearsHttpRequest::Send);
  RegisterProperty("onreadystatechange",
                   &GearsHttpRequest::GetOnReadyStateChange,
                   &GearsHttpRequest::SetOnReadyStateChange);
  RegisterProperty("readyState", &GearsHttpRequest::GetReadyState, NULL);
#ifndef OFFICIAL_BUILD
  RegisterProperty("responseBlob", &GearsHttpRequest::GetResponseBlob, NULL);
#endif
  RegisterProperty("responseText", &GearsHttpRequest::GetResponseText, NULL);
  RegisterProperty("status", &GearsHttpRequest::GetStatus, NULL);
  RegisterProperty("statusText", &GearsHttpRequest::GetStatusText, NULL);
}

const std::string GearsHttpRequest::kModuleName("GearsHttpRequest");

GearsHttpRequest::GearsHttpRequest()
    : ModuleImplBaseClassVirtual(kModuleName),
      request_(NULL),
      content_type_header_was_set_(false),
      has_fired_completion_event_(false) {
}

GearsHttpRequest::~GearsHttpRequest() {
  AbortRequest();
}

void GearsHttpRequest::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
  onreadystatechangehandler_.reset(NULL);
  unload_monitor_.reset(NULL);
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
  bool async;  // We ignore this parameter.
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &method },
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &url },
    { JSPARAM_OPTIONAL, JSPARAM_BOOL, &async },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) {
    return;
  }
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
  InitUnloadMonitor();
  content_type_header_was_set_ = false;
  has_fired_completion_event_ = false;
  if (!request_->Open(method.c_str(), full_url.c_str(), true)) {
    ReleaseRequest();
    context->SetException(kInternalError);
    return;
  }
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
    if (StringCompareIgnoreCase(header, kDisallowedHeaders[i]) == 0) {
      return true;
    }
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

  std::string16 post_data_string;
#ifndef OFFICIAL_BUILD
  ModuleImplBaseClass *post_data_module = NULL;
#endif

  int post_data_type = context->GetArgumentType(0);
  // The Gears JS API treats a (JavaScript) null and undefined the same.
  // Furthermore, if there is no arg at all (rather than an explicit null or
  // undefined arg) then GetArgumentType will return JSPARAM_UNKNOWN.
  if ((post_data_type != JSPARAM_NULL) &&
      (post_data_type != JSPARAM_UNDEFINED) &&
      (post_data_type != JSPARAM_UNKNOWN)) {
    JsArgument argv[] = {
      { JSPARAM_OPTIONAL, JSPARAM_UNKNOWN, NULL }
    };
    if (post_data_type == JSPARAM_STRING16) {
      argv[0].type = JSPARAM_STRING16;
      argv[0].value_ptr = &post_data_string;
#ifndef OFFICIAL_BUILD
    // Dispatcher modules are also JavaScript objects, and so it is valid for
    // GetArgumentType to return JSPARAM_OBJECT for a Dispatcher module.
    // TODO(nigeltao): fix this, so that it's always just
    // JSPARAM_DISPATCHER_MODULE, and not JSPARAM_OBJECT.
    } else if ((post_data_type == JSPARAM_DISPATCHER_MODULE) ||
               (post_data_type == JSPARAM_OBJECT)) {
      argv[0].type = JSPARAM_DISPATCHER_MODULE;
      argv[0].value_ptr = &post_data_module;
#endif
    } else {
      context->SetException(
          STRING16(L"First parameter must be a Blob or a string."));
      return;
    }
    context->GetArguments(ARRAYSIZE(argv), argv);
    if (context->is_exception_set()) {
      return;
    }
#ifndef OFFICIAL_BUILD
    if (post_data_module &&
        GearsBlob::kModuleName != post_data_module->get_module_name()) {
      context->SetException(
          STRING16(L"First parameter must be a Blob or a string."));
      return;
    }
#endif
  }

  scoped_refptr<HttpRequest> request_being_sent = request_;

  bool ok = false;
  if (!post_data_string.empty()) {
    if (!content_type_header_was_set_) {
      request_->SetRequestHeader(HttpConstants::kContentTypeHeader,
                                 HttpConstants::kMimeTextPlain);
    }
    ok = request_->SendString(post_data_string.c_str());
#ifndef OFFICIAL_BUILD
  } else if (post_data_module) {
    if (!content_type_header_was_set_) {
      request_->SetRequestHeader(HttpConstants::kContentTypeHeader,
                                 HttpConstants::kMimeApplicationOctetStream);
    }
    scoped_refptr<BlobInterface> blob;
    static_cast<GearsBlob*>(post_data_module)->GetContents(&blob);
    ok = request_->SendBlob(blob.get());
#endif
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
  if (context->is_exception_set()) {
    return;
  }
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
  JsRootedCallback *callback = onreadystatechangehandler_.get();
  // TODO(nigeltao): possibly move the null check into
  // JsCallContext::SetReturnValue, so that I can call
  // SetReturnValue(JSPARAM_FUNCTION, NULL) to get a (JavaScript) null, rather
  // than having to explicitly check for a NULL JsRootedCallback*.
  if (callback == NULL) {
    context->SetReturnValue(JSPARAM_NULL, 0);
  } else {
    context->SetReturnValue(JSPARAM_FUNCTION, callback);
  }
}

void GearsHttpRequest::SetOnReadyStateChange(JsCallContext *context) {
  JsRootedCallback *function = NULL;
  JsArgument argv[] = {
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &function },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) {
    return;
  }
  onreadystatechangehandler_.reset(function);
  InitUnloadMonitor();
}

void GearsHttpRequest::GetReadyState(JsCallContext *context) {
  int ready_state = GetState();
  context->SetReturnValue(JSPARAM_INT, &ready_state);
}

#ifndef OFFICIAL_BUILD
void GearsHttpRequest::GetResponseBlob(JsCallContext *context) {
  if (!IsComplete()) {
    context->SetException(kNotCompleteError);
    return;
  }
  if (!IsValidResponse()) {
    context->SetReturnValue(JSPARAM_NULL, 0);
    return;
  }

  // Re-use the previously created GearsBlob object, if it exists.
  if (response_blob_.get()) {
    ReleaseNewObjectToScript(response_blob_.get());
    context->SetReturnValue(JSPARAM_DISPATCHER_MODULE, response_blob_.get());
    return;
  }

  // On some platforms' HttpRequest implementation (for example, IE/Windows),
  // GetResponseBody() swaps the underyling data buffer, and so without this,
  // calling GetResponseText() after calling GetResponseBody will incorrectly
  // return an empty string.  This explicit read of response_text_ fixes that,
  // but it does a possibly-unnecessary copy.
  // TODO(nigeltao): once Blobs become part of the official build, make the
  // Blob version of the canonical representation of the HTTP response, and
  // implement GetResponseText simply as calling something like
  // response_blob_->GetAsText(&str, encoding), and hence remove the need for
  // this copy.
  if (response_text_ == NULL) {
    response_text_.reset(new std::string16);
    request_->GetResponseBodyAsText(response_text_.get());
  }

  scoped_refptr<BlobInterface> blob(NULL);
  scoped_ptr<std::vector<uint8> >body(request_->GetResponseBody());
  if (body.get()) {
    blob.reset(new BufferBlob(body.get()));
  } else {
    context->SetReturnValue(JSPARAM_NULL, 0);
    return;
  }

  CreateModule<GearsBlob>(GetJsRunner(), &response_blob_);
  if (!response_blob_->InitBaseFromSibling(this)) {
    context->SetException(STRING16(L"Initializing base class failed."));
    return;
  }
  response_blob_->Reset(blob.get());
  context->SetReturnValue(JSPARAM_DISPATCHER_MODULE, response_blob_.get());
  ReleaseNewObjectToScript(response_blob_.get());
}
#endif

void GearsHttpRequest::GetResponseText(JsCallContext *context) {
  if (!(IsInteractive() || IsComplete())) {
    context->SetException(kNotInteractiveError);
    return;
  }
  // First, check the cached result (the response_text_ member variable).  It
  // is only set if GetResponseText was previously called when IsComplete().
  if (response_text_.get()) {
    context->SetReturnValue(JSPARAM_STRING16, response_text_.get());
    return;
  }
  scoped_ptr<std::string16> result(new std::string16);
  bool is_valid_response = IsValidResponse();
  if (is_valid_response && !request_->GetResponseBodyAsText(result.get())) {
    context->SetException(kInternalError);
    return;
  }
  context->SetReturnValue(JSPARAM_STRING16, result.get());
  if (is_valid_response && IsComplete()) {
    response_text_.swap(result);
  }
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
  if (request_.get()) {
    request_->SetListener(NULL, false);
    request_->Abort();
    ReleaseRequest();
  }
}

void GearsHttpRequest::CreateRequest() {
  ReleaseRequest();
  HttpRequest::CreateSafeRequest(&request_);
  request_->SetListener(this, true);
  request_->SetCachingBehavior(HttpRequest::USE_ALL_CACHES);
  request_->SetRedirectBehavior(HttpRequest::FOLLOW_WITHIN_ORIGIN);
}


void GearsHttpRequest::ReleaseRequest() {
  if (request_.get()) {
    request_->SetListener(NULL, false);
    request_.reset(NULL);
  }
  response_text_.reset(NULL);
#ifndef OFFICIAL_BUILD
  response_blob_.reset(NULL);
#endif
}

HttpRequest::ReadyState GearsHttpRequest::GetState() {
  HttpRequest::ReadyState state = HttpRequest::UNINITIALIZED;
  if (request_.get()) {
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
  assert(source == request_.get());
  ReadyStateChanged(source);
}

void GearsHttpRequest::ReadyStateChanged(HttpRequest *source) {
  assert(source == request_.get());

  // To remove cyclic dependencies we drop our reference to the callback when
  // the request is complete.
  bool is_complete = IsComplete();
  if (is_complete) {
    has_fired_completion_event_ = true;
  }
  JsRootedCallback *handler = is_complete
      ? onreadystatechangehandler_.release()
      : onreadystatechangehandler_.get();
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

void GearsHttpRequest::InitUnloadMonitor() {
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(
        new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD, this));
  }
}

void GearsHttpRequest::UploadProgress(HttpRequest *source,
                                      int64 position, int64 total) {
  // TODO(bgarcia): implement
}
