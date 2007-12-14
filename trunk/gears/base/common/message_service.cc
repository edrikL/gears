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
#include <set>
#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/message_queue.h"
#include "gears/base/common/message_service.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// TODO(michaeln) When an OS thread dies, remove all observers associated with
// that thread.

static const int kNotificationMessageType = 1;

typedef std::set<MessageObserverInterface*> ObserverSet;
typedef std::map<ThreadId, ObserverSet> ThreadObserversMap;


// When NotifyObservers is called, the message service needs to broadcast
// the notification to observers on multiple threads. It uses the thread
// message queue to do so. However instead of creating multiples copies of
// the data our client wishes to broadcast, we send a message which contains
// a shared reference to the callers original data to each thread. In this
// way, we avoid making copies of the callers original data.
// See NotificationMessage.
class SharedNotificationData {
 public:
  SharedNotificationData(const char16 *topic, NotificationData *data)
      : topic_(topic), data_(data), refcount_(1) {
#ifdef DEBUG
    AtomicIncrement(&g_instance_count_, 1);
#endif  
  }

#ifdef DEBUG
  ~SharedNotificationData() {
    AtomicIncrement(&g_instance_count_, -1);
  }
#endif  

  void AddReference() {
    AtomicIncrement(&refcount_, 1);
  }

  void RemoveReference() {
    if (AtomicIncrement(&refcount_, -1) == 0)
      delete this;
  }

  std::string16 topic_;
  scoped_ptr<NotificationData> data_;
  int refcount_;
#ifdef DEBUG
  static int g_instance_count_;
#endif
};

#ifdef DEBUG
// Instrumentation to observe that object lifecycles are working as expected
// TODO(michaeln): remove this instrumentation once comfortable that things
// are working as they should be
int SharedNotificationData::g_instance_count_ = 0;
#endif


class NotificationMessage : public MessageData {
 public:
  NotificationMessage(SharedNotificationData *shared)
      : shared_(shared) {
    shared_->AddReference();
  }

  ~NotificationMessage() {
    shared_->RemoveReference();
  }

  SharedNotificationData *shared_;
};


// Helper class used by MessageService. The MessageService creates
// a collection for each unique topic. All observers registered for
// a particular topic are stored in the same collection.
class ObserverCollection {
 public:
  ObserverCollection(MessageService *service)
    : service_(service) {}

  bool Add(MessageObserverInterface *observer);
  bool Remove(MessageObserverInterface *observer);
  bool IsEmpty() const;

  void PostThreadNotifications(const char16 *topic,
                               NotificationData *data);
  void ProcessThreadNotification(NotificationMessage *message);

 private:
  MessageService *service_;

  // This class organizes all observers for a topic into distinct sets, with
  // one set containing all of the observers for a particular thread.
  ThreadObserversMap observer_sets_;
  ObserverSet *GetThreadObserverSet(ThreadId thread_id, bool create_if_needed);

  DISALLOW_EVIL_CONSTRUCTORS(ObserverCollection);
};


// static
MessageService *MessageService::GetInstance() {
  static MessageService g_service_(ThreadMessageQueue::GetInstance());
  return &g_service_;
}


MessageService::MessageService(ThreadMessageQueue *message_queue)
    : message_queue_(message_queue) {
  message_queue_->RegisterHandler(kNotificationMessageType, this);
}


MessageService::~MessageService() {
}


bool MessageService::AddObserver(MessageObserverInterface *observer,
                                 const char16 *topic) {
  MutexLock lock(&observer_collections_mutex_);
  ObserverCollection *topic_observers =
                          GetTopicObserverCollection(topic, true);
  return topic_observers->Add(observer);
}


bool MessageService::RemoveObserver(MessageObserverInterface *observer,
                                    const char16 *topic) {
  MutexLock lock(&observer_collections_mutex_);
  ObserverCollection *topic_observers =
                          GetTopicObserverCollection(topic, false);
  if (!topic_observers) return false;
  bool removed = topic_observers->Remove(observer);
  if (removed) {
    if (topic_observers->IsEmpty()) {
      DeleteTopicObserverCollection(topic);
    }
  }
  return removed;
}


void MessageService::NotifyObservers(const char16 *topic,
                                     NotificationData *data) {
  MutexLock lock(&observer_collections_mutex_);
  ObserverCollection *topic_observers =
                          GetTopicObserverCollection(topic, false);
  if (!topic_observers) { 
    delete data; 
    return;
  }
  topic_observers->PostThreadNotifications(topic, data);
}


void MessageService::HandleThreadMessage(int message_type,
                                         MessageData *message_data) {
  MutexLock lock(&observer_collections_mutex_);
  assert(message_type == kNotificationMessageType);
  NotificationMessage *notification =
                          static_cast<NotificationMessage*>(message_data);
  const char16 *topic = notification->shared_->topic_.c_str();
  ObserverCollection *topic_observers =
                          GetTopicObserverCollection(topic, false);
  if (!topic_observers) return;
  topic_observers->ProcessThreadNotification(notification);
  return;
}


