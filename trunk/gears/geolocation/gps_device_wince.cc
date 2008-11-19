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
// We use the GPS Intermediate Driver to access the GPS. See
// http://msdn.microsoft.com/en-us/library/ms850332.aspx.
//
// Note that the GPS driver may not be enabled by default. To enable it on a
// WinMo 6 device, you must set the virtual port to be used by the driver.
// - Go to Start -> Settings.
// - In the System tab, go to External GPS.
// - In the Programs tab, set 'GPS Program port' to any unused port.
// See http://blogs.msdn.com/windowsmobile/archive/2006/06/07/620387.aspx
// and http://www.expansys-usa.com/ft.aspx?k=100176&page=3.
//
// TODO(steveblock): consider adding this information to the Geolocation API
// documentation so that developers can communicate it to users if appropriate.

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented
#ifdef OS_WINCE

#include "gears/geolocation/gps_device_wince.h"

#include <gpsapi.h>
#include <service.h>  // For SERVICE_STATE constants
#include "gears/base/common/stopwatch.h"
#include "gears/base/common/time_utils_win32.h"  // For FiletimeToMilliseconds()


// Maximum age of GPS data we'll accept.
static const int kGpsDataMaximumAgeMilliseconds = 5 * 1000;
// Maximum time we'll wait to connect to the GPS.
static const int kMaximumConnectionTimeMilliseconds = 45 * 1000;
// Maximum time we'll wait for the GPS to acquire a fix.
static const int kMaximumAcquistionTimeMilliseconds = 5 * 60 *1000;

static const char16 *kFailedToConnectErrorMessage =
    STRING16(L"Failed to connect to GPS.");
static const char16 *kFailedToGetFixErrorMessage =
    STRING16(L"GPS failed to get a position fix.");

// Local function
// This assumes that position_2 is a good fix.
static bool PositionsDiffer(const Position &position_1,
                            const Position &position_2);


// GpsLocationProvider factory method for platform-dependent GPS devices.
// static
GpsDeviceBase *GpsLocationProvider::NewGpsDevice(
    GpsDeviceBase::ListenerInterface *listener) {
  return new WinceGpsDevice(listener);
}


WinceGpsDevice::WinceGpsDevice(GpsDeviceBase::ListenerInterface *listener)
    : GpsDeviceBase(listener) {
  assert(listener_);

  // Initialise member events.
  stop_event_.Create(NULL, FALSE, FALSE, NULL);

  // Start the worker thread.
  Start();
}

WinceGpsDevice::~WinceGpsDevice() {
  // Shut down the worker thread.
  stop_event_.Set();
  Join();
}

// Thread implementation
void WinceGpsDevice::Run() {
  // Initialise events.
  CEvent position_update_event;
  CEvent state_change_event;
  position_update_event.Create(NULL, FALSE, FALSE, NULL);
  state_change_event.Create(NULL, FALSE, FALSE, NULL);
  HANDLE events[] = {stop_event_,
                     position_update_event,
                     state_change_event};

  // Connect to GPS.
  gps_handle_ = GPSOpenDevice(position_update_event,
                              state_change_event,
                              NULL,  // Reserved
                              0);  // Reserved
  if (gps_handle_ == NULL) {
    LOG(("WinceGpsDevice::Run() : Failed to open handle to GPS device.\n"));
    listener_->GpsFatalError(Position::ERROR_CODE_POSITION_UNAVAILABLE,
                             kFailedToConnectErrorMessage);
    return;
  }

  // Wait for updates from the GPS driver. It seems that if GPS is not present
  // or the driver is disabled we still get callbacks. This isn't very useful
  // for detecting the fact that there's no GPS available, so we must rely on
  // timeouts.
  state_ = STATE_CONNECTING;
  int64 start_time = GetCurrentTimeMillis();
  bool shutting_down = false;
  while (!shutting_down) {
    DWORD wait_milliseconds;
    switch (state_) {
      case STATE_CONNECTING:
        wait_milliseconds =
            kMaximumConnectionTimeMilliseconds -
            static_cast<DWORD>(GetCurrentTimeMillis() - start_time);
        break;
      case STATE_ACQUIRING_FIX:
        wait_milliseconds =
            kMaximumConnectionTimeMilliseconds +
            kMaximumAcquistionTimeMilliseconds -
            static_cast<DWORD>(GetCurrentTimeMillis() - start_time);
        break;
      case STATE_FIX_ACQUIRED:
        wait_milliseconds = INFINITE;
        break;
      default:
        assert(false);
    }
 
    // It's possible that we were woken up just before the timer expired, in
    // which case the wait time may now be negative. In this case, set the
    // timeout to zero so that we wake up immediately and continue as normal.
    if (wait_milliseconds != INFINITE && wait_milliseconds < 0) {
      wait_milliseconds = 0;
    }

    DWORD event_index = WaitForMultipleObjects(ARRAYSIZE(events),
                                               events,
                                               false,
                                               wait_milliseconds);
    switch (event_index) {
      case WAIT_OBJECT_0:  // stop_event_
        shutting_down = true;
        break;
      case WAIT_OBJECT_0 + 1:  // position_update_event
        HandlePositionUpdate();
        break;
      case WAIT_OBJECT_0 + 2:  // state_change_event
        HandleStateChange();
        break;
      default:
        // Wait timed out.
        assert(wait_milliseconds != INFINITE);
        switch (state_) {
          case STATE_CONNECTING:
            listener_->GpsFatalError(Position::ERROR_CODE_POSITION_UNAVAILABLE,
                                     kFailedToConnectErrorMessage);
            break;
          case STATE_ACQUIRING_FIX:
            listener_->GpsFatalError(Position::ERROR_CODE_POSITION_UNAVAILABLE,
                                     kFailedToGetFixErrorMessage);
            break;
          default:
            assert(false);
        }
        shutting_down = true;
        break;
    }
  }

  if (GPSCloseDevice(gps_handle_) != ERROR_SUCCESS) {
    LOG(("Failed to close connection to GPS.\n"));
  }
}

