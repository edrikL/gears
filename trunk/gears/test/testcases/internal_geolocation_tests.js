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

// Error code values. These values must match those in geolocation.h.
var ERROR_CODE_POSITION_UNAVAILABLE = 2;

if (isUsingCCTests) {
  var internalTests = google.gears.factory.create('beta.test');
}

// Tests the parsing of the options passed to Geolocation.GetCurrentPosition and
// Geolocation.WatchPosition.
function testParseOptions() {
  if (isUsingCCTests) {
    var dummyFunction = function() {};

    // Test correct parsing.
    var defaultUrlArray = ['http://www.google.com/loc/json'];
    var urls = ['url1', 'url2'];
    var parsedOptions;

    // No options.
    parsedOptions = internalTests.testParseGeolocationOptions(dummyFunction);
    assertEqual(false, parsedOptions.repeats);
    assertEqual(false, parsedOptions.enableHighAccuracy);
    assertEqual(false, parsedOptions.gearsRequestAddress);
    assertEqual('', parsedOptions.gearsAddressLanguage);
    assertArrayEqual(defaultUrlArray, parsedOptions.gearsLocationProviderUrls);

    // Empty options.
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction, dummyFunction, {});
    assertEqual(false, parsedOptions.repeats);
    assertEqual(false, parsedOptions.enableHighAccuracy);
    assertEqual(false, parsedOptions.gearsRequestAddress);
    assertEqual('', parsedOptions.gearsAddressLanguage);
    assertArrayEqual(defaultUrlArray, parsedOptions.gearsLocationProviderUrls);

    // Empty provider URLs.
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction, dummyFunction, {gearsLocationProviderUrls: []});
    assertEqual(false, parsedOptions.repeats);
    assertEqual(false, parsedOptions.enableHighAccuracy);
    assertEqual(false, parsedOptions.gearsRequestAddress);
    assertEqual('', parsedOptions.gearsAddressLanguage);
    assertArrayEqual([], parsedOptions.gearsLocationProviderUrls);

    // Null provider URLs.
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction, dummyFunction, {gearsLocationProviderUrls: null});
    assertEqual(false, parsedOptions.repeats);
    assertEqual(false, parsedOptions.enableHighAccuracy);
    assertEqual(false, parsedOptions.gearsRequestAddress);
    assertEqual('', parsedOptions.gearsAddressLanguage);
    assertArrayEqual([], parsedOptions.gearsLocationProviderUrls);

    // All properties false, provider URLs set.
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction,
        dummyFunction,
        {
          enableHighAccuracy: false,
          gearsRequestAddress: false,
          gearsAddressLanguage: '',
          gearsLocationProviderUrls: urls
        });
    assertEqual(false, parsedOptions.repeats);
    assertEqual(false, parsedOptions.enableHighAccuracy);
    assertEqual(false, parsedOptions.gearsRequestAddress);
    assertEqual('', parsedOptions.gearsAddressLanguage);
    assertArrayEqual(urls, parsedOptions.gearsLocationProviderUrls);

    // All properties true, provider URLs set.
    parsedOptions = internalTests.testParseGeolocationOptions(
      dummyFunction,
      dummyFunction,
      {
        enableHighAccuracy: true,
        gearsRequestAddress: true,
        gearsAddressLanguage: 'test',
        gearsLocationProviderUrls: urls
      });
    assertEqual(false, parsedOptions.repeats);
    assertEqual(true, parsedOptions.enableHighAccuracy);
    assertEqual(true, parsedOptions.gearsRequestAddress);
    assertEqual('test', parsedOptions.gearsAddressLanguage);
    assertArrayEqual(urls, parsedOptions.gearsLocationProviderUrls);
  }
}

// Tests forming the JSON request body for a network location provider. Note
// that the inputs to the conversion are set on the C++ side.
function testFormRequestBody() {
  if (isUsingCCTests) {
    var body = internalTests.testGeolocationFormRequestBody();
    var correctBody = '{ ' +
                      '"access_token" : "access token", ' +
                      '"address_language" : "en-GB", ' +
                      '"cell_towers" : [ { ' +
                      '"cell_id" : 23874, ' +
                      '"location_area_code" : 98, ' +
                      '"mobile_country_code" : 234, ' +
                      '"mobile_network_code" : 15, ' +
                      '"signal_strength" : -65 ' +
                      '} ], ' +
                      '"host" : "www.google.com", ' +
                      '"radio_type" : "gsm", ' +
                      '"request_address" : true, ' +
                      '"version" : "1.1.0", ' +
                      '"wifi_towers" : [ { ' +
                      '"age" : 15, ' +
                      '"channel" : 19, ' +
                      '"mac_address" : "00-0b-86-d7-6a-42", ' +
                      '"signal_strength" : -50, ' +
                      '"signal_to_noise" : 10, ' +
                      '"ssid" : "Test SSID" ' +
                      '} ] ' +
                      '}\n';  // Note trailing line break.
    assertEqual(correctBody, body);
  }
}

