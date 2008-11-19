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

// GPS is only available on WinCE and Android.
// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented
#if defined(OS_WINCE) || defined(ANDROID)

#include "gears/geolocation/gps_location_provider.h"

#include <math.h>  // For fabs()
#include "gears/base/common/stopwatch.h"
#include "gears/geolocation/backoff_manager.h"

// Local function
// This assumes that position_2 is a good fix.
static bool PositionsDifferSiginificantly(const Position &position_1,
                                          const Position &position_2);

// Static factory function for GPS device. Initialize to default value.
// static
GpsLocationProvider::GpsDeviceFactoryFunction
    GpsLocationProvider::gps_device_factory_function_ =
    GpsLocationProvider::NewGpsDevice;

LocationProviderBase *NewGpsLocationProvider(
    BrowsingContext *browsing_context,
    const std::string16 &reverse_geocode_url,
    const std::string16 &host_name,
    const std::string16 &address_language) {
  return new GpsLocationProvider(browsing_context,
                                 reverse_geocode_url,
                                 host_name,
                                 address_language);
}


GpsLocationProvider::GpsLocationProvider(
    BrowsingContext *browsing_context,
    const std::string16 &reverse_geocode_url,
    const std::string16 &host_name,
    const std::string16 &address_language)
    : reverse_geocode_url_(reverse_geocode_url),
      host_name_(host_name),
      address_language_(address_language),
      is_shutting_down_(false),
      is_new_position_available_(false),
      is_new_reverse_geocode_available_(false),
      is_new_listener_waiting_(false),
      reverse_geocoder_(NULL),
      earliest_reverse_geocode_time_(0),
      is_address_requested_(false),
      browsing_context_(browsing_context),
      gps_device_(NULL) {
  // Open the device
  gps_device_.reset((*gps_device_factory_function_)(this));
  // Start the worker thread.
  Start();
}

GpsLocationProvider::~GpsLocationProvider() {
  // Shut down the worker thread.
  is_shutting_down_ = true;
  worker_event_.Signal();
  Join();
}

void GpsLocationProvider::RegisterListener(
    LocationProviderBase::ListenerInterface *listener,
    bool request_address) {
  assert(listener);

  // Lazily create the reverse geocoder.
  if (request_address && reverse_geocoder_ == NULL) {
    if (!reverse_geocode_url_.empty()) {
      reverse_geocoder_.reset(new ReverseGeocoder(reverse_geocode_url_,
                                                  host_name_,
                                                  address_language_,
                                                  this));
    }
    // Ignore the request_address flag if we fail to create a reverse geocoder.
    if (reverse_geocoder_ == NULL) {
      request_address = false;
    }
  }

  if (request_address) {
    MutexLock lock(&address_mutex_);
    new_listeners_requiring_address_.push_back(listener);
    is_address_requested_ = true;
  } else {
    LocationProviderBase::RegisterListener(listener, false);
  }

  is_new_listener_waiting_ = true;
  worker_event_.Signal();
}

void GpsLocationProvider::UnregisterListener(
    LocationProviderBase::ListenerInterface *listener) {
  assert(listener);

  // First try removing the listener from the set of new listeners waiting for
  // an address. Otherwise, try the regular listeners.
  MutexLock lock(&address_mutex_);
  ListenerVector::iterator iter =
      std::find(new_listeners_requiring_address_.begin(),
                new_listeners_requiring_address_.end(),
                listener);
  if (iter != new_listeners_requiring_address_.end()) {
    new_listeners_requiring_address_.erase(iter);
  } else {
    LocationProviderBase::UnregisterListener(listener);
  }

  // Update whether or not we need to request an address.
  if (is_address_requested_) {
    MutexLock listeners_lock(GetListenersMutex());
    ListenerMap *listeners = GetListeners();
    for (ListenerMap::const_iterator iter = listeners->begin();
         iter != listeners->end();
         iter++) {
      if (iter->second.first == true) {
        return;
      }
    }
    is_address_requested_ = false;
  }
}

