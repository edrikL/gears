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
// This file declares GearsGeolocation, the main class of the Gears Geolocation
// API. The GearsGeolocation object provides the API methods to JavaScript. It
// uses a set of location providers to gather position information.
//
// This file also declares the Position structure, which is used to represent a
// position fix.

#ifndef GEARS_GEOLOCATION_GEOLOCATION_H__
#define GEARS_GEOLOCATION_GEOLOCATION_H__

#include <set>
#include "gears/base/common/base_class.h"
#include "gears/base/common/scoped_refptr.h"
#include "gears/geolocation/location_provider.h"
#ifdef USING_CCTESTS
#include "gears/geolocation/geolocation_test.h"
#endif

// The internal representation of an address.
struct Address {
  std::string16 street_number; // street number
  std::string16 street;        // street address
  std::string16 premises;      // premises, e.g. building name
  std::string16 city;          // city name
  std::string16 county;        // county name
  std::string16 region;        // region, e.g. a state in the US
  std::string16 country;       // country
  std::string16 country_code;  // country code (ISO 3166-1)
  std::string16 postal_code;   // postal code
};

// The internal representation of a position. We use kint32min to represent
// unknown values for integer fields. Some properties use different types when
// passed to JavaScript.
struct Position {
 public:
  Position()
      : altitude(kint32min),
        horizontal_accuracy(kint32min),
        vertical_accuracy(kint32min),
        timestamp(-1) {}
  bool IsValid() const {
    return -1 != timestamp;
  }

  double latitude;          // In degrees
  double longitude;         // In degrees
  int altitude;             // In metres
  int horizontal_accuracy;  // In metres
  int vertical_accuracy;    // In metres
  int64 timestamp;          // Milliseconds since 1st Jan 1970
  Address address;
};

// The principal class of the Geolocation API.
class GearsGeolocation
    : public ModuleImplBaseClassVirtual,
      public LocationProviderBase::ListenerInterface {
 public:
#ifdef USING_CCTESTS
  // Uses ParseArguments for testing.
  friend void TestParseGeolocationOptions(JsCallContext *context,
                                          JsRunnerInterface *js_runner);
  // Uses ConvertPositionToJavaScriptObject for testing.
  friend void TestGeolocationGetLocationFromResponse(
      JsCallContext *context,
      JsRunnerInterface *js_runner);
#endif

  GearsGeolocation();
  virtual ~GearsGeolocation();

  // API methods. See the Geolocation API design document at
  // http://code.google.com/p/google-gears/wiki/LocationAPI for a more complete
  // description of these methods.

  // Gets the last, cached, position fix. This method does not cause Gears to
  // actively seek a position update.
  // IN: nothing
  // OUT: object position
  void GetLastPosition(JsCallContext *context);

  // Instructs Gears to get a new position fix. The supplied callback function
  // is called with a valid position as soon as it is available, or with NULL on
  // failure.
  // IN: function callback_function, object position_options
  // OUT: nothing
  void GetCurrentPosition(JsCallContext *context);

  // Instructs Gears to get a new position fix. The supplied callback function
  // is called repeatedly with position updates as they become available. The
  // return value is a unique ID for this watch which can be used to cancel it.
  // IN: function callback_function, object position_options
  // OUT: int watch_id
  void WatchPosition(JsCallContext *context);

  // Cancels the position watch specified by the supplied ID.
  // IN: int watch_id
  // OUT: nothing
  void ClearWatch(JsCallContext *context);

  // Maintains all the data for a position fix.
  typedef std::set<scoped_refptr<LocationProviderBase> > ProviderSet;
  struct FixRequestInfo {
    ProviderSet providers;
    bool enable_high_accuracy;
    bool request_address;
    std::string16 address_language;
    bool repeats;
  };

 private:
  // LocationProviderBase::ListenerInterface implementation.
  virtual bool LocationUpdateAvailable(LocationProviderBase *provider,
                                       const Position &position);
  // Internal method used by GetCurrentPosition and WatchPosition to get a
  // position fix.
  void GetPositionFix(JsCallContext *context, bool repeats);

  // Parses the JavaScript arguments passed to the GetCurrentPosition and
  // WatchPosition methods.
  static bool ParseArguments(JsCallContext *context,
                             bool repeats,
                             std::vector<std::string16> *urls,
                             GearsGeolocation::FixRequestInfo *info);
  // Parses a JsObject representing the options parameter. The output is a
  // vector of URLs and the fix request info. Return value indicates success.
  static bool ParseOptions(JsCallContext *context,
                           const JsObject &options,
                           std::vector<std::string16> *urls,
                           GearsGeolocation::FixRequestInfo *info);
  // Parses a JsScopedToken representing the gearsLocationProviderUrls field.
  // The output is a vector of URLs. Return value indicates success.
  static bool ParseLocationProviderUrls(JsCallContext *context,
                                        const JsScopedToken &token,
                                        std::vector<std::string16> *urls);

  // Converts a Gears position object to a JavaScript object.
  static bool ConvertPositionToJavaScriptObject(const Position &position,
                                                const char16 *error,
                                                JsRunnerInterface *js_runner,
                                                JsObject *js_object);

  int next_fix_request_id_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsGeolocation);
};

#endif  // GEARS_GEOLOCATION_GEOLOCATION_H__