// Tests extracting a position object from the JSON response from a network
// location provider.
function testGetLocationFromResponse() {
  if (isUsingCCTests) {
    var dummy_server = 'http://test.server.com';
    var malformedResponseError = 'Response from network provider at ' +
                                 dummy_server +
                                 ' was malformed.';
    var noGoodFixError = 'Network provider at ' +
                         dummy_server +
                         ' did not provide a good position fix.'
    var position;
    var correctPosition;

    // Error code values. These values must match those in geolocation.h.
    var ERROR_CODE_POSITION_UNAVAILABLE = 2;

    // Test good response with valid position.
    var responseBody = '{ ' +
                       '"location" : { ' +
                       '"latitude" : 53.1, ' +
                       '"longitude" : -0.1, ' +
                       '"altitude" : 30.1, ' +
                       '"accuracy" : 1200.1, ' +
                       '"altitude_accuracy" : 10.1, ' +
                       '"address" : { ' +
                       '"street_number": "100", ' +
                       '"street": "Amphibian Walkway", ' +
                       '"city": "Mountain View", ' +
                       '"county": "Mountain View County", ' +
                       '"region": "California", ' +
                       '"country": "United States of America", ' +
                       '"country_code": "US", ' +
                       '"postal_code": "94043" ' +
                       '} ' +
                       '} ' +
                       '}';
    position = internalTests.testGeolocationGetLocationFromResponse(
        true,  // HttpPost result
        200,   // status code
        responseBody,
        42,    // timestamp
        '');   // server URL
    correctPosition = new Object();
    correctPosition.latitude = 53.1;
    correctPosition.longitude = -0.1;
    correctPosition.altitude = 30.1;
    correctPosition.accuracy = 1200.1;
    correctPosition.altitudeAccuracy = 10.1;
    correctPosition.gearsAddress = new Object();
    correctPosition.gearsAddress.streetNumber = '100';
    correctPosition.gearsAddress.street = 'Amphibian Walkway';
    correctPosition.gearsAddress.city = 'Mountain View';
    correctPosition.gearsAddress.county = 'Mountain View County';
    correctPosition.gearsAddress.region = 'California';
    correctPosition.gearsAddress.country = 'United States of America';
    correctPosition.gearsAddress.countryCode = 'US';
    correctPosition.gearsAddress.postalCode = '94043';
    correctPosition.timestamp = new Date(42);
    assertObjectEqual(correctPosition, position);

    // We should also accept integer values for floating point fields.
    var responseBody = '{ ' +
                       '"location" : { ' +
                       '"latitude" : 53, ' +
                       '"longitude" : 0, ' +
                       '"altitude" : 30, ' +
                       '"accuracy" : 1200, ' +
                       '"altitude_accuracy" : 10, ' +
                       '"address" : { ' +
                       '"street_number": "100", ' +
                       '"street": "Amphibian Walkway", ' +
                       '"city": "Mountain View", ' +
                       '"county": "Mountain View County", ' +
                       '"region": "California", ' +
                       '"country": "United States of America", ' +
                       '"country_code": "US", ' +
                       '"postal_code": "94043" ' +
                       '} ' +
                       '} ' +
                       '}';
    position = internalTests.testGeolocationGetLocationFromResponse(
        true,  // HttpPost result
        200,   // status code
        responseBody,
        42,    // timestamp
        '');   // server URL
    correctPosition.latitude = 53;
    correctPosition.longitude = 0;
    correctPosition.altitude = 30;
    correctPosition.accuracy = 1200;
    correctPosition.altitudeAccuracy = 10;
    assertObjectEqual(correctPosition, position);

    // Test no response.
    position = internalTests.testGeolocationGetLocationFromResponse(
        false,  // HttpPost result
        0,      // status code
        '',     // response body
        0,      // timestamp
        dummy_server);
    correctPosition = new Object();
    correctPosition.code = ERROR_CODE_POSITION_UNAVAILABLE;
    correctPosition.message = 'No response from network provider at ' +
                              dummy_server +
                              '.';
    assertObjectEqual(correctPosition, position);

    // Test bad response.
    position = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        400,    // status code
        '',     // response body
        0,      // timestamp
        dummy_server);
    correctPosition = new Object();
    correctPosition.code = ERROR_CODE_POSITION_UNAVAILABLE;
    correctPosition.message = 'Network provider at ' +
                              dummy_server +
                              ' returned error code 400.';
    assertObjectEqual(correctPosition, position);

    // Test good response with malformed body.
    position = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        'malformed response body',
        0,      // timestamp
        dummy_server);
    correctPosition = new Object();
    correctPosition.code = ERROR_CODE_POSITION_UNAVAILABLE;
    correctPosition.message = malformedResponseError;
    assertObjectEqual(correctPosition, position);

    // Test good response with empty body.
    position = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        '',     // response body
        0,      // timestamp
        dummy_server);
    correctPosition = new Object();
    correctPosition.code = ERROR_CODE_POSITION_UNAVAILABLE;
    correctPosition.message = malformedResponseError;
    assertObjectEqual(correctPosition, position);

    // Test good response where body is not an object.
    position = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        '"a string"',
        0,      // timestamp
        dummy_server);
    correctPosition = new Object();
    correctPosition.code = ERROR_CODE_POSITION_UNAVAILABLE;
    correctPosition.message = malformedResponseError;
    assertObjectEqual(correctPosition, position);

    // Test good response with unknown position.
    position = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        '{}',   // response body
        0,      // timestamp
        dummy_server);
    correctPosition = new Object();
    correctPosition.code = ERROR_CODE_POSITION_UNAVAILABLE;
    correctPosition.message = noGoodFixError;
    assertObjectEqual(correctPosition, position);

    // Test good response with explicit unknown position.
    position = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        '{"position": null}',
        0,      // timestamp
        dummy_server);
    correctPosition = new Object();
    correctPosition.code = ERROR_CODE_POSITION_UNAVAILABLE;
    correctPosition.message = noGoodFixError;
    assertObjectEqual(correctPosition, position);
  }
}