// LocationProviderBase implementation
void GpsLocationProvider::GetPosition(Position *position) {
  assert(position);
  MutexLock lock(&position_mutex_);
  *position = position_;
}

// static
void GpsLocationProvider::SetGpsDeviceFactoryFunction(
    GpsDeviceFactoryFunction gps_device_factory_function) {
  gps_device_factory_function_ = gps_device_factory_function;
}

// Thread implementation
void GpsLocationProvider::Run() {

  int reverse_geocode_wait_time;
  bool is_reverse_geocode_complete = true;
  bool is_reverse_geocode_wait_expired = true;
  bool is_reverse_geocode_required = false;
  bool is_listener_callback_required  = false;

  while (true) {
    // Update the minimum wait time for a reverse geocode request.
    earliest_reverse_geocode_time_mutex_.Lock();
    if (earliest_reverse_geocode_time_ == 0) {
      reverse_geocode_wait_time = 0;
    } else {
      reverse_geocode_wait_time =
          static_cast<int>(earliest_reverse_geocode_time_ -
                           GetCurrentTimeMillis());
    }
    earliest_reverse_geocode_time_mutex_.Unlock();

    // Wait for an event.
    if (reverse_geocode_wait_time > 0) {
      if (!worker_event_.WaitWithTimeout(reverse_geocode_wait_time)) {
        // Wait timed out.
        is_reverse_geocode_wait_expired = true;
      }
    } else {
      worker_event_.Wait();
    }

    // Handle a new listener
    if (is_new_listener_waiting_) {
      is_new_listener_waiting_ = false;
      // See if we have any new listeners that require an address.
      bool new_listeners_require_address = false;
      address_mutex_.Lock();
      while (!new_listeners_requiring_address_.empty()) {
        LocationProviderBase::RegisterListener(
            new_listeners_requiring_address_.back(), true);
        new_listeners_requiring_address_.pop_back();
        new_listeners_require_address = true;
      }
      address_mutex_.Unlock();

      // If we have a good position but no address and we need one, we should
      // start a new reverse geocode, unless one is already in progress. If we
      // have a good position and it includes an address if required, we should
      // call back. In all other cases, do nothing, as the listener will be
      // updated once we have a good position fix or when the current reverse
      // geocode completes.
      MutexLock lock(&position_mutex_);
      if (position_.IsGoodFix()) {
        // The position is good.
        if (new_listeners_require_address && !position_.IncludesAddress()) {
          // The new listener requires an address but the current
          // position doesn't have one.
          if (is_reverse_geocode_complete) {
            // There isn't a reverse geocode in progress, so start a
            // new one.
            is_reverse_geocode_required = true;
          }
          // There is a reverse geocode in progress, so do nothing
          // (i.e. wait for it to finish. The new listener will be
          // updated at that point).
        } else {
          // The new listener does not require an address or it
          // does require an address and the current position includes
          // one. In such a case, we call back immmediately.
          is_listener_callback_required = true;
        }
      } else if (position_.IsInitialized()) {
        // The position denotes an error (it is initialized but it is
        // not good). We therefore call back immediately.
        is_listener_callback_required = true;
      }
    }

    // Handle a position update from the GPS.
    if (is_new_position_available_) {
      is_new_position_available_ = false;
      MutexLock postion_lock(&position_mutex_);
      MutexLock address_lock(&address_mutex_);
      assert(new_position_.IsInitialized());
      if (!new_position_.IsGoodFix()) {
        // The new position represents an error. We call back to let
        // the listener know about this.
        position_ = new_position_;
        is_listener_callback_required = true;
      } else if (PositionsDifferSiginificantly(position_, new_position_) &&
                 is_address_requested_) {
        // The positions differ significantly and an address was requested, so
        // we need a new reverse geocode. Don't update position_ because we
        // don't want it to change if a reverse geocode is currently in
        // progress.
        is_reverse_geocode_required = true;
      } else {
        // The position changed, but we don't need a new reverse geocode, so
        // update our best position estimate but use the exisiting address.
        Address address = position_.address;
        position_ = new_position_;
        position_.address = address;
        is_listener_callback_required = true;
      }
    }

    // Handle a reverse geocode.
    if (is_new_reverse_geocode_available_) {
      is_new_reverse_geocode_available_ = false;
      is_listener_callback_required = true;
    }

    // See if we need to callack to listeners.
    if (is_listener_callback_required) {
      is_listener_callback_required = false;
      UpdateListeners();
    }

    // See if we need to quit. Note that this must come after call to
    // UpdateListeners to handle case of fatal error.
    if (is_shutting_down_) {
      break;
    }

    // Se if we need to make a new reverse geocode request.
    if (is_reverse_geocode_required &&
        is_reverse_geocode_complete &&
        is_reverse_geocode_wait_expired) {
      is_reverse_geocode_required = false;
      is_reverse_geocode_complete = false;
      is_reverse_geocode_wait_expired = false;

      // We delay updating position_ until to now to make sure that position_ is
      // kept in sync with our latest reverse geocode. Now that we are starting
      // a new reverse geocode, we can update position_.
      position_mutex_.Lock();
      position_ = new_position_;
      position_mutex_.Unlock();

      MakeReverseGeocodeRequest();
    }
  }
}