void WinceGpsDevice::HandlePositionUpdate() {
  Position new_position;

  GPS_POSITION gps_position = {0};
  gps_position.dwSize = sizeof(gps_position);
  gps_position.dwVersion = GPS_VERSION_1;  // The only valid value
  if (GPSGetPosition(gps_handle_,
                     &gps_position,
                     kGpsDataMaximumAgeMilliseconds,
                     0) == ERROR_SUCCESS) {  // Reserved
    if ((gps_position.dwValidFields & GPS_VALID_LATITUDE) &&
        (gps_position.dwValidFields & GPS_VALID_LONGITUDE)) {
      new_position.latitude = gps_position.dblLatitude;
      new_position.longitude = gps_position.dblLongitude;
    }

    if (gps_position.dwValidFields & GPS_VALID_ALTITUDE_WRT_ELLIPSOID) {
      new_position.altitude = gps_position.flAltitudeWRTEllipsoid;
    }

    // The GPS Intermediate Driver API does not provide a numerical value for
    // the postition accuracy. However, we can estimate the accuracy from the
    // Position Dilution of Precision (PDOP), which describes how well the
    // geometric configuration of the satellites currently in view lends itself
    // to getting a high accuracy fix (see
    // http://en.wikipedia.org/wiki/Dilution_of_precision_(GPS) and
    // http://www.cgrer.uiowa.edu/cgrer_lab/gps/gpsdefs.html).
    if (gps_position.dwValidFields & GPS_VALID_POSITION_DILUTION_OF_PRECISION) {
      if (gps_position.flPositionDilutionOfPrecision <= 3) {
        new_position.accuracy = 1;
      } else if (gps_position.flPositionDilutionOfPrecision <= 6) {
        new_position.accuracy = 10;
      } else if (gps_position.flPositionDilutionOfPrecision <= 8) {
        new_position.accuracy = 20;
      } else {
        new_position.accuracy = 50;
      }
    }
  }

  GPS_DEVICE device_state = {0};
  device_state.dwSize = sizeof(device_state);
  device_state.dwVersion = GPS_VERSION_1;  // The only valid value
  if (GPSGetDeviceState(&device_state) == ERROR_SUCCESS) {
    if (device_state.ftLastDataReceived.dwHighDateTime != 0 ||
        device_state.ftLastDataReceived.dwLowDateTime != 0) {
      new_position.timestamp =
          FiletimeToMilliseconds(device_state.ftLastDataReceived);
    }
  }

  if (!new_position.IsGoodFix()) {
    return;
  }

  state_ = STATE_FIX_ACQUIRED;

  MutexLock lock(&position_mutex_);
  if (PositionsDiffer(position_, new_position)) {
    position_ = new_position;
    listener_->GpsPositionUpdateAvailable(position_);
  }
}

void WinceGpsDevice::HandleStateChange() {
  // It seems that if GPS is not present, or the GPS driver is disabled, we get
  // this callback exactly once, with hardware and driver states
  // SERVICE_STATE_ON.
  GPS_DEVICE device_state = {0};
  device_state.dwSize = sizeof(device_state);
  device_state.dwVersion = GPS_VERSION_1;  // The only valid value
  if (GPSGetDeviceState(&device_state) == ERROR_SUCCESS) {
    if (state_ == STATE_CONNECTING &&
        (device_state.dwDeviceState == SERVICE_STATE_STARTING_UP ||
         device_state.dwDeviceState == SERVICE_STATE_ON)) {
      state_ = STATE_ACQUIRING_FIX;
    }
  }
}

// Local function
// static
bool PositionsDiffer(const Position &position_1, const Position &position_2) {
  assert(position_2.IsGoodFix());

  if (!position_1.IsGoodFix()) {
    return true;
  }
  return position_1.latitude != position_2.latitude ||
         position_1.longitude != position_2.longitude ||
         position_1.accuracy != position_2.accuracy ||
         position_1.altitude != position_2.altitude;
}

#endif  // OS_WINCE
