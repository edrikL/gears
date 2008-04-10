// Copyright 2007, Google Inc.
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

#ifdef WIN32
#include <windows.h> // must manually #include before nsIEventQueueService.h
#endif
#include <gecko_internal/nsIDOMClassInfo.h>

#include "gears/httprequest/firefox/httprequest_ff.h"

#include "gears/base/common/async_router.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/firefox/dom_utils.h"
#ifndef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#include "gears/blob/buffer_blob.h"
#include "gears/blob/blob_interface.h"
#endif
#include "gears/localserver/common/http_constants.h"

// Returns true if the currently executing thread is the main UI thread,
// firefox/mozila has one such very special thread
// See cache_intercept.cc for implementation
bool IsUiThread();
// Returns the thread id of the main UI thread,
// firefox/mozila has one such very special thread
// See cache_intercept.cc for implementation
PRThread* GetUiThread();

// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_THREADSAFE_ADDREF(GearsHttpRequest)
NS_IMPL_THREADSAFE_RELEASE(GearsHttpRequest)
NS_INTERFACE_MAP_BEGIN(GearsHttpRequest)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsHttpRequestInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsHttpRequest)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsHttpRequest)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsHttpRequestClassName = "GearsHttpRequest";
const nsCID kGearsHttpRequestClassId = // {033D8D37-95A2-478e-91A3-B0FA60A9CA8D}
                { 0x33d8d37, 0x95a2, 0x478e,
                  { 0x91, 0xa3, 0xb0, 0xfa, 0x60, 0xa9, 0xca, 0x8d } };

// Error messages
static const char16 *kRequestFailedError = STRING16(L"The request failed.");
static const char16 *kInternalError = STRING16(L"Internal error.");
static const char16 *kAlreadyOpenError =  STRING16(L"Request is already open.");
static const char16 *kNotOpenError = STRING16(L"Request is not open.");
static const char16 *kNotInteractiveError =
                        STRING16(L"Request is not loading or done.");

static const char16 *kEmptyString = STRING16(L"");

GearsHttpRequest::GearsHttpRequest()
    : current_request_id_(0),
      native_request_(NULL), native_request_id_(0),
      apartment_thread_id_(
          ThreadMessageQueue::GetInstance()->GetCurrentThreadId()),
      page_is_unloaded_(false) {
  ThreadMessageQueue::GetInstance()->InitThreadMessageQueue();
}

GearsHttpRequest::~GearsHttpRequest() {
  assert(!native_request_);
}

//------------------------------------------------------------------------------
// attribute ReadyStateChangeEventListener onreadystatechange
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::SetOnreadystatechange(
                  ReadyStateChangeEventListener * onreadystatechange) {
  assert(IsApartmentThread());

  JsParamFetcher js_params(this);
  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"Value is required."));
  }

  JsRootedCallback *callback;
  if (!js_params.GetAsNewRootedCallback(0, &callback)) {
    RETURN_EXCEPTION(
        STRING16(L"Invalid value for onreadystatechange property."));
  }

  onreadystatechange_.reset(callback);
  InitUnloadMonitor();

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsHttpRequest::GetOnreadystatechange(
                  ReadyStateChangeEventListener **onreadystatechange) {
  assert(IsApartmentThread());
  return NS_ERROR_NOT_IMPLEMENTED;
}

