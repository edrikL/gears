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
// A device data provider provides data from the device that is used by a
// NetworkLocationProvider to obtain a position fix. This data may be either
// cell radio data or wifi data. For a given type of data, we use a singleton
// instance of the device data provider, which is used by multiple
// NetworkLocationProvider objects.
//
// This file implements a base class to be used by all device data providers.
// Primarily, this class declares interface methods to be implemented by derived
// classes. It also provides static methods to access the singleton instance and
// implements ref-counting for the singleton.
//
// This file also declares the data structures used to represent cell radio data
// and wifi data.

#ifndef GEARS_GEOLOCATION_DEVICE_DATA_PROVIDER_H__
#define GEARS_GEOLOCATION_DEVICE_DATA_PROVIDER_H__

#include <algorithm>
#include <set>
#include <vector>
#include "gears/base/common/basictypes.h"  // For int64
#include "gears/base/common/common.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/scoped_refptr.h"  // For RefCount
#include "gears/base/common/string16.h"

// The following data structures are used to store cell radio data and wifi
// data. See the Geolocation API design document at
// http://code.google.com/p/google-gears/wiki/LocationAPI for a more complete
// description.
//
// For all integer fields, we use kint32min to represent unknown values.

// Cell radio data relating to a single cell tower.
struct CellData {
  CellData()
      : cell_id(kint32min),
        location_area_code(kint32min),
        mobile_network_code(kint32min),
        mobile_country_code(kint32min),
        age(kint32min),
        radio_signal_strength(kint32min),
        timing_advance(kint32min) {}
  bool Matches(const CellData &other) const {
    // Ignore age and radio_signal_strength when matching.
    return cell_id == other.cell_id &&
           location_area_code == other.location_area_code &&
           mobile_network_code == other.mobile_network_code &&
           mobile_country_code == other.mobile_country_code &&
           timing_advance == other.timing_advance;
  }

  int cell_id;                // Unique identifier of the cell
  int location_area_code;     // For current location area
  int mobile_network_code;    // For current cell
  int mobile_country_code;    // For current cell
  int age;                    // Milliseconds since this cell was primary
  int radio_signal_strength;  // Measured in dBm.
  int timing_advance;         // Timing advance representing the distance from
                              // the cell tower. Each unit is roughly 550
                              // meters.
};

static bool CellDataMatches(const CellData &data1, const CellData &data2) {
  return data1.Matches(data2);
}

enum RadioType {
  RADIO_TYPE_UNKNOWN,
  RADIO_TYPE_GSM,
  RADIO_TYPE_CDMA,
  RADIO_TYPE_WCDMA,
};

// All data for the cell radio.
struct RadioData {
  RadioData()
      : home_mobile_network_code(kint32min),
        home_mobile_country_code(kint32min),
        radio_type(RADIO_TYPE_UNKNOWN) {}
  bool Matches(const RadioData &other) const {
    if (cell_data.size() != other.cell_data.size()) {
      return false;
    }
    if (!std::equal(cell_data.begin(), cell_data.end(), other.cell_data.begin(),
                    CellDataMatches)) {
      return false;
    }
    return device_id == other.device_id &&
           home_mobile_network_code == other.home_mobile_network_code &&
           home_mobile_country_code == other.home_mobile_country_code &&
           radio_type == other.radio_type &&
           carrier == other.carrier;
  }

  std::string16 device_id;
  std::vector<CellData> cell_data;
  int home_mobile_network_code;  // For the device's home network.
  int home_mobile_country_code;  // For the device's home network.
  RadioType radio_type;          // Mobile radio type.
  std::string16 carrier;         // Carrier name.
};

// Wifi data relating to a single access point.
struct AccessPointData {
  AccessPointData()
      : radio_signal_strength(kint32min),
        age(kint32min),
        channel(kint32min),
        signal_to_noise(kint32min) {}
  bool Matches(const AccessPointData &other) const {
    // Ignore radio_signal_strength, age and signal_to_noise when matching.
    return mac_address == other.mac_address &&
           channel == other.channel &&
           ssid == other.ssid;
  }

  std::string16 mac_address;
  int radio_signal_strength; // Measured in dBm
  int age;              // Milliseconds since this access point was detected
  int channel;
  int signal_to_noise;  // Ratio in dB
  std::string16 ssid;   // Network identifier
};

static bool AccessPointDataMatches(const AccessPointData &data1,
                                   const AccessPointData &data2) {
  return data1.Matches(data2);
}

