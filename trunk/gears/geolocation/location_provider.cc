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
// This file implements a mock location provider and the factory functions for
// creating various types of location provider.

#ifdef OFFICIAL_BUILD
// The Geolocation API has not been finalized for official builds.
#else

#include "gears/geolocation/location_provider.h"

#include <assert.h>

void LocationProviderBase::AddListener(ListenerInterface *listener) {
  assert(listener);
  MutexLock lock(&listeners_mutex_);
  listeners_.insert(listener);
}

void LocationProviderBase::RemoveListener(ListenerInterface *listener) {
  assert(listener);
  MutexLock lock(&listeners_mutex_);
  ListenerSet::iterator iter = listeners_.find(listener);
  if (iter != listeners_.end()) {
    listeners_.erase(iter);
  }
}

void LocationProviderBase::UpdateListeners() {
  MutexLock lock(&listeners_mutex_);
  for (ListenerSet::const_iterator iter = listeners_.begin();
       iter != listeners_.end();
       ++iter) {
    (*iter)->LocationUpdateAvailable(this);
  }
}

#ifdef USING_CCTESTS
#include "gears/base/common/common.h"
#include "gears/base/common/stopwatch.h"  // For GetCurrentTimeMillis()
#include "gears/geolocation/geolocation.h"  // For Position

// Mock implementation of a location provider for testing.
class MockLocationProvider : public LocationProviderBase {
 public:
  MockLocationProvider() {}
  virtual ~MockLocationProvider() {}
  // LocationProviderBase implementation.
  virtual void GetPosition(Position *position) {
    assert(position);
    position->latitude = 54.0;
    position->longitude = 0.0;
    position->altitude = 0;
    position->horizontal_accuracy = 1000;
    position->vertical_accuracy = 10;
    position->timestamp = GetCurrentTimeMillis();
  }
 private:
  LocationProviderBase::ListenerInterface *listener_;
  DISALLOW_EVIL_CONSTRUCTORS(MockLocationProvider);
};

LocationProviderBase *NewMockLocationProvider() {
  return new MockLocationProvider();
}

#endif

// Win32, Linux and OSX do not have a GPS location provider.
#if (defined(WIN32) && !defined(WINCE)) || defined(LINUX) || defined(OS_MACOSX)

LocationProviderBase *NewGpsLocationProvider() {
  return NULL;
}

#endif

#endif  // OFFICIAL_BUILD