//------------------------------------------------------------------------------
// void open(string method, string url, bool async)
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::Open() {
  assert(IsApartmentThread());
  { // Scoped to unlock prior to invoking the readystatechange script callback
    MutexLock locker(&lock_);
    
    if (IsComplete()) {
      SetCurrentRequestInfo(NULL);
    }

    if (!IsUninitialized()) {
      RETURN_EXCEPTION(kAlreadyOpenError);
    }

    JsParamFetcher js_params(this);
    if (js_params.GetCount(false) < 2) {
      RETURN_EXCEPTION(
          STRING16(L"The method and url parameters are required."));
    }

    std::string16 method;
    if (!js_params.GetAsString(0, &method) || method.empty()) {
      RETURN_EXCEPTION(STRING16(L"The method parameter must be a string."));
    }

    std::string16 url;
    if (!js_params.GetAsString(1, &url) || url.empty()) {
      RETURN_EXCEPTION(STRING16(L"The url parameter must be a string."));
    }

    scoped_ptr<RequestInfo> scoped_info(new RequestInfo());
    scoped_info->ready_state = HttpRequest::UNINITIALIZED;
    scoped_info->upcoming_ready_state = HttpRequest::OPEN;
    scoped_info->method = MakeUpperString(method);
    
    std::string16 exception_message;
    if (!ResolveUrl(url.c_str(), &scoped_info->full_url, &exception_message)) {
      RETURN_EXCEPTION(exception_message.c_str());
    }

    SetCurrentRequestInfo(scoped_info.release());
  }

  OnReadyStateChangedCall(current_request_id_);

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// void setRequestHeader(name, value)
//------------------------------------------------------------------------------

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
  for (int i = 0; i < static_cast<int>(ARRAYSIZE(kDisallowedHeaders)); ++i) {
    if (StringCompareIgnoreCase(header, kDisallowedHeaders[i]) == 0)
      return true;
  }
  return false;
}