function testMockDeviceDataProvider() {
  if (isUsingCCTests) {
    var geolocation = google.gears.factory.create('beta.geolocation');
    var mockNetworkLocationProvider = '/testcases/cgi/location_provider.py';

    function makeSuccessfulRequest() {
      internalTests.configureGeolocationWifiDataProviderForTest(
          {mac_address: 'good_mac_address'});
      geolocation.getCurrentPosition(
          successCallback,
          function() {
            assert(false, 'makeSuccessfulRequest failed');
          },
          {
            gearsRequestAddress: true,
            gearsLocationProviderUrls: [mockNetworkLocationProvider]
          });
    }

    function makeUnsuccessfulRequest() {
      internalTests.configureGeolocationWifiDataProviderForTest(
          {mac_address: 'no_location_mac_address'});
      geolocation.getCurrentPosition(
          function() {
            assert(false, 'makeUnsuccessfulRequest succeeded');
          },
          noPositionErrorCallback,
          {gearsLocationProviderUrls: [mockNetworkLocationProvider]});
    }

    function makeMalformedRequest() {
      internalTests.configureGeolocationWifiDataProviderForTest(
          {mac_address: '00-00-00-00-00-00'});
      internalTests.configureGeolocationRadioDataProviderForTest({cell_id: 88});
      geolocation.getCurrentPosition(
          function() {
            assert(false, 'makeMalformedRequest succeeded');
          },
          malformedRequestErrorCallback,
          {gearsLocationProviderUrls: [mockNetworkLocationProvider]});
    }

    function successCallback(position) {
      var correctPosition = {
        latitude: 51.59,
        longitude: -1.49,
        altitude: 30,
        accuracy: 1200,
        altitudeAccuracy: 10,
        gearsAddress: {
          streetNumber: '76',
          street: 'Buckingham Palace Road',
          postalCode: 'SW1W 9TQ',
          city: 'London',
          county: 'London',
          region: 'London',
          country: 'United Kingdom',
          countryCode: 'uk'
        }
      };
      position.timestamp = undefined;
      assertObjectEqual(correctPosition, position);
      makeUnsuccessfulRequest();
    };

    function noPositionErrorCallback(error) {
      assertEqual(ERROR_CODE_POSITION_UNAVAILABLE, error.code);
      assert(error.message.search('did not provide a good position fix') > 0);
      makeMalformedRequest();
    };

    function malformedRequestErrorCallback(error) {
      assertEqual(ERROR_CODE_POSITION_UNAVAILABLE, error.code);
      assert(error.message.search('returned error code 400') > 0);
      completeAsync();
    };

    startAsync();
    makeSuccessfulRequest();
  }
}

