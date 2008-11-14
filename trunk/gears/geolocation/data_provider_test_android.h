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

#ifdef OS_ANDROID
#if USING_CCTESTS

#ifndef GEARS_GEOLOCATION_DATA_PROVIDER_TEST_ANDROID_H__
#define GEARS_GEOLOCATION_DATA_PROVIDER_TEST_ANDROID_H__

#include "gears/base/common/async_router.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/thread.h"

class JsRunnerInterface;
class JsRootedToken;
typedef JsRootedToken JsRootedCallback;

template <class DataProvider>
class AndroidDataProviderTest
    : public DataProvider::ListenerInterface {
 public:
  static AndroidDataProviderTest* Create(JsRunnerInterface *js_runner,
                                         JsRootedCallback *callback) {
    return new AndroidDataProviderTest(js_runner, callback);
  }

  // DataProvider::ListenerInterface
  virtual void DeviceDataUpdateAvailable(DataProvider *provider) {
    assert(callback_functor_);
    assert(provider == provider_);
    AsyncRouter::GetInstance()->CallAsync(javascript_thread_id_,
                                          callback_functor_);
  }

 private:
  // Callback functor used to complete the test.
  class TestCallbackFunctor : public AsyncFunctor {
   public:
    TestCallbackFunctor(JsRunnerInterface *js_runner,
                        JsRootedCallback *callback,
                        AndroidDataProviderTest *test_object)
        : js_runner_(js_runner),
          callback_(callback),
          test_object_(test_object) {
    }

    // AsyncFunctor
    virtual void Run() {
      assert(js_runner_);
      assert(callback_.get());
      // We must unregister here. Unregistering in DeviceDataUpdateAvailable
      // would recursively lock the RadioDataProvider::listener_mutex_.
      bool result = DataProvider::Unregister(test_object_);
      assert(result);
      js_runner_->InvokeCallback(callback_.get(), 0, NULL, NULL);
      delete test_object_;
    }

   private:
    JsRunnerInterface *js_runner_;
    scoped_ptr<JsRootedCallback> callback_;
    AndroidDataProviderTest *test_object_;

    DISALLOW_EVIL_CONSTRUCTORS(TestCallbackFunctor);
  };

  AndroidDataProviderTest(JsRunnerInterface *js_runner,
                          JsRootedCallback *callback)
      : callback_functor_(NULL) {
    ThreadMessageQueue::GetInstance()->InitThreadMessageQueue();
    javascript_thread_id_ =
        ThreadMessageQueue::GetInstance()->GetCurrentThreadId();
    callback_functor_ = new TestCallbackFunctor(js_runner, callback, this);
    provider_ = DataProvider::Register(this);
  }

  virtual ~AndroidDataProviderTest() {}
  DataProvider *provider_;
  TestCallbackFunctor *callback_functor_;
  ThreadId javascript_thread_id_;

  friend class TestCallbackFunctor;

  DISALLOW_EVIL_CONSTRUCTORS(AndroidDataProviderTest);
};

#endif  // GEARS_GEOLOCATION_DATA_PROVIDER_TEST_ANDROID_H__

#endif  // USING_CCTESTS
#endif  // OS_ANDROID
