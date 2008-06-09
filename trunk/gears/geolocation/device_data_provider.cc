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
// This file implements mock device data providers and the static methods used
// to access the singleton instance of the device data provider.

#ifdef OFFICIAL_BUILD
// The Geolocation API has not been finalized for official builds.
#else

#include "gears/geolocation/device_data_provider.h"

typedef DeviceDataProviderBase<RadioData> RadioDataProviderBase;
typedef DeviceDataProviderBase<WifiData> WifiDataProviderBase;

#if USING_MOCK_DEVICE_DATA_PROVIDERS

#include <assert.h>
#include "gears/base/common/event.h"
#include "gears/geolocation/thread.h"

// A mock implementation of DeviceDataProviderBase for testing. It simply calls
// back once every second with constant data.
template<typename DataType>
class MockDeviceDataProvider
    : public DeviceDataProviderBase<DataType>,
      public Thread {
 protected:
  // Protected constructor and destructor, callers access singleton through
  // Register and Unregister.
  MockDeviceDataProvider() {
    Start();
  }
  virtual ~MockDeviceDataProvider() {
    stop_event_.Signal();
    Join();
  }

 private:
  // Thread implementation.
  virtual void Run() {
    while (!stop_event_.WaitWithTimeout(1000)) {
      NotifyListeners();
    }
  }

  Event stop_event_;
  DISALLOW_EVIL_CONSTRUCTORS(MockDeviceDataProvider);
};

class MockRadioDataProvider : public MockDeviceDataProvider<RadioData> {
 private:
  // Allow RadioDataProviderBase::Create() to access our private constructor.
  friend class RadioDataProviderBase;

  MockRadioDataProvider() {}
  virtual ~MockRadioDataProvider() {}

  // DeviceDataProviderBase<RadioData> implementation.
  virtual bool GetData(RadioData *data) {
    assert(data);
    // The following are real-world values, captured in May 2008.
    CellData cell_data;
    cell_data.cell_id = 23874;
    cell_data.location_area_code = 98;
    cell_data.mobile_network_code = 15;
    cell_data.mobile_country_code = 234;
    cell_data.radio_signal_strength = -65;
    data->cell_data.clear();
    data->cell_data.push_back(cell_data);
    data->radio_type = RADIO_TYPE_GSM;
    // We always have all the data we can get, so return true.
    return true;
  }
  DISALLOW_EVIL_CONSTRUCTORS(MockRadioDataProvider);
};

class MockWifiDataProvider : public MockDeviceDataProvider<WifiData> {
 private:
  // Allow WifiDataProviderBase::Create() to access our private constructor.
  friend class WifiDataProviderBase;

  MockWifiDataProvider() {}
  virtual ~MockWifiDataProvider() {}

  // DeviceDataProviderBase<WifiData> implementation.
  virtual bool GetData(WifiData *data) {
    assert(data);
    // The following are real-world values, captured in May 2008.
    AccessPointData access_point_data;
    data->access_point_data.clear();
    access_point_data.mac_address = STRING16(L"00-0b-86-ca-bb-c8");
    data->access_point_data.push_back(access_point_data);
    access_point_data.mac_address = STRING16(L"00-0b-86-ca-bb-c9");
    data->access_point_data.push_back(access_point_data);
    access_point_data.mac_address = STRING16(L"00-0b-86-ce-51-80");
    data->access_point_data.push_back(access_point_data);
    access_point_data.mac_address = STRING16(L"00-0d-97-04-85-9d");
    data->access_point_data.push_back(access_point_data);
    // We always have all the data we can get, so return true.
    return true;
  }
  DISALLOW_EVIL_CONSTRUCTORS(MockWifiDataProvider);
};

// static
template <>
RadioDataProviderBase *RadioDataProviderBase::Create() {
  return new MockRadioDataProvider();
}

// static
template <>
WifiDataProviderBase *WifiDataProviderBase::Create() {
  return new MockWifiDataProvider();
}

#else  // USING_MOCK_DEVICE_DATA_PROVIDERS

#if defined(WINCE)
// WinCE uses WinceWifiDataProvider.
#elif defined(WIN32) && !defined(WINCE)
// Win32 uses Win32WifiDataProvider.
#elif defined(LINUX)
// Linux uses LinuxWifiDataProvider.
#else
// static
template <>
WifiDataProviderBase* WifiDataProviderBase::Create() {
  // Temporarily implement this method here to avoid link errors.
  // TODO(steveblock): Implement WifiDataProviderBase for other platforms.
  assert(false);
  return NULL;
}
#endif

#endif  // USING_MOCK_DEVICE_DATA_PROVIDERS

#endif  // OFFICIAL_BUILD