function testMockProvider() {
  if (isUsingCCTests) {
    // Use mock location provider.
    var mockPosition = {
      latitude: 51.0,
      longitude: -0.1,
      accuracy: 100.1
    };
    internalTests.configureGeolocationMockLocationProviderForTest(mockPosition);
    function locationAvailable(position) {
      assertEqualAnyType(mockPosition.latitude, position.latitude);
      assertEqualAnyType(mockPosition.longitude, position.longitude);
      assertEqualAnyType(mockPosition.accuracy, position.accuracy);
      internalTests.removeGeolocationMockLocationProvider();
      completeAsync();
    };
    var geolocation = google.gears.factory.create('beta.geolocation');
    startAsync();
    geolocation.getCurrentPosition(locationAvailable,
                                   function() {},
                                   {gearsLocationProviderUrls: []});
  }
}

function testRadioDataProvider() {
  // This test only works on the Android emulator.
  // On WinCE, we cannot test the RIL library while
  // desktop platforms do not use radio data.
  if (isUsingCCTests && isAndroid) {
    startAsync();
    internalTests.removeMockRadioDataProvider();
    internalTests.testRadioDataProvider(completeAsync);
  }
}

function testWifiDataProvider() {
  // This test only works on the Android emulator.
  if (isUsingCCTests && isAndroid) {
    startAsync();
    internalTests.removeMockWifiDataProvider();
    internalTests.testWifiDataProvider(completeAsync);
  }
}

function testMockGpsDevice() {
  if (isUsingCCTests) {
    var geolocation = google.gears.factory.create('beta.geolocation');
    var options = {
      enableHighAccuracy: true,
      gearsLocationProviderUrls: null
    };

    // GPS is available only on WinCE and Android.
    if (isWince || isAndroid) {
      var mockPositionOne = {
        latitude: -33.0,
        longitude: 137.0,
        accuracy: 100.0
      };
      function setPositionOne() {
        internalTests.configureGeolocationMockGpsDeviceForTest(mockPositionOne);
        geolocation.getCurrentPosition(callbackOne, null, options);
      }
      function callbackOne(position) {
        position.timestamp = undefined;
        assertObjectEqual(mockPositionOne, position);
        setPositionTwo();
      }

      var mockPositionTwo = {
        latitude: 45.0,
        longitude: -100.0,
        accuracy: 100.0
      };
      function setPositionTwo() {
        internalTests.configureGeolocationMockGpsDeviceForTest(mockPositionTwo);
        geolocation.getCurrentPosition(callbackTwo, null, options);
      }
      function callbackTwo(position) {
        position.timestamp = undefined;
        assertObjectEqual(mockPositionTwo, position);
        completeAsync();
      }

      // Start the tests.
      startAsync();
      setPositionOne();
    } else {
      // On desktop platforms, these options mean that no location providers
      // are specified, so we expect an exception.
      assertError(
          function() {geolocation.watchPosition(function() {}, null, options);},
          'Fix request has no location providers.');
    }
  }
}

function testReverseGeocoding() {
  if (isUsingCCTests) {
    // GPS is available only on WinCE and Android.
    if (isWince || isAndroid) {
      var geolocation = google.gears.factory.create('beta.geolocation');

      // This network location provider will always fail to provide a position
      // for all location requests. For all reverse-geocoding requests, it
      // returns a fixed address.
      var reverseGeocoder = '/testcases/cgi/reverse_geocoder.py';

      var options = {
        enableHighAccuracy: true,
        gearsRequestAddress: true,
        gearsLocationProviderUrls: [reverseGeocoder]
      };

      var mockGpsPosition = {
        latitude: 0.1,
        longitude: 2.3,
        altitude: 4,
        accuracy: 5,
        altitudeAccuracy: 6
      };

      function successCallback(position) {
        var correctPosition = mockGpsPosition;
        // This is hard-coded in reverse_geocoder.py.
        correctPosition.gearsAddress = {
          streetNumber: '76',
          street: 'Buckingham Palace Road',
          postalCode: 'SW1W 9TQ',
          city: 'London',
          county: 'London',
          region: 'London',
          country: 'United Kingdom',
          countryCode: 'uk'
        };
        position.timestamp = undefined;
        assertObjectEqual(correctPosition, position);
        completeAsync();
      }

      function errorCallback(error) {
        assert(false, 'testReverseGeocoding failed: ' + error);
      }

      internalTests.configureGeolocationMockGpsDeviceForTest(mockGpsPosition);
      startAsync();
      geolocation.getCurrentPosition(successCallback, errorCallback, options);
    }
  }
}
