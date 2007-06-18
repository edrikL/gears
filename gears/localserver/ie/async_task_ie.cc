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

#include "gears/base/ie/activex_utils.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/http_cookies.h"
#include "gears/localserver/common/critical_section.h"
#include "gears/localserver/ie/async_task_ie.h"

const char16 *AsyncTask::kCookieRequiredErrorMessage =
                  STRING16(L"Required cookie is not present");

const char16 *kIsOfflineErrorMessage =
                  STRING16(L"The browser is offline");

//------------------------------------------------------------------------------
// AsyncTask
//------------------------------------------------------------------------------
AsyncTask::AsyncTask() :
    is_initialized_(false),
    is_aborted_(false),
    is_running_async_(false),
    delete_when_done_(false),
    listener_window_(NULL),
    listener_message_base_(WM_USER),
    thread_(NULL) {
}

//------------------------------------------------------------------------------
// ~AsyncTask
//------------------------------------------------------------------------------
AsyncTask::~AsyncTask() {
  if (thread_) {
    if (WaitForSingleObject(thread_, 0) == WAIT_TIMEOUT) {
      ATLTRACE(_T("~AsyncTask - thread is still running\n"));
      Abort();
      WaitForSingleObject(thread_, INFINITE);
    }
    CloseHandle(thread_);
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

  is_aborted_ = false;

  // create events, both are auto-reset and initially unsignalled
  if (!abort_event_.Create(NULL, FALSE, FALSE, NULL)) {
    return false;
  }
  if (!ready_state_changed_event_.Create(NULL, FALSE, FALSE, NULL)) {
    return false;
  }
  is_initialized_ = true;
  return true;
}

//------------------------------------------------------------------------------
// SetListenerWindow
//------------------------------------------------------------------------------
void AsyncTask::SetListenerWindow(HWND listener_window,
                                  int listener_message_base) {
  CritSecLock locker(lock_);
  listener_window_ = listener_window;
  listener_message_base_ = listener_message_base;
}

//------------------------------------------------------------------------------
// Start
//------------------------------------------------------------------------------
bool AsyncTask::Start() {
  if (!is_initialized_ || is_running_async_) {
    return false;
  }

  if (thread_) {
    CloseHandle(thread_);
    thread_ = NULL;
  }

  is_aborted_ = false;
  abort_event_.Reset();
  ready_state_changed_event_.Reset();
  unsigned int thread_id;
  thread_ = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, ThreadMain, 
                                                    this, 0, &thread_id));
  is_running_async_ = (thread_ != NULL);
  return is_running_async_;
}

//------------------------------------------------------------------------------
// Abort
//------------------------------------------------------------------------------
void AsyncTask::Abort() {
  CritSecLock locker(lock_);
  if (is_running_async_ && !is_aborted_) {
    ATLTRACE(_T("AsyncTask::Abort\n"));
    is_aborted_ = true;
    abort_event_.Set();
  }
}

