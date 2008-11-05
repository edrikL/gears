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
// This file defines the WinceGpsDevice class, which implements a representation
// of a GPS device on WinCE.

#ifndef GEARS_GEOLOCATION_GPS_DEVICE_WINCE_H__
#define GEARS_GEOLOCATION_GPS_DEVICE_WINCE_H__

#include "gears/geolocation/gps_location_provider.h"

#include <atlsync.h>  // For CEvent.
#include "gears/base/common/common.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/thread.h"

class WinceGpsDevice
    : public GpsDeviceBase,
      public Thread {
 public:
  friend GpsDeviceBase *GpsLocationProvider::NewGpsDevice(
      GpsDeviceBase::ListenerInterface *listener);

 private:
  // Create with factory function
  WinceGpsDevice(GpsDeviceBase::ListenerInterface *listener);
  virtual ~WinceGpsDevice();

  // Thread implementation
  virtual void Run();

  // Callbacks used to handle updates from the GPS Intermediate Driver library.
  void HandlePositionUpdate();
  void HandleStateChange();

  // The current best position estimate and its mutex.
  Position position_;
  Mutex position_mutex_;

  // Handle to the GPS Intermediate Driver library.
  HANDLE gps_handle_;

  // Events signalled to the thread that waits for events from the GPS API.
  CEvent stop_event_;

  // The state of the attempt to get a fix from the GPS. Used for determining
  // timeouts.
  typedef enum {
    STATE_CONNECTING,
    STATE_ACQUIRING_FIX,
    STATE_FIX_ACQUIRED,
  } State;
  State state_;

  DISALLOW_EVIL_CONSTRUCTORS(WinceGpsDevice);
};

#endif  // GEARS_GEOLOCATION_GPS_DEVICE_WINCE_H__
