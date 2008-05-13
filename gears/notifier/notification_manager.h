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
//
// Manages all of the desktop notifications.

#ifndef GEARS_NOTIFIER_NOTIFICATION_MANAGER_H__
#define GEARS_NOTIFIER_NOTIFICATION_MANAGER_H__

#include <deque>

#include "gears/base/common/basictypes.h"
#include "gears/base/common/string16.h"
#include "gears/notifier/balloon_collection.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

#if USING_CCTESTS
class BalloonCollectionMock;
#endif  // USING_CCTESTS
class Notification;
class UserActivityInterface;

// Handles all aspects of the notifications to be displayed.
class NotificationManager : public BalloonCollectionObserver {
 public:
  NotificationManager(UserActivityInterface *activity);
  ~NotificationManager();

  void Add(const Notification &notification);

  // Returns
  //   True if a match for the notification information was found
  //     (either in the queue or on the display).
  //   False if no match was found.
  bool Delete(const std::string16 &service, const std::string16 &id);

  // Handles adjusting notifications when any of display settings is changed.
  void OnDisplaySettingsChanged();

  // Hides/unhides notifications depending on the user's mode.
  void OnUserPresentationModeChange(bool in_presentation);

  // Called when the user becomes active/inactive.
  void OnUserActivityChange();

  // BalloonCollectionObserver implementation.
  virtual void OnBalloonSpaceChanged();

#if USING_CCTESTS
  BalloonCollectionMock *UseBalloonCollectionMock();
#endif  // USING_CCTESTS

 private:
  void ShowNotifications();

  UserActivityInterface *activity_;
  scoped_ptr<BalloonCollectionInterface> balloon_collection_;
  std::deque<Notification*> notifications_;
  DISALLOW_EVIL_CONSTRUCTORS(NotificationManager);
};

#endif  // GEARS_NOTIFIER_NOTIFICATION_MANAGER_H__
