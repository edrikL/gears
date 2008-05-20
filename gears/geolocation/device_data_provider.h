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

#ifndef GEARS_GEOLOCATION_DEVICE_DATA_PROVIDER_H__
#define GEARS_GEOLOCATION_DEVICE_DATA_PROVIDER_H__

#include <set>
#include <vector>
#include "gears/base/common/basictypes.h"  // For int64
#include "gears/base/common/common.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/scoped_refptr.h"  // For RefCount
#include "gears/base/common/string16.h"

// For all integer fields, we use kint32min to represent unknown.
//
// See the Geolocation API design document at
// http://code.google.com/p/google-gears/wiki/LocationAPI for a description of
// these fields.

struct CellData {
  CellData() : cid(kint32min), lac(kint32min), mnc(kint32min), mcc(kint32min),
               age(kint32min), rss(kint32min), adv(kint32min) {}
  bool Matches(const CellData &other) const {
    // Ignore signal strength and age when matching.
    return cid == other.cid && lac == other.lac && mnc == other.mnc &&
           mcc == other.mcc && adv == other.adv;
  }
  int cid;
  int lac;
  int mnc;
  int mcc;
  int age;
  int rss;
  int adv;
};

enum RadioType {
  RADIO_TYPE_UNKNOWN = -1,
  RADIO_TYPE_GSM     =  3,
  RADIO_TYPE_CDMA    =  4,
  RADIO_TYPE_WCDMA   =  5,
};

struct RadioData {
  RadioData() : home_mnc(kint32min), home_mcc(kint32min),
                radio_type(RADIO_TYPE_UNKNOWN) {}
  bool Matches(const RadioData &other) const {
    if (cell_data.size() != other.cell_data.size()) {
      return false;
    }
    for (int i = 0; i < static_cast<int>(cell_data.size()); ++i) {
      if (!cell_data[i].Matches(other.cell_data[i])) {
        return false;
      }
    }
    return device_id == other.device_id && home_mnc == other.home_mnc &&
           home_mcc == other.home_mcc && radio_type == other.radio_type &&
           carrier == other.carrier;
  }
  std::string16 device_id;
  std::vector<CellData> cell_data;
  int home_mnc;
  int home_mcc;
  RadioType radio_type;
  std::string16 carrier;
};

struct AccessPointData {
  AccessPointData()
      : rss(kint32min), age(kint32min), cha(kint32min), snr(kint32min) {}
  bool Matches(const AccessPointData &other) const {
    // Ignore signal strength, age and signal-to-noise ratio when matching.
    return mac == other.mac && cha == other.cha && ssi == other.ssi;
  }
  std::string16 mac;
  int rss;
  int age;
  int cha;
  int snr;
  std::string16 ssi;
};

struct WifiData {
  bool Matches(const WifiData &other) const {
    if (access_point_data.size() != other.access_point_data.size()) {
      return false;
    }
    for (int i = 0; i < static_cast<int>(access_point_data.size()); ++i) {
      if (!access_point_data[i].Matches(other.access_point_data[i])) {
        return false;
      }
    }
    return true;
  }
  std::vector<AccessPointData> access_point_data;
};

// A base class for all device data providers. A device data provider gathers
// system data (eg radio or wifi data) for use by a network location provider.
// It is templated on the type of data it provides.
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
  static DeviceDataProviderBase* Register(ListenerInterface *listener);
  // Removes a listener. If this is the last listener, deletes the singleton
  // instance. Return value indicates success.
  static bool Unregister(ListenerInterface *listener);
  // Provides whatever data the provider has, which may be nothing. Return
  // value indicates whether this is all the data the provider could ever
  // obtain.
  virtual bool GetData(DataType *data) = 0;

 protected:
  // Protected constructor and destructor, callers access singleton through
  // Register and Unregister.
  DeviceDataProviderBase() {}
  virtual ~DeviceDataProviderBase() {}

  typedef std::set<ListenerInterface*> ListenersSet;
  ListenersSet listeners_;
  Mutex listeners_mutex_;

 private:
  void Ref();
  // Returns true when the ref count transitions from 1 to 0.
  bool Unref();
  void AddListener(ListenerInterface *listener);
  bool RemoveListener(ListenerInterface *listener);
  // Factory constructor.
  static DeviceDataProviderBase* Create();

  // The singleton instance of this class.
  static DeviceDataProviderBase *instance;
  static Mutex instance_mutex;
  RefCount count_;
  DISALLOW_EVIL_CONSTRUCTORS(DeviceDataProviderBase);
};

// Need to include -inl.h for template definitions.
#include "gears/geolocation/device_data_provider-inl.h"

#endif  // GEARS_GEOLOCATION_DEVICE_DATA_PROVIDER_H__
