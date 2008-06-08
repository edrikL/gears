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

#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"

// TODO(steveblock): Update default URL when finalized.
static const char16 *kDefaultLocationProviderUrl =
    STRING16(L"http://www.google.com/");
static const char16 *kEnableHighAccuracy = STRING16(L"enableHighAccuracy");
static const char16 *kRequestAddress = STRING16(L"requestAddress");
static const char16 *kAddressLanguage = STRING16(L"addressLanguage");
static const char16 *kGearsLocationProviderUrls =
    STRING16(L"gearsLocationProviderUrls");

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

DECLARE_GEARS_WRAPPER(GearsGeolocation);

template<>
void Dispatcher<GearsGeolocation>::Init() {
  RegisterProperty("lastPosition", &GearsGeolocation::GetLastPosition, NULL);
  RegisterMethod("getCurrentPosition", &GearsGeolocation::GetCurrentPosition);
  RegisterMethod("watchPosition", &GearsGeolocation::WatchPosition);
  RegisterMethod("clearWatch", &GearsGeolocation::ClearWatch);
}

GearsGeolocation::GearsGeolocation()
    : ModuleImplBaseClassVirtual("GearsGeolocation"),
      next_fix_request_id_(1) {
}

GearsGeolocation::~GearsGeolocation() {
}

// API Methods

void GearsGeolocation::GetLastPosition(JsCallContext *context) {
  // TODO(steveblock): Remove hard-coded data.
  double latitude = 37.784;  // Moscone Center, San Francisco
  double longitude = -122.399;

  scoped_ptr<JsObject> return_object(GetJsRunner()->NewObject());

  // TODO(steveblock): Add remaining object properties.
  if (!return_object->SetPropertyDouble(STRING16(L"latitude"), latitude) ||
      !return_object->SetPropertyDouble(STRING16(L"longitude"), longitude)) {
    context->SetException(STRING16(L"Failed to set return value."));
    return;
  }

  context->SetReturnValue(JSPARAM_OBJECT, return_object.get());
}

void GearsGeolocation::GetCurrentPosition(JsCallContext *context) {
  GetPositionFix(context, false);
}

void GearsGeolocation::WatchPosition(JsCallContext *context) {
  GetPositionFix(context, true);
}

void GearsGeolocation::ClearWatch(JsCallContext *context) {
  int id;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &id },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) {
    return;
  }
  // TODO(steveblock): Cancel the fix.
}

// Non-API method.
void GearsGeolocation::GetPositionFix(JsCallContext *context, bool repeats) {
  // Get the arguments.
  std::vector<std::string16> urls;
  FixRequestInfo info;
  if (!ParseArguments(context, repeats, &urls, &info)) {
    assert(context->is_exception_set());
    return;
  }

  // TODO(steveblock): Create a set of LocationProviders and request them to get
  // a position fix.

  // Return the ID of this fix if it repeats.
  if (info.repeats) {
    context->SetReturnValue(JSPARAM_INT, &next_fix_request_id_);
  }

  // Store the fix info and increment the id for next time.
  ++next_fix_request_id_;
}

// LocationProviderBase::ListenerInterface implementation.
bool GearsGeolocation::LocationUpdateAvailable(
    LocationProviderBase *provider, const Position &position) {
  // TODO(steveblock): Determine the fix request(s) to which this provider
  // belongs. Update our current position estimate and call back to JavaScript.
  return true;
}

bool GearsGeolocation::ParseArguments(JsCallContext *context,
                                      bool repeats,
                                      std::vector<std::string16> *urls,
                                      GearsGeolocation::FixRequestInfo *info) {
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
  scoped_ptr<JsRootedCallback> scoped_function(function);
  assert(scoped_function.get());

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
                                    GearsGeolocation::FixRequestInfo *info) {
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
    const char16 *error,
    JsRunnerInterface *js_runner,
    JsObject *js_object) {
  assert(js_object);
  assert(position.IsValid());
  bool result = true;
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
  result &= SetObjectPropertyIfValidString(STRING16(L"errorMessage"),
                                           error,
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

#endif  // OFFICIAL_BUILD
