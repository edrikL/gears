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
#include <nspr.h> // for PR_*
#ifdef WIN32
#include <windows.h> // must manually #include before nsIEventQueueService.h
#endif
#include "gears/base/firefox/dom_utils.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/http_cookies.h"
#include "gears/localserver/firefox/async_task_ff.h"
#include "gears/third_party/gecko_internal/nsIEventQueue.h"
#include "gears/third_party/gecko_internal/nsIEventQueueService.h"

const char16 *AsyncTask::kCookieRequiredErrorMessage =
                  STRING16(L"Required cookie is not present");

const char16 *kIsOfflineErrorMessage =
                  STRING16(L"The browser is offline");

// Returns true if the currently executing thread is the main UI thread,
// firefox/mozila has one such very special thread
// See cache_intercept.cc for implementation
bool IsUiThread();

//------------------------------------------------------------------------------
// AsyncTask
//------------------------------------------------------------------------------
AsyncTask::AsyncTask() :
    is_aborted_(false),
    is_initialized_(false),
    is_destructing_(false),
    delete_when_done_(false),
    listener_(NULL),
    thread_(NULL),
    http_request_(NULL),
    params_(NULL) {
}

//------------------------------------------------------------------------------
// ~AsyncTask
//------------------------------------------------------------------------------
AsyncTask::~AsyncTask() {
  if (thread_) {
    LOG(("~AsyncTask - thread is still running\n"));
    // extra scope to lock our monitor
    {
      CritSecLock locker(lock_);
      is_destructing_ = true;
      Abort();
    }
    PR_JoinThread(thread_);
    thread_ = NULL;
  }

  if (listener_event_queue_) {
    listener_event_queue_->RevokeEvents(this);
  }
  if (ui_event_queue_) {
    ui_event_queue_->RevokeEvents(this);
  }
}

//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
bool AsyncTask::Init() {
  if (is_initialized_) {
    assert(!is_initialized_);
    return false;
  }

  if (!static_cast<PRMonitor*>(lock_)) {
    return false;
  }

  is_aborted_ = false;

  nsresult rv;
  nsCOMPtr<nsIEventQueueService> event_queue_service =
      do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return false;
  }

  rv = event_queue_service->GetThreadEventQueue(
            NS_CURRENT_THREAD, getter_AddRefs(listener_event_queue_));
  if (NS_FAILED(rv) || !listener_event_queue_) {
    return false;
  }

  rv = event_queue_service->GetThreadEventQueue(
            NS_UI_THREAD, getter_AddRefs(ui_event_queue_));
  if (NS_FAILED(rv) || !ui_event_queue_) {
    return false;
  }

  is_initialized_ = true;
  return true;
}

//------------------------------------------------------------------------------
// SetListener
//------------------------------------------------------------------------------
void AsyncTask::SetListener(Listener *listener) {
  listener_ = listener;
}

//------------------------------------------------------------------------------
// Start
//------------------------------------------------------------------------------
bool AsyncTask::Start() {
  if (!is_initialized_) {
    assert(is_initialized_);
    return false;
  }

  is_aborted_ = false;

  // Start our worker thread
  thread_ = PR_CreateThread(PR_USER_THREAD, ThreadEntry, // type, func
                            this, PR_PRIORITY_NORMAL,   // arg, priority
                            PR_LOCAL_THREAD,          // scheduled by whom?
                            PR_JOINABLE_THREAD, 0); // joinable?, stack bytes
  return (thread_ != NULL);
}

//------------------------------------------------------------------------------
// Abort
//------------------------------------------------------------------------------
void AsyncTask::Abort() {
  if (!IsUiThread()) {
    // We only initiate / terminate HTTP requests from the UI thread as it's
    // not safe to do from random worker threads. If Abort() has not been
    // called on the UI thread, post a message to the UI thread to execute
    // the block of code below else on the UI thread.
    CallAsync(ui_event_queue_, kAbortMessageCode, NULL);
  } else {
    CritSecLock locker(lock_);
    if (thread_ && !is_aborted_) {
      LOG(("AsyncTask::Abort\n"));
      is_aborted_ = true;
      if (http_request_.get()) {
        http_request_.get()->SetOnReadyStateChange(NULL);
        http_request_.get()->Abort();
        http_request_.reset(NULL);
      }
      PR_Notify(lock_);
    }
  }
}

//------------------------------------------------------------------------------
// DeleteWhenDone
//------------------------------------------------------------------------------
void AsyncTask::DeleteWhenDone() {
  CritSecLock locker(lock_);
  assert(!delete_when_done_);
  if (!delete_when_done_) {
    LOG(("AsyncTask::DeleteWhenDone\n"));
    if (!thread_) {
      // In this particular code path, we have to call unlock prior to delete 
      // otherwise the locker would try to access deleted memory, &lock_,
      // after it's been freed.
      locker.Unlock();
      delete this;
    } else {
      delete_when_done_ = true;
    }
  }
}


