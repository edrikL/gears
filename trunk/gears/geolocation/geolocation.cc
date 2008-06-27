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
#include "third_party/googleurl/src/gurl.h"

// TODO(steveblock): Update default URL when finalized.
static const char16 *kDefaultLocationProviderUrl =
    STRING16(L"http://www.google.com/");
static const char16 *kEnableHighAccuracy = STRING16(L"enableHighAccuracy");
static const char16 *kRequestAddress = STRING16(L"requestAddress");
static const char16 *kAddressLanguage = STRING16(L"addressLanguage");
static const char16 *kGearsLocationProviderUrls =
    STRING16(L"gearsLocationProviderUrls");
static const int64 kMinimumCallbackInterval = 1000;  // 1 second.

// Temporarily define DeviceDataProvider::DefaultFactoryFunction() here for
// platforms where the device data provider is not yet implemented.
// TODO(steveblock): Implement device data providers for remaining platforms.
#if defined(WINCE)
// WinCE uses WinceRadioDataProvider.
#elif (defined(WIN32) && !defined(WINCE)) || defined(LINUX) || defined(OS_MACOSX)
// Win32, Linux and OSX use EmptyDeviceDataProvider<RadioData>.
#else
// static
template<>
RadioDataProviderImplBase *RadioDataProvider::DefaultFactoryFunction() {
  assert(false);
  return NULL;
}
#endif

#if defined(WINCE)
// WinCE uses WinceWifiDataProvider.
#elif defined(WIN32) && !defined(WINCE)
// Win32 uses Win32WifiDataProvider.
#elif defined(LINUX) && !defined(OS_MACOSX)
// Linux uses LinuxWifiDataProvider.
#elif defined(OS_MACOSX)
// OSX uses OsxWifiDataProvider.
#else
// static
template<>
WifiDataProviderImplBase *WifiDataProvider::DefaultFactoryFunction() {
  assert(false);
  return NULL;
}
#endif

