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

// Balloon collection is a queue of balloons with notifications currently
// displayed on the main screen. The notifications expire with time and
// disappear, at which moment they are removed from the collection.

#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
#else
#include <string>
#include <assert.h>

#include "gears/notifier/balloons.h"

BalloonCollection::BalloonCollection(BalloonCollectionObserver *observer)
  : observer_(observer),
    root_node_(NULL),
    balloon_container_(NULL),
    has_space_(true) {
  assert(observer);
  observer_->OnBalloonSpaceChanged();
}

BalloonCollection::~BalloonCollection() {
  Clear();
}

void BalloonCollection::Clear() {
  for (Balloons::iterator it = balloons_.begin();
       it != balloons_.end();
       ++it) {
    Balloon *removed = *it;
    // TODO(dimich): Do we need this here? it = balloons_->erase(it);
    delete removed;
  }
}

Balloon *BalloonCollection::FindBalloon(const std::string16 &service,
                                        const std::string16 &id,
                                        bool and_remove) {
  for (Balloons::iterator it = balloons_.begin();
       it != balloons_.end();
       ++it) {
    if ((*it)->notification().id() == id &&
        (*it)->notification().service() == service) {
      Balloon *result = *it;
      if (and_remove) {
        balloons_.erase(it);
      }
      return result;
    }
  }
  return NULL;
}

void BalloonCollection::Show(const Notification &notification) {
  Balloon *balloon = FindBalloon(notification.service(),
                                 notification.id(),
                                 false);  // no remove
  assert(!balloon);
  // If someone repeatedly adds the same notification, avoid flood - return.
  if (balloon)
    return;

  balloons_.push_front(new Balloon(notification));
  observer_->OnBalloonSpaceChanged();
}

bool BalloonCollection::Update(const Notification &notification) {
  Balloon *balloon = FindBalloon(notification.service(),
                                 notification.id(),
                                 false);  // no remove
  if (!balloon)
    return false;
  balloon->mutable_notification()->CopyFrom(notification);
  return true;
}

bool BalloonCollection::Delete(const std::string16 &service,
                               const std::string16 &id) {
  Balloon *balloon = FindBalloon(service,
                                 id,
                                 true);  // remove from list
  if (!balloon)
    return false;

  delete balloon;

  observer_->OnBalloonSpaceChanged();
  return true;
}

Balloon::Balloon(const Notification &from) {
  notification_.CopyFrom(from);
}

Balloon::~Balloon() {
}


#endif  // OFFICIAL_BULID