NS_IMETHODIMP GearsHttpRequest::SetRequestHeader() {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (!IsOpen()) {
    RETURN_EXCEPTION(kNotOpenError);
  }

  JsParamFetcher js_params(this);
  if (js_params.GetCount(false) < 2) {
    RETURN_EXCEPTION(
        STRING16(L"The name and value parameters are required."));
  }

  std::string16 name;
  if (!js_params.GetAsString(0, &name)) {
    RETURN_EXCEPTION(STRING16(L"The name parameter must be a string."));
  }

  std::string16 value;
  if (!js_params.GetAsString(1, &value)) {
    RETURN_EXCEPTION(STRING16(L"The value parameter must be a string."));
  }

  if (IsDisallowedHeader(name.c_str())) {
    RETURN_EXCEPTION(STRING16(L"This header may not be set."));
  }

  current_request_info_->headers.push_back(std::make_pair(name, value));

  if (StringCompareIgnoreCase(name.c_str(),
                              HttpConstants::kContentTypeHeader) == 0) {
    current_request_info_->content_type_header_was_set = true;
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// void send(opt_data)
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::Send() {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (!IsOpen()) {
    RETURN_EXCEPTION(kNotOpenError);
  }

  if (current_request_info_->method == HttpConstants::kHttpPOST ||
      current_request_info_->method == HttpConstants::kHttpPUT) {
    JsParamFetcher js_params(this);
    if (js_params.GetCount(false) > 0) {
#ifndef OFFICIAL_BUILD
      ModuleImplBaseClass *module;
      if (js_params.GetAsString(0, &current_request_info_->post_data)) {
        current_request_info_->has_post_data =
                                   !current_request_info_->post_data.empty();
      } else if (js_params.GetAsDispatcherModule(0, GetJsRunner(), &module) &&
                 GearsBlob::kModuleName == module->get_module_name()) {
        static_cast<GearsBlob*>(module)->GetContents(
                                             &(current_request_info_->blob_));
      } else {
        RETURN_EXCEPTION(STRING16(L"Data parameter must be a string or blob."));
      }
#else
      if (!js_params.GetAsString(0, &current_request_info_->post_data)) {
        RETURN_EXCEPTION(STRING16(L"Data parameter must be a string."));
      }
      current_request_info_->has_post_data = 
                                 !current_request_info_->post_data.empty();
#endif
    }
  }

  if (page_is_unloaded_) {
    // We silently fail for this particular error condition to prevent callers
    // from detecting errors and making noises after the page has been unloaded
    return true;
  }

  InitUnloadMonitor();

  if (!CallSendOnUiThread()) {
    // go back to the uninitialized state
    SetCurrentRequestInfo(NULL);
    RETURN_EXCEPTION(kInternalError);
  }

  RETURN_NORMAL();
}

bool GearsHttpRequest::CallSendOnUiThread() {
  return NS_OK == CallAsync(GetUiThread(), kSend);
}

void GearsHttpRequest::OnSendCall(int request_id) {
  assert(IsUiThread());
  assert(!native_request_);

  MutexLock locker(&lock_);
  if (request_id != current_request_id_) {
    // The request was aborted after this message was sent, ignore it.
    return;
  }
  assert(current_request_info_.get());

  CreateNativeRequest(request_id);
  bool ok = native_request_->Open(current_request_info_->method.c_str(),
                                  current_request_info_->full_url.c_str(),
                                  true);  // async
  if (!ok) {
    current_request_info_->upcoming_ready_state = HttpRequest::COMPLETE;
    RemoveNativeRequest();
    CallReadyStateChangedOnApartmentThread();
    return;
  }

  // We defer setting up the listener to skip the OPEN callback
  native_request_->SetOnReadyStateChange(this);

  for (size_t i = 0; i < current_request_info_->headers.size(); ++i) {
    native_request_->SetRequestHeader(
                         current_request_info_->headers[i].first.c_str(), 
                         current_request_info_->headers[i].second.c_str());
  }

  if (current_request_info_->has_post_data) {
    if (!current_request_info_->content_type_header_was_set) {
      native_request_->SetRequestHeader(
                           HttpConstants::kContentTypeHeader,
                           HttpConstants::kMimeTextPlain);
    }
    ok = native_request_->SendString(current_request_info_->post_data.c_str());
#ifndef OFFICIAL_BUILD
  } else if (current_request_info_->blob_.get()) {
    if (!current_request_info_->content_type_header_was_set) {
      native_request_->SetRequestHeader(
                           HttpConstants::kContentTypeHeader,
                           HttpConstants::kMimeApplicationOctetStream);
    }
    ok = native_request_->SendBlob(current_request_info_->blob_.get());
#endif
  } else {
    ok = native_request_->Send();
  }
  
  if (!ok) {
    current_request_info_->upcoming_ready_state = HttpRequest::COMPLETE;
    RemoveNativeRequest();
    CallReadyStateChangedOnApartmentThread();
    return;
  }
}


//------------------------------------------------------------------------------
// void abort()
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::Abort() {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (current_request_info_.get()) {
    CallAbortOnUiThread();
    // go to the uninitialized state
    SetCurrentRequestInfo(NULL);
  }
  RETURN_NORMAL();
}

bool GearsHttpRequest::CallAbortOnUiThread() {
  if (IsUiThread()) {
    OnAbortCall(current_request_id_);
    return true;
  } else {
    return NS_OK == CallAsync(GetUiThread(), kAbort);
  }
}

void GearsHttpRequest::OnAbortCall(int request_id) {
  assert(IsUiThread());
  if (native_request_) {
    // No need to lock here, native_request is only accessed on the ui thread.
    // Furthermore, there should not be a lock here; depending on whether
    // or not the 'apartment thread' is also the 'ui thread', the lock may
    // already be held and our locks are not recursive.
    assert(native_request_id_ == request_id);
    nsCOMPtr<GearsHttpRequest> reference(this);
    native_request_->SetOnReadyStateChange(NULL);
    native_request_->Abort();
    RemoveNativeRequest();
  }
}

//------------------------------------------------------------------------------
// readonly attribute long readyState
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::GetReadyState(PRInt32 *retval) {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  *retval = GetState();
  RETURN_NORMAL();
}

bool GearsHttpRequest::IsValidResponse() {
  assert(IsInteractive() || IsComplete());
  return ::IsValidResponseCode(current_request_info_->response.status);
}

//------------------------------------------------------------------------------
// AString getAllResponseHeaders()
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::GetAllResponseHeaders(nsAString &retval) {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (!(IsInteractive() || IsComplete())) {
    RETURN_EXCEPTION(kNotInteractiveError);
  }
  if (!IsValidResponse()) {
    retval.Assign(kEmptyString);
    RETURN_NORMAL();
  }
  retval.Assign(current_request_info_->response.headers.c_str());
  RETURN_NORMAL();
}


//------------------------------------------------------------------------------
// AString getResponseHeader()
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::GetResponseHeader(nsAString &retval) {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (!(IsInteractive() || IsComplete())) {
    RETURN_EXCEPTION(kNotInteractiveError);
  }
  if (!IsValidResponse()) {
    retval.Assign(kEmptyString);
    RETURN_NORMAL();
  }
  if (!current_request_info_->response.parsed_headers.get()) {
    scoped_ptr<HTTPHeaders> parsed_headers(new HTTPHeaders);
    std::string headers_utf8;
    String16ToUTF8(current_request_info_->response.headers.c_str(), 
                   current_request_info_->response.headers.length(),
                   &headers_utf8);
    const char *body = headers_utf8.c_str();
    uint32 body_len = headers_utf8.length();
    if (!HTTPUtils::ParseHTTPHeaders(&body, &body_len, parsed_headers.get(),
                                     true /* allow_const_cast */)) {
      return false;
    }
    current_request_info_->response.parsed_headers.swap(parsed_headers);
  }

  JsParamFetcher js_params(this);
  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"The name parameter is required."));
  }
  std::string16 name;
  if (!js_params.GetAsString(0, &name)) {
    RETURN_EXCEPTION(STRING16(L"The name parameter must be a string."));
  }

  std::string name_utf8;
  String16ToUTF8(name.c_str(), &name_utf8);
  const char *value_utf8 =
      current_request_info_->response.parsed_headers->GetHeader(
                                                          name_utf8.c_str());
  std::string16 value;
  UTF8ToString16(value_utf8 ? value_utf8 : "", &value);
  retval.Assign(value.c_str());
  RETURN_NORMAL();
}