// ReverseGeocoder::ListenerInterface implementation
void GpsLocationProvider::ReverseGeocodeAvailable(const Position &position,
                                                  bool server_error) {
  // Update the earliest next request time for a reverse geocode request.
  earliest_reverse_geocode_time_mutex_.Lock();
  earliest_reverse_geocode_time_ =
      BackoffManager::ReportResponse(reverse_geocode_url_, server_error);
  earliest_reverse_geocode_time_mutex_.Unlock();

  // Copy only the address, so as to preserve the position if an update has been
  // made since the reverse geocode started.
  position_mutex_.Lock();
  position_.address = position.address;
  position_mutex_.Unlock();

  is_new_reverse_geocode_available_ = true;
  worker_event_.Signal();
}

// GpsDeviceBase::ListenerInterface implementation
void GpsLocationProvider::GpsFatalError(const Position::ErrorCode &code,
                                        const std::string16 &message) {
  assert(code > 0);

  // Update the listeners then terminate the worker thread.
  Position position;
  position.error_code = code;
  position.error_message = message;
  position_mutex_.Lock();
  new_position_ = position;
  position_mutex_.Unlock();

  is_new_position_available_ = true;
  worker_event_.Signal();
}

void GpsLocationProvider::GpsPositionUpdateAvailable(const Position &position) {
  // This should only be called with a new, good position.
  assert(position.IsGoodFix());

  // We don't update position_ here, because we may want to use the new position
  // for a reverse geocode, in which case position_ is not updated until the
  // reverse geocode starts.
  position_mutex_.Lock();
  new_position_ = position;
  position_mutex_.Unlock();

  is_new_position_available_ = true;
  worker_event_.Signal();
}

void GpsLocationProvider::MakeReverseGeocodeRequest() {
  BackoffManager::ReportRequest(reverse_geocode_url_);
  assert(position_.IsGoodFix());

  // We must make all requests from the same thread - the run loop.
  // Note that this will fail if a request is already in progress.
  assert(reverse_geocoder_.get());
  if (!reverse_geocoder_->MakeRequest(browsing_context_, position_)) {
    assert(false);
  }
}

// Local function
// static
bool PositionsDifferSiginificantly(const Position &position_1,
                                   const Position &position_2) {
  assert(position_2.IsGoodFix());

  if (!position_1.IsGoodFix()) {
    return true;
  }
  double delta = fabs(position_1.latitude - position_2.latitude) +
                 fabs(position_1.longitude - position_2.longitude);
  // Convert to metres. 1 second of arc of latitude (or longitude at the
  // equator) is 1 nautical mile or 1852m.
  delta *= 60 * 1852;
  return delta > 100;
}

#endif  // defined(OS_WINCE) || defined(ANDROID)
