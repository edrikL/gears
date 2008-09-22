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

#ifndef GEARS_LOCALSERVER_COMMON_ASYNC_TASK_TEST_H__
#define GEARS_LOCALSERVER_COMMON_ASYNC_TASK_TEST_H__

#if USING_CCTESTS

class JsRootedToken;
typedef JsRootedToken JsRootedCallback;
class JsRunnerInterface;

#include "gears/localserver/common/async_task.h"
#include "gears/base/common/thread.h"  // For ThreadId

class JsRootedToken;
typedef JsRootedToken JsRootedCallback;
class JsRunnerInterface;
class CallbackFunctor;

class AsyncTaskTest : public AsyncTask {
 public:
  AsyncTaskTest(const std::string16 &url,
                bool send_cookies,
                JsRunnerInterface *js_runner,
                JsRootedCallback *callback);

  bool MakePostRequest();

 private:
  // Private destructor. Instance deletes itself.
  ~AsyncTaskTest() {}

  // AsyncTask implementation.
  virtual void Run();

  std::string16 url_;
  bool send_cookies_;

  CallbackFunctor *callback_functor_;
  ThreadId javascript_thread_id_;

  DISALLOW_EVIL_CONSTRUCTORS(AsyncTaskTest);
};

#endif  // USING_CCTESTS

#endif  // GEARS_LOCALSERVER_COMMON_ASYNC_TASK_TEST_H__