//------------------------------------------------------------------------------
// DeleteWhenDone
//------------------------------------------------------------------------------
void AsyncTask::DeleteWhenDone() {
  CritSecLock locker(lock_);
  assert(!delete_when_done_);
  if (!delete_when_done_) {
    ATLTRACE(_T("AsyncTask::DeleteWhenDone\n"));
    if (!is_running_async_) {
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
// ThreadMain
//------------------------------------------------------------------------------
unsigned int __stdcall AsyncTask::ThreadMain(void *task) {
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  AsyncTask *self = reinterpret_cast<AsyncTask*>(task);

  self->Run();

  {
    CritSecLock locker(self->lock_);
    self->is_running_async_ = false;
    if (self->delete_when_done_) {
      CloseHandle(self->thread_);
      self->thread_ = NULL;
      // In this particular code path, we have to call unlock prior to delete 
      // otherwise the locker would try to access deleted memory, &lock_,
      // after it's been freed.
      locker.Unlock();
      delete self;
    }
  }

  CoUninitialize();
  return 0;
}

//------------------------------------------------------------------------------
// NotifyListener
//------------------------------------------------------------------------------
void AsyncTask::NotifyListener(int code, int param) {
  CritSecLock locker(lock_);
  if (listener_window_) {
    PostMessage(listener_window_,
                code + listener_message_base_,
                param,
                reinterpret_cast<LPARAM>(this));
  }
}

//------------------------------------------------------------------------------
// HttpGet
//------------------------------------------------------------------------------
bool AsyncTask::HttpGet(const char16 *full_url,
                        bool is_capturing,
                        const char16 *if_mod_since_date,
                        const char16 *required_cookie,
                        WebCacheDB::PayloadInfo *payload,
                        bool *was_redirected,
                        std::string16 *full_redirect_url,
                        std::string16 *error_message) {
  if (was_redirected) {
    *was_redirected = false;
  }
  if (full_redirect_url) {
    full_redirect_url->clear();
  }
  if (error_message) {
    error_message->clear();
  }

  if (is_capturing && !ActiveXUtils::IsOnline()) {
    if (error_message) {
      *error_message = kIsOfflineErrorMessage;
    }
    return false;
  }

  if (required_cookie && required_cookie[0]) {
    if (!IsCookiePresent(full_url, required_cookie)) {
      if (error_message) {
        *error_message = kCookieRequiredErrorMessage;
      }
      return false;
    }
  }

  ScopedHttpRequestPtr scoped_http_request(HttpRequest::Create());
  HttpRequest *http_request = scoped_http_request.get();
  if (!http_request) {
    return false;
  }

  if (!http_request->open(HttpConstants::kHttpGET, full_url, true)) {
    return false;
  }

  if (is_capturing) {
    http_request->setFollowRedirects(false);
    if (!http_request->setRequestHeader(HttpConstants::kXGoogleGearsHeader,
                                        STRING16(L"1"))) {
      return false;
    }
  }

  if (!http_request->setRequestHeader(HttpConstants::kCacheControlHeader,
                                      HttpConstants::kNoCache)) {
    return false;
  }

  if (!http_request->setRequestHeader(HttpConstants::kPragmaHeader,
                                      HttpConstants::kNoCache)) {
    return false;
  }

  if (if_mod_since_date && if_mod_since_date[0]) {
    if (!http_request->setRequestHeader(HttpConstants::kIfModifiedSinceHeader,
                                        if_mod_since_date)) {
      return false;
    }
  }

  payload->data.reset();
  http_request->setOnReadyStateChange(this);
  ready_state_changed_event_.Reset();

  if (!http_request->send()) {
    http_request->setOnReadyStateChange(NULL);
    return false;
  }

  enum {
    kReadyStateChangedEvent = WAIT_OBJECT_0,
    kAbortEvent 
  };
  HANDLE handles[] = { ready_state_changed_event_.m_h,
                       abort_event_.m_h };
  bool done = false;
  while (!done) {
    // We need to run a message loop receive COM callbacks from URLMON
    DWORD rv = MsgWaitForMultipleObjectsEx(
                   ARRAYSIZE(handles), handles, INFINITE, QS_ALLINPUT,
                   MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
    if (rv == kReadyStateChangedEvent) {
      long state;
      // TODO(michaeln): perhaps simplify the HttpRequest interface to
      // include a getResponse(&payload) method?
      if (http_request->getReadyState(&state)) {
        if (state == 4) {
          done = true;
          if (!is_aborted_) {
            long status;
            if (http_request->getStatus(&status)) {
              payload->status_code = status;
              if (http_request->getStatusLine(&payload->status_line)) {
                if (http_request->getAllResponseHeaders(&payload->headers)) {
                  payload->data.reset(http_request->getResponseBody());
                }
              }
            }
          }
        }
      } else {
        ATLTRACE(_T("AsyncTask - getReadyState failed, aborting request\n"));
        http_request->abort();
        done = true;
      }
    } else if (rv == kAbortEvent) {
      ATLTRACE(_T("AsyncTask - abort event signalled, aborting request\n"));
      // We abort the request but continue the loop waiting for it to complete
      // TODO(michaeln): paranoia, what if it never does complete, timer?
      http_request->abort();
    } else {
      // We have message queue input to pump. We pump the queue dry
      // prior to looping and calling MsgWaitForMultipleObjects
      MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  http_request->setOnReadyStateChange(NULL);

  if (http_request->wasRedirected()) {
    if (was_redirected) {
      *was_redirected = true;
    }
    if (full_redirect_url) {
      http_request->getRedirectUrl(full_redirect_url);
    }
  }
  
  return !is_aborted_ && payload->data.get();
}


//------------------------------------------------------------------------------
// HttpRequest::ReadyStateListener::ReadyStateChanged
//------------------------------------------------------------------------------
void AsyncTask::ReadyStateChanged(HttpRequest *source) {
  ready_state_changed_event_.Set();
}
