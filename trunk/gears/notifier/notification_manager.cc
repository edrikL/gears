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

#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
#else
#include <assert.h>

#include "gears/notifier/notification_manager.h"

#include "gears/base/common/common.h"
#include "gears/base/common/stopwatch.h"
#include "gears/notifier/notification.h"
#include "gears/notifier/notification_manager_test.h"
#include "gears/notifier/user_activity.h"

// TODO(levin): Add logging support to gears for exe's.
// LOG_F is like LOG but it also includes the function name in
// addition to the message.
#define LOG_F(x) LOG(x)

typedef std::deque<QueuedNotification*> QueuedNotifications;

// Holds a notification plus additional information needed while the
// notification is waiting to be displayed.
class QueuedNotification {
 public:
  QueuedNotification(const GearsNotification &notification)
      : manager_(NULL),
        user_delayed_(false) {
    notification_.CopyFrom(notification);
  }

  ~QueuedNotification() {
    // TODO(levin): cancel the timer here
  }

  const GearsNotification &notification() const { return notification_; }
  GearsNotification *mutable_notification()  { return &notification_; }

  bool is_user_delayed() const { return user_delayed_; }

  bool Matches(const std::string16 &service,
               const std::string16 &id) const {
    return notification_.Matches(service, id);
  }

  void ShowWithDelay(NotificationManager *manager,
                     int64 delay_time,
                     bool user_delayed) {
    manager_ = manager;
    user_delayed_ = user_delayed;
    // TODO(levin): create a timer here
  }

  void CancelTimer() {
    // TODO(levin): cancel the timer here
    // This should be robust to the timer already begin fired (and being
    // processed or not being set-up at all.)
    user_delayed_ = false;
    manager_ = NULL;
  }

 private:
  // Callback for the timer for showing with a delay.
  static void MoveToShowQueue(void *cookie) {
    QueuedNotification *queued_notification =
        reinterpret_cast<QueuedNotification*>(cookie);
    assert(queued_notification && queued_notification->manager_);

    const GearsNotification &notification = queued_notification->notification();

    queued_notification->manager_->MoveFromDelayedToShowQueue(
        notification.service(),
        notification.id());

    // Clean up state associated with the timer.
    queued_notification->CancelTimer();
  }

  GearsNotification notification_;
  NotificationManager *manager_;
  bool user_delayed_;
  DISALLOW_EVIL_CONSTRUCTORS(QueuedNotification);
};

void Clear(std::deque<QueuedNotification*> *notifications) {
  while (!notifications->empty()) {
    QueuedNotifications::reverse_iterator it =
        notifications->rbegin();
    QueuedNotification *removed = *it;
    notifications->pop_back();
    delete removed;
  }
}

NotificationManager::NotificationManager(UserActivityInterface *activity)
    : activity_(activity) {
  assert(activity);
  balloon_collection_.reset(new BalloonCollection(this));
}

NotificationManager::~NotificationManager() {
  Clear(&show_queue_);
  Clear(&delayed_queue_);
}

QueuedNotification* FindNotification(
    const std::string16 &service,
    const std::string16 &id,
    bool and_remove,
    QueuedNotifications *queue) {
  assert(queue);
  for (QueuedNotifications::iterator it = queue->begin();
       it != queue->end();
       ++it) {
    if ((*it)->notification().Matches(service, id)) {
      QueuedNotification *queued_notification = *it;
      if (and_remove) {
        queue->erase(it);
      }
      return queued_notification;
    }
  }
  return NULL;
}

QueuedNotification *NotificationManager::Find(
    const std::string16 &service,
    const std::string16 &id,
    bool and_remove) {

  QueuedNotification *queued_notification = FindNotification(
      service, id, and_remove, &show_queue_);
  if (queued_notification) {
    return queued_notification;
  }
  return FindNotification(service, id, and_remove, &delayed_queue_);
}

void NotificationManager::AddWithDelay(const GearsNotification &notification,
                                       int delay_ms) {
  QueuedNotification *queued_notification =
      new QueuedNotification(notification);
  AddToDelayedQueue(queued_notification, delay_ms, true);
}

