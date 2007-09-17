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
#include "gears/third_party/gecko_internal/nsIDOMClassInfo.h"
#include "gears/third_party/gecko_internal/nsIEventQueueService.h"

#include "gears/httprequest/firefox/httprequest_ff.h"

#include "gears/base/common/js_runner.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/localserver/common/http_constants.h"

// Returns true if the currently executing thread is the main UI thread,
// firefox/mozila has one such very special thread
// See cache_intercept.cc for implementation
bool IsUiThread();

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
static const char16 *kNotCompleteError = STRING16(L"Request is not done.");
static const char16 *kNotInteractiveError =
                        STRING16(L"Request is not loading or done.");

static const char16 *kEmptyString = STRING16(L"");

GearsHttpRequest::GearsHttpRequest()
    : request_(NULL), apartment_thread_(PR_GetCurrentThread()),
      content_type_header_was_set_(false), page_is_unloaded_(false) {
}

GearsHttpRequest::~GearsHttpRequest() {
  assert(!request_);
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
      request_info_.reset(NULL);
      response_info_.reset(NULL);
      assert(!request_);
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
    scoped_info->method = MakeUpperString(method);
    
    std::string16 exception_message;
    if (!ResolveUrl(url.c_str(), &scoped_info->full_url, &exception_message)) {
      RETURN_EXCEPTION(exception_message.c_str());
    }

    request_info_.swap(scoped_info);
    content_type_header_was_set_ = false;

    response_info_.reset(new ResponseInfo());
    response_info_->pending_ready_state = HttpRequest::OPEN;
    response_info_->ready_state = HttpRequest::UNINITIALIZED;
  }

  OnReadyStateChangedCall();

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

  request_info_->headers.push_back(std::make_pair(name, value));

  if (StringCompareIgnoreCase(name.c_str(),
                              HttpConstants::kContentTypeHeader) == 0) {
    content_type_header_was_set_ = true;
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

  if (!InitEventQueues()) {
    RETURN_EXCEPTION(kInternalError);
  }

  if (request_info_->method == HttpConstants::kHttpPOST ||
      request_info_->method == HttpConstants::kHttpPUT) {
    JsParamFetcher js_params(this);
    if (js_params.GetCount(false) > 0) {
      if (!js_params.GetAsString(0, &request_info_->post_data)) {
        RETURN_EXCEPTION(STRING16(L"Data parameter must be a string."));
      }
      request_info_->has_post_data = !request_info_->post_data.empty();
    }
  }

  if (page_is_unloaded_) {
    // We silently fail for this particular error condition to prevent callers
    // from detecting errors and making noises after the page has been unloaded
    return true;
  }

  if (!EnvIsWorker() && !page_unload_monitor_.get()) {
    // TODO(michaeln): Do this for workers too.
    // If we haven't already done so, we attach a handler to the OnUnload
    // event so we can abort request when the page is unloaded.
    page_unload_monitor_.reset(new HtmlEventMonitor(kEventUnload,
                                                    OnPageUnload, this));
    nsCOMPtr<nsIDOMEventTarget> event_source;
    if (NS_SUCCEEDED(DOMUtils::GetWindowEventTarget(
                                   getter_AddRefs(event_source)))) {
      page_unload_monitor_->Start(event_source);
    }
  }

  if (!CallSendOnUiThread()) {
    response_info_.reset(NULL);
    RETURN_EXCEPTION(kInternalError);
  }

  RETURN_NORMAL();
}

bool GearsHttpRequest::CallSendOnUiThread() {
  return CallAsync(ui_event_queue_, kSend) == NS_OK;
}

void GearsHttpRequest::OnSendCall() {
  assert(IsUiThread());
  assert(!request_);
  assert(request_info_.get());
  assert(response_info_.get());

  MutexLock locker(&lock_);
  CreateRequest();
  bool ok = request_->Open(request_info_->method.c_str(),
                           request_info_->full_url.c_str(),
                           true);  // async
  if (!ok) {
    response_info_->pending_ready_state = HttpRequest::COMPLETE;
    RemoveRequest();
    CallReadyStateChangedOnApartmentThread();
    return;
  }

  // We defer setting up the listener to skip the OPEN callback
  request_->SetOnReadyStateChange(this);

  for (size_t i = 0; i < request_info_->headers.size(); ++i) {
    request_->SetRequestHeader(request_info_->headers[i].first.c_str(), 
                               request_info_->headers[i].second.c_str());
  }

  if (request_info_->has_post_data) {
    if (!content_type_header_was_set_) {
      request_->SetRequestHeader(HttpConstants::kContentTypeHeader,
                                 HttpConstants::kMimeTextPlain);
    }
    ok = request_->SendString(request_info_->post_data.c_str());
  } else {
    ok = request_->Send();
  }
  
  if (!ok) {
    response_info_->pending_ready_state = HttpRequest::COMPLETE;
    RemoveRequest();
    CallReadyStateChangedOnApartmentThread();
    return;
  }
}


