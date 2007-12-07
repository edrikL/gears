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
#include <nsCOMPtr.h>
#include <nspr.h>  // for PR_*
#ifdef WIN32
#include <windows.h> // must manually #include before nsIEventQueueService.h
#endif

#include "gears/base/common/message_queue.h"
#include "gears/third_party/gecko_internal/nsIEventQueueService.h"


// Protects callbacks_ during inserts and finds.
static Mutex callback_mutex_;
static std::map<int, ThreadMessageQueue::HandlerCallback> callbacks_;

bool ThreadMessageQueue::InitThreadMessageQueue() {
  return true;
}

bool ThreadMessageQueue::RegisterHandler(int message_id,
                                         HandlerCallback message_handler) {
  MutexLock lock(&callback_mutex_);
  callbacks_[message_id] = message_handler;
  return true;
}

ThreadId ThreadMessageQueue::GetCurrentThreadId() {
  return PR_GetCurrentThread();
}

struct MessageEvent {
  PLEvent e;
  ThreadId thread;
  int message_id;
  std::string message_data_1;
  std::string message_data_2;
};

void *OnReceiveMessageEvent(MessageEvent *event) {
  // Sanity check the thread.
  assert(event->thread == ThreadMessageQueue::GetCurrentThreadId());

  // Check that a callback has been registered for this event type.
  std::map<int, ThreadMessageQueue::HandlerCallback>::iterator handler;
  {
    // We only need to lock during the find.
    MutexLock lock(&callback_mutex_);
    handler = callbacks_.find(event->message_id);
  }
  if (handler != callbacks_.end()) {
    handler->second(event->message_id, event->message_data_1.c_str(),
                    event->message_data_2.c_str());
  }

  return NULL;
}

static void OnDestroyMessageEvent(MessageEvent *event) {
  delete event;
}

void ThreadMessageQueue::Send(ThreadId thread,
                              int message_id,
                              const char *message_data_1,
                              const char *message_data_2) {
  nsresult nr;

  nsCOMPtr<nsIEventQueueService> event_queue_service =
      do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID, &nr);
  if (NS_FAILED(nr)) {
    return;
  }

  // If there's no event queue on this thread, we can't do anything.
  nsCOMPtr<nsIEventQueue> event_queue;
  nr = event_queue_service->GetThreadEventQueue(thread,
                                                getter_AddRefs(event_queue));
  if (NS_FAILED(nr)) {
    return;
  }

  MessageEvent *event = new MessageEvent;
  event->thread = thread;
  event->message_id = message_id;
  event->message_data_1 = message_data_1;
  event->message_data_2 = message_data_2;

  if (NS_FAILED(event_queue->InitEvent(
      &event->e, nsnull,
      reinterpret_cast<PLHandleEventProc>(OnReceiveMessageEvent),
      reinterpret_cast<PLDestroyEventProc>(OnDestroyMessageEvent)))) {
    delete event;
    return;
  }
  if (NS_FAILED(event_queue->PostEvent(&event->e))) {
    // Cleaning up the event at this point would requires access to private
    // functions, so just clear the strings so that we leak less.
    event->message_data_1.clear();
    event->message_data_2.clear();
    return;
  }
}
