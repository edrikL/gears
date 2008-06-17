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
// The Geolocation API has not been finalized for official builds.
#else

#include "gears/geolocation/network_location_provider.h"

#include "gears/base/common/event.h"
#include "gears/base/common/stopwatch.h"  // For GetCurrentTimeMillis

// The maximum period of time we'll wait for a complete set of device data
// before sending the request.
static const int kDataCompleteWaitPeriod = 1000 * 5;  // 5 seconds
// The minimum period between network requests.
static const int kMinimumRequestInterval = 1000 * 5;  // 5 seconds

NetworkLocationProvider::NetworkLocationProvider(const std::string16 &url,
                                                 const std::string16 &host_name)
    : is_shutting_down_(false),
      url_(url),
      host_name_(host_name) {
  // Get the device data providers. The first call to Register will create the
  // provider and it will be deleted by ref counting.
  radio_data_provider_ = DeviceDataProviderBase<RadioData>::Register(this);
  wifi_data_provider_ = DeviceDataProviderBase<WifiData>::Register(this);

  // Currently, request_address and address_language are properties of a given
  // fix request, not of a location provider.
  // TODO(steveblock): Determine the best way to pass these parameters to the
  // request.
  request_address_ = false;
  address_language_ = STRING16(L"");

  // Start the worker thread
  Start();
}

NetworkLocationProvider::~NetworkLocationProvider() {
  if (request_) {
    request_->StopThreadAndDelete();
  }

  // Shut down the worker thread
  is_shutting_down_ = true;
  thread_notification_event_.Signal();
  Join();

  radio_data_provider_->Unregister(this);
  wifi_data_provider_->Unregister(this);
}

// LocationProviderBase implementation
void NetworkLocationProvider::GetPosition(Position *position) {
  assert(position);
  MutexLock lock(&position_mutex_);
  *position = position_;
}

// DeviceDataProviderInterface::ListenerInterface implementation.
void NetworkLocationProvider::DeviceDataUpdateAvailable(
    DeviceDataProviderBase<RadioData> *provider) {
  MutexLock lock(&data_mutex_);
  assert(provider == radio_data_provider_);
  is_radio_data_complete_ = radio_data_provider_->GetData(&radio_data_);

  DeviceDataUpdateAvailableImpl();
}

void NetworkLocationProvider::DeviceDataUpdateAvailable(
    DeviceDataProviderBase<WifiData> *provider) {
  assert(provider == wifi_data_provider_);
  MutexLock lock(&data_mutex_);
  is_wifi_data_complete_ = wifi_data_provider_->GetData(&wifi_data_);

  DeviceDataUpdateAvailableImpl();
}

// NetworkLocationRequest::ListenerInterface implementation.
void NetworkLocationProvider::LocationResponseAvailable(
    const Position &position) {
  // Cache the position
  position_mutex_.Lock();
  position_ = position;
  position_mutex_.Unlock();

  // Let listeners know that we now have a position available.
  UpdateListeners();

  // Signal to the worker thread that this request has completed.
  is_last_request_complete_ = true;
  thread_notification_event_.Signal();
}

// Thread implementation
void NetworkLocationProvider::Run() {
  // Create the network request object. We must do this on the same thread from
  // which we'll call Start().
  request_ = NetworkLocationRequest::Create(url_, host_name_, this);
  if (!request_) {
    LOG(("Failed to create NetworkLocationRequest object.\n"));
    assert(false);
    return;
  }

  // Get the device data.
  data_mutex_.Lock();
  is_radio_data_complete_ = radio_data_provider_->GetData(&radio_data_);
  is_wifi_data_complete_ = wifi_data_provider_->GetData(&wifi_data_);
  timestamp_ = GetCurrentTimeMillis();

  // For the first request, wait for a certain maximum time period to get as
  // much device data as possible.
  int64 start_time = timestamp_;
  while (true) {
    if (is_radio_data_complete_ && is_wifi_data_complete_) {
      data_mutex_.Unlock();
      break;
    }
    data_mutex_.Unlock();

    int64 elapsed_time = GetCurrentTimeMillis() - start_time;
    int timeout = kDataCompleteWaitPeriod - static_cast<int>(elapsed_time);
    if (timeout <= 0) {
      break;
    }
    if (!thread_notification_event_.WaitWithTimeout(timeout)) {
      // Quit waiting if we time out.
      break;
    }
    // Terminate the thread if requested.
    if (is_shutting_down_) {
      return;
    }
    // The event should be due to new device data.
    assert(is_new_data_available_);
    // Lock the data mutex to test is_radio_data_complete_ and
    // is_wifi_data_complete_ on the next loop.
    data_mutex_.Lock();
  }

  MakeRequest();

  // Loop continually, making requests whenever new data becomes available,
  // subject to the minimum interval.
  //
  // This loop is structured such that we don't require mutex locks to
  // synchronise changes to is_new_data_available_ etc with signals on
  // thread_notification_event_.
  int64 last_request_time = GetCurrentTimeMillis();
  bool minimum_time_elapsed = false;
  while (!is_shutting_down_) {
    if (minimum_time_elapsed) {
      // If the minimum time period has elapsed, just wait for the event.
      thread_notification_event_.Wait();
    } else {
      // If the minimum time period has not yet elapsed, set the timeout such
      // that the wait times out kMinimumRequestInterval after the last request
      // was made.
      int64 elapsed_time = GetCurrentTimeMillis() - last_request_time;
      int timeout = kMinimumRequestInterval - static_cast<int>(elapsed_time);
      // If the last call to WaitWithTimeout was interrupted by the event just
      // before the time expired, timeout may be non-positive. In this case, we
      // should try again to make the request without waiting.
      if (timeout <= 0 ) {
        minimum_time_elapsed = true;
      } else {
        if (!thread_notification_event_.WaitWithTimeout(timeout)) {
          minimum_time_elapsed = true;
        }
      }
    }
    // If the thread is not shutting down, we have new data, the last request
    // has completed, and the minimum time has elapsed, make the next request.
    //
    // TODO(steveblock): If the request does not complete within some maximum
    // time period, we should kill it and start a new request.
    if (!is_shutting_down_ &&
        is_new_data_available_ &&
        is_last_request_complete_ &&
        minimum_time_elapsed) {
      MakeRequest();
      last_request_time = GetCurrentTimeMillis();
      minimum_time_elapsed = false;
    }
  }
}

// Other methods

bool NetworkLocationProvider::MakeRequest() {
  is_new_data_available_ = false;
  is_last_request_complete_ = false;
  return request_->MakeRequest(radio_data_,
                               wifi_data_,
                               request_address_,
                               address_language_,
                               -1000,  // We don't have a position to pass to
                               -1000,  // the server.
                               timestamp_);
}

void NetworkLocationProvider::DeviceDataUpdateAvailableImpl() {
  timestamp_ = GetCurrentTimeMillis();

  // Signal to the worker thread that new data is available.
  is_new_data_available_ = true;
  thread_notification_event_.Signal();
}

#endif  // OFFICIAL_BUILD