//------------------------------------------------------------------------------
// ThreadEntry - Our worker thread's entry procedure
//------------------------------------------------------------------------------
void AsyncTask::ThreadEntry(void *task) {
  AsyncTask *self = reinterpret_cast<AsyncTask*>(task);
  self->Run();
  self->CallAsync(self->listener_event_queue_, kThreadDoneMessageCode, NULL);
}

//------------------------------------------------------------------------------
// NotifyListener
//------------------------------------------------------------------------------
void AsyncTask::NotifyListener(int code, int param) {
  CallAsync(listener_event_queue_, code, reinterpret_cast<void*>(param));
}

//------------------------------------------------------------------------------
// OnListenerEvent
//------------------------------------------------------------------------------
void AsyncTask::OnListenerEvent(int msg_code, int msg_param) {
  if (listener_) {
    listener_->HandleEvent(msg_code, msg_param, this);
  }
}

//------------------------------------------------------------------------------
// OnThreadDone
//------------------------------------------------------------------------------
void AsyncTask::OnThreadDone() {
  CritSecLock locker(lock_);
  thread_ = NULL;
  if (delete_when_done_) {
    locker.Unlock();
    delete this;
  }
}

//------------------------------------------------------------------------------
// struct HttpRequestParameters
//------------------------------------------------------------------------------
struct AsyncTask::HttpRequestParameters {
  const char16 *full_url;
  bool is_capturing;
  const char16 *if_mod_since_date;
  const char16 *required_cookie;
  WebCacheDB::PayloadInfo *payload;
  bool *was_redirected;
  std::string16 *full_redirect_url;
  std::string16 *error_message;
};

//------------------------------------------------------------------------------
// HttpGet - Performs an HTTP get which appears synchronous to our caller on
// the worker thread. Under the covers a message is posted to the UI thread and
// the current thread waits for our lock to be signaled. The UI thread
// initiates an async HTTP request and signals our lock when the request
// completes.
//------------------------------------------------------------------------------
bool AsyncTask::HttpGet(const char16 *full_url,
                        bool is_capturing,
                        const char16 *if_mod_since_date,
                        const char16 *required_cookie,
                        WebCacheDB::PayloadInfo *payload,
                        bool *was_redirected,
                        std::string16 *full_redirect_url,
                        std::string16 *error_message) {
  // This method cannot be called from the UI thread.
  assert(!IsUiThread());

  if (was_redirected) {
    *was_redirected = false;
  }
  if (full_redirect_url) {
    full_redirect_url->clear();
  }
  if (error_message) {
    error_message->clear();
  }

  if (!DOMUtils::IsOnline()) {
    if (error_message) {
      *error_message = kIsOfflineErrorMessage;
    }
    return false;
  }

  CritSecLock locker(lock_);
  if (is_aborted_) {
    return false;
  }
  payload->data.reset(NULL);

  // We actually initiate / terminate HTTP requests from the UI thread as
  // it's not safe to do from a worker thread.
  // Setup parameters for OnStartHttpGet executing on the UI thread to look at
  HttpRequestParameters params;
  params.full_url = full_url;
  params.is_capturing = is_capturing;
  params.if_mod_since_date = if_mod_since_date;
  params.required_cookie = required_cookie;
  params.payload = payload;
  params.was_redirected = was_redirected;
  params.full_redirect_url = full_redirect_url;
  params.error_message = error_message;
  params_ = &params;

  // Send a message to the UI thread to initiate the get
  CallAsync(ui_event_queue_, kStartHttpGetMessageCode, NULL);

  // Wait for completion
  PR_Wait(lock_, PR_INTERVAL_NO_TIMEOUT);

  return !is_aborted_ && payload->data.get();
}



