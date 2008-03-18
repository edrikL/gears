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
#include <deque>
#include <map>
#include <windows.h>

#include "gears/base/common/message_queue.h"
#include "gears/third_party/linked_ptr/linked_ptr.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

class ThreadMessageWindow;

// A concrete implementation that uses HWNDs and PostMessage
class IEThreadMessageQueue : public ThreadMessageQueue {
 public:
  virtual bool InitThreadMessageQueue();
  virtual ThreadId GetCurrentThreadId();
  virtual bool Send(ThreadId thread_handle,
                    int message_type,
                    MessageData *message);
 private:
  friend class ThreadMessageWindow;
  void HandleThreadMessage(int message_type, MessageData *message_data);
};

// Owns the window that has the message queue for a thread, and manages the
// transfer of messages across thread boundries.
class ThreadMessageWindow : public CWindowImpl<ThreadMessageWindow> {
 public:
  static const DWORD WM_THREAD_MESSAGE = WM_USER + 1;
  BEGIN_MSG_MAP(ThreadMessageWindow)
    MESSAGE_HANDLER(WM_THREAD_MESSAGE , OnThreadMessage)
  END_MSG_MAP()

  ThreadMessageWindow() {
    if (!Create(kMessageOnlyWindowParent,
                NULL, NULL,
                kMessageOnlyWindowStyle)) {
      assert(false);
    }
  }

  ~ThreadMessageWindow() {
    if (IsWindow()) {
      DestroyWindow();
    }
  }

  LRESULT OnThreadMessage(UINT msg, WPARAM unused_wparam,
                          LPARAM unused_lparam, BOOL &handled);
  void PostThreadMessage(int message_tyoe, MessageData *message_data);
 private:
  struct MessageEvent {
    MessageEvent() {}
    MessageEvent(int message_type, MessageData *message_data)
        : message_type(message_type), message_data(message_data) {}
    int message_type;
    linked_ptr<MessageData> message_data;
  };

  std::deque<MessageEvent> events_;
};

static Mutex window_mutex_;  // Protects the message_windows_ collection
static std::map<ThreadId, linked_ptr<ThreadMessageWindow> > *message_windows_;
static IEThreadMessageQueue g_instance;

// static
ThreadMessageQueue *ThreadMessageQueue::GetInstance() {
  return &g_instance;
}

bool IEThreadMessageQueue::InitThreadMessageQueue() {
  MutexLock lock(&window_mutex_);
  if (!message_windows_) {
    message_windows_ = new std::map<ThreadId,
                                    linked_ptr<ThreadMessageWindow> >;
  }
  ThreadId thread_id = GetCurrentThreadId();
  if (message_windows_->find(thread_id) == message_windows_->end()) {
    (*message_windows_)[thread_id] =
        linked_ptr<ThreadMessageWindow>(new ThreadMessageWindow);
  }
  return true;
}

// This is only called in gears/base/ie/module.cc on thread detatch.
void ShutdownThreadMessageQueue() {
  MutexLock lock(&window_mutex_);
  if (message_windows_)
    message_windows_->erase(g_instance.GetCurrentThreadId());
}

ThreadId IEThreadMessageQueue::GetCurrentThreadId() {
  return ::GetCurrentThreadId();
}

bool IEThreadMessageQueue::Send(ThreadId thread,
                                int message_type,
                                MessageData *message_data) {
  MutexLock lock(&window_mutex_);
  if (!message_windows_) {
    delete message_data;
    return false;
  }
  std::map<ThreadId, linked_ptr<ThreadMessageWindow> >::iterator window;
  window = message_windows_->find(thread);
  if (window == message_windows_->end()) {
    delete message_data;
    return false;
  }
  window->second->PostThreadMessage(message_type, message_data);
  return true;
}

void IEThreadMessageQueue::HandleThreadMessage(int message_type,
                                               MessageData *message_data) {
  RegisteredHandler handler;
  if (GetRegisteredHandler(message_type, &handler)) {
    handler.Invoke(message_type, message_data);
  }
}

LRESULT ThreadMessageWindow::OnThreadMessage(UINT msg, WPARAM unused_wparam,
                                             LPARAM unused_lparam,
                                             BOOL &handled) {
  assert(msg == WM_THREAD_MESSAGE);
  MessageEvent event;
  {
    MutexLock lock(&window_mutex_);
    assert(events_.size() > 0);
    event = events_.front();
    events_.pop_front();
  }
  g_instance.HandleThreadMessage(event.message_type, event.message_data.get());
  handled = TRUE;
  return 0;
}

void ThreadMessageWindow::PostThreadMessage(int message_type,
                                            MessageData *message_data) {
  events_.push_back(MessageEvent(message_type, message_data));
  PostMessage(WM_THREAD_MESSAGE, 0, 0);
}
