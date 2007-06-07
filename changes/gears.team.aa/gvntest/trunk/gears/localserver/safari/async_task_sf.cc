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

#include <assert.h>

#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/http_cookies.h"
#include "gears/localserver/safari/async_task_sf.h"

const char16 *AsyncTask::kCookieRequiredErrorMessage =
                  STRING16(L"Required cookie is not present");

//------------------------------------------------------------------------------
// AsyncTask
//------------------------------------------------------------------------------
AsyncTask::AsyncTask() :
    is_aborted_(false),
    is_initialized_(false),
    is_destructing_(false),
    delete_when_done_(false),
    thread_(NULL),
    listener_(NULL),
    msg_port_(NULL) {
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
      locker.Unlock();
      Abort();
    }
    pthread_join(thread_, NULL);
    thread_ = NULL;
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
  is_initialized_ = true;

  // Create a mach port to communicate between the threads
  CFMachPortContext context;
  context.version = 0;
  context.info = this;
  context.retain = NULL;
  context.release = NULL;
  context.copyDescription = NULL;
  Boolean should_free;
  msg_port_.reset(CFMachPortCreate(kCFAllocatorDefault, ReceiveMessage, 
                                   &context, &should_free));

  if (!msg_port_.get() || should_free) {
    LOG(("Couldn't make mach port\n"));
    return false;
  }

  // Create a RL source and add it to the current runloop.  This is so that
  // the thread can send a message to the main thread via PostMessage()
  scoped_CFRunLoopSource 
    msg_source(CFMachPortCreateRunLoopSource(kCFAllocatorDefault, 
                                             msg_port_.get(), 0));
  CFRunLoopAddSource(CFRunLoopGetCurrent(), msg_source.get(),
                     kCFRunLoopCommonModes);

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
  if (!pthread_create(&thread_, NULL, ThreadEntry, this))
    return false;

  return (thread_ != NULL);
}

//------------------------------------------------------------------------------
// Abort
//------------------------------------------------------------------------------
void AsyncTask::Abort() {
  CritSecLock locker(lock_);
  if (thread_ && !is_aborted_) {
    LOG(("AsyncTask::Abort\n"));
    is_aborted_ = true;
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
void *AsyncTask::ThreadEntry(void *task) {
  AsyncTask *self = reinterpret_cast<AsyncTask*>(task);
  self->Run();
  {
    CritSecLock locker(self->lock_);
    if (self->delete_when_done_) {
      self->thread_ = NULL;
      // In this particular code path, we have to call unlock prior to delete
      // otherwise the locker would try to access deleted memory, &lock_,
      // after it's been freed.
      locker.Unlock();
      delete self;
    }
  }

  return NULL;
}

//------------------------------------------------------------------------------
// NotifyListener
//------------------------------------------------------------------------------
void AsyncTask::NotifyListener(int code, int param) {
  ThreadMessage msg;
  
  // Post a Mach message to be recieved by the main thread via ReceiveMessage()
  memset(&msg, 0, sizeof(msg));
  msg.header.msgh_size = sizeof(ThreadMessage);
  msg.header.msgh_remote_port = CFMachPortGetPort(msg_port_.get());
  msg.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
                                        MACH_MSG_TYPE_MAKE_SEND_ONCE);
  msg.code = code;
  msg.param = param;
  mach_msg(&(msg.header),
           MACH_SEND_MSG | MACH_SEND_TIMEOUT,
           msg.header.msgh_size, 0, 0, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
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
  if (!http_request)
    return false;

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

  if (!http_request->send())
    return false;

  // Wait for the data to come back from the HTTP request
  while (1) {
    long ready_state;
    http_request->getReadyState(&ready_state);

    if (ready_state == 4) // Completed
      break;

    if (is_aborted_) {
      http_request->abort();
      break;
    }

    // Run for a second in the run loop
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1, true);
  }

  // Extract the status & data
  long status;
  if (http_request->getStatus(&status)) {
    payload->status_code = status;
    if (http_request->getStatusLine(&payload->status_line)) {
      if (http_request->getAllResponseHeaders(&payload->headers)) {
        payload->data.reset(http_request->getResponseBody());
      }
    }
  }

  // If we were redirected during the load, setup the return variables
  if (http_request->wasRedirected()) {
    if (was_redirected)
      *was_redirected = true;

    if (full_redirect_url)
      http_request->getRedirectUrl(full_redirect_url);
  }

  return !is_aborted_ && payload->data.get();
}

//------------------------------------------------------------------------------
// static ReceiveMessage
//------------------------------------------------------------------------------
void AsyncTask::ReceiveMessage(CFMachPortRef port, void *mach_msg, CFIndex size,
                               void *info) {
  ThreadMessage *msg = (ThreadMessage *)mach_msg;
  AsyncTask *self = (AsyncTask *)info;

  if (self->listener_)
    self->listener_->HandleEvent(msg->code, msg->param, self);
}