ObserverCollection *MessageService::GetTopicObserverCollection(
                                        const char16 *topic,
                                        bool create_if_needed) {
  // assert(mutex_.IsLockedByCurrentThread());
  std::string16 key(topic);
  TopicObserverMap::const_iterator found = observer_collections_.find(key);
  if (found != observer_collections_.end()) {
    return found->second.get();
  } else if (!create_if_needed) {
    return NULL;
  } else {
    ObserverCollection *collection = new ObserverCollection(this);
    observer_collections_[key] = linked_ptr<ObserverCollection>(collection);
    return collection;
  }
  // unreachable
}


void MessageService::DeleteTopicObserverCollection(const char16 *topic) {
  // assert(mutex_.IsLockedByCurrentThread());
  std::string16 key(topic);
  TopicObserverMap::iterator found = observer_collections_.find(key);
  if (found != observer_collections_.end()) {
    observer_collections_.erase(found);    
  }
}


bool ObserverCollection::Add(MessageObserverInterface *observer) {
  // assert(serveice_->mutex_.IsLockedByCurrentThread());
  ThreadId thread_id = service_->message_queue_->GetCurrentThreadId();
  ObserverSet *set = GetThreadObserverSet(thread_id, true);
  if (set->find(observer) != set->end())
    return false;  // observer is already registered on this thread
  set->insert(observer);
  return true;
}


bool ObserverCollection::Remove(MessageObserverInterface *observer) {
  // assert(serveice_->mutex_.IsLockedByCurrentThread());
  ThreadId thread_id = service_->message_queue_->GetCurrentThreadId();
  ObserverSet *set = GetThreadObserverSet(thread_id, false);
  if (!set) return false;
  ObserverSet::iterator found = set->find(observer);
  if (found == set->end())
    return false;  // observer is not registered on this thread
  set->erase(found);
  if (set->empty())
    observer_sets_.erase(thread_id);
  return true;
}


bool ObserverCollection::IsEmpty() const {
  // assert(serveice_->mutex_.IsLockedByCurrentThread());
  return observer_sets_.empty();
}


ObserverSet *ObserverCollection::GetThreadObserverSet(ThreadId thread_id,
                                                      bool create_if_needed) {
  ThreadObserversMap::iterator found = observer_sets_.find(thread_id);
  if (found != observer_sets_.end()) {
    return &found->second;
  } else if (!create_if_needed) {
    return NULL;
  } else {
    service_->message_queue_->InitThreadMessageQueue();
    observer_sets_[thread_id] = ObserverSet();
    return &observer_sets_[thread_id];
  }
}


void ObserverCollection::PostThreadNotifications(const char16 *topic,
                                                 NotificationData *data) {
  // assert(serveice_->mutex_.IsLockedByCurrentThread());
  SharedNotificationData *shared_data = new SharedNotificationData(topic,
                                                                   data);

  // Send one message for each thread containing observers of this topic
  ThreadObserversMap::iterator iter;
  for (iter = observer_sets_.begin(); iter != observer_sets_.end(); ++iter) {
    service_->message_queue_->Send(iter->first, kNotificationMessageType,
                                   new NotificationMessage(shared_data));
  }
  shared_data->RemoveReference();
}


void ObserverCollection::ProcessThreadNotification(
                             NotificationMessage *message) {
  // assert(serveice_->mutex_.IsLockedByCurrentThread());
  const char16 *topic = message->shared_->topic_.c_str();
  const NotificationData *data = message->shared_->data_.get();

  // Dispatch this notification to all topic observers in this thread.
  //
  // Note, this ObserverCollection instance can be modified or deleted
  // during the iteration below because we unlock and relock the service's
  // mutex around the calls to observer->OnNotify. We guard against this be
  // making local copies of instance data we need, and by testing for whether 
  // or not the collection has been deleted or a particular observer has been
  // removed from the collection prior to calling OnNotify.

  ThreadId thread_id = service_->message_queue_->GetCurrentThreadId();
  ObserverSet *set = GetThreadObserverSet(thread_id, false);
  if (!set) return;

  MessageService *service = service_;
  ObserverSet observer_set_copy = *set;

  ObserverSet::iterator iter;
  for (iter = observer_set_copy.begin();
       iter != observer_set_copy.end();
       ++iter) {
    // ensure this collection and the set for this thread has not been deleted
    ObserverCollection *should_be_us =
                            service->GetTopicObserverCollection(topic, false);
    if (should_be_us != this) {
      return;
    }
    ObserverSet *should_be_same_set = GetThreadObserverSet(thread_id, false);
    if (should_be_same_set != set) {
      return;
    }

    // ensure this observer has not been removed
    if (set->find(*iter) != set->end()) {
      service->observer_collections_mutex_.Unlock();
      (*iter)->OnNotify(service, topic, data);
      service->observer_collections_mutex_.Lock();    
    }
  }
}
