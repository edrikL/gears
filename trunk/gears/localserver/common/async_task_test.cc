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

#include "gears/base/common/event.h"
#include "gears/base/common/url_utils.h"
#include "gears/blob/blob_interface.h"
#include "gears/blob/buffer_blob.h"
#include "gears/localserver/common/async_task.h"

const char16* kSetCookiePath = STRING16(L"/testcases/set_cookie.txt");
const char16* kServeIfCookiesPresentPath =
    STRING16(L"/testcases/serve_if_cookies_present.txt");
const char16* kServeIfCookiesAbsentPath =
    STRING16(L"/testcases/serve_if_cookies_absent.txt");

// A simple class used to test AsyncTask::HttpPost().
class AsyncTaskTest : public AsyncTask {
 public:
  AsyncTaskTest() : AsyncTask(NULL), status_code_(0) {
    if (!Init()) {
      assert(false);
    }
  }

  int MakePostRequest(const std::string16 &url, bool send_cookies) {
    url_ = url;
    send_cookies_ = send_cookies;
    if (Start()) {
      status_available_event_.Wait();
    }
    int return_value = status_code_;

    // Note that we can't use a single AsyncTask object for multiple requests
    // because it's not possible to know when the thread has been shutdown and
    // cleaned up once our Run method completes. If the thread has already
    // completed, this call will delete this object.
    DeleteWhenDone();

    return return_value;
  }

 private:
  // Private destructor. Instance deletes itself once MakePostRequest completes.
  ~AsyncTaskTest() {}

  // AsyncTask implementation.
  virtual void Run() {
    WebCacheDB::PayloadInfo payload;
    // TODO(andreip): remove this once WebCacheDB::PayloadInfo.data is a Blob.
    scoped_refptr<BlobInterface> payload_data;
    // The testwebserver can't handle POST requests with empty bodies.
    scoped_refptr<BlobInterface> post_body(new BufferBlob("test\n\r", 6));
    bool result = HttpPost(url_.c_str(),
                           false,            // Not capturing, follow redirects
                           NULL,             // reason_header_value
                           NULL,             // mod_since_date
                           NULL,             // required_cookie
                           !send_cookies_,   // disable_browser_cookies
                           post_body.get(),
                           &payload,
                           &payload_data,
                           NULL,             // was_redirected
                           NULL,             // full_redirect_url
                           NULL);            // error_message
    if (result) {
      status_code_ = payload.status_code;
    }
    status_available_event_.Signal();
  }

  std::string16 url_;
  bool send_cookies_;

  int status_code_;
  Event status_available_event_;

  DISALLOW_EVIL_CONSTRUCTORS(AsyncTaskTest);
};

bool TestAsyncTaskPostCookies(const std::string16 &base_url,
                              std::string16 *error) {
  assert(error);

  std::string16 set_cookie_url;
  std::string16 serve_if_cookies_present_url;
  std::string16 serve_if_cookies_absent_url;
  if (!ResolveAndNormalize(base_url.c_str(), kSetCookiePath, &set_cookie_url) ||
      !ResolveAndNormalize(base_url.c_str(),
                           kServeIfCookiesPresentPath,
                           &serve_if_cookies_present_url) ||
      !ResolveAndNormalize(base_url.c_str(),
                           kServeIfCookiesAbsentPath,
                           &serve_if_cookies_absent_url)) {
    *error += STRING16(L"TestAsyncTaskPostCookies(): Failed to normalize "
                       L"URLs. ");
    return false;
  }


  // Get the cookie from the server.
  AsyncTaskTest *task1 = new AsyncTaskTest();
  if (task1->MakePostRequest(set_cookie_url, true) != 200) {
    *error += STRING16(L"TestAsyncTaskPostCookies(): Did not get code 200 for "
                       L"set_cookie.txt. ");
    return false;
  }

  AsyncTaskTest *task2 = new AsyncTaskTest();
  if (task2->MakePostRequest(serve_if_cookies_present_url, true) != 200) {
    *error += STRING16(L"TestAsyncTaskPostCookies(): Did not get code 200 for "
                       L"serve_if_cookies_present.txt when cookies present. ");
    return false;
  }

  AsyncTaskTest *task3 = new AsyncTaskTest();
  if (task3->MakePostRequest(serve_if_cookies_present_url, false) != 404) {
    *error += STRING16(L"TestAsyncTaskPostCookies(): Did not get code 404 for "
                       L"serve_if_cookies_present.txt when cookies absent. ");
    return false;
  }

  AsyncTaskTest *task4 = new AsyncTaskTest();
  if (task4->MakePostRequest(serve_if_cookies_absent_url, true) != 404) {
    *error += STRING16(L"TestAsyncTaskPostCookies(): Did not get code 404 for "
                       L"serve_if_cookies_absent.txt when cookies present. ");
    return false;
  }

  AsyncTaskTest *task5 = new AsyncTaskTest();
  if (task5->MakePostRequest(serve_if_cookies_absent_url, false) != 200) {
    *error += STRING16(L"TestAsyncTaskPostCookies(): Did not get code 200 for "
                       L"serve_if_cookies_absent.txt when cookies absent. ");
    return false;
  }

  return true;
}

#endif  // USING_CCTESTS
