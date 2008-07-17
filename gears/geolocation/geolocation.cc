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
// This file implements GearsGeolocation, the main class of the Gears
// Geolocation API.

#ifdef OFFICIAL_BUILD
// The Geolocation API has not been finalized for official builds.
#else

#include "gears/geolocation/geolocation.h"

#include <math.h>
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/base/common/permissions_manager.h"
#include "gears/base/common/stopwatch.h"  // For GetCurrentTimeMillis()
#include "gears/geolocation/location_provider_pool.h"
#include "gears/geolocation/device_data_provider.h"
#include "gears/geolocation/geolocation_db.h"
#include "third_party/googleurl/src/gurl.h"

// TODO(steveblock): Update default URL when finalized.
static const char16 *kDefaultLocationProviderUrl =
    STRING16(L"http://www.google.com/");

// API options constants.
static const char16 *kEnableHighAccuracy = STRING16(L"enableHighAccuracy");
static const char16 *kRequestAddress = STRING16(L"requestAddress");
static const char16 *kAddressLanguage = STRING16(L"addressLanguage");
static const char16 *kGearsLocationProviderUrls =
    STRING16(L"gearsLocationProviderUrls");

// Timing constants.
static const int64 kMinimumCallbackInterval = 1000;  // 1 second.
static const int64 kMaximumPositionFixAge = 60 * 1000;  // 1 minute.

// DB caching constants.
static const char16 *kLastPositionName = STRING16(L"LastPosition");

// MessageService constants.
static const char16 *kLocationAvailableObserverTopic
    = STRING16(L"location available");
static const char16 *kCallbackRequiredObserverTopic
    = STRING16(L"callback required");

// Data classes for use with MessageService.
class NotificationDataGeoBase : public NotificationData {
 public:
  NotificationDataGeoBase(GearsGeolocation *object_in)
    : object(object_in) {}
  virtual ~NotificationDataGeoBase() {}

  friend class GearsGeolocation;

 private:
  // NotificationData implementation. These methods are not required.
  virtual SerializableClassId GetSerializableClassId() const {
    assert(false);
    return SERIALIZABLE_NULL;
  }
  virtual bool Serialize(Serializer *out) const {
    assert(false);
    return false;
  }
  virtual bool Deserialize(Deserializer *in) {
    assert(false);
    return false;
  }

  GearsGeolocation *object;

  DISALLOW_EVIL_CONSTRUCTORS(NotificationDataGeoBase);
};

class LocationAvailableNotificationData : public NotificationDataGeoBase {
 public:
  LocationAvailableNotificationData(GearsGeolocation *object_in,
                                    LocationProviderBase *provider_in)
      : NotificationDataGeoBase(object_in),
        provider(provider_in) {}
  virtual ~LocationAvailableNotificationData() {}

 friend class GearsGeolocation;

 private:
  LocationProviderBase *provider;

  DISALLOW_EVIL_CONSTRUCTORS(LocationAvailableNotificationData);
};

struct CallbackRequiredNotificationData : public NotificationDataGeoBase {
 public:
  CallbackRequiredNotificationData(
      GearsGeolocation *object_in,
      GearsGeolocation::FixRequestInfo *fix_info_in)
      : NotificationDataGeoBase(object_in),
        fix_info(fix_info_in) {}
  virtual ~CallbackRequiredNotificationData() {}

  friend class GearsGeolocation;

 private:
  GearsGeolocation::FixRequestInfo *fix_info;

  DISALLOW_EVIL_CONSTRUCTORS(CallbackRequiredNotificationData);
};

// Helper function that checks if the caller had the required permissions
// to use this API. If the permissions are not set, it prompts the user.
// If the permissions cannot be acquired, it sets an exception and returns
// false. Else it returns true.
static bool AcquirePermissionForLocationData(ModuleImplBaseClass *geo_module,
                                             JsCallContext *context);

// Local functions

// Gets the requested property only if it is specified. Returns true on success.
static bool GetPropertyIfSpecified(JsCallContext *context,
                                   const JsObject &object,
                                   const std::string16 &name,
                                   JsScopedToken *token);

// Sets an object integer property if the input value is valid.
static bool SetObjectPropertyIfValidInt(const std::string16 &property_name,
                                        int value,
                                        JsObject *object);

