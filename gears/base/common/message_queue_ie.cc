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
#include <deque>
#include <windows.h>

#include "gears/base/common/message_queue.h"
#include "gears/third_party/linked_ptr/linked_ptr.h"

const DWORD WM_THREAD_MESSAGE = WM_USER + 1;

// Owns the window that has the message queue for a thread, and manages the
// transfer of messages across thread boundries.
class ThreadMessageWindow : public CWindowImpl<ThreadMessageWindow> {
 public:
  BEGIN_MSG_MAP(ThreadMessageWindow)
    MESSAGE_HANDLER(WM_THREAD_MESSAGE , OnThreadMessage)
  END_MSG_MAP()

  ThreadMessageWindow() {
    if (!Create(kMessageOnlyWindowParent,
                NULL,
                NULL,
                kMessageOnlyWindowStyle)) {
      assert(false);
    }
  }
  ~ThreadMessageWindow() {
    if (IsWindow()) {
      DestroyWindow();
    }
    for (std::deque<MessageEvent *>::iterator event = events_.begin();
         event != events_.end(); ++event) {
      delete *event;
    }
  }

  LRESULT OnThreadMessage(UINT msg, WPARAM unused_param_1,
                          LPARAM unused_param_2, BOOL& handled);

  void PostThreadMessage(int message_id, const char16 *message_data_1,
                         const char16 *message_data_2);
 private:
  struct MessageEvent {
    MessageEvent() : message_id(0) {}
    MessageEvent(int id, const char16 *data_1, const char16 *data_2)
      : message_id(id), message_data_1(data_1), message_data_2(data_2) {}

    int message_id;
    std::string16 message_data_1;
    std::string16 message_data_2;
  };

  std::deque<MessageEvent *> events_;
};

// Protects callbacks_ during inserts and finds.
static Mutex callback_mutex_;
static std::map<int, ThreadMessageQueue::HandlerCallback> callbacks_;

// Protectes message_windows_.
static Mutex window_mutex_;
static std::map<ThreadId, linked_ptr<ThreadMessageWindow> > message_windows_;

bool ThreadMessageQueue::InitThreadMessageQueue() {
  MutexLock lock(&window_mutex_);
  ThreadId thread_id = GetCurrentThreadId();
  if (message_windows_.find(thread_id) == message_windows_.end()) {
    message_windows_[thread_id] =
        linked_ptr<ThreadMessageWindow>(new ThreadMessageWindow);
  }
  return true;
}

// This is only called in gears/base/ie/module.cc on thread detatch.
void ShutdownThreadMessageQueue() {
  MutexLock lock(&window_mutex_);
  message_windows_.erase(ThreadMessageQueue::GetCurrentThreadId());
}

bool ThreadMessageQueue::RegisterHandler(int message_id,
                                         HandlerCallback message_handler) {
  MutexLock lock(&callback_mutex_);
  callbacks_[message_id] = message_handler;
  return true;
}

ThreadId ThreadMessageQueue::GetCurrentThreadId() {
  return ::GetCurrentThreadId();
}

bool ThreadMessageQueue::Send(ThreadId thread,
                              int message_id,
                              const char16 *message_data_1,
                              const char16 *message_data_2) {
  MutexLock lock(&window_mutex_);
  std::map<ThreadId, linked_ptr<ThreadMessageWindow> >::iterator window;

  window = message_windows_.find(thread);

  if (window == message_windows_.end()) {
    return false;
  }

  window->second->PostThreadMessage(message_id, message_data_1, message_data_2);
  return true;
}

inline ThreadMessageQueue::HandlerCallback GetRegisteredHandler(int id) {
  MutexLock lock(&callback_mutex_);
  std::map<int, ThreadMessageQueue::HandlerCallback>::iterator handler_loc;
  handler_loc = callbacks_.find(id);
  if (handler_loc != callbacks_.end()) {
    return handler_loc->second;
  } else {
    return NULL;
  }
}

LRESULT ThreadMessageWindow::OnThreadMessage(UINT msg, WPARAM unused_param_1,
                                             LPARAM unused_param_2,
                                             BOOL& handled) {
  assert(msg == WM_THREAD_MESSAGE);

  MessageEvent *event;
  {
    MutexLock lock(&window_mutex_);
    assert(events_.size() > 0);
    event = events_.front();
    events_.pop_front();
  }

  ThreadMessageQueue::HandlerCallback handler =
                          GetRegisteredHandler(event->message_id);
  if (handler) {
    handler(event->message_id, event->message_data_1.c_str(),
            event->message_data_2.c_str());
  }
  handled = TRUE;
  return 0;
}

void ThreadMessageWindow::PostThreadMessage(int message_id,
                                            const char16 *message_data_1,
                                            const char16 *message_data_2) {
  events_.push_back(new MessageEvent(message_id, message_data_1,
                                     message_data_2));
  PostMessage(WM_THREAD_MESSAGE, 0, 0);
}
