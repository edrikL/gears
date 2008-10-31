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

#include "gears/base/common/async_router.h"

#include <assert.h>

// static
AsyncRouter *AsyncRouter::GetInstance() {
  static AsyncRouter *instance = new AsyncRouter();
  return instance;
}

AsyncRouter::AsyncRouter() {
  ThreadMessageQueue::GetInstance()->RegisterHandler(kAsyncRouter_Call, this);
  ThreadMessageQueue::GetInstance()->RegisterHandler(kSyncRouter_Call, this);
}

AsyncRouter::~AsyncRouter() {
}

bool AsyncRouter::CallAsync(ThreadId thread_id, AsyncFunctor *functor) {
  return ThreadMessageQueue::GetInstance()->Send(
      thread_id, kAsyncRouter_Call, functor);
}

bool AsyncRouter::CallSync(ThreadId thread_id, SyncFunctor *functor) {
  Event handled_event;

  functor->SetHandledEvent(&handled_event);
  if (!ThreadMessageQueue::GetInstance()->Send(
      thread_id, kSyncRouter_Call, functor)) {
    return false;
  }

  handled_event.Wait();

  return true;
}

void AsyncRouter::HandleThreadMessage(int message_type,
                                      MessageData *message_data) {
  assert(message_type == kAsyncRouter_Call || message_type == kSyncRouter_Call);
  if (message_type == kAsyncRouter_Call) {
    AsyncFunctor *functor = static_cast<AsyncFunctor*>(message_data);
    functor->Run();
  } else if (message_type == kSyncRouter_Call) {
    SyncFunctor *functor = static_cast<SyncFunctor*>(message_data);
    functor->Run();
    functor->SignalHandledEvent();
  }
  // functor is deleted by the ThreadMessageQueue when this returns
}