// Sets an object string property if the input value is valid.
static bool SetObjectPropertyIfValidString(const std::string16 &property_name,
                                           const std::string16 &value,
                                           JsObject *object);

// Returns true if there has been movement from the old position to the new
// position.
static bool IsNewPositionMovement(const Position &old_position,
                                  const Position &new_position);

// Returns true if the new position is more accurate than the old position.
static bool IsNewPositionMoreAccurate(const Position &old_position,
                                      const Position &new_position);

// Returns true if the old position is out of date.
static bool IsNewPositionMoreTimely(const Position &old_position,
                                    const Position &new_position);

DECLARE_GEARS_WRAPPER(GearsGeolocation);

template<>
void Dispatcher<GearsGeolocation>::Init() {
  RegisterProperty("lastPosition", &GearsGeolocation::GetLastPosition, NULL);
  RegisterMethod("getCurrentPosition", &GearsGeolocation::GetCurrentPosition);
  RegisterMethod("watchPosition", &GearsGeolocation::WatchPosition);
  RegisterMethod("clearWatch", &GearsGeolocation::ClearWatch);
  RegisterMethod("getPermission", &GearsGeolocation::GetPermission);
}

GearsGeolocation::GearsGeolocation()
    : ModuleImplBaseClassVirtual("GearsGeolocation"),
      next_watch_id_(1) {
  // Set up the thread message queue.
  if (!ThreadMessageQueue::GetInstance()->InitThreadMessageQueue()) {
    LOG(("Failed to set up thread message queue.\n"));
    assert(false);
    return;
  }

  MessageService::GetInstance()->AddObserver(this,
                                             kLocationAvailableObserverTopic);
  MessageService::GetInstance()->AddObserver(this,
                                             kCallbackRequiredObserverTopic);

  // Retrieve the cached last known position, if available.
  GeolocationDB *db = GeolocationDB::GetDB();
  if (db) {
    db->RetrievePosition(kLastPositionName, &last_position_);
  }
}

GearsGeolocation::~GearsGeolocation() {
  // Cancel all pending requests by unregistering from our location providers.
  for (ProviderMap::iterator iter = providers_.begin();
       iter != providers_.end();
       ++iter) {
    LocationProviderPool::GetInstance()->Unregister(iter->first, this);
  }

  MessageService::GetInstance()->RemoveObserver(
      this,
      kLocationAvailableObserverTopic);
  MessageService::GetInstance()->RemoveObserver(
      this,
      kCallbackRequiredObserverTopic);

  // Store the last known position.
  if (last_position_.IsGoodFix()) {
    GeolocationDB *db = GeolocationDB::GetDB();
    if (db) {
      db->StorePosition(kLastPositionName, last_position_);
    }
  }
}

// API Methods

void GearsGeolocation::GetLastPosition(JsCallContext *context) {
  // Check permissions first.
  if (!AcquirePermissionForLocationData(this, context)) return;

  // If there's no good current position, we simply return null.
  if (!last_position_.IsGoodFix()) {
    return;
  }

  // Create the object for returning to JavaScript.
  scoped_ptr<JsObject> return_object(GetJsRunner()->NewObject());
  assert(return_object.get());
  if (!ConvertPositionToJavaScriptObject(last_position_,
                                         true,  // Use address if present.
                                         GetJsRunner(),
                                         return_object.get())) {
    LOG(("GearsGeolocation::GetLastPosition() : Failed to create position "
         "object.\n"));
    assert(false);
    return;
  }

  context->SetReturnValue(JSPARAM_OBJECT, return_object.get());
}

void GearsGeolocation::GetCurrentPosition(JsCallContext *context) {
  // Check permissions first.
  if (!AcquirePermissionForLocationData(this, context)) return;

  GetPositionFix(context, false);
}

void GearsGeolocation::WatchPosition(JsCallContext *context) {
  // Check permissions first.
  if (!AcquirePermissionForLocationData(this, context)) return;

  GetPositionFix(context, true);
}

void GearsGeolocation::ClearWatch(JsCallContext *context) {
  // Check permissions first.
  if (!AcquirePermissionForLocationData(this, context)) return;

  int id;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &id },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) {
    return;
  }
  if (!CancelWatch(id)) {
    context->SetException(STRING16(L"Unknown watch ID."));
  }
}