//------------------------------------------------------------------------------
// readonly attribute AString responseText
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::GetResponseText(nsAString &retval) {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (!(IsInteractive() || IsComplete())) {
    RETURN_EXCEPTION(kNotInteractiveError);
  }
  if (!IsValidResponse()) {
    retval.Assign(kEmptyString);
    RETURN_NORMAL();
  }
  retval.Assign(current_request_info_->response.text.c_str());
  RETURN_NORMAL();
}

#ifdef OFFICIAL_BUILD
// Blob support is not ready for prime time yet
#else
//------------------------------------------------------------------------------
// readonly attribute Blob responseBlob
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::GetResponseBlob(nsISupports **retval) {
  // TODO(nigeltao): when GearsHttpRequest becomes a Dispatcher-based module,
  // re-enable this code.
#if 0
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (!IsComplete()) {
    RETURN_EXCEPTION(STRING16(L"responseBlob is not supported before request "
                              L"is complete"));
  }

  if (current_request_info_->response.blob == NULL) {
    // Not already cached - make a new blob and copy the contents in
    scoped_ptr<GearsBlob> blob(new GearsBlob());
    if (IsValidResponse()) {
      std::vector<uint8> *body =
               current_request_info_->response.body.release();
      if (body) {
        blob->Reset(new BufferBlob(body));
      }
    }
    // else blob stays empty

    if (!blob->InitBaseFromSibling(this)) {
      RETURN_EXCEPTION(STRING16(L"Initializing base class failed."));
    }
    current_request_info_->response.blob = blob.release();
  }

  *retval = current_request_info_->response.blob;
  (*retval)->AddRef();
  RETURN_NORMAL();
#endif
  RETURN_EXCEPTION(STRING16(L"Not implemented."));
}
#endif  // not OFFICIAL_BUILD


//------------------------------------------------------------------------------
// readonly attribute long status
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::GetStatus(PRInt32 *retval) {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (!(IsInteractive() || IsComplete())) {
    RETURN_EXCEPTION(kNotInteractiveError);
  }
  if (!IsValidResponse()) {
    RETURN_EXCEPTION(kRequestFailedError);
  }
  *retval = current_request_info_->response.status;
  RETURN_NORMAL();
}


