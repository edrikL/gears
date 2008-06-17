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

#include "gears/geolocation/thread.h"

#include <assert.h>
#if defined(WIN32) && !defined(WINCE)
#include <atlsync.h>  // For _beginthreadex()
#elif defined(WINCE)
#include "gears/base/common/wince_compatibility.h"
#elif defined(OS_ANDROID)
#include "gears/base/android/java_jni.h"
#endif

struct ThreadStartData {
  explicit ThreadStartData(Thread* thread_self)
      : thread_id(0),
        self(thread_self) {
  }
  Event started_event;
  ThreadId thread_id;
  Thread *self;
};

// Helper method that does the thread creation.
static bool CreateThread(ThreadStartData* thread_data);

//------------------------------------------------------------------------------
// Thread

Thread::Thread()
  : run_complete_event_(),
    is_running_(false) {
}

Thread::~Thread() {
  assert(!is_running_);
}

ThreadId Thread::Start() {
  assert(!is_running_);
  ThreadStartData thread_data(this);

  if (CreateThread(&thread_data)) {
    thread_data.started_event.Wait();
  } else {
    LOG(("Failed to start thread."));
  }

  return thread_data.thread_id;
}

void Thread::Join() {
  run_complete_event_.Wait();
}

//------------------------------------------------------------------------------
// Internal

void ThreadMain(void *user_data) {
  ThreadStartData *thread_data = reinterpret_cast<ThreadStartData*>(user_data);

  // Initialize the message queue.
  ThreadMessageQueue* queue = ThreadMessageQueue::GetInstance();
  queue->InitThreadMessageQueue();
  // Get this thread's id.
  thread_data->thread_id = queue->GetCurrentThreadId();
  // Let our creator know that we started ok.

  // WARINING: thread_data must not be used after
  // the start_event is signaled. 
  // The Thread pointer, on the other hand, is guaranteed
  // to be valid for the entire lifetime of this function.
  Thread* thread = thread_data->self;
  thread->is_running_ = true;
  thread_data->started_event.Signal();
  // Do the actual work.
  thread->Run();
  thread->is_running_ = false;
  // Let our creator know we're about to exit.
  thread->run_complete_event_.Signal();
  thread->CleanUp();
}

#if defined(WIN32) || defined(WINCE)
static unsigned int __stdcall OSThreadMain(void *user_data) {
#elif defined(LINUX) || defined(OS_MACOSX) || defined(OS_ANDROID)
static void *OSThreadMain(void *user_data) {
#endif

#ifdef OS_ANDROID
  JniAttachCurrentThread();
#endif

  ThreadMain(user_data);

#ifdef OS_ANDROID
  JniDetachCurrentThread();
#endif

  return 0;
}

static bool CreateThread(ThreadStartData* thread_data) {
#if defined(WIN32) || defined(WINCE)
  return (_beginthreadex(NULL, 0, &OSThreadMain, thread_data, 0, NULL) != NULL);
#elif defined(LINUX) || defined(OS_MACOSX) || defined(OS_ANDROID)
  pthread_t thread_id;
  return (pthread_create(&thread_id, NULL, &OSThreadMain, thread_data) == 0 &&
          thread_id != 0);
#endif
}