// Data structure for use with ThreadMessageQueue.
struct LocationAvailableMessageData : public MessageData {
 public:
  LocationAvailableMessageData(GearsGeolocation *object_in,
                               LocationProviderBase *provider_in)
      : object(object_in),
        provider(provider_in) {}
  virtual ~LocationAvailableMessageData() {}
  GearsGeolocation *object;
  LocationProviderBase *provider;
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

// Determines which of two position fixes is better. This is based on the
// accuracy and timestamp of each fix, and the current time.
static bool IsBetterFix(const Position &old_position,
                        const Position &new_position);

// Returns true if the new position differs significantly from the old position.
static bool PositionHasChanged(const Position &old_position,
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
  ThreadMessageQueue::GetInstance()->RegisterHandler(
      kLocationUpdateAvailable, this);
  java_script_thread_id_ =
      ThreadMessageQueue::GetInstance()->GetCurrentThreadId();
}

GearsGeolocation::~GearsGeolocation() {
  // Cancel all pending requests by unregistering from our location providers.
  for (ProviderMap::iterator iter = providers_.begin();
       iter != providers_.end();
       ++iter) {
    LocationProviderPool::GetInstance()->Unregister(iter->first, this);
  }
}

// API Methods

void GearsGeolocation::GetLastPosition(JsCallContext *context) {
  // Check permissions first.
  if (!AcquirePermissionForLocationData(this, context)) return;

  // If there's no good current position, we simply return null.
  // TODO(steveblock): Cache the last known position in the database to provide
  // state between sessions.
  if (!last_position_.IsGoodFix()) {
    return;
  }

  // Create the object for returning to JavaScript.
  scoped_ptr<JsObject> return_object(GetJsRunner()->NewObject());
  assert(return_object.get());
  if (!ConvertPositionToJavaScriptObject(last_position_,
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
  bool result = ThreadMessageQueue::GetInstance()->Send(
    java_script_thread_id_,
    kLocationUpdateAvailable,
    new LocationAvailableMessageData(this, provider));
  if (!result) {
    LOG(("GearsGeolocation::LocationUpdateAvailable() : Failed to send "
         "message.\n"));
    assert(false);
    return false;
  }
  return true;
}

// ThreadMessageQueue::HandlerInterface implementation.
void GearsGeolocation::HandleThreadMessage(int message_type,
                                           MessageData *message_data) {
  assert(message_data);
  // Check that the message is of the right type.
  if (kLocationUpdateAvailable != message_type) {
    return;
  }

  LocationAvailableMessageData *data =
      reinterpret_cast<LocationAvailableMessageData*>(message_data);

  // Only respond to messages sent by this object.
  if (this != data->object) {
    return;
  }

  // Invoke the implementation.
  data->object->LocationUpdateAvailableImpl(data->provider);
}

// Non-API methods

void GearsGeolocation::GetPositionFix(JsCallContext *context, bool repeats) {
  // Get the arguments.
  std::vector<std::string16> urls;
  FixRequestInfo *info = new FixRequestInfo();
  if (!ParseArguments(context, repeats, &urls, info)) {
    assert(context->is_exception_set());
    return;
  }

  // Add the providers. The lifetime of the providers is handled by the location
  // provider pool, through Register and Unregister.
  std::string16 host_name = EnvPageSecurityOrigin().host();
  LocationProviderPool *pool = LocationProviderPool::GetInstance();

  // Native providers
#ifdef WINCE
  // TODO(steveblock): Add GPS for WinCE
  //info->providers.push_back(pool->Register(STRING16(L"GPS"),
  //                          host_name, this));
#endif

  // Network providers
  for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
    // Check if the url is valid. If not, skip this URL. This also handles the
    // case where the URL is 'GPS', which would confuse the location provider
    // pool.
    GURL url(urls[i]);
    if (url.is_valid()) {
      info->providers.push_back(pool->Register(urls[i], host_name, this));
    }
  }

  // Record this fix. This updates the map of providers and fix requests.
  MutexLock lock(&providers_mutex_);
  RecordNewFixRequest(info);

  // Store and return the ID of this fix if it repeats.
  if (info->repeats) {
    context->SetReturnValue(JSPARAM_INT, &next_watch_id_);
    watches_[next_watch_id_++] = info;
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

void GearsGeolocation::HandleRepeatingRequestUpdate(
    LocationProviderBase *provider,
    FixRequestInfo *fix_info) {
  assert(fix_info->repeats);
  // If the postion has changed significantly, and the minimum time since our
  // last callback to JavaScript has elapsed, we make a callback.
  if (PositionHasChanged(fix_info->last_position, last_position_)) {
    // The position has changed significantly.
    int64 elapsed_time =
        GetCurrentTimeMillis() - fix_info->last_callback_time;
    if (elapsed_time > kMinimumCallbackInterval) {
      if (!MakeCallback(fix_info)) {
        LOG(("GearsGeolocation::HandleRepeatingRequestUpdate() : JavaScript "
             "callback failed.\n"));
        assert(false);
      }
    } else {
      // TODO(steveblock): Start a timer and call back exactly when the
      // minimum time period has elapsed.
    }
  }
}

void GearsGeolocation::HandleSingleRequestUpdate(
    LocationProviderBase *provider,
    FixRequestInfo *fix_info) {
  assert(!fix_info->repeats);
  // Remove this provider from the this fix so that future to callbacks to this
  // Geolocation object don't trigger handling for this fix.
  RemoveProvider(provider, fix_info);
  // We callback in two cases ...
  // - This response gives a good position
  // - The fix has no remaining providers, so we'll never get a valid position
  // We then cancel any pending requests and delete the fix request.
  if (last_position_.IsGoodFix() || fix_info->providers.empty()) {
    // There should not have been a previous callback.
    assert(fix_info->last_callback_time == 0);
    if (!MakeCallback(fix_info)) {
      LOG(("GearsGeolocation::HandleSingleRequestUpdate() : JavaScript "
           "callback failed.\n"));
      assert(false);
    }
    RemoveFixRequest(fix_info);
  }
}

void GearsGeolocation::LocationUpdateAvailableImpl(
    LocationProviderBase *provider) {
  // Update the last known position, which is the best fix we currently have.
  // this is the only position value we ever return to JavaScript.
  //
  // We share the current position between all fix requests. This means that a
  // given fix request may produce a callback with a position which was obtained
  // by a different fix request, which may have had different options specified.
  Position position;
  provider->GetPosition(&position);
  last_position_mutex_.Lock();
  if (IsBetterFix(last_position_, position)) {
    last_position_ = position;
  }
  last_position_mutex_.Unlock();
  // Even if this fix isn't an improvement, we may still make a callback.
  //
  // Iterate over all fix requests of which this provider is a part.
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
      if (fix_info->repeats) {
        HandleRepeatingRequestUpdate(provider, fix_info);
      } else {
        HandleSingleRequestUpdate(provider, fix_info);
      }
    }
  }
}

bool GearsGeolocation::MakeCallback(FixRequestInfo *fix_info) {
  scoped_ptr<JsObject> callback_param(GetJsRunner()->NewObject());
  assert(callback_param.get());
  if (!ConvertPositionToJavaScriptObject(last_position_,
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
  fix_info->last_position = last_position_;
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
    result &= js_object->SetPropertyObject(STRING16(L"address"),
                                           address_object.get());
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

// Helper function for IsBetterFix and PositionHasChanged. Checks whether the
// old or new position is bad or unintialised, in which case the decision of
// which to use is easy. Return value is true if this is the case detected, in
// which cases result indicates whether the new position is the better fix.
static bool CheckForBadPosition(const Position &old_position,
                                const Position &new_position,
                                bool *result) {
  assert(new_position.IsInitialized());
  assert(result);
  // If the old position is unitialized or bad, always return true. A newer
  // bad fix is better because we want the updated error message.
  if (!old_position.IsInitialized() || !old_position.IsGoodFix()) {
    *result = true;
    return true;
  }
  // The old position is good, so if the new fix is bad, return false.
  if (!new_position.IsGoodFix()) {
    *result = false;
    return true;
  }
  return false;
}

// TODO(steveblock): Do something more intelligent here.
static bool IsBetterFix(const Position &old_position,
                        const Position &new_position) {
  bool result;
  if (CheckForBadPosition(old_position, new_position, &result)) {
    return result;
  }
  // Both are valid.
  int64 current_time = GetCurrentTimeMillis();
  int64 old_position_age = current_time - old_position.timestamp;
  int64 new_position_age = current_time - new_position.timestamp;
  const int64 kMaximumPositionFixAge = 60 * 1000;  // 1 minute.
  if (old_position_age > kMaximumPositionFixAge &&
      new_position_age > kMaximumPositionFixAge) {
    // Both positions are out of date, choose the most recent.
    return new_position_age < old_position_age;
  } else if (old_position_age > kMaximumPositionFixAge) {
    return true;
  } else if (new_position_age > kMaximumPositionFixAge) {
    return false;
  }
  // Both are in date, choose the most accurate.
  return new_position.horizontal_accuracy < old_position.horizontal_accuracy;
}

// TODO(steveblock): Do something more intelligent here.
static bool PositionHasChanged(const Position &old_position,
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
  // The threshold is when the distance between the two positions exceeds 10% of
  // the lesser of the two accuracies.
  double min_accuracy = std::min(old_position.horizontal_accuracy,
                                 new_position.horizontal_accuracy);
  return delta / min_accuracy > 0.1;
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