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

#include "gears/geolocation/geolocation.h"

#include "gears/base/common/module_wrapper.h"

// TODO(steveblock): Update default URL when finalized.
static const char16 *kDefaultLocationProviderUrl =
    STRING16(L"http://www.google.com/");
static const char16 *kEnableHighAccuracy = STRING16(L"enableHighAccuracy");
static const char16 *kRequestAddress = STRING16(L"requestAddress");
static const char16 *kGearsLocationProviderUrls =
    STRING16(L"gearsLocationProviderUrls");

// Local functions
static bool ParseOptions(JsCallContext *context, bool repeats,
                         std::vector<std::string16> *urls,
                        GearsGeolocation::FixRequestInfo *info);

DECLARE_GEARS_WRAPPER(GearsGeolocation);

template<>
void Dispatcher<GearsGeolocation>::Init() {
  RegisterMethod("lastPosition", &GearsGeolocation::GetLastPosition);
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
  // TODO(steveblock): Implement me.
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
  if (context->GetArguments(ARRAYSIZE(argv), argv) != ARRAYSIZE(argv)) {
    assert(context->is_exception_set());
    return;
  }
  // TODO(steveblock): Cancel the fix.
}

// Non-API methods

void GearsGeolocation::GetPositionFix(JsCallContext *context, bool repeats) {
  // Get the arguments.
  std::vector<std::string16> urls;
  FixRequestInfo info;
  ParseOptions(context, repeats, &urls, &info);

// TODO(steveblock): Create a set of LocationProviders and request them to get
  // a position fix.

  context->SetReturnValue(JSPARAM_INT, &next_fix_request_id_);
  ++next_fix_request_id_;
}

// LocationProviderInterface::ListenerInterface implementation.

bool GearsGeolocation::LocationUpdateAvailable(
    LocationProviderInterface *provider, const Position &position) {
  // TODO(steveblock): Determine the fix request(s) to which this provider
  // belongs. Update our current position estimate and call back to JavaScript.
  return true;
}

// Local functions

// Gets the requested property only if it is specified. Returns true on success.
bool GetPropertyIfSpecified(JsCallContext *context, const JsObject &object,
                            const std::string16 &name, JsScopedToken *token) {
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

static bool ParseOptions(JsCallContext *context, bool repeats,
                         std::vector<std::string16> *urls,
                         GearsGeolocation::FixRequestInfo *info) {
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
  // We have to check that options is present because it's not valid to use an
  // uninitialised JsObject.
  if (num_arguments > 1) {
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
    if (GetPropertyIfSpecified(context, options, kGearsLocationProviderUrls,
                               &token)) {
      std::string16 error = STRING16(L"options.");
      error += kGearsLocationProviderUrls;
      error += STRING16(L" should be null or an array of strings.");
      if (JsTokenGetType(token, context->js_context()) == JSPARAM_ARRAY) {
        // gearsLocationProviderUrls is an array.
        JsArray js_array;
        if (!js_array.SetArray(token, context->js_context())) {
          LOG(("GearsGeolocation::ParseOptions() : Failed to set array with "
               "gearsLocationProviderUrls."));
          assert(false);
          return false;
        }
        int length;
        if (!js_array.GetLength(&length)) {
          LOG(("GearsGeolocation::ParseOptions() : Failed to get length of "
               "gearsLocationProviderUrls."));
          assert(false);
          return false;
        }
        for (int i = 0; i < length; ++i) {
          JsScopedToken token;
          if (!js_array.GetElement(i, &token)) {
            LOG(("GearsGeolocation::ParseOptions() : Failed to get element "
                 "from gearsLocationProviderUrls."));
            assert(false);
            return false;
          }
          std::string16 url;
          if (!JsTokenToString_NoCoerce(token, context->js_context(), &url)) {
            context->SetException(error);
            return false;
          }
          urls->push_back(url);
        }
      } else if (JsTokenGetType(token, context->js_context()) != JSPARAM_NULL) {
        // If gearsLocationProviderUrls is null, we do not use the default URL.
        // If it's not an array and not null, this is an error.
        context->SetException(error);
        return false;
      }
    } else {
    // gearsLocationProviderUrls is not specified, so use the default URL.
    urls->push_back(kDefaultLocationProviderUrl);
    }
  } else {
    // options is not specified, so use the default URL.
    urls->push_back(kDefaultLocationProviderUrl);
  }
  return true;
}

#ifdef USING_CCTESTS
// This method is defined in cctests\test.h and is used only for testing as
// a means to access the static function defined here.
bool ParseGeolocationOptionsTest(JsCallContext *context, bool repeats,
                                 std::vector<std::string16> *urls,
                                 GearsGeolocation::FixRequestInfo *info) {
  return ParseOptions(context, repeats, urls, info);
}
#endif
