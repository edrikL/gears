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

#include <map>
#include <vector>
#include "gears/base/common/base_class.h"
#include "gears/base/common/message_service.h"
#ifdef USING_CCTESTS
#include "gears/geolocation/geolocation_test.h"
#endif
#include "gears/geolocation/location_provider.h"
#include "gears/geolocation/timed_callback.h"
#include "third_party/linked_ptr/linked_ptr.h"

static const int kBadLatLng = 200;

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
      : latitude(kBadLatLng),
        longitude(kBadLatLng),
        altitude(kint32min),
        horizontal_accuracy(kint32min),
        vertical_accuracy(kint32min),
        timestamp(-1) {}
  bool IsGoodFix() const {
    // A good fix has a valid latitude, longitude, horizontal accuracy and
    // timestamp.
    return latitude >= -90.0 && latitude <= 90.0 &&
           longitude >= -180.0 && longitude <= 180.0 &&
           horizontal_accuracy != kint32min &&
           timestamp != -1;
  }
  bool IsInitialized() const {
    return IsGoodFix() || !error.empty();
  }

  double latitude;          // In degrees
  double longitude;         // In degrees
  int altitude;             // In metres
  int horizontal_accuracy;  // In metres
  int vertical_accuracy;    // In metres
  int64 timestamp;          // Milliseconds since 1st Jan 1970
  std::string16 error;      // Human-readable error message
  Address address;
};

// The principal class of the Geolocation API.
class GearsGeolocation
    : public ModuleImplBaseClass,
      public LocationProviderBase::ListenerInterface,
      public MessageObserverInterface,
      public TimedCallback::ListenerInterface {
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

  // Triggers the geolocation-specific permissions dialog.
  // IN:  string site_name, string image_url, string extra_message
  // OUT: boolean permission
  void GetPermission(JsCallContext *context);

  // Maintains all the data for a position fix.
  typedef std::vector<LocationProviderBase*> ProviderVector;
  struct FixRequestInfo {
    FixRequestInfo() : last_callback_time(0) {}
    ProviderVector providers;
    bool enable_high_accuracy;
    bool request_address;
    std::string16 address_language;
    bool repeats;
    // Linked_ptr so we can use FixRequestInfo in STL containers.
    linked_ptr<JsRootedCallback> callback;
    // The last position sent back to JavaScript. Used by repeating requests
    // only.
    Position last_position;
    // The time at which we last called back to JavaScript, in ms since the
    // epoch.
    int64 last_callback_time;
    // The timer used for pending future callbacks in a watch.
    linked_ptr<TimedCallback> callback_timer;
  };

 private:
  // LocationProviderBase::ListenerInterface implementation.
  virtual bool LocationUpdateAvailable(LocationProviderBase *provider);

  // MessageObserverInterface implementation.
  virtual void OnNotify(MessageService *service,
                        const char16 *topic,
                        const NotificationData *data);

  // TimedCallback::ListenerInterface implementation.
  virtual void OnTimeout(TimedCallback *caller, void *user_data);

  // Internal method used by OnNotify.
  void LocationUpdateAvailableImpl(LocationProviderBase *provider);

  // Internal method used by GetCurrentPosition and WatchPosition to get a
  // position fix.
  void GetPositionFix(JsCallContext *context, bool repeats);

  // Cancels an ongoing watch.
  bool CancelWatch(const int &watch_id);

  // Internal method used by LocationUpdateAvailable to handle an update for a
  // repeating fix request. 
  void HandleRepeatingRequestUpdate(FixRequestInfo *fix_info);

  // Internal method used by LocationUpdateAvailable to handle an update for a
  // non-repeating fix request.
  void HandleSingleRequestUpdate(LocationProviderBase *provider,
                                 FixRequestInfo *fix_info,
                                 const Position &position);

  // Internal method to make the callback to JavaScript once we have a postion
  // fix.
  bool MakeCallback(FixRequestInfo *fix_info, const Position &position);

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
                                                bool use_address,
                                                JsRunnerInterface *js_runner,
                                                JsObject *js_object);

  // Takes a pointer to a new fix request and records it in our map.
  void RecordNewFixRequest(FixRequestInfo *fix_request);

  // Removes a fix request. Cancels any pending requests to the location
  // providers it uses.
  void RemoveFixRequest(FixRequestInfo *fix_request);

  // Removes a location provider from a fix request.
  void RemoveProvider(LocationProviderBase *provider,
                      FixRequestInfo *fix_request);

  // Causes a callback to JavaScript to be made at the specified number of
  // milliseconds in the future.
  void MakeFutureCallback(int timeout_milliseconds, FixRequestInfo *fix_info);

  // A map of listners to fix requests. This is used when processing position
  // updates from providers. It is also used to unregister from a listener
  // when all requests that use that listener are complete.
  typedef std::vector<FixRequestInfo*> FixVector;
  typedef std::map<LocationProviderBase*, FixVector> ProviderMap;
  ProviderMap providers_;
  Mutex providers_mutex_;

  // Map from watch ID to repeating fix request.
  typedef std::map<int, const FixRequestInfo*> FixRequestInfoMap;
  FixRequestInfoMap watches_;
  int next_watch_id_;

  // The current best estimate for our position. This is the position returned
  // by GetLastPosition and is shared by all watches.
  Position last_position_;
  Mutex last_position_mutex_;

  ThreadId java_script_thread_id_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsGeolocation);
};

#endif  // GEARS_GEOLOCATION_GEOLOCATION_H__
