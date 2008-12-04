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

#if USING_CCTESTS

#include "gears/localserver/common/async_task_test.h"

#include "gears/base/common/async_router.h"
#include "gears/base/common/js_types.h"  // For JsRootedCallback
#include "gears/base/common/js_runner.h"
#include "gears/blob/blob_interface.h"
#include "gears/blob/buffer_blob.h"

// Functor for use with AsyncRouter.
class CallbackFunctor : public AsyncFunctor {
 public:
  CallbackFunctor(JsRunnerInterface *js_runner, JsRootedCallback* callback)
    : js_runner_(js_runner),
      callback_(callback),
      status_code_(-1) {
  }

  void SetStatusCode(int status_code) {
    status_code_ = status_code;
  }

  // AsyncFunctor implementation
  virtual void Run() {
    assert(js_runner_);
    assert(callback_.get());
    assert(status_code_ > -1);
    JsParamToSend argv[] = { { JSPARAM_INT, &status_code_ } };
    js_runner_->InvokeCallback(
        callback_.get(), NULL, ARRAYSIZE(argv), argv, NULL);
  }

 private:
  JsRunnerInterface *js_runner_;
  scoped_ptr<JsRootedCallback> callback_;
  int status_code_;

  DISALLOW_EVIL_CONSTRUCTORS(CallbackFunctor);
};


AsyncTaskTest::AsyncTaskTest(const std::string16 &url,
                             bool send_cookies,
                             JsRunnerInterface *js_runner,
                             JsRootedCallback *callback)
    : AsyncTask(NULL),
      url_(url),
      send_cookies_(send_cookies) {
  ThreadMessageQueue::GetInstance()->InitThreadMessageQueue();
  javascript_thread_id_ =
      ThreadMessageQueue::GetInstance()->GetCurrentThreadId();
  callback_functor_ = new CallbackFunctor(js_runner, callback);
}

bool AsyncTaskTest::MakePostRequest() {
  if (!Init() || !Start()) {
    delete callback_functor_;
    DeleteWhenDone();
    return false;
  }
  return true;
}


// AsyncTask implementation.
void AsyncTaskTest::Run() {
  WebCacheDB::PayloadInfo payload;
  // TODO(andreip): remove this once WebCacheDB::PayloadInfo.data is a Blob.
  scoped_refptr<BlobInterface> payload_data;
  // The testwebserver can't handle POST requests with empty bodies.
  scoped_refptr<BlobInterface> post_body(new BufferBlob("test\n\r", 6));
  bool result = HttpPost(url_.c_str(),
                         false,            // Not capturing, follow redirects
                         NULL,             // reason_header_value
                         NULL,             // content_type_header_value
                         NULL,             // mod_since_date
                         NULL,             // required_cookie
                         !send_cookies_,   // disable_browser_cookies
                         post_body.get(),
                         &payload,
                         &payload_data,
                         NULL,             // was_redirected
                         NULL,             // full_redirect_url
                         NULL);            // error_message

  // Marshall to JavaScript thread to make the callback. We pass ownership of
  // callback_functor_ to the AsyncRouter.
  callback_functor_->SetStatusCode(result ? payload.status_code : 0);
  AsyncRouter::GetInstance()->CallAsync(javascript_thread_id_,
      callback_functor_);
  DeleteWhenDone();
}

#endif  // USING_CCTESTS
