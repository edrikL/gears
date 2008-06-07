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

#ifdef OS_ANDROID

#include <assert.h>
#include <deque>
#include <map>
#include <unistd.h>

#include "gears/base/common/message_queue.h"

#include "gears/base/android/java_jni.h"
#include "gears/base/common/event.h"
#include "gears/base/common/thread_locals.h"
#include "gears/base/npapi/module.h"
#include "gears/geolocation/thread.h"
#include "third_party/linked_ptr/linked_ptr.h"
#include "third_party/npapi/nphostapi.h"

class ThreadSpecificQueue;

// A concrete (and naive) implementation that uses Event.
class AndroidThreadMessageQueue : public ThreadMessageQueue {
 public:
  AndroidThreadMessageQueue();
  ThreadId MainThreadId();

  // ThreadMessageQueue
  virtual bool InitThreadMessageQueue();

  virtual ThreadId GetCurrentThreadId();

  virtual bool Send(ThreadId thread_handle,
                    int message_type,
                    MessageData *message);

  void HandleThreadMessage(int message_type, MessageData *message_data);

 private:

  void InitThreadEndHook();
  static void ThreadEndHook(void* value);

  static Mutex queue_mutex_;  // Protects the message_queues_ map.
  typedef std::map<ThreadId, linked_ptr<ThreadSpecificQueue> > QueueMap;
  static QueueMap message_queues_;
  ThreadId  main_thread_id_;

  friend class ThreadSpecificQueue;
  friend class AndroidMessageLoop;
};

// A message queue for a thread.
class ThreadSpecificQueue {
 public:
  // Sends the message to the thread owning this queue.
  virtual void SendMessage(int message_type, MessageData* message_data) = 0;

  virtual ~ThreadSpecificQueue() {}
 protected:
  ThreadSpecificQueue() {}
  Mutex* GetAndroidMessageQueueMutex() {
    return &AndroidThreadMessageQueue::queue_mutex_;
  }

  struct Message {
    Message(int type, MessageData* data)
        : message_type(type),
          message_data(data) {
    }
    int message_type;
    linked_ptr<MessageData> message_data;
  };

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ThreadSpecificQueue);
};

class BackgroundThreadQueue : public ThreadSpecificQueue {
 public:
  BackgroundThreadQueue() {}
  virtual ~BackgroundThreadQueue() {}
  // Waits for a message to arrive and dispatches them to the
  // appropriate handlers.
  void GetAndDispatchMessages(int exit_type);

  // ThreadSpecificQueue
  virtual void SendMessage(int message_type, MessageData* message_data);

 private:
  std::deque<Message> event_queue_;
  Event event_;
};

class MainThreadQueue : public ThreadSpecificQueue {
 public:
  static void Dispatch(void* instance);
  MainThreadQueue() {}
  virtual ~MainThreadQueue() {}

  // MessageQueue
  virtual void SendMessage(int message_type, MessageData* message_data);
};

// The thread message queue singleton.
static AndroidThreadMessageQueue g_instance;


//------------------------------------------------------------------------------
// AndroidThreadMessageQueue

Mutex AndroidThreadMessageQueue::queue_mutex_;
AndroidThreadMessageQueue::QueueMap AndroidThreadMessageQueue::message_queues_;

AndroidThreadMessageQueue::AndroidThreadMessageQueue() {
  main_thread_id_ = GetCurrentThreadId();
}

ThreadId AndroidThreadMessageQueue::MainThreadId() {
  return main_thread_id_;
}

// static
ThreadMessageQueue *ThreadMessageQueue::GetInstance() {
  return &g_instance;
}

bool AndroidThreadMessageQueue::InitThreadMessageQueue() {
  MutexLock lock(&queue_mutex_);
  ThreadId thread_id = GetCurrentThreadId();
  if (message_queues_.find(thread_id) == message_queues_.end()) {
    ThreadSpecificQueue* queue = NULL;
    if (thread_id == MainThreadId()) {
      // We are initializing the main thread.
      queue = new MainThreadQueue();
    } else {
      // We are initializing a worker.
      queue = new BackgroundThreadQueue();
      InitThreadEndHook();
    }
    message_queues_[thread_id] = linked_ptr<ThreadSpecificQueue>(queue);
  }
  return true;
}

ThreadId AndroidThreadMessageQueue::GetCurrentThreadId() {
  return pthread_self();
}

