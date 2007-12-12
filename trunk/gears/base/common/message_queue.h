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
#include <set>
#include <vector>
#include "gears/base/common/mutex.h"


// TODO(michaeln): These should be defined by a threading abstraction
// layer.
#if BROWSER_FF
typedef PRThread (*ThreadId);
#elif BROWSER_IE
typedef DWORD ThreadId;
#endif


// A facility for sending and receiving messages asynchrously 
// across thread boundaries. 
// TODO(michaeln): would be real nice to handle message objects instead
// the arbitrary two data strings currently supported.
class ThreadMessageQueue {
 public:
  // Returns a pointer to the singleton
  static ThreadMessageQueue *GetInstance();

  // An interface for use when registering an instance as a message handler.
  class HandlerInterface {
   public:
    virtual void HandleThreadMessage(int message_id,
                                     const char16 *message_data_1,
                                     const char16 *message_data_2) = 0;
  };

  // A function definition for use when registering a static function as
  // a message handler.
  typedef void (*HandlerStaticCallback)(int message_id,
                                        const char16 *message_data_1,
                                        const char16 *message_data_2);

  // Registers an instance as a handler callback. There is
  // no means to unregister a handler, so this should only be used
  // for singleton instances if handler class.
  void RegisterHandler(int message_id,
                       HandlerInterface *handler_instance);

  // Registers a static method as a handler callback.
  void RegisterStaticHandler(int message_id,
                             HandlerStaticCallback handler_callback);

  // Initializes the message queue for the current thread. If the
  // queue is already initialized, this is a no-op. Returns true
  // if the queue is successfully initialized.
  virtual bool InitThreadMessageQueue() = 0;

  // Returns the thread id of the currently executing thread.
  virtual ThreadId GetCurrentThreadId() = 0;

  // Posts a message to the indicated thread.  Returns true if the
  // message is successfully queued for delivery.
  virtual bool Send(ThreadId thread_handle,
                    int message_id,
                    const char16 *message_data_1,
                    const char16 *message_data_2) = 0;
 protected:
  ThreadMessageQueue() {}
  virtual ~ThreadMessageQueue() {}

  struct RegisteredHandler {
    RegisteredHandler()
      : instance(NULL), static_callback(NULL) {}
    RegisteredHandler(HandlerInterface *inst)
      : instance(inst), static_callback(NULL) {}
    RegisteredHandler(HandlerStaticCallback callback)
      : instance(NULL), static_callback(callback) {}

    HandlerInterface *instance;
    HandlerStaticCallback static_callback;

    void Invoke(int message_id,
                const char16 *message_data_1,
                const char16 *message_data_2) {
      if (instance)
        instance->HandleThreadMessage(message_id,
                                      message_data_1, message_data_2);
      else if (static_callback)
        static_callback(message_id, message_data_1, message_data_2);
    }
  };

  bool GetRegisteredHandler(int message_id, RegisteredHandler *handler);

  // Protects handlers_ during inserts and finds.
  Mutex handler_mutex_;
  std::map<int, RegisteredHandler> handlers_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ThreadMessageQueue);
};


#ifdef DEBUG
// A mock implementation of the ThreadMessageQueue that
// can be used for testing.
class MockThreadMessageQueue : public ThreadMessageQueue {
 public:
  MockThreadMessageQueue() : current_thread_id_(0) {}

  virtual bool InitThreadMessageQueue() {
    initialized_threads_.insert(current_thread_id_);
    return true;
  }

  virtual ThreadId GetCurrentThreadId() {
    return current_thread_id_;
  }

  virtual bool Send(ThreadId thread_handle,
                    int message_id,
                    const char16 *message_data_1,
                    const char16 *message_data_2);

  void SetMockCurrentThreadId(ThreadId thread_id) {
    current_thread_id_ = thread_id;
  }

  // Delivers messages that have been sent via the Send method
  // since the last time DeliverMockMessages was called.
  void DeliverMockMessages();

 private:
  ThreadId current_thread_id_;
  std::set<ThreadId> initialized_threads_;
  std::vector<ThreadId> pending_message_thread_ids_;
  std::vector<int> pending_message_ids_;
  std::vector<std::string16> pending_message_topics_;
  std::vector<std::string16> pending_message_data_;

  DISALLOW_EVIL_CONSTRUCTORS(MockThreadMessageQueue);
};
#endif  // DEBUG

#endif  // GEARS_BASE_COMMON_MESSAGE_QUEUE_H__