void GearsGeolocation::GetPermission(JsCallContext *context) {
  scoped_ptr<PermissionsDialog::CustomContent> custom_content(
      PermissionsDialog::CreateCustomContent(context));
 
  if (!custom_content.get()) { return; }
 
  bool has_permission = GetPermissionsManager()->AcquirePermission(
      PermissionsDB::PERMISSION_LOCATION_DATA,
      custom_content.get());

  context->SetReturnValue(JSPARAM_BOOL, &has_permission);
}

// LocationProviderBase::ListenerInterface implementation.
bool GearsGeolocation::LocationUpdateAvailable(LocationProviderBase *provider) {
  assert(provider);
  // We marshall this callback onto the JavaScript thread. This simplifies
  // issuing new fix requests and calling back to JavaScript, which must be done
  // from the JavaScript thread.
  MessageService::GetInstance()->NotifyObservers(
      kLocationAvailableObserverTopic,
      new LocationAvailableNotificationData(this, provider));
  return true;
}

// MessageObserverInterface implementation.
void GearsGeolocation::OnNotify(MessageService *service,
                                const char16 *topic,
                                const NotificationData *data) {
  assert(data);

  // Only respond to notifications made by this object.
  const NotificationDataGeoBase *geolocation_data =
      reinterpret_cast<const NotificationDataGeoBase*>(data);
  if (this != geolocation_data->object) {
    return;
  }

  if (char16_wmemcmp(kLocationAvailableObserverTopic,
                     topic,
                     char16_wcslen(topic)) == 0) {
    const LocationAvailableNotificationData *location_available_data =
        reinterpret_cast<const LocationAvailableNotificationData*>(data);

    // Invoke the implementation.
    LocationUpdateAvailableImpl(location_available_data->provider);
  } else if (char16_wmemcmp(kCallbackRequiredObserverTopic,
                            topic,
                            char16_wcslen(topic)) == 0) {
    const CallbackRequiredNotificationData *callback_required_data =
        reinterpret_cast<const CallbackRequiredNotificationData*>(data);

    // Delete this callback timer.
    FixRequestInfo *fix_info = callback_required_data->fix_info;
    assert(fix_info->callback_timer.get());
    fix_info->callback_timer.reset();
    MakeCallback(fix_info, last_position_);
  }
}

// TimedCallback::ListenerInterface implementation.
void GearsGeolocation::OnTimeout(TimedCallback *caller, void *user_data) {
  assert(user_data);
  // Send a message to the JavaScriptThread to make the callback.
  FixRequestInfo *fix_info = reinterpret_cast<FixRequestInfo*>(user_data);
  MessageService::GetInstance()->NotifyObservers(
      kCallbackRequiredObserverTopic,
      new CallbackRequiredNotificationData(this, fix_info));
}

// Non-API methods

void GearsGeolocation::GetPositionFix(JsCallContext *context, bool repeats) {
  // Get the arguments.
  std::vector<std::string16> urls;
  scoped_ptr<FixRequestInfo> info(new FixRequestInfo());
  if (!ParseArguments(context, repeats, &urls, info.get())) {
    assert(context->is_exception_set());
    return;
  }

  // Add the providers. The lifetime of the providers is handled by the location
  // provider pool, through Register and Unregister.
  std::string16 host_name = EnvPageSecurityOrigin().host();
  LocationProviderPool *pool = LocationProviderPool::GetInstance();

  // Native providers
  if (info->enable_high_accuracy) {
    LocationProviderBase *gps_provider =
        pool->Register(STRING16(L"GPS"),
                       host_name,
                       info->request_address,
                       info->address_language,
                       this);
    if (gps_provider) {
      info->providers.push_back(gps_provider);
    }
  }

  // Network providers
  for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
    // Check if the url is valid. If not, skip this URL. This also handles the
    // case where the URL is 'GPS', which would confuse the location provider
    // pool.
    GURL url(urls[i]);
    if (url.is_valid()) {
      LocationProviderBase *network_provider =
          pool->Register(urls[i],
                         host_name,
                         info->request_address,
                         info->address_language,
                         this);
      if (network_provider) {
        info->providers.push_back(network_provider);
      }
    }
  }

  // If this fix has no providers, throw an exception and quit.
  if (info->providers.empty()) {
    context->SetException(STRING16(L"Fix request has no location providers."));
    return;
  }

  // If this is a non-repeating request, hint to all providers that new data is
  // required ASAP.
  if (!info->repeats) {
    for (ProviderVector::iterator iter = info->providers.begin();
         iter != info->providers.end();
         ++iter) {
      (*iter)->UpdatePosition();
    }
  }

  // Store and return the ID of this fix if it repeats.
  if (info->repeats) {
    context->SetReturnValue(JSPARAM_INT, &next_watch_id_);
    watches_[next_watch_id_++] = info.get();
  }

  // Record this fix. This updates the map of providers and fix requests. The
  // map takes ownership of the info structure.
  MutexLock lock(&providers_mutex_);
  RecordNewFixRequest(info.release());
}

