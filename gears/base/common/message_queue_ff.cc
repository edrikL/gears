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
#include <nsCOMPtr.h>
#include <nspr.h>  // for PR_*
#ifdef WIN32
#include <windows.h> // must manually #include before nsIEventQueueService.h
#endif

#include "gears/base/common/message_queue.h"
#include "gecko_internal/nsIEventQueueService.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

struct MessageEvent;

// A concrete implementation that uses Firefox's nsIEventQueue
class FFThreadMessageQueue : public ThreadMessageQueue {
 public:
  virtual bool InitThreadMessageQueue();
  virtual ThreadId GetCurrentThreadId();
  virtual bool Send(ThreadId thread_handle,
                    int message_type,
                    MessageData *message_data);
 private:
  struct MessageEvent {
    MessageEvent(int message_type, MessageData *message_data)
        : message_type(message_type), message_data(message_data) {}
    PLEvent pl_event;  // must be first in the struct
    int message_type;
    scoped_ptr<MessageData> message_data;
  };

  static void *OnReceiveMessageEvent(MessageEvent *event);
  static void OnDestroyMessageEvent(MessageEvent *event);
};

static FFThreadMessageQueue g_instance;

// static
ThreadMessageQueue *ThreadMessageQueue::GetInstance() {
  return &g_instance;
}

bool FFThreadMessageQueue::InitThreadMessageQueue() {
  return true;
}

ThreadId FFThreadMessageQueue::GetCurrentThreadId() {
  return PR_GetCurrentThread();
}

// static
void *FFThreadMessageQueue::OnReceiveMessageEvent(MessageEvent *event) {
  RegisteredHandler handler;
  if (g_instance.GetRegisteredHandler(event->message_type, &handler)) {
    handler.Invoke(event->message_type, event->message_data.get());
  }
  return NULL;
}

// static
void FFThreadMessageQueue::OnDestroyMessageEvent(MessageEvent *event) {
  delete event;
}

bool FFThreadMessageQueue::Send(ThreadId thread,
                                int message_type,
                                MessageData *message_data) {
  nsresult nr;
  nsCOMPtr<nsIEventQueueService> event_queue_service =
      do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &nr);
  if (NS_FAILED(nr)) {
    delete message_data;
    return false;
  }

  nsCOMPtr<nsIEventQueue> event_queue;
  nr = event_queue_service->GetThreadEventQueue(thread,
                                                getter_AddRefs(event_queue));
  if (NS_FAILED(nr)) {
    delete message_data;
    return false;
  }

  MessageEvent *event = new MessageEvent(message_type, message_data);
  if (NS_FAILED(event_queue->InitEvent(
          &event->pl_event, nsnull,
          reinterpret_cast<PLHandleEventProc>(OnReceiveMessageEvent),
          reinterpret_cast<PLDestroyEventProc>(OnDestroyMessageEvent)))) {
    delete event;
    return false;
  }
  if (NS_FAILED(event_queue->PostEvent(&event->pl_event))) {
    // Cleaning up the event at this point requires access to private
    // functions, so just clear the message data to leak less.
    event->message_data.reset(NULL);
    return false;
  }
  return true;
}
