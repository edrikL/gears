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

#ifdef OFFICIAL_BUILD
// The Geolocation API has not been finalized for official builds.
#else

#if USING_CCTESTS

#include "gears/base/common/js_types.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/scoped_refptr.h"
#include "gears/blob/blob.h"
#include "gears/geolocation/geolocation.h"
#include "gears/geolocation/device_data_provider.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

// From geolocation.cc
bool ParseGeolocationOptionsTest(JsCallContext *context,
                                 bool repeats,
                                 std::vector<std::string16> *urls,
                                 GearsGeolocation::FixRequestInfo *info);
bool ConvertPositionToJavaScriptObjectTest(const Position &p,
                                           const char16 *error,
                                           JsRunnerInterface *js_runner,
                                           JsObject *js_object);

// From network_location_request.cc
bool FormRequestBodyTest(const std::string &host_name,
                         const RadioData &radio_data,
                         const WifiData &wifi_data,
                         scoped_refptr<BlobInterface> *blob);
bool GetLocationFromResponseTest(const std::vector<uint8> &response,
                                 Position *position);

void TestParseGeolocationOptions(JsCallContext *context,
                                 JsRunnerInterface *js_runner) {
  std::vector<std::string16> urls;
  GearsGeolocation::FixRequestInfo info;
  if (!ParseGeolocationOptionsTest(context, false, &urls, &info)) {
    if (!context->is_exception_set()) {
      context->SetException(
          STRING16(L"Internal error parsing geolocation options."));
    }
    return;
  }
  // Add the urls as a property of the returned object.
  scoped_ptr<JsObject> return_object(js_runner->NewObject());
  assert(return_object.get());
  scoped_ptr<JsArray> url_array(js_runner->NewArray());
  assert(url_array.get());
  for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
    if (!url_array->SetElementString(i, urls[i])) {
      context->SetException(STRING16(L"Failed to set return value."));
      return;
    }
  }
  if (!return_object->SetPropertyBool(STRING16(L"repeats"), info.repeats) ||
      !return_object->SetPropertyBool(STRING16(L"enableHighAccuracy"),
                                      info.enable_high_accuracy) ||
      !return_object->SetPropertyBool(STRING16(L"requestAddress"),
                                      info.request_address) ||
      !return_object->SetPropertyArray(STRING16(L"gearsLocationProviderUrls"),
                                       url_array.get())) {
    context->SetException(STRING16(L"Failed to set return value."));
    return;
  }
  context->SetReturnValue(JSPARAM_OBJECT, return_object.get());
}

void TestGeolocationFormRequestBody(JsCallContext *context) {
  // Parsing radio and wifi data from JavaScript would involve more complex code
  // than that which we're trying to test. Instead we hard-code these values.
  // The values must be kept in sync with internal_tests.js.

  RadioData radio_data;
  CellData cell_data;
  cell_data.cid = 23874;
  cell_data.lac = 98;
  cell_data.mnc = 15;
  cell_data.mcc = 234;
  cell_data.rss = -65;
  radio_data.cell_data.push_back(cell_data);
  radio_data.radio_type = RADIO_TYPE_GSM;

  WifiData wifi_data;
  AccessPointData access_point_data;
  access_point_data.mac = STRING16(L"test mac");
  wifi_data.access_point_data.push_back(access_point_data);

  scoped_refptr<BlobInterface> blob;
  if (!FormRequestBodyTest("www.google.com", radio_data, wifi_data, &blob)) {
    context->SetException(STRING16(L"Failed to form request body."));
    return;
  }
  assert(blob.get());
  int64 length = blob->Length();
  // We only handle the case where the size of the blob is less than kuint32max.
  // All good request bodies should be much smaller than this.
  assert(length > 0 && length < kuint32max);
  std::vector<uint8> request_body_vector;
  request_body_vector.resize(static_cast<unsigned int>(length));
  if (!blob->Read(&request_body_vector[0], 0, length)) {
    context->SetException(STRING16(L"Failed to extract request string from "
                                   L"blob."));
    return;
  }
  std::string16 request_body;
  if (!UTF8ToString16(reinterpret_cast<char*>(&request_body_vector[0]),
                      request_body_vector.size(),
                      &request_body)) {
    context->SetException(STRING16(L"Failed to convert request string from "
                                   L"UTF8."));
    return;
  }
  context->SetReturnValue(JSPARAM_STRING16, &request_body);
} 

void TestGeolocationGetLocationFromResponse(JsCallContext *context,
                                 JsRunnerInterface *js_runner) {
  std::string16 response_string;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &response_string },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) {
    return;
  }
  std::string response_string_utf8;
  if (!String16ToUTF8(response_string.c_str(), response_string.size(),
                      &response_string_utf8)) {
    context->SetException(STRING16(L"Failed to convert input string to UTF8."));
    return;
  }
  std::vector<uint8> response_vector(response_string_utf8.begin(),
                                     response_string_utf8.end());
  Position position;
  if (!GetLocationFromResponseTest(response_vector, &position)) {
    context->SetException(STRING16(L"Failed to get location."));
    return;
  }
  position.timestamp = 42;
  scoped_ptr<JsObject> position_object(js_runner->NewObject());

  if (!ConvertPositionToJavaScriptObjectTest(position, STRING16(L"test error"),
                                             js_runner, position_object.get())) {
    context->SetException(STRING16(L"Failed to convert position to JavaScript"
                                   L"object."));
    return;
  }
  context->SetReturnValue(JSPARAM_OBJECT, position_object.get());
} 

#endif  // USING_CCTESTS

#endif  // OFFICIAL_BUILD
