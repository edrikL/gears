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

#ifndef GEARS_BASE_COMMON_MESSAGE_QUEUE_H__
#define GEARS_BASE_COMMON_MESSAGE_QUEUE_H__

#include <map>
#include "gears/base/common/mutex.h"

#if BROWSER_FF
typedef PRThread (*ThreadId);
#elif BROWSER_IE
#endif

class ThreadMessageQueue {
 public:
  // Handler callback used to process messages in the destination thread.
  // TODO(michaeln): would be real nice to handle 'serializable' messages here
  typedef void (*HandlerCallback)(int message_id,
                                  const char *message_data_1,
                                  const char *message_data_2);

  // Initializes the message queue for the current thread. If the
  // queue is already initialized, this is a no-op. Returns true
  // if the queue is initialized.
  static bool InitThreadMessageQueue();

  // Registers the handler callback
  static bool RegisterHandler(int message_id, HandlerCallback message_handler);

  static ThreadId GetCurrentThreadId();

  // Posts a message to the indicated thread.
  static void Send(ThreadId thread_handle,
                   int message_id,
                   const char *message_data_1,
                   const char *message_data_2);
 private:
  // This class only has static functions, so the constructor is declared
  // private to prohibit instantiation.
  ThreadMessageQueue() {};
};
#endif  // GEARS_BASE_COMMON_MESSAGE_QUEUE_H__