bool AndroidThreadMessageQueue::Send(ThreadId thread,
                                     int message_type,
                                     MessageData *message_data) {
  MutexLock lock(&queue_mutex_);

  // Find the queue for the target thread.
  AndroidThreadMessageQueue::QueueMap::iterator queue;
  queue = message_queues_.find(thread);
  if (queue == message_queues_.end()) {
    delete message_data;
    return false;
  }
  queue->second->SendMessage(message_type, message_data);
  return true;
}

void AndroidThreadMessageQueue::HandleThreadMessage(int message_type,
                                                    MessageData *message_data) {
  RegisteredHandler handler;
  if (GetRegisteredHandler(message_type, &handler)) {
    handler.Invoke(message_type, message_data);
  }
}


void AndroidThreadMessageQueue::InitThreadEndHook() {
  // We use a ThreadLocal to get called when an OS thread terminates
  // and use that opportunity to remove all observers that remain
  // registered on that thread.
  //
  // We store the thread id in the ThreadLocal variable because on some
  // OSes (linux), the destructor proc is called from a different thread,
  // and on others (windows), the destructor proc is called from the
  // terminating thread.
  //
  // Also, we only do this for the actual singleton instance of the
  // MessageService class as opposed to instances created for unit testing.
  const std::string kThreadLocalKey("base:ThreadMessageQueue.ThreadEndHook");
  if (!ThreadLocals::HasValue(kThreadLocalKey)) {
    ThreadId *id = new ThreadId(GetCurrentThreadId());
    ThreadLocals::SetValue(kThreadLocalKey, id, &ThreadEndHook);
  }
}

// static
void AndroidThreadMessageQueue::ThreadEndHook(void* value) {
  ThreadId *id = reinterpret_cast<ThreadId*>(value);
  if (id) {
    MutexLock lock(&queue_mutex_);
    message_queues_.erase(*id);
    delete id;
  }
}

//------------------------------------------------------------------------------
// AndroidMessageLoop

// static
void AndroidMessageLoop::Start() {
  // TODO(andreip): Fix this for timer API.
  BackgroundThreadQueue* queue = NULL;
  {
    MutexLock lock(&AndroidThreadMessageQueue::queue_mutex_);
    // Find the queue for this thread.
    AndroidThreadMessageQueue::QueueMap::iterator queue_iter;
    queue_iter = AndroidThreadMessageQueue::message_queues_.find(
        ThreadMessageQueue::GetInstance()->GetCurrentThreadId());
    assert (queue_iter != AndroidThreadMessageQueue::message_queues_.end());
    queue = static_cast<BackgroundThreadQueue*>(queue_iter->second.get());
  }
  assert(queue);
  // Start the loop.
  queue->GetAndDispatchMessages(kAndroidLoop_Exit);
}

//static
void AndroidMessageLoop::Stop(ThreadId thread_id) {
  ThreadMessageQueue::GetInstance()->Send(thread_id, kAndroidLoop_Exit, NULL);
}

//------------------------------------------------------------------------------
// BackgroundThreadQueue

void BackgroundThreadQueue::SendMessage(int message_type,
                                        MessageData* message_data) {
  // Put a message in the queue. Note that the Message object
  // takes ownership of message_data.
  event_queue_.push_back(Message(message_type, message_data));
  event_.Signal();
}

void BackgroundThreadQueue::GetAndDispatchMessages(int exit_type) {
  bool done = false;
  while(!done) {
    event_.Wait();
    // Move existing messages to a local queue.
    std::deque<Message> local_event_queue;
    {
      MutexLock lock(GetAndroidMessageQueueMutex());
      assert(event_queue_.size() > 0);
      event_queue_.swap(local_event_queue);
      // Unlock the mutex before invoking any callbacks.
    }

    // Dispatch the local events
    while (!local_event_queue.empty()) {
      Message msg = local_event_queue.front();
      local_event_queue.pop_front();
      if (msg.message_type == exit_type) {
        done = true;
        break;
      }
      g_instance.HandleThreadMessage(msg.message_type, msg.message_data.get());
    }
  }
}

//------------------------------------------------------------------------------
// MainThreadQueue

void MainThreadQueue::SendMessage(int message_type,
                                  MessageData* message_data) {
  NPN_PluginThreadAsyncCall(
      NULL, Dispatch, new Message(message_type, message_data));
}