//------------------------------------------------------------------------------
// readonly attribute AString statusText
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::GetStatusText(nsAString &retval) {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (!(IsInteractive() || IsComplete())) {
    RETURN_EXCEPTION(kNotInteractiveError);
  }
  if (!IsValidResponse()) {
    RETURN_EXCEPTION(kRequestFailedError);
  }
  retval.Assign(current_request_info_->response.status_text.c_str());
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// DataAvailable called by our lower level HttpRequest class
//------------------------------------------------------------------------------
void GearsHttpRequest::DataAvailable(HttpRequest *source) {
  assert(IsUiThread());
  assert(source == native_request_);

  MutexLock locker(&lock_);
  if (native_request_id_ != current_request_id_) {
    // The request we're processing has been aborted, but we have not yet
    // received the OnAbort message. We pre-emptively call abort here, when
    // the message does arrive, it will be ignored.
    OnAbortCall(native_request_id_);
    return;
  }
  assert(current_request_info_.get());

  source->GetResponseBodyAsText(&current_request_info_->response.text);
  CallDataAvailableOnApartmentThread();
}

bool GearsHttpRequest::CallDataAvailableOnApartmentThread() {
  return NS_OK == CallAsync(apartment_thread_id_, kDataAvailable);
}

void GearsHttpRequest::OnDataAvailableCall(int request_id) {
  assert(IsApartmentThread());
  OnReadyStateChangedCall(request_id);
}


//------------------------------------------------------------------------------
// ReadyStateChanged called by our lower level HttpRequest class
//------------------------------------------------------------------------------
void GearsHttpRequest::ReadyStateChanged(HttpRequest *source) {
  assert(IsUiThread());
  assert(source == native_request_);

  nsCOMPtr<GearsHttpRequest> reference(this);
  { // The extra scope is to ensure we unlock prior to reference.Release
    HttpRequest::ReadyState state;
    source->GetReadyState(&state);

    MutexLock locker(&lock_);
    if (current_request_id_ != native_request_id_) {
      // The request we're processing has been aborted, but we have not yet
      // received the OnAbort message. We pre-emptively call abort here, when
      // the message does arrive, it will be ignored.
      OnAbortCall(native_request_id_);
      return;
    }
    assert(current_request_info_.get());

    HttpRequest::ReadyState previous_state =
                     current_request_info_->upcoming_ready_state;
    if (state > previous_state) {
      current_request_info_->upcoming_ready_state = state;
      if (state >= HttpRequest::INTERACTIVE &&
          previous_state < HttpRequest::INTERACTIVE) {
        // For HEAD requests, we skip INTERACTIVE and jump straight to COMPLETE
        source->GetAllResponseHeaders(&current_request_info_->response.headers);
        source->GetStatus(&current_request_info_->response.status);
        source->GetStatusText(&current_request_info_->response.status_text);
      }
      if (state == HttpRequest::COMPLETE) {
        source->GetResponseBodyAsText(&current_request_info_->response.text);
        current_request_info_->response.body.reset(source->GetResponseBody());
        RemoveNativeRequest();
      }
      CallReadyStateChangedOnApartmentThread();
    }
  }
}

bool GearsHttpRequest::CallReadyStateChangedOnApartmentThread() {
  return NS_OK == CallAsync(apartment_thread_id_, kReadyStateChanged);
}

void GearsHttpRequest::OnReadyStateChangedCall(int request_id) {
  assert(IsApartmentThread());
  bool is_complete;
  {
    MutexLock locker(&lock_);
    if (current_request_id_ != request_id) {
      // The request was aborted after this message was sent, ignore it.
      return;
    }
    assert(current_request_info_.get());
    current_request_info_->ready_state =
                               current_request_info_->upcoming_ready_state;
    is_complete = IsComplete();
  }

  // To remove cyclic dependencies we drop our reference to the
  // callback when the request is complete.
  JsRootedCallback *handler = is_complete ? onreadystatechange_.release()
                                          : onreadystatechange_.get();
  if (handler) {
    JsRunnerInterface *runner = GetJsRunner();
    assert(runner);
    if (runner) {
      runner->InvokeCallback(handler, 0, NULL, NULL);
    }
    if (is_complete)
      delete handler;
  }
}


//------------------------------------------------------------------------------
// SetCurrentRequestInfo, CreateNativeRequest, RemoveNativeRequest
//------------------------------------------------------------------------------

void GearsHttpRequest::SetCurrentRequestInfo(RequestInfo *info) {
  assert(IsApartmentThread());
  current_request_info_.reset(info);
  ++current_request_id_;  
}

void GearsHttpRequest::CreateNativeRequest(int request_id) {
  assert(IsUiThread());
  RemoveNativeRequest();
  HttpRequest::Create(&native_request_);
  native_request_->SetCachingBehavior(HttpRequest::USE_ALL_CACHES);
  native_request_->SetRedirectBehavior(HttpRequest::FOLLOW_WITHIN_ORIGIN);
  native_request_id_ = request_id;
  this->AddRef();
}

void GearsHttpRequest::RemoveNativeRequest() {
  assert(IsUiThread());
  if (native_request_) {
    native_request_->SetOnReadyStateChange(NULL);
    native_request_ = NULL;
    native_request_id_ = 0;
    this->Release();
  }
}


//------------------------------------------------------------------------------
// This helper does several things:
// - resolve relative urls based on the page location, the 'url' may also
//   be an absolute url to start with, if so this step does not modify it
// - normalizes the resulting absolute url, ie. removes path navigation
// - removes the fragment part of the url, ie. truncates at the '#' character
// - ensures the the resulting url is from the same-origin
// - ensures the requested url is HTTP or HTTPS
//------------------------------------------------------------------------------
bool GearsHttpRequest::ResolveUrl(const char16 *url,
                                  std::string16 *resolved_url,
                                  std::string16 *exception_message) {
  assert(url && resolved_url && exception_message);
  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url, resolved_url)) {
    *exception_message = STRING16(L"Failed to resolve URL.");
    return false;
  }

  SecurityOrigin url_origin;
  if (!url_origin.InitFromUrl(resolved_url->c_str()) ||
      !url_origin.IsSameOrigin(EnvPageSecurityOrigin())) {
    *exception_message = STRING16(L"URL is not from the same origin.");
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


//------------------------------------------------------------------------------
// GetState, IsUninitialized, IsOpen, IsSent, IsInteractive, and IsComplete.
// The caller is responsible for acquiring the lock prior to calling these
// state accessors.
//------------------------------------------------------------------------------

HttpRequest::ReadyState GearsHttpRequest::GetState() {
  assert(IsApartmentThread());
  return current_request_info_.get() ? current_request_info_->ready_state
                                     : HttpRequest::UNINITIALIZED;
}


//------------------------------------------------------------------------------
// InitUnloadMonitor
//------------------------------------------------------------------------------
void GearsHttpRequest::InitUnloadMonitor() {
  // Create an event monitor to alert us when the page unloads.
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD,
                                             this));
  }
}

