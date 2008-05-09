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

// NetworkLocationProvider is currently only implemented for Win32.
#ifdef WIN32

#include "gears/geolocation/network_location_provider.h"

NetworkLocationProvider::NetworkLocationProvider(
    const std::string16 &url, const std::string16 &host_name) {
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
  // TODO(steveblock): Implement me.
  assert(false);
  return false;
}

// DeviceDataProviderInterface::ListenerInterface implementation.

void NetworkLocationProvider::DeviceDataUpdateAvailable(
    DeviceDataProviderBase<RadioData> *provider) {
  // TODO(steveblock): Implement me.
  assert(false);
}

void NetworkLocationProvider::DeviceDataUpdateAvailable(
    DeviceDataProviderBase<WifiData> *provider) {
  // TODO(steveblock): Implement me.
  assert(false);
}

// NetworkLocationRequest::ListenerInterface implementation.

void NetworkLocationProvider::LocationResponseAvailable(
    const Position &position) {
  // If we have a listener, let them know that we now have a position available.
  if (listener_) {
    listener_->LocationUpdateAvailable(this, position);
  }
}

#endif  // WIN32