//------------------------------------------------------------------------------
// void abort()
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::Abort() {
  assert(IsApartmentThread());
  CallAbortOnUiThread();
  RETURN_NORMAL();
}

bool GearsHttpRequest::CallAbortOnUiThread() {
  if (IsUiThread()) {
    OnAbortCall();
    return true;
  } else {
    return CallAsync(ui_event_queue_, kAbort) == NS_OK;
  }
}

void GearsHttpRequest::OnAbortCall() {
  assert(IsUiThread());
  nsCOMPtr<GearsHttpRequest> reference(this);
  { 
    // The extra scope is to ensure we unlock prior to reference.Release
    MutexLock locker(&lock_);
    if (request_) {
      request_info_.reset(NULL);
      response_info_.reset(NULL);
      request_->SetOnReadyStateChange(NULL);
      request_->Abort();
      RemoveRequest();
    }
  }
}

//------------------------------------------------------------------------------
// readonly attribute long readyState
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsHttpRequest::GetReadyState(PRInt32 *retval) {
  assert(IsApartmentThread());
  MutexLock locker(&lock_);
  if (IsUninitialized()) {
    *retval = HttpRequest::UNINITIALIZED;  // 0
  } else if (IsOpen()) {
    *retval = HttpRequest::OPEN;  // 1
  } else if (IsSent()) {
    *retval = HttpRequest::SENT;  // 2
  } else if (IsInteractive()) {
    *retval = HttpRequest::INTERACTIVE;  // 3
  } else if (IsComplete()) {
    *retval = HttpRequest::COMPLETE; // 4
  } else {
    assert(false);
    RETURN_EXCEPTION(kInternalError);
  }
  RETURN_NORMAL();
}

bool GearsHttpRequest::IsValidResponse() {
  assert(IsInteractive() || IsComplete());
  return ::IsValidResponseCode(response_info_->status);
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
  retval.Assign(response_info_->headers.c_str());
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
  if (!response_info_->parsed_headers.get()) {
    scoped_ptr<HTTPHeaders> parsed_headers(new HTTPHeaders);
    std::string headers_utf8;
    String16ToUTF8(response_info_->headers.c_str(), 
                   response_info_->headers.length(),
                   &headers_utf8);
    const char *body = headers_utf8.c_str();
    uint32 body_len = headers_utf8.length();
    if (!HTTPUtils::ParseHTTPHeaders(&body, &body_len, parsed_headers.get(),
                                     true /* allow_const_cast */)) {
      return false;
    }
    response_info_->parsed_headers.swap(parsed_headers);
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
                  response_info_->parsed_headers->GetHeader(name_utf8.c_str());
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
  if (!IsComplete()) {
    RETURN_EXCEPTION(kNotCompleteError);
  }
  if (!IsValidResponse()) {
    retval.Assign(kEmptyString);
    RETURN_NORMAL();
  }
  retval.Assign(response_info_->response_text.c_str());
  RETURN_NORMAL();
}


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
  *retval = response_info_->status;
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
  retval.Assign(response_info_->status_text.c_str());
  RETURN_NORMAL();
}


//------------------------------------------------------------------------------
// ReadyStateChanged called by our lower level HttpRequest class
//------------------------------------------------------------------------------
void GearsHttpRequest::ReadyStateChanged(HttpRequest *source) {
  assert(IsUiThread());
  assert(source == request_);

  MutexLock locker(&lock_);

  HttpRequest::ReadyState previous_state = response_info_->pending_ready_state;
  HttpRequest::ReadyState state;
  source->GetReadyState(&state);
  if (state > previous_state) {
    response_info_->pending_ready_state = state;
    if (state >= HttpRequest::INTERACTIVE &&
        previous_state < HttpRequest::INTERACTIVE) {
      // For HEAD requests, we skip INTERACTIVE and jump straight to COMPLETE
      source->GetAllResponseHeaders(&response_info_->headers);
      source->GetStatus(&response_info_->status);
      source->GetStatusText(&response_info_->status_text);
    }
    if (state == HttpRequest::COMPLETE) {
      source->GetResponseBodyAsText(&response_info_->response_text);
      RemoveRequest();
    }
    CallReadyStateChangedOnApartmentThread();
  }
}

bool GearsHttpRequest::CallReadyStateChangedOnApartmentThread() {
  return CallAsync(apartment_event_queue_, kReadyStateChanged) == NS_OK;
}