void GearsHttpRequest::HandleEvent(JsEventType event_type) {
  assert(IsApartmentThread());
  assert(event_type == JSEVENT_UNLOAD);

  onreadystatechange_.reset();  // drop reference, js context is going away

  // The object can live past the life of js_runner, so remove the monitor
  // manually.
  unload_monitor_.reset(NULL);

  page_is_unloaded_ = true;
  Abort();
}


//------------------------------------------------------------------------------
// OnAsyncCall - Called when a message sent via CallAsync is delivered to us
// on the target thread of control.
//------------------------------------------------------------------------------

void GearsHttpRequest::OnAsyncCall(AsyncCallType call_type, int request_id) {
  switch (call_type) {
    case kDataAvailable:
      OnDataAvailableCall(request_id);
      break;
    case kReadyStateChanged:
      OnReadyStateChangedCall(request_id);
      break;
    case kSend:
      OnSendCall(request_id);
      break;
    case kAbort:
      OnAbortCall(request_id);
      break;
  }
}


class GearsHttpRequest::AsyncCallEvent : public AsyncFunctor {
public:
  AsyncCallEvent(GearsHttpRequest *request, AsyncCallType call_type,
                 int request_id)
      : request(request), call_type(call_type), request_id(request_id) {}
  virtual void Run() {
    request->OnAsyncCall(call_type, request_id);
  }
  nsCOMPtr<GearsHttpRequest> request;
  AsyncCallType call_type;
  int request_id;
};


// CallAsync - Posts a message to another thead's event queue. The message will
// be delivered to this AsyncTask instance on that thread via OnAsyncCall.
nsresult GearsHttpRequest::CallAsync(ThreadId thread_id, 
                                     AsyncCallType call_type) {
  // assert(lock_.IsLockedByCurrentThread());
  AsyncCallEvent *event = new AsyncCallEvent(this, call_type,
                                             current_request_id_);
  AsyncRouter::GetInstance()->CallAsync(thread_id, event);
  return NS_OK;
}