//------------------------------------------------------------------------------
// OnStartHttpGet
//------------------------------------------------------------------------------
bool AsyncTask::OnStartHttpGet() {
  if (is_aborted_) {
    // This can happen if Abort() is called after the worker thread has
    // queued a call to start a request but prior to having started that
    // request. In this case params_ will have been deleted.
    return false;
  }

  if (params_->required_cookie && params_->required_cookie[0]) {
    if (!IsCookiePresent(params_->full_url, params_->required_cookie)) {
      if (params_->error_message) {
        *(params_->error_message) = kCookieRequiredErrorMessage;
      }
      return false;
    }
  }

  ScopedHttpRequestPtr scoped_http_request(HttpRequest::Create());
  HttpRequest *http_request = scoped_http_request.get();
  if (!http_request) {
    return false;
  }

  http_request->SetCachingBehavior(HttpRequest::BYPASS_ALL_CACHES);

  if (!http_request->Open(HttpConstants::kHttpGET, params_->full_url, true)) {
    return false;
  }

  if (params_->is_capturing) {
    http_request->SetFollowRedirects(false);
    if (!http_request->SetRequestHeader(HttpConstants::kXGoogleGearsHeader,
                                        STRING16(L"1"))) {
      return false;
    }
  }

  if (!http_request->SetRequestHeader(HttpConstants::kCacheControlHeader,
                                      HttpConstants::kNoCache)) {
    return false;
  }

  if (!http_request->SetRequestHeader(HttpConstants::kPragmaHeader,
                                      HttpConstants::kNoCache)) {
    return false;
  }

  if (params_->if_mod_since_date && params_->if_mod_since_date[0]) {
    if (!http_request->SetRequestHeader(HttpConstants::kIfModifiedSinceHeader,
                                        params_->if_mod_since_date)) {
      return false;
    }
  }

  http_request->SetOnReadyStateChange(this);

  http_request_.reset(scoped_http_request.release());
  if (!http_request->Send()) {
    http_request->SetOnReadyStateChange(NULL);
    http_request_.reset(NULL);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
// HttpRequest::ReadyStateListener::ReadyStateChanged
//------------------------------------------------------------------------------
void AsyncTask::ReadyStateChanged(HttpRequest *http_request) {
  assert(params_);
  assert(http_request_.get() == http_request);
  int state;
  if (http_request->GetReadyState(&state)) {
    if (state == 4) {
      if (!is_aborted_) {
        int status;
        if (http_request->GetStatus(&status)) {
          WebCacheDB::PayloadInfo *payload = params_->payload;
          payload->status_code = status;
          if (http_request->GetStatusLine(&payload->status_line)) {
            if (http_request->GetAllResponseHeaders(&payload->headers)) {
              payload->data.reset(http_request->GetResponseBody());
            }
          }
        }
      }
      http_request->SetOnReadyStateChange(NULL);
      if (http_request->WasRedirected()) {
        if (params_->was_redirected) {
          *(params_->was_redirected) = true;
        }
        if (params_->full_redirect_url) {
          http_request->GetRedirectUrl(params_->full_redirect_url);
        }
      }
      http_request_.reset(NULL);
      CritSecLock locker(lock_);
      PR_Notify(lock_);
    }
  } else {
    http_request->SetOnReadyStateChange(NULL);
    http_request->Abort();
    http_request_.reset(NULL);
    CritSecLock locker(lock_);
    PR_Notify(lock_);
  }
}

//------------------------------------------------------------------------------
// OnAsyncCall - Called when a message sent via CallAsync is delivered to us
// on the target thread of control.
//------------------------------------------------------------------------------
void AsyncTask::OnAsyncCall(int msg_code, void *msg_param) {
  switch (msg_code) {
    case kThreadDoneMessageCode:
      OnThreadDone();
      break;
    case kStartHttpGetMessageCode:
      assert(IsUiThread());
      if (!OnStartHttpGet()) {
        CritSecLock locker(lock_);
        PR_Notify(lock_);
      }
      break;
    case kAbortMessageCode:
      assert(IsUiThread());
      Abort();
      break;
    default:
      OnListenerEvent(msg_code, reinterpret_cast<int>(msg_param));
      break;
  }
}

//------------------------------------------------------------------------------
// AsyncCallEvent - Custom event class used to post messages across threads
//------------------------------------------------------------------------------
struct AsyncTask::AsyncCallEvent : PLEvent {
  AsyncCallEvent(AsyncTask *task, int code, void *param)
      : task(task), msg_code(code), msg_param(param) {}
  AsyncTask *task;
  int msg_code;
  void *msg_param;
};

//------------------------------------------------------------------------------
// CallAsync - Posts a message to another thead's event queue. The message will
// be delivered to this AsyncTask instance on that thread via OnAsyncCall.
//------------------------------------------------------------------------------
nsresult AsyncTask::CallAsync(nsIEventQueue *event_queue, 
                              int msg_code, void *msg_param) {
  CritSecLock locker(lock_);
  if (is_destructing_) {
    return NS_ERROR_FAILURE;
  }

  AsyncCallEvent *event = new AsyncCallEvent(this, msg_code, msg_param);
  if (!event) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult rv = event_queue->InitEvent(event, this,
      reinterpret_cast<PLHandleEventProc>(AsyncCall_EventHandlerFunc),
      reinterpret_cast<PLDestroyEventProc>(AsyncCall_EventCleanupFunc));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = event_queue->PostEvent(event);
  if (NS_FAILED(rv)) {
    // TODO(michaeln): PL_DestroyEvent(event);
  }
  return rv;
}

//------------------------------------------------------------------------------
// AsyncCall_EventHandlerFunc
//------------------------------------------------------------------------------
// static
void *PR_CALLBACK
AsyncTask::AsyncCall_EventHandlerFunc(AsyncCallEvent *event) {
  event->task->OnAsyncCall(event->msg_code, event->msg_param);
  return nsnull;
}

//------------------------------------------------------------------------------
// AsyncCall_EventCleanupFunc
//------------------------------------------------------------------------------
// static
void PR_CALLBACK
AsyncTask::AsyncCall_EventCleanupFunc(AsyncCallEvent *event) {
  delete event;
}