void GearsHttpRequest::OnReadyStateChangedCall() {
  assert(IsApartmentThread());
  lock_.Lock();
  response_info_->ready_state = response_info_->pending_ready_state;
  bool is_complete = IsComplete();
  lock_.Unlock();
  
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
// CreateRequest, RemoveRequest
//------------------------------------------------------------------------------

void GearsHttpRequest::CreateRequest() {
  assert(IsUiThread());
  RemoveRequest();
  request_ = HttpRequest::Create();
  request_->SetCachingBehavior(HttpRequest::USE_ALL_CACHES);
  request_->SetRedirectBehavior(HttpRequest::FOLLOW_WITHIN_ORIGIN);
  this->AddRef();
}

void GearsHttpRequest::RemoveRequest() {
  assert(IsUiThread());
  if (request_) {
    request_->SetOnReadyStateChange(NULL);
    request_->ReleaseReference();
    request_ = NULL;
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
// IsUninitialized, IsOpen, IsSent, IsInteractive, and IsComplete.
// The caller is responsible for acquiring the lock prior to calling these
// state accessors.
//------------------------------------------------------------------------------

bool GearsHttpRequest::IsUninitialized() {
  assert(IsApartmentThread());
  return !response_info_.get() ||
         response_info_->ready_state == HttpRequest::UNINITIALIZED;
}

bool GearsHttpRequest::IsOpen() {
  assert(IsApartmentThread());
  return response_info_.get() &&
         response_info_->ready_state == HttpRequest::OPEN;
}

bool GearsHttpRequest::IsSent() {
  assert(IsApartmentThread());
  return response_info_.get() &&
         response_info_->ready_state == HttpRequest::SENT;
}

bool GearsHttpRequest::IsInteractive() {
  assert(IsApartmentThread());
  return response_info_.get() &&
         response_info_->ready_state == HttpRequest::INTERACTIVE;
}

bool GearsHttpRequest::IsComplete() {
  assert(IsApartmentThread());
  return response_info_.get() &&
         response_info_->ready_state == HttpRequest::COMPLETE;
}


//------------------------------------------------------------------------------
// InitEventQueues
//------------------------------------------------------------------------------
bool GearsHttpRequest::InitEventQueues() {
  assert(IsApartmentThread());
  nsresult rv = NS_OK;

  nsCOMPtr<nsIEventQueueService> event_queue_service =
      do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv) || !event_queue_service) {
    return false;
  }

  if (!apartment_event_queue_) {
    rv = event_queue_service->GetThreadEventQueue(
              NS_CURRENT_THREAD, getter_AddRefs(apartment_event_queue_));
    if (NS_FAILED(rv) || !apartment_event_queue_) {
      return false;
    }
  }

  if (!ui_event_queue_) {
    rv = event_queue_service->GetThreadEventQueue(
              NS_UI_THREAD, getter_AddRefs(ui_event_queue_));
    if (NS_FAILED(rv) || !ui_event_queue_) {
      return false;
    }
  }

  return true;
}


//------------------------------------------------------------------------------
// OnAsyncCall - Called when a message sent via CallAsync is delivered to us
// on the target thread of control.
//------------------------------------------------------------------------------

void GearsHttpRequest::OnAsyncCall(AsyncCallType call_type) {
  switch (call_type) {
    case kReadyStateChanged:
      OnReadyStateChangedCall();
      break;
    case kSend:
      OnSendCall();
      break;
    case kAbort:
      OnAbortCall();
      break;
  }
}


// AsyncCallEvent - Custom event class used to post messages across threads
struct GearsHttpRequest::AsyncCallEvent : PLEvent {
  AsyncCallEvent(GearsHttpRequest *request, AsyncCallType call_type)
      : request(request), call_type(call_type) {}
  nsCOMPtr<GearsHttpRequest> request;
  AsyncCallType call_type;
};


// CallAsync - Posts a message to another thead's event queue. The message will
// be delivered to this AsyncTask instance on that thread via OnAsyncCall.
nsresult GearsHttpRequest::CallAsync(nsIEventQueue *event_queue, 
                                     AsyncCallType call_type) {
  AsyncCallEvent *event = new AsyncCallEvent(this, call_type);
  if (!event) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult rv = event_queue->InitEvent(event, this,
      reinterpret_cast<PLHandleEventProc>(AsyncCall_EventHandlerFunc),
      reinterpret_cast<PLDestroyEventProc>(AsyncCall_EventCleanupFunc));
  if (NS_FAILED(rv)) {
    delete event;
    return rv;
  }

  rv = event_queue->PostEvent(event);
  if (NS_FAILED(rv)) {
    delete event;  // TODO(michaeln): should call PL_DestroyEvent(event) here?
  }
  return rv;
}


// static
void *PR_CALLBACK
GearsHttpRequest::AsyncCall_EventHandlerFunc(AsyncCallEvent *event) {
  event->request->OnAsyncCall(event->call_type);
  return nsnull;
}


// static
void PR_CALLBACK
GearsHttpRequest::AsyncCall_EventCleanupFunc(AsyncCallEvent *event) {
  delete event;
}


// static
void GearsHttpRequest::OnPageUnload(void *self) {
  (reinterpret_cast<GearsHttpRequest*>(self))->page_is_unloaded_ = true;
  (reinterpret_cast<GearsHttpRequest*>(self))->Abort();
}