// All data for wifi. 
struct WifiData {
  bool Matches(const WifiData &other) const {
    if (access_point_data.size() != other.access_point_data.size()) {
      return false;
    }
    if (!std::equal(access_point_data.begin(), access_point_data.end(),
                    other.access_point_data.begin(), AccessPointDataMatches)) {
      return false;
    }
    return true;
  }

  std::vector<AccessPointData> access_point_data;
};

// A base class for all device data providers. This class is templated on the
// type of data it provides.
//
// This class implements reference counting to allow a single device data
// provider to be shared by multiple network location providers.
template<typename DataType>
class DeviceDataProviderBase {
 public:
  // Interface to be implemented by listeners to a device data provider.
  class ListenerInterface {
   public:
    virtual void DeviceDataUpdateAvailable(
        DeviceDataProviderBase<DataType> *provider) = 0;
    virtual ~ListenerInterface() {}
  };

  // Adds a listener, which will be called back with DeviceDataUpdateAvailable
  // whenever new data is available. Returns the singleton instance.
  static DeviceDataProviderBase *Register(ListenerInterface *listener) {
    // We protect against Register and Unregister being called asynchronously
    // from different threads. This is the case when a device data provider is
    // used by a NetworkLocationProvider object. Register is always called from
    // the JavaScript thread. Unregister is called when NetworkLocationProvider
    // objects are destructed, which happens asynchronously once the
    // NetworkLocationProvider HTTP request has completed.
    MutexLock mutex(&instance_mutex);
    if (!instance) {
      instance = Create();
    }
    assert(instance);
    instance->Ref();
    instance->AddListener(listener);
    return instance;
  }

  // Removes a listener. If this is the last listener, deletes the singleton
  // instance. Return value indicates success.
  static bool Unregister(ListenerInterface *listener) {
    MutexLock mutex(&instance_mutex);
    if (!instance->RemoveListener(listener)) {
      return false;
    }
    if (instance->Unref()) {
      delete instance;
      instance = NULL;
    }
    return true;
  }

  // Provides whatever data the provider has, which may be nothing. Return
  // value indicates whether this is all the data the provider could ever
  // obtain.
  virtual bool GetData(DataType *data) = 0;

 protected:
  // Protected constructor and destructor, callers access singleton through
  // Register and Unregister.
  DeviceDataProviderBase() {}
  virtual ~DeviceDataProviderBase() {}

  // Provides access to listeners for use in derived classes.
  typedef std::set<ListenerInterface*> ListenersSet;
  ListenersSet *listeners() {
    return &listeners_;
  }
  Mutex *listeners_mutex() {
    return &listeners_mutex_;
  }

  // Calls DeviceDataUpdateAvailable() on all registered listeners.
  void NotifyListeners() {
  MutexLock lock(&listeners_mutex_);
    for (typename ListenersSet::const_iterator iter = listeners_.begin();
         iter != listeners_.end();
         ++iter) {
      (*iter)->DeviceDataUpdateAvailable(this);
    }
  }

 private:
  void Ref() {
    count_.Ref();
  }
  // Returns true when the ref count transitions from 1 to 0.
  bool Unref() {
    return count_.Unref();
  }

  void AddListener(ListenerInterface *listener) {
    MutexLock mutex(&listeners_mutex_);
    listeners_.insert(listener);
  }
  bool RemoveListener(ListenerInterface *listener) {
    MutexLock mutex(&listeners_mutex_);
    typename ListenersSet::iterator iter = find(listeners_.begin(),
                                                listeners_.end(),
                                                listener);
    if (iter == listeners_.end()) {
      return false;
    }
    listeners_.erase(iter);
    return true;
  }

  // Factory constructor.
  static DeviceDataProviderBase* Create();

  // The singleton instance of this class and its mutex.
  static DeviceDataProviderBase *instance;
  static Mutex instance_mutex;
  RefCount count_;

  // The listeners to this class and their mutex.
  ListenersSet listeners_;
  Mutex listeners_mutex_;
  DISALLOW_EVIL_CONSTRUCTORS(DeviceDataProviderBase);
};

// static
template <typename DataType>
Mutex DeviceDataProviderBase<DataType>::instance_mutex;

// static
template <typename DataType>
DeviceDataProviderBase<DataType>* DeviceDataProviderBase<DataType>::instance =
    NULL;

#endif  // GEARS_GEOLOCATION_DEVICE_DATA_PROVIDER_H__
