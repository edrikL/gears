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

#ifndef GEARS_GEOLOCATION_GEOLOCATION_H__
#define GEARS_GEOLOCATION_GEOLOCATION_H__

#include <set>
#include "gears/base/common/base_class.h"
#include "gears/base/common/scoped_refptr.h"
#include "gears/geolocation/location_provider.h"

// The internal representation of a position. We use kint32min to represent
// unknown values for integer fields. Some properties use different types when
// passed to JavaScript.
struct Position {
 public:
  Position() : altitude(kint32min), horizontal_accuracy(kint32min),
               vertical_accuracy(kint32min), timestamp(-1) {}
  bool IsValid() const {
    return -1 != timestamp;
  }
  double latitude;          // In degrees
  double longitude;         // In degrees
  int altitude;             // In metres
  int horizontal_accuracy;  // In metres
  int vertical_accuracy;    // In metres
  int64 timestamp;          // Milliseconds since 1st Jan 1970
  // TODO(steveblock): Add address field once finalized.
};

// The principal class of the location API.
class GearsGeolocation
    : public ModuleImplBaseClassVirtual,
      public LocationProviderInterface::ListenerInterface {
 public:
  GearsGeolocation();
  virtual ~GearsGeolocation();

  // API methods.

  // IN: nothing
  // OUT: object position
  void GetLastPosition(JsCallContext *context);

  // IN: function callback_function, object position_options
  // OUT: nothing
  void GetCurrentPosition(JsCallContext *context);

  // IN: function callback_function, object position_options
  // OUT: int watch_id
  void WatchPosition(JsCallContext *context);

  // IN: int watch_id
  // OUT: nothing
  void ClearWatch(JsCallContext *context);

  // Maintains all the data for a position fix.
  typedef std::set<scoped_refptr<LocationProviderInterface> > ProviderSet;
  struct FixRequestInfo {
    ProviderSet providers;
    bool enable_high_accuracy;
    bool request_address;
    bool repeats;
  };

 private:
  // LocationProviderInterface::ListenerInterface implementation.
  virtual bool LocationUpdateAvailable(LocationProviderInterface *provider,
                                       const Position &position);
  void GetPositionFix(JsCallContext *context, bool repeats);

  int next_fix_request_id_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsGeolocation);
};

#endif  // GEARS_GEOLOCATION_GEOLOCATION_H__
