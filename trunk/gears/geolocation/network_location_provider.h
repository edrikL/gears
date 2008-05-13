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

#ifndef GEARS_GEOLOCATION_NETWORK_LOCATION_PROVIDER_H__
#define GEARS_GEOLOCATION_NETWORK_LOCATION_PROVIDER_H__

// NetworkLocationProvider is currently only implemented for Win32.
#ifdef WIN32

#include "gears/base/common/common.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/string16.h"
#include "gears/geolocation/device_data_provider.h"
#include "gears/geolocation/location_provider.h"
#include "gears/geolocation/network_location_request.h"

// An implementation detail of NetworkLocationProvider. Defined in
// network_location_provider.cc
class AsyncWait;

class NetworkLocationProvider
    : public LocationProviderInterface,
      public DeviceDataProviderBase<RadioData>::ListenerInterface,
      public DeviceDataProviderBase<WifiData>::ListenerInterface,
      public NetworkLocationRequest::ListenerInterface {
 public:
  friend class AsyncWait;

  NetworkLocationProvider(const std::string16 &url,
                          const std::string16 &host_name);
  virtual ~NetworkLocationProvider();

  // LocationProviderInterface implementation.
  virtual void SetListener(
      LocationProviderInterface::ListenerInterface *listener);
  virtual bool GetCurrentPosition();

 private:
  // DeviceDataProviderBase::ListenerInterface implementation.
  virtual void DeviceDataUpdateAvailable(
      DeviceDataProviderBase<RadioData> *provider);
  virtual void DeviceDataUpdateAvailable(
      DeviceDataProviderBase<WifiData> *provider);

  // NetworkLocationRequest::ListenerInterface implementation.
  virtual void LocationResponseAvailable(const Position &position);

  // Used by the AsyncWait object to make the request when its wait expires.
  bool MakeRequest();
  // Used by the device data provider callbacks to make the request if all
  // device data is now present and if we're waiting for this to be the case.
  bool MakeRequestIfDataNowAvailable();

  NetworkLocationRequest *request_;
  LocationProviderInterface::ListenerInterface *listener_;
  RadioData radio_data_;
  WifiData wifi_data_;
  Mutex data_mutex_;
  DeviceDataProviderBase<RadioData> *radio_data_provider_;
  DeviceDataProviderBase<WifiData> *wifi_data_provider_;
  // The timestamp for the latest device data update.
  int64 timestamp_;
  bool is_radio_data_complete_;
  bool is_wifi_data_complete_;
  AsyncWait *wait_;
  Mutex wait_mutex_;
  DISALLOW_EVIL_CONSTRUCTORS(NetworkLocationProvider);
};

#endif  // WIN32

#endif  // GEARS_GEOLOCATION_NETWORK_LOCATION_PROVIDER_H__