// static
void MainThreadQueue::Dispatch(void* instance) {
  Message* msg = reinterpret_cast<Message*>(instance);
  g_instance.HandleThreadMessage(msg->message_type, msg->message_data.get());
  delete msg;
}

//------------------------------------------------------------------------------
// BrowserThreadMessageQueue

#ifdef USING_CCTESTS

const int kMessageType1 = 100;
const int kMessageType2 = 101;
const int kMessageType3 = 102;

class TestMessage : public MessageData {
 public:
  TestMessage(ThreadId sender, int data) : sender_(sender), data_(data) {}
  ~TestMessage() {}
  ThreadId Sender() {return sender_;}
  int Data() {return data_;}
 private:
  ThreadId sender_;
  int data_;
};

class BackgroundMessageHandler
    : public ThreadMessageQueue::HandlerInterface {
 public:
  explicit BackgroundMessageHandler(ThreadId sender) 
      : sender_(sender),
        count_(0) {}

  int Count() {return count_;}
  // ThreadMessageQueue::HandlerInterface override
  virtual void HandleThreadMessage(int message_type,
                                   MessageData *message_data) {
    assert(message_data);
    TestMessage* message = static_cast<TestMessage*>(message_data);
    assert(sender_ == message->Sender());
    // Accumulate the data.
    count_ += message->Data();
    LOG(("Got data %d", count_));
    if (count_ == 600) {
      // Tell the main thread we got all data.
      LOG(("Sending to main thread"));
      message = new TestMessage(
          ThreadMessageQueue::GetInstance()->GetCurrentThreadId(), count_);
      ThreadMessageQueue::GetInstance()->Send(message->Sender(),
                                              kMessageType3,
                                              message);
    }
  }

 private:
  ThreadId sender_;
  int count_;
};

class MainMessageHandler
    : public ThreadMessageQueue::HandlerInterface {
 public:
  // ThreadMessageQueue::HandlerInterface override
  virtual void HandleThreadMessage(int message_type,
                                   MessageData *message_data) {
    TestMessage* message = static_cast<TestMessage*>(message_data);
    assert (message->Data() == 600);
    // The worker must have received all data. Let's stop it.
    AndroidMessageLoop::Stop(message->Sender());
  }
};

class TestThread : public Thread {
 public:
  virtual void Run() {
    AndroidThreadMessageQueue* queue = static_cast<AndroidThreadMessageQueue*>(
        ThreadMessageQueue::GetInstance());

    BackgroundMessageHandler handler1(queue->MainThreadId());
    BackgroundMessageHandler handler2(queue->MainThreadId());
    ThreadMessageQueue::GetInstance()->RegisterHandler(
        kMessageType1, &handler1);
    ThreadMessageQueue::GetInstance()->RegisterHandler(
        kMessageType2, &handler2);

    // Run the message loop
    AndroidMessageLoop::Start();
  }

  virtual void CleanUp() {
    LOG(("Test worker finished"));
    delete this;
  }
};

// Global message handler
MainMessageHandler global_handler;

bool TestThreadMessageQueue(std::string16* error) {
  AndroidThreadMessageQueue* queue = static_cast<AndroidThreadMessageQueue*>(
      ThreadMessageQueue::GetInstance());

  // Init the message queue for the main thread
  queue->InitThreadMessageQueue();
  queue->RegisterHandler(kMessageType3, &global_handler);

  // Start the worker.
  TestThread *thread = new TestThread;  // deletes itself when the thread exits.
  ThreadId worker_thread_id = thread->Start();

  // Send three messages, sleep, send another three.
  ThreadId main_thread_id = queue->MainThreadId();
  TestMessage* message = new TestMessage(main_thread_id, 1);
  queue->Send(worker_thread_id, kMessageType1, message);
  message = new TestMessage(main_thread_id, 2);
  queue->Send(worker_thread_id, kMessageType1, message);
  message = new TestMessage(main_thread_id, 3);
  queue->Send(worker_thread_id, kMessageType1, message);
  sleep(1);
  message = new TestMessage(main_thread_id, 100);
  queue->Send(worker_thread_id, kMessageType2, message);
  message = new TestMessage(main_thread_id, 200);
  queue->Send(worker_thread_id, kMessageType2, message);
  message = new TestMessage(main_thread_id, 300);
  queue->Send(worker_thread_id, kMessageType2, message);

  return true;
}

#endif  // USING_CCTESTS
#endif  // OS_ANDROID