bool GearsGeolocation::CancelWatch(const int &watch_id) {
  FixRequestInfoMap::iterator watch_iter = watches_.find(watch_id);
  if (watch_iter == watches_.end()) {
    return false;
  }
  FixRequestInfo *info = const_cast<FixRequestInfo*>(watch_iter->second);
  assert(info->repeats);
  // Update our list of providers.
  MutexLock providers_lock(&providers_mutex_);
  RemoveFixRequest(info);
  watches_.erase(watch_iter);
  return true;
}

void GearsGeolocation::HandleRepeatingRequestUpdate(FixRequestInfo *fix_info) {
  assert(fix_info->repeats);
  // If there's already a timer active for the callback, there's nothing to do.
  if (fix_info->callback_timer.get()) {
    return;
  }
  // If the postion has changed significantly or the accuracy has improved, and
  // the minimum time since our last callback to JavaScript has elapsed, we make
  // a callback.
  if (IsNewPositionMovement(fix_info->last_position, last_position_) ||
      IsNewPositionMoreAccurate(fix_info->last_position, last_position_)) {
    // The position has changed significantly. See if the minimum time interval
    // since the last callback has expired.
    int64 time_remaining =
        kMinimumCallbackInterval -
        (GetCurrentTimeMillis() - fix_info->last_callback_time);
    if (time_remaining <= 0) {
      if (!MakeCallback(fix_info, last_position_)) {
        LOG(("GearsGeolocation::HandleRepeatingRequestUpdate() : JavaScript "
             "callback failed.\n"));
        assert(false);
      }
    } else {
      // Start an asynchronous timer which will post a message back to this
      // thread once the minimum time period has elapsed.
      MakeFutureCallback(static_cast<int>(time_remaining), fix_info);
    }
  }
}

void GearsGeolocation::HandleSingleRequestUpdate(
    LocationProviderBase *provider,
    FixRequestInfo *fix_info,
    const Position &position) {
  assert(!fix_info->repeats);
  assert(fix_info->last_callback_time == 0);
  // Remove this provider from the this fix so that future to callbacks to this
  // Geolocation object don't trigger handling for this fix.
  RemoveProvider(provider, fix_info);
  // We callback in two cases ...
  // - This response gives a good position and we haven't yet called back
  // - The fix has no remaining providers, so we'll never get a valid position
  // We then cancel any pending requests and delete the fix request.
  if (position.IsGoodFix() || fix_info->providers.empty()) {
    if (!MakeCallback(fix_info, position)) {
      LOG(("GearsGeolocation::HandleSingleRequestUpdate() : JavaScript "
           "callback failed.\n"));
      assert(false);
    }
    RemoveFixRequest(fix_info);
  }
}