void NotificationManager::Add(const GearsNotification &notification) {
  LOG_F(("service: %s, id: %s", notification.service().c_str(),
         notification.id().c_str()));
  // First, try to update the notification in the display
  // in case it is already showing.
  if (balloon_collection_->Update(notification)) {
    return;
  }

  // Next, see if it is already queued up (and update it).
  QueuedNotification *queued_notification =
      Find(notification.service(), notification.id(), false);
  if (queued_notification) {
    LOG_F(("Updated. service: %s, id: %s", notification.service().c_str(),
           notification.id().c_str()));
    int64 now = GetCurrentTimeMillis();
    int64 previous_display_at_time_ms =
        queued_notification->notification().display_at_time_ms();
    queued_notification->mutable_notification()->CopyFrom(notification);

    // Handle display_at_time changes.
    if (notification.display_at_time_ms() != previous_display_at_time_ms) {
      // Several cases to consider:
      if (notification.display_at_time_ms() <= now) {
        if (previous_display_at_time_ms <= now) {
          // Both times are in the past.
          //     Result: Do nothing.
        } else {
          // The previous time is in the future.  The new time is in the past.
          //     Result: Move it to the display queue.
          MoveFromDelayedToShowQueue(notification.service(),
                                     notification.id());
        }
      } else {
        if (previous_display_at_time_ms <= now) {
          // The previous time is in the past.  The new time is in the future.
          //    In this case, the notification is in one of two states:
          if (queued_notification->is_user_delayed()) {
            //    1. in a snooze state (i.e. delayed by the user).
            //       Result: Do nothing.
          } else {
            //    2. waiting to be displayed
            //       Result: Delay the notification.
            MoveToDelayedQueue(notification.service(),
                               notification.id(),
                               notification.display_at_time_ms() - now,
                               false);
          }
        } else {
          // Both times are in the future.
          //     Result: Reschedule the display.
          queued_notification->ShowWithDelay(
              this, notification.display_at_time_ms() - now, false);
        }
      }
    }

    return;
  }

  // It appears that this is a new notification so add it.
  LOG_F(("Added. service: %s, id: %s", notification.service().c_str(),
         notification.id().c_str()));
  queued_notification = new QueuedNotification(notification);

  // If it is delayed until sometime in the future, set-up the delay.
  int64 display_at_time_ms =
      queued_notification->notification().display_at_time_ms();
  int64 delay_ms = display_at_time_ms - GetCurrentTimeMillis();
  if (delay_ms > 0) {
    AddToDelayedQueue(queued_notification, delay_ms, false);
    return;
  }
  AddToShowQueue(queued_notification);
}

void NotificationManager::AddToDelayedQueue(
    QueuedNotification *queued_notification,
    int64 delay_ms,
    bool user_delayed) {
  assert(queued_notification);
  delayed_queue_.push_back(queued_notification);
  queued_notification->ShowWithDelay(this, delay_ms, user_delayed);
}

void NotificationManager::AddToShowQueue(
    QueuedNotification *queued_notification) {
  assert(queued_notification);
  assert(queued_notification->notification().display_at_time_ms() <=
         GetCurrentTimeMillis());
  show_queue_.push_back(queued_notification);
  ShowNotifications();
}

void NotificationManager::MoveFromDelayedToShowQueue(
    const std::string16 &service,
    const std::string16 &id) {
  QueuedNotification *queued_notification =
      FindNotification(service, id, true, &delayed_queue_);
  // This should only be called when it is known that the notification is
  // in the delayed queue.
  assert(queued_notification);
  queued_notification->CancelTimer();
  AddToShowQueue(queued_notification);
}

void NotificationManager::MoveToDelayedQueue(
    const std::string16 &service,
    const std::string16 &id,
    int64 delay_ms,
    bool user_delayed) {
  QueuedNotification *queued_notification = Find(service, id, true);
  // This should only be called when it is known that the notification is
  // in a queue.
  assert(queued_notification);
  AddToDelayedQueue(queued_notification, delay_ms, user_delayed);
}

bool NotificationManager::Delete(const std::string16 &service,
                                 const std::string16 &id) {
  LOG_F(("Deleting. service: %s, id: %s", service.c_str(), id.c_str()));

  // If displayed, then delete it.
  if (balloon_collection_->Delete(service, id)) {
    return true;
  }

  QueuedNotification *queued_notification = Find(service, id, true);
  if (queued_notification) {
    delete queued_notification;
    return true;
  }

  // Not found.
  LOG_F(("Not found. service: %s, id: %s", service.c_str(), id.c_str()));
  return false;
}

void NotificationManager::ShowNotifications() {
  // Is it ok to show the notification now?
  if (!IsActiveUserMode(activity_->CheckUserActivity())) {
    return;
  }
  while (!show_queue_.empty() && balloon_collection_->has_space()) {
    QueuedNotification* queued_notification = show_queue_.front();
    show_queue_.pop_front();
    balloon_collection_->Show(queued_notification->notification());
    delete queued_notification;
  }
}

void NotificationManager::OnDisplaySettingsChanged() {
  // TODO(levin): implement
}

void NotificationManager::OnUserPresentationModeChange(
    bool /* in_presentation */) {
  // TODO(levin): implement
}

void NotificationManager::OnUserActivityChange() {
  ShowNotifications();
}

void NotificationManager::OnBalloonSpaceChanged() {
  ShowNotifications();
}
#endif  // OFFICIAL_BUILD
