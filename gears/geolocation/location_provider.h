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
// A location provider provides position information from a particular source
// (GPS, network etc). The GearsGeolocation object uses a set of location
// providers to obtain a position fix.
//
// This file declares a base class to be used by all location providers.
// Primarily, this class declares interface methods to be implemented by derived
// classes. It also provides ref-counting.

#ifndef GEARS_GEOLOCATION_LOCATION_PROVIDER_H__
#define GEARS_GEOLOCATION_LOCATION_PROVIDER_H__

#include "gears/base/common/scoped_refptr.h"
#include "gears/base/common/string16.h"

struct Position;

// The base class used by all location providers.
class LocationProviderBase : public RefCounted {
 public:
  class ListenerInterface {
   public:
    virtual bool LocationUpdateAvailable(LocationProviderBase *provider,
                                         const Position &position) = 0;
    virtual ~ListenerInterface() {}
  };

  virtual ~LocationProviderBase() {}

  // Interface methods.
  virtual void SetListener(ListenerInterface *listener) = 0;
  // This should call back to ListenerInterface::LocationUpdateAvailable.
  virtual bool GetCurrentPosition() = 0;
};

// Factory functions for the various types of location provider to abstract over
// the platform-dependent implementations.
#ifdef USING_CCTESTS
LocationProviderBase *NewMockLocationProvider();
#endif
LocationProviderBase *NewGpsLocationProvider();
LocationProviderBase *NewNetworkLocationProvider(
    const std::string16 &url, const std::string16 &host_name);

#endif  // GEARS_GEOLOCATION_LOCATION_PROVIDER_H__
