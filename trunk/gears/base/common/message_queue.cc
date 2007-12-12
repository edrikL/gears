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
#include <map>
#include <vector>

#include "gears/base/common/message_queue.h"


// The subset of the actual ThreadMessageQueue implementation
// common to all platforms, handler registration.

void ThreadMessageQueue::RegisterHandler(int message_id,
                                         HandlerInterface *instance) {
  MutexLock lock(&handler_mutex_);
  handlers_[message_id] = RegisteredHandler(instance);
}

void ThreadMessageQueue::RegisterStaticHandler(
                             int message_id,
                             HandlerStaticCallback callback) {
  MutexLock lock(&handler_mutex_);
  handlers_[message_id] = RegisteredHandler(callback);
}

bool ThreadMessageQueue::GetRegisteredHandler(int message_id,
                                              RegisteredHandler *handler) {
  MutexLock lock(&handler_mutex_);
  std::map<int, RegisteredHandler>::iterator handler_loc;
  handler_loc = handlers_.find(message_id);
  if (handler_loc == handlers_.end())
    return false;
  *handler = handler_loc->second;
  return true;
}


#ifdef DEBUG
// The MockThreadMessageQueue implementation

bool MockThreadMessageQueue::Send(ThreadId thread_id,
                                  int message_id,
                                  const char16 *message_data_1,
                                  const char16 *message_data_2) {
  if (initialized_threads_.find(thread_id) == 
      initialized_threads_.end())
    return false;
  pending_message_thread_ids_.push_back(thread_id);
  pending_message_ids_.push_back(message_id);
  pending_message_topics_.push_back(std::string16(message_data_1));
  pending_message_data_.push_back(std::string16(message_data_2));
  return true;
}

void MockThreadMessageQueue::DeliverMockMessages() {
  size_t count = pending_message_thread_ids_.size();
  assert(count == pending_message_ids_.size());
  assert(count == pending_message_topics_.size());
  assert(count == pending_message_data_.size());
  
  for (size_t i = 0; i < count; ++i) {
    ThreadId thread_id = pending_message_thread_ids_[i];
    int message_id = pending_message_ids_[i];
    const char16 *topic = pending_message_topics_[i].c_str();
    const char16 *data = pending_message_data_[i].c_str();

    SetMockCurrentThreadId(pending_message_thread_ids_[i]);
    RegisteredHandler handler;
    if (GetRegisteredHandler(message_id, &handler)) {
      handler.Invoke(message_id, topic, data);
    }
  }

  pending_message_thread_ids_.clear();
  pending_message_ids_.clear();
  pending_message_topics_.clear();
  pending_message_data_.clear();
}

#endif  // DEBUG
