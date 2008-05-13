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

#ifndef GEARS_NOTIFIER_BALLOON_COLLECTION_H__
#define GEARS_NOTIFIER_BALLOON_COLLECTION_H__

#include "gears/base/common/basictypes.h"
#include "gears/base/common/string16.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

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
  virtual void Show(const Notification &notification) = 0;
  virtual bool Update(const Notification &notification) = 0;
  virtual bool Delete(const std::string16 &service,
                      const std::string16 &id) = 0;

  // Is there room to add another notification?
  virtual bool has_space() = 0;
};
#endif  // GEARS_NOTIFIER_BALLOON_COLLECTION_H__
