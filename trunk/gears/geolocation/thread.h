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

#ifndef GEARS_GEOLOCATION_THREAD_H__
#define GEARS_GEOLOCATION_THREAD_H__

#include "gears/base/common/event.h"
#include "gears/base/common/message_queue.h"
#include "gears/base/common/mutex.h"

// This class abstracts creating a new thread. Derived classes implement the Run
// method, which is run in a new thread when Start is called. Derived classes
// may also override the CleanUp method, which is called in the new thread after
// Run completes and can be used to delete the object.
class Thread {
 public:
  Thread();
  virtual ~Thread();

  // Creates a thread and invokes this->Run() as its body.
  ThreadId Start();

  // Waits for the Run method to complete. Note that the thread may execute code
  // in CleanUp() after this method has returned.
  virtual void Join();

 private:
  // This method is called in the new thread.
  virtual void Run() = 0;

  // This method is called in the new thread after Run has completed. No member
  // objects are touched after this method has been called, so it it safe, for
  // instance, to delete the object from this method. Default implementation
  // does nothing.
  virtual void CleanUp() {}

  Event run_complete_event_;

#ifdef DEBUG
  bool is_running_;
  Mutex is_running_mutex_;
#endif

  // Initializes the message queue for this thread and calls Run().
  friend void ThreadMain(void *user_data);
};

#endif  // GEARS_GEOLOCATION_THREAD_H__