void GearsGeolocation::LocationUpdateAvailableImpl(
    LocationProviderBase *provider) {
  // Update the last known position, which is the best position estimate we
  // currently have. This is shared between all repeating fix requests.
  Position position;
  provider->GetPosition(&position);
  last_position_mutex_.Lock();
  if (IsNewPositionMovement(last_position_, position) ||
      IsNewPositionMoreAccurate(last_position_, position) ||
      IsNewPositionMoreTimely(last_position_, position)) {
    last_position_ = position;
  }
  last_position_mutex_.Unlock();

  // Iterate over all non-repeating fix requests of which this provider is a
  // part. We call back to Javascript if this is the first good position for
  // that request.
  MutexLock lock(&providers_mutex_);
  ProviderMap::iterator provider_iter = providers_.find(provider);
  // We may not have an entry in the map for this provider. This situation can
  // occur when a provider calls back to this object, but the request is then
  // cancelled before the call is marshalled to the JavaScript thread.
  if (provider_iter != providers_.end()) {
    // Take a copy of the vector of requests because the handlers below may
    // remove items from the list, so ++iter will fail.
    FixVector fix_requests = provider_iter->second;
    for (FixVector::iterator request_iter = fix_requests.begin();
         request_iter != fix_requests.end();
         ++request_iter) {
      FixRequestInfo *fix_info = *request_iter;
      // TODO(steveblock): Consider storing only non-repeating fix requests in
      // the map.
      if (!fix_info->repeats) {
        HandleSingleRequestUpdate(provider, fix_info, position);
      }
    }
  }

  // Iterate over all repeating fix requests.
  for (FixRequestInfoMap::const_iterator iter = watches_.begin();
       iter != watches_.end();
       iter++) {
    FixRequestInfo *fix_info = const_cast<FixRequestInfo*>(iter->second);
    HandleRepeatingRequestUpdate(fix_info);
  }
}

bool GearsGeolocation::MakeCallback(FixRequestInfo *fix_info,
                                    const Position &position) {
  scoped_ptr<JsObject> callback_param(GetJsRunner()->NewObject());
  assert(callback_param.get());
  if (!ConvertPositionToJavaScriptObject(position,
                                         fix_info->request_address,
                                         GetJsRunner(),
                                         callback_param.get())) {
    LOG(("GearsGeolocation::MakeCallback() : Failed to create "
         "position object.\n"));
    assert(false);
    return false;
  }
  JsParamToSend argv[] = { JSPARAM_OBJECT, callback_param.get() };
  // InvokeCallback returns false if the callback enounters an error.
  GetJsRunner()->InvokeCallback(fix_info->callback.get(),
                                ARRAYSIZE(argv), argv, NULL);
  fix_info->last_position = position;
  fix_info->last_callback_time = GetCurrentTimeMillis();
  return true;
}

bool GearsGeolocation::ParseArguments(JsCallContext *context,
                                      bool repeats,
                                      std::vector<std::string16> *urls,
                                      FixRequestInfo *info) {
  assert(context);
  assert(urls);
  assert(info);
  info->repeats = repeats;
  // Note that GetArguments allocates a new JsRootedCallback.
  JsRootedCallback *function = NULL;
  JsObject options;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &function },
    { JSPARAM_OPTIONAL, JSPARAM_OBJECT, &options },
  };
  int num_arguments = context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) {
    delete function;
    return false;
  }
  assert(function);
  info->callback.reset(function);

  // Set default values for options.
  info->enable_high_accuracy = false;
  info->request_address = false;
  urls->clear();
  // We have to check that options is present because it's not valid to use an
  // uninitialised JsObject.
  if (num_arguments > 1) {
    if (!ParseOptions(context, options, urls, info)) {
      assert(context->is_exception_set());
      return false;
    }
  } else {
    // options is not specified, so use the default URL.
    urls->push_back(kDefaultLocationProviderUrl);
  }
  return true;
}

bool GearsGeolocation::ParseOptions(JsCallContext *context,
                                    const JsObject &options,
                                    std::vector<std::string16> *urls,
                                    FixRequestInfo *info) {
  assert(context);
  assert(urls);
  assert(info);
  JsScopedToken token;
  if (GetPropertyIfSpecified(context, options, kEnableHighAccuracy, &token)) {
    if (!JsTokenToBool_NoCoerce(token, context->js_context(),
                                &(info->enable_high_accuracy))) {
      std::string16 error = STRING16(L"options.");
      error += kEnableHighAccuracy;
      error += STRING16(L" should be a boolean.");
      context->SetException(error);
      return false;
    }
  }
  if (GetPropertyIfSpecified(context, options, kRequestAddress, &token)) {
    if (!JsTokenToBool_NoCoerce(token, context->js_context(),
                                &(info->request_address))) {
      std::string16 error = STRING16(L"options.");
      error += kRequestAddress;
      error += STRING16(L" should be a boolean.");
      context->SetException(error);
      return false;
    }
  }
  if (GetPropertyIfSpecified(context, options, kAddressLanguage, &token)) {
    if (!JsTokenToString_NoCoerce(token, context->js_context(),
                                  &(info->address_language))) {
      std::string16 error = STRING16(L"options.");
      error += kAddressLanguage;
      error += STRING16(L" should be a string.");
      context->SetException(error);
      return false;
    }
  }
  if (GetPropertyIfSpecified(context, options, kGearsLocationProviderUrls,
                             &token)) {
    if (!ParseLocationProviderUrls(context, token, urls)) {
      std::string16 error = STRING16(L"options.");
      error += kGearsLocationProviderUrls;
      error += STRING16(L" should be null or an array of strings.");
      context->SetException(error);
      return false;
    }
  } else {
  // gearsLocationProviderUrls is not specified, so use the default URL.
  urls->push_back(kDefaultLocationProviderUrl);
  }
  return true;
}

