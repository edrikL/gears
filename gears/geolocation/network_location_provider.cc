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

#include "gears/geolocation/network_location_provider.h"

#include "gears/base/common/event.h"
#include "gears/base/common/stopwatch.h"  // For GetCurrentTimeMillis
#include "gears/localserver/common/async_task.h"

// The maximum period of time we'll wait for a complete set of device data
// before sending the request.
static const int kDataCompleteWaitPeriod = 1000 * 5;  // 5 seconds

// This class implements an asynchronous wait. When the wait expires, it calls
// MakeRequest() on the NetworkLocationProvider object passed to the
// constructor.
class AsyncWait : public AsyncTask {
 public:
  explicit AsyncWait(NetworkLocationProvider *provider) : provider_(provider) {
    if (!Init()) {
      assert(false);
      return;
    }
    if (!Start()) {
      assert(false);
      return;
    }
  }
  // Instructs the thread to stop and delete the object when it has completed.
  // Waits for the Run method to complete.
  void StopThreadAndDelete() {
    stop_event_.Signal();
    run_complete_event_.Wait();
    DeleteWhenDone();
  }
  // Instructs the thread to stop and delete the object when it has completed.
  void StopThreadAndDeleteNoWait() {
    stop_event_.Signal();
    DeleteWhenDone();
  }
 private:
  // Private destructor. Callers use StopThreadAndDelete().
  ~AsyncWait() {}
  // AsyncTask implementation.
  virtual void Run() {
    if (!stop_event_.WaitWithTimeout(kDataCompleteWaitPeriod)) {
      // Timeout, try to make the request.
      provider_->MakeRequest();
    }
    run_complete_event_.Signal();
  }
 private:
  NetworkLocationProvider *provider_;
  Event stop_event_;
  Event run_complete_event_;
  DISALLOW_EVIL_CONSTRUCTORS(AsyncWait);
};

// NetworkLocationProvider

NetworkLocationProvider::NetworkLocationProvider(const std::string16 &url,
                                                 const std::string16 &host_name)
    : timestamp_(-1),
      is_radio_data_complete_(false),
      is_wifi_data_complete_(false),
      wait_(NULL) {
  request_ = NetworkLocationRequest::Create(url, host_name, this);
  assert(request_);
  // Get the device data providers. The first call to Register will create the
  // provider and it will be deleted by ref counting.
  radio_data_provider_ = DeviceDataProviderBase<RadioData>::Register(this);
  wifi_data_provider_ = DeviceDataProviderBase<WifiData>::Register(this);
}

NetworkLocationProvider::~NetworkLocationProvider() {
  if (request_) {
    request_->StopThreadAndDelete();
  }
  if (wait_) {
    wait_->StopThreadAndDelete();
  }
  radio_data_provider_->Unregister(this);
  wifi_data_provider_->Unregister(this);
}

// LocationProviderInterface implementation.

void NetworkLocationProvider::SetListener(
    LocationProviderInterface::ListenerInterface *listener) {
  assert(listener);
  listener_ = listener;
}

bool NetworkLocationProvider::GetCurrentPosition() {
  {
    MutexLock data_mutex_lock(&data_mutex_);
    // If we already have a complete set of device data, make the request.
    if (is_radio_data_complete_ && is_wifi_data_complete_) {
      if (!request_->MakeRequest(radio_data_, wifi_data_, timestamp_)) {
        // This should only fail if a request is already in progress. This
        // method should never be called when this is the case.
        LOG(("NetworkLocationProvider::GetCurrentPosition() : Failed to make "
             "request.\n"));
        assert(false);
        return false;
      }
      return true;
    }
  }
  // If not, launch a new thread which will wait for a certain maximum period
  // before making the request. If we receive all possible data before the wait
  // period expires, we make the request immediately.
  MutexLock wait_mutex_lock(&wait_mutex_);
  // We should never be called when we already have a wait in progress.
  assert(!wait_);
  wait_ = new AsyncWait(this);
  return true;
}

// DeviceDataProviderInterface::ListenerInterface implementation.

void NetworkLocationProvider::DeviceDataUpdateAvailable(
    DeviceDataProviderBase<RadioData> *provider) {
  MutexLock lock(&data_mutex_);
  assert(provider == radio_data_provider_);
  is_radio_data_complete_ = radio_data_provider_->GetData(&radio_data_);
  timestamp_ = GetCurrentTimeMillis();
  MakeRequestIfDataNowAvailable();
}

void NetworkLocationProvider::DeviceDataUpdateAvailable(
    DeviceDataProviderBase<WifiData> *provider) {
  assert(provider == wifi_data_provider_);
  MutexLock lock(&data_mutex_);
  is_wifi_data_complete_ = wifi_data_provider_->GetData(&wifi_data_);
  timestamp_ = GetCurrentTimeMillis();
  MakeRequestIfDataNowAvailable();
}

// NetworkLocationRequest::ListenerInterface implementation.

void NetworkLocationProvider::LocationResponseAvailable(
    const Position &position) {
  // If we have a listener, let them know that we now have a position available.
  if (listener_) {
    listener_->LocationUpdateAvailable(this, position);
  }
}

// Other methods.

bool NetworkLocationProvider::MakeRequest() {
  MutexLock data_mutex_lock(&data_mutex_);
  MutexLock wait_mutex_lock(&wait_mutex_);
  // Check that the wait object still exists. It is possible that while blocked
  // on the above mutexes, we received a callback from a device data provider
  // and have just deleted the wait object. In this case, there's nothing to do.
  if (wait_) {
    wait_ = NULL;
    if (-1 == timestamp_) {
      // If we've never received a callback from either the radio or wifi device
      // data provider, the timestamp won't be set. In this case, set it here.
      timestamp_ = GetCurrentTimeMillis();
    }
    if (!request_->MakeRequest(radio_data_, wifi_data_, timestamp_)) {
      LOG(("MakeRequest() : Failed to make position request.\n"));
      return false;
    }
  }
  return true;
}

bool NetworkLocationProvider::MakeRequestIfDataNowAvailable() {
  // If the device data is now complete, check to see if we're waiting for this.
  // If so, stop the waiting thread and make the request. Note that data_mutex_
  // should already be locked.
  MutexLock lock(&wait_mutex_);
  if (wait_ && is_radio_data_complete_ && is_wifi_data_complete_) {
    // Stop the wait thread and delete the object. We don't wait for the thread
    // to complete because it may be blocked on wait_mutex_ in MakeRequest().
    wait_->StopThreadAndDeleteNoWait();
    wait_ = NULL;
    if (!request_->MakeRequest(radio_data_, wifi_data_, timestamp_)) {
      LOG(("MakeRequestIfDataNowAvailable() : Failed to make position "
           "request.\n"));
      return false;
    }
  }
  return true;
}
