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
// This file defines the GpsLocationProvider class. This class gets position
// information from a GPS and uses a network location provider to obtain a
// reverse-geocoded address where required.
//
// It also defines the GpsDeviceBase class, from which the platform-specific
// GPS device classes inherit.

#ifndef GEARS_GEOLOCATION_GPS_LOCATION_PROVIDER_H__
#define GEARS_GEOLOCATION_GPS_LOCATION_PROVIDER_H__

#include "gears/base/common/common.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/thread.h"
#include "gears/geolocation/location_provider.h"
#include "gears/geolocation/geolocation.h"
#include "gears/geolocation/reverse_geocoder.h"

class GpsDeviceBase {
 public:
  class ListenerInterface {
   public:
     // Implmentations should call this method when a fatal error occurs, such
     // that no further location updates will be possible.
     virtual void GpsFatalError(int code, const std::string16 &message) = 0;
     // Implementations should call this method when good position is first
     // available, and again whenever an updated position is available.
     virtual void GpsPositionUpdateAvailable(const Position &position) = 0;
  };

  GpsDeviceBase(ListenerInterface *listener) : listener_(listener) {}
  virtual ~GpsDeviceBase() {}

 protected:
  ListenerInterface *listener_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GpsDeviceBase);
};


class GpsLocationProvider
    : public LocationProviderBase,
      public Thread,
      public ReverseGeocoder::ReverseGeocoderListenerInterface,
      public GpsDeviceBase::ListenerInterface {
 public:
   GpsLocationProvider(BrowsingContext *browsing_context,
                       const std::string16 &reverse_geocode_url,
                       const std::string16 &host_name,
                       const std::string16 &address_language);
  virtual ~GpsLocationProvider();

  // Override LocationProviderBase implementation.
  virtual void RegisterListener(
      LocationProviderBase::ListenerInterface *listener,
      bool request_address);
  virtual void UnregisterListener(
      LocationProviderBase::ListenerInterface *listener);

  // LocationProviderBase implementation.
  virtual void GetPosition(Position *position);

  // Setter for GPS device factory function
  typedef GpsDeviceBase *(*GpsDeviceFactoryFunction)(
      GpsDeviceBase::ListenerInterface*);
  static void SetGpsDeviceFactoryFunction(
      GpsDeviceFactoryFunction gps_device_factory_function);

  // Factory method for platform-dependent GPS devices
  static GpsDeviceBase *NewGpsDevice(
      GpsDeviceBase::ListenerInterface *listener);

 private:
  // Thread implementation
  virtual void Run();

  // ReverseGeocoder::ListenerInterface implementation
  virtual void ReverseGeocodeAvailable(const Position &position,
                                       bool server_error);

  // GpsDeviceBase::ListenerInterface implementation
  virtual void GpsFatalError(int code, const std::string16 &message);
  virtual void GpsPositionUpdateAvailable(const Position &position);

  void MakeReverseGeocodeRequest();

  // Properties of the GPS location provider.
  std::string16 reverse_geocode_url_;
  std::string16 host_name_;
  std::string16 address_language_;

  // The current best position estimate and its mutex.
  Position position_;
  Position new_position_;
  Address new_address_;
  Mutex position_mutex_;

  // Event used to signal to the worker thread.
  Event worker_event_;

  // Status flags
  bool is_shutting_down_;
  bool is_new_position_available_;
  bool is_new_reverse_geocode_available_;
  bool is_new_listener_waiting_;

  scoped_ptr<ReverseGeocoder> reverse_geocoder_;

  int64 earliest_reverse_geocode_time_;
  Mutex earliest_reverse_geocode_time_mutex_;

  // Stuff to do with listeners and associated mutex
  typedef std::vector<LocationProviderBase::ListenerInterface*> ListenerVector;
  ListenerVector new_listeners_requiring_address_;
  bool is_address_requested_;
  Mutex address_mutex_;
  BrowsingContext *browsing_context_;

  // The GPS device itself.
  scoped_ptr<GpsDeviceBase> gps_device_;

  // The factory function used to create the GPS device.
  static GpsDeviceFactoryFunction gps_device_factory_function_;

  DISALLOW_EVIL_CONSTRUCTORS(GpsLocationProvider);
};

#endif  // GEARS_GEOLOCATION_GPS_LOCATION_PROVIDER_H__
