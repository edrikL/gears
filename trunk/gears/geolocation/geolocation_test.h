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

#ifndef GEARS_GEOLOCATION_GEOLOCATION_TEST_H__
#define GEARS_GEOLOCATION_GEOLOCATION_TEST_H__

class JsCallContext;
class JsRunnerInterface;

// IN: object position_options
// OUT: object parsed_options
void TestParseGeolocationOptions(JsCallContext *context,
                                 JsRunnerInterface *js_runner);

// IN: nothing
// OUT: string request_body
void TestGeolocationFormRequestBody(JsCallContext *context);

// IN: bool http_post_result, int status_code, string response_body,
//     int64 timestamp, string server_url
// OUT: object position
void TestGeolocationGetLocationFromResponse(JsCallContext *context,
                                            JsRunnerInterface *js_runner);

// Sets the device data provider factories to use mock radio and WiFi device
// data providers. This means that subsequent calls to the methods of the
// Geolocation API will return constant data, thus allowing it to be tested.
// IN: nothing
// OUT: nothing
void ConfigureGeolocationForTest(JsCallContext *context);

#endif  // GEARS_GEOLOCATION_GEOLOCATION_TEST_H__
