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
// Handles the visible notification (or balloons).

#ifndef GEARS_NOTIFIER_BALLOONS_H__
#define GEARS_NOTIFIER_BALLOONS_H__

#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
#else
#include <deque>
#include "gears/base/common/basictypes.h"
#include "gears/base/common/string16.h"
#include "gears/notifier/notification.h"

class Notification;

class BalloonCollectionObserver {
 public:
  // Called when there is more or less space for balloons due to
  // monitor size changes or balloons disappearing.
  virtual void OnBalloonSpaceChanged() = 0;
};

class BalloonCollectionInterface {
 public:
  virtual ~BalloonCollectionInterface() {}
  // Adds a new balloon for the specified notification. Does not check for
  // already showing notifications with the same service and id. Asserts if
  // there is already a notification with the same service/id.
  virtual void Show(const Notification &notification) = 0;
  // Updates the existing balloon with notification that matches service and id
  // of the specified one. In case there is no balloon showing the matching
  // notification, returns 'false'.
  virtual bool Update(const Notification &notification) = 0;
  // Immediately "expires" any notification with the same service and id from
  // the screen display. Returns 'false' if there is no such notification.
  virtual bool Delete(const std::string16 &service,
                      const std::string16 &id) = 0;

  // Is there room to add another notification?
  virtual bool has_space() = 0;
};

// Glint classes
namespace glint {
class Node;
class RootUI;
}

// Represents a Notification on the screen.
class Balloon {
 public:
  explicit Balloon(const Notification &from);
  ~Balloon();

  const Notification &notification() const {
    return notification_;
  }

  Notification *mutable_notification() {
    return &notification_;
  }

  glint::Node *ui_root() {
    if (!ui_root_) {
      ui_root_ = CreateTree();
    }
    return ui_root_;
  }

  void UpdateUI();

 private:
  glint::Node *CreateTree();
  bool SetTextField(const char *id, const std::string16 &text);
  Notification notification_;
  glint::Node *ui_root_;
  DISALLOW_EVIL_CONSTRUCTORS(Balloon);
};

typedef std::deque<Balloon*> Balloons;

class BalloonCollection : public BalloonCollectionInterface {
 public:
  explicit BalloonCollection(BalloonCollectionObserver *observer);
  virtual ~BalloonCollection();

  // BalloonCollectionInterface overrides
  virtual void Show(const Notification &notification);
  virtual bool Update(const Notification &notification);
  virtual bool Delete(const std::string16 &service, const std::string16 &id);
  virtual bool has_space() { return has_space_; }

 private:
  void Clear();  // clears balloons_
  Balloon *FindBalloon(const std::string16 &service,
                       const std::string16 &id,
                       bool and_remove);
  void AddToUI(Balloon *balloon);
  void RemoveFromUI(Balloon *balloon);
  void EnsureRoot();

  Balloons balloons_;
  BalloonCollectionObserver *observer_;
  glint::RootUI *root_ui_;
  bool has_space_;
  DISALLOW_EVIL_CONSTRUCTORS(BalloonCollection);
};

#endif  // OFFICIAL_BUILD
#endif  // GEARS_NOTIFIER_BALLOONS_H__