bool GearsGeolocation::ParseLocationProviderUrls(
    JsCallContext *context,
    const JsScopedToken &token,
    std::vector<std::string16> *urls) {
  assert(context);
  assert(urls);
  if (JsTokenGetType(token, context->js_context()) == JSPARAM_ARRAY) {
    // gearsLocationProviderUrls is an array.
    JsArray js_array;
    if (!js_array.SetArray(token, context->js_context())) {
      LOG(("GearsGeolocation::ParseLocationProviderUrls() : Failed to set "
           "array with gearsLocationProviderUrls."));
      assert(false);
      return false;
    }
    int length;
    if (!js_array.GetLength(&length)) {
      LOG(("GearsGeolocation::ParseLocationProviderUrls() : Failed to get "
           "length of gearsLocationProviderUrls."));
      assert(false);
      return false;
    }
    for (int i = 0; i < length; ++i) {
      JsScopedToken token;
      if (!js_array.GetElement(i, &token)) {
        LOG(("GearsGeolocation::ParseLocationProviderUrls() : Failed to get "
             "element from gearsLocationProviderUrls."));
        assert(false);
        return false;
      }
      std::string16 url;
      if (!JsTokenToString_NoCoerce(token, context->js_context(), &url)) {
        return false;
      }
      urls->push_back(url);
    }
  } else if (JsTokenGetType(token, context->js_context()) != JSPARAM_NULL) {
    // If gearsLocationProviderUrls is null, we do not use the default URL.
    // If it's not an array and not null, this is an error.
    return false;
  }
  return true;
}

bool GearsGeolocation::ConvertPositionToJavaScriptObject(
    const Position &position,
    bool use_address,
    JsRunnerInterface *js_runner,
    JsObject *js_object) {
  assert(js_object);
  assert(position.IsInitialized());
  bool result = true;

  if (!position.IsGoodFix()) {
    // Position is not a good fix, copy only the error message.
    assert(!position.error.empty());
    result &= js_object->SetPropertyString(STRING16(L"errorMessage"),
                                           position.error);
    return result;
  }

  // latitude, longitude and date should always be valid.
  result &= js_object->SetPropertyDouble(STRING16(L"latitude"),
                                         position.latitude);
  result &= js_object->SetPropertyDouble(STRING16(L"longitude"),
                                         position.longitude);
  scoped_ptr<JsObject> date_object(js_runner->NewDate(position.timestamp));
  result &= NULL != date_object.get();
  if (date_object.get()) {
    result &= js_object->SetPropertyObject(STRING16(L"timestamp"),
                                           date_object.get());
  }
  // Other properties may not be valid.
  result &= SetObjectPropertyIfValidInt(STRING16(L"altitude"),
                                        position.altitude,
                                        js_object);
  result &= SetObjectPropertyIfValidInt(STRING16(L"horizontalAccuracy"),
                                        position.horizontal_accuracy,
                                        js_object);
  result &= SetObjectPropertyIfValidInt(STRING16(L"verticalAccuracy"),
                                        position.vertical_accuracy,
                                        js_object);
  // Address
  if (use_address) {
    scoped_ptr<JsObject> address_object(js_runner->NewObject());
    result &= NULL != address_object.get();
    if (address_object.get()) {
      result &= SetObjectPropertyIfValidString(STRING16(L"streetNumber"),
                                               position.address.street_number,
                                               address_object.get());
      result &= SetObjectPropertyIfValidString(STRING16(L"street"),
                                               position.address.street,
                                               address_object.get());
      result &= SetObjectPropertyIfValidString(STRING16(L"premises"),
                                               position.address.premises,
                                               address_object.get());
      result &= SetObjectPropertyIfValidString(STRING16(L"city"),
                                               position.address.city,
                                               address_object.get());
      result &= SetObjectPropertyIfValidString(STRING16(L"county"),
                                               position.address.county,
                                               address_object.get());
      result &= SetObjectPropertyIfValidString(STRING16(L"region"),
                                               position.address.region,
                                               address_object.get());
      result &= SetObjectPropertyIfValidString(STRING16(L"country"),
                                               position.address.country,
                                               address_object.get());
      result &= SetObjectPropertyIfValidString(STRING16(L"countryCode"),
                                               position.address.country_code,
                                               address_object.get());
      result &= SetObjectPropertyIfValidString(STRING16(L"postalCode"),
                                               position.address.postal_code,
                                               address_object.get());

      // Only add the address object if it has some properties.
      std::vector<std::string16> properties;
      if (address_object.get()->GetPropertyNames(&properties) &&
          !properties.empty()) {
        result &= js_object->SetPropertyObject(STRING16(L"address"),
                                               address_object.get());
      }
    }
  }
  return result;
}

void GearsGeolocation::RecordNewFixRequest(FixRequestInfo *fix_request) {
  // For each location provider used by this request, update the provider's
  // list of fix requests in the map.
  ProviderVector *member_providers = &fix_request->providers;
  for (ProviderVector::iterator iter = member_providers->begin();
       iter != member_providers->end();
       ++iter) {
    LocationProviderBase *provider = *iter;
    // If providers_ does not yet have an entry for this provider, this will
    // create one.
    providers_[provider].push_back(fix_request);
  }
}

void GearsGeolocation::RemoveFixRequest(FixRequestInfo *fix_request) {
  // For each location provider used by this request, update the provider's
  // list of fix requests in the map.
  ProviderVector *member_providers = &fix_request->providers;
  for (ProviderVector::iterator iter = member_providers->begin();
       iter != member_providers->end();
       ++iter) {
    LocationProviderBase *provider = *iter;
    // Check that we have an entry for this provider.
    ProviderMap::iterator provider_iter = providers_.find(provider);
    assert(provider_iter != providers_.end());
    FixVector *fixes = &(provider_iter->second);
    FixVector::iterator fix_iterator =
        std::find(fixes->begin(), fixes->end(), fix_request);
    // If we can't find this request in our map, something has gone wrong.
    assert(fix_iterator != fixes->end());
    fixes->erase(fix_iterator);
    // If this location provider is no longer used in any fixes, remove it from
    // our map.
    if (fixes->empty()) {
      providers_.erase(provider_iter);
    }
    // Unregister from the provider, via the pool. This will cancel any pending
    // requests and may block if a callback is currently in progress.
    LocationProviderPool::GetInstance()->Unregister(provider, this);
  }
  // Delete the request object itself.
  delete fix_request;
}

void GearsGeolocation::RemoveProvider(LocationProviderBase *provider,
                                      FixRequestInfo *fix_request) {
  ProviderVector *member_providers = &fix_request->providers;
  ProviderVector::iterator iter = std::find(member_providers->begin(),
                                            member_providers->end(),
                                            provider);
  // Check that this provider is used by the fix request.
  assert(iter != member_providers->end());
  // Remove the location provider from the fix request.
  member_providers->erase(iter);
  // Remove this fix request from the provider in the map of providers.
  ProviderMap::iterator provider_iter = providers_.find(provider);
  assert(provider_iter != providers_.end());
  FixVector *fixes = &(provider_iter->second);
  FixVector::iterator fix_iterator =
      std::find(fixes->begin(), fixes->end(), fix_request);
  assert(fix_iterator != fixes->end());
  fixes->erase(fix_iterator);
  // If this location provider is no longer used in any fixes, remove it from
  // our map.
  if (fixes->empty()) {
    providers_.erase(provider_iter);
  }
  // Unregister from the provider, via the pool. This will cancel any pending
  // requests and may block if a callback is currently in progress.
  LocationProviderPool::GetInstance()->Unregister(provider, this);
}

void GearsGeolocation::MakeFutureCallback(int timeout_milliseconds,
                                          FixRequestInfo *fix_info) {
  // Check that there isn't already a timer running for this request.
  assert(!fix_info->callback_timer.get());
  fix_info->callback_timer.reset(
      new TimedCallback(this, timeout_milliseconds, fix_info));
}

// Local functions

static bool GetPropertyIfSpecified(JsCallContext *context,
                                   const JsObject &object,
                                   const std::string16 &name,
                                   JsScopedToken *token) {
  assert(token);
  // GetProperty should always succeed, but will get a token of type
  // JSPARAM_UNDEFINED if the requested property is not present.
  JsScopedToken token_local;
  if (!object.GetProperty(name, &token_local)) {
    assert(false);
    return false;
  }
  if (JsTokenGetType(token_local, context->js_context()) == JSPARAM_UNDEFINED) {
    return false;
  }
  *token = token_local;
  return true;
}

static bool SetObjectPropertyIfValidInt(const std::string16 &property_name,
                                        int value,
                                        JsObject *object) {
  assert(object);
  if (kint32min != value) {
    return object->SetPropertyInt(property_name, value);
  }
  return true;
}

static bool SetObjectPropertyIfValidString(const std::string16 &property_name,
                                           const std::string16 &value,
                                           JsObject *object) {
  assert(object);
  if (!value.empty()) {
    return object->SetPropertyString(property_name, value);
  }
  return true;
}

// Helper function for IsMovement, IsMoreAccurate and IsNewPositionMoreTimely.
// Checks whether the old or new position is bad, in which case the decision of
// which to use is easy. If this case is detected, the return value is true and
// result indicates whether the new position should be used.
static bool CheckForBadPosition(const Position &old_position,
                                const Position &new_position,
                                bool *result) {
  assert(result);
  if (!new_position.IsGoodFix()) {
    // New is bad.
    *result = false;
    return true;
  } if (!old_position.IsGoodFix()) {
    // Old is bad, new is good.
    *result = true;
    return true;
  }
  return false;
}

// Returns true if there has been movement from the old position to the new
// position.
static bool IsNewPositionMovement(const Position &old_position,
                                  const Position &new_position) {
  bool result;
  if (CheckForBadPosition(old_position, new_position, &result)) {
    return result;
  }
  // Correctly calculating the distance between two positions isn't necessary
  // given the small distances we're interested in.
  double delta = fabs(new_position.latitude - old_position.latitude) +
                 fabs(new_position.longitude - old_position.longitude);
  // Convert to metres. 1 second of arc of latitude (or longitude at the
  // equator) is 1 nautical mile or 1852m.
  delta *= 60 * 1852;
  // The threshold is when the distance between the two positions exceeds the
  // worse (larger value) of the two accuracies.
  double max_accuracy = std::max(old_position.horizontal_accuracy,
                                 new_position.horizontal_accuracy);
  return delta > max_accuracy;
}

static bool IsNewPositionMoreAccurate(const Position &old_position,
                                      const Position &new_position) {
  bool result;
  if (CheckForBadPosition(old_position, new_position, &result)) {
    return result;
  }
  return new_position.horizontal_accuracy < old_position.horizontal_accuracy;
}

static bool IsNewPositionMoreTimely(const Position &old_position,
                                    const Position &new_position) {
  bool result;
  if (CheckForBadPosition(old_position, new_position, &result)) {
    return result;
  }
  return GetCurrentTimeMillis() - old_position.timestamp >
      kMaximumPositionFixAge;
}

static bool AcquirePermissionForLocationData(ModuleImplBaseClass *geo_module,
                                             JsCallContext *context) {
  if (!geo_module->GetPermissionsManager()->AcquirePermission(
      PermissionsDB::PERMISSION_LOCATION_DATA)) {
    std::string16 error = STRING16(L"Page does not have permission to access "
                                   L"location information using "
                                   PRODUCT_FRIENDLY_NAME);
    context->SetException(error);
    return false;
  }
  return true;
}

#endif  // OFFICIAL_BUILD
