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
#endif

Thread::Thread() {
#ifdef DEBUG
  is_running_ = false;
#endif
}

Thread::~Thread() {
#ifdef DEBUG
  MutexLock lock(&is_running_mutex_);
  assert(!is_running_);
#endif
}

void Thread::Start() {
#ifdef DEBUG
  MutexLock lock(&is_running_mutex_);
  assert(!is_running_);
#endif
#if defined(WIN32) || defined(WINCE)
  if (_beginthreadex(NULL, 0, &ThreadMain, this, 0, NULL) != NULL) {
#elif defined(LINUX) || defined(OS_MACOSX) || defined(OS_ANDROID)
  pthread_t thread;
  if (pthread_create(&thread, NULL, ThreadMain, this) == 0 && thread != 0) {
#endif
#ifdef DEBUG
    is_running_ = true;
#endif
  } else {
    LOG(("Failed to start thread."));
  }
}

void Thread::Join() {
  run_complete_event_.Wait();
}

// static
#if defined(WIN32) || defined(WINCE)
unsigned int __stdcall Thread::ThreadMain(void *user_data) {
#elif defined(LINUX) || defined(OS_MACOSX) || defined(OS_ANDROID)
void *Thread::ThreadMain(void *user_data) {
#endif
  Thread *self = reinterpret_cast<Thread*>(user_data);
  self->Run();
#ifdef DEBUG
  self->is_running_mutex_.Lock();
  self->is_running_ = false;
#endif
  self->run_complete_event_.Signal();
#ifdef DEBUG
  self->is_running_mutex_.Unlock();
#endif
  self->CleanUp();
  return 0;
}
