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
#include "gears/notifier/notification.h"
#include "gears/notifier/notification_manager_test.h"
#include "gears/notifier/user_activity.h"

typedef std::deque<Notification*> Notifications;

// TODO(levin): Add logging support to gears for exe's.
// LOG_F is like LOG but it also includes the function name in
// addition to the message.
#define LOG_F(x) LOG(x)

void Clear(Notifications *notifications) {
  for (Notifications::iterator it = notifications->begin();
       it != notifications->end();
       ++it) {
    Notification *removed = *it;
    it = notifications->erase(it);
    delete removed;
  }
}

NotificationManager::NotificationManager(UserActivityInterface *activity)
    : activity_(activity) {
  assert(activity);
  balloon_collection_.reset(new BalloonCollection(this));
}

NotificationManager::~NotificationManager() {
  Clear(&notifications_);
}

void NotificationManager::Add(const Notification &notification) {
  LOG_F(("service: %s, id: %s", notification.service().c_str(),
         notification.id().c_str()));
  if (balloon_collection_->Update(notification)) {
    return;
  }

  for (Notifications::iterator it = notifications_.begin();
       it != notifications_.end();
       ++it) {
    if ((*it)->id() == notification.id() &&
        (*it)->service() == notification.service()) {
      LOG_F(("Updated. service: %s, id: %s", notification.service().c_str(),
             notification.id().c_str()));
      (*it)->CopyFrom(notification);
      return;
    }
  }
  LOG_F(("Added. service: %s, id: %s", notification.service().c_str(),
         notification.id().c_str()));
  Notification* copy = new Notification;
  copy->CopyFrom(notification);
  notifications_.push_back(copy);
  ShowNotifications();
}

bool NotificationManager::Delete(const std::string16 &service,
                                 const std::string16 &id) {
  LOG_F(("Deleting. service: %s, id: %s", service.c_str(), id.c_str()));
  // If displayed, then delete it.
  if (balloon_collection_->Delete(service, id)) {
    return true;
  }

  for (Notifications::iterator it = notifications_.begin();
       it != notifications_.end();
       ++it) {
    if ((*it)->id() == id && (*it)->service() == service) {
      Notification *removed = *it;
      it = notifications_.erase(it);
      delete removed;
      return true;
    }
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
  while (!notifications_.empty() && balloon_collection_->has_space()) {
    Notification* notification = notifications_.front();
    notifications_.pop_front();
    balloon_collection_->Show(*notification);
    delete notification;
  }
}

void NotificationManager::OnDisplaySettingsChanged() {
  // TODO(levin): implement
}

void NotificationManager::OnUserPresentationModeChange(bool /* in_presentation */) {
  // TODO(levin): implement
}

void NotificationManager::OnUserActivityChange() {
  ShowNotifications();
}

void NotificationManager::OnBalloonSpaceChanged() {
  ShowNotifications();
}

#if USING_CCTESTS
BalloonCollectionMock *NotificationManager::UseBalloonCollectionMock() {
  BalloonCollectionMock *mock = new BalloonCollectionMock;
  balloon_collection_.reset(mock);
  return mock;
}
#endif  // USING_CCTESTS
#endif  // OFFICIAL_BUILD
