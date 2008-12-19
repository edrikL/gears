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

if (isUsingCCTests) {
  var internalTests = google.gears.factory.create('beta.test');
}

// Helper function for testing that two error objects are equal.
function assertErrorEqual(expectedCode, expectedMessage, error) {
  assertEqual(0, error.UNKNOWN_ERROR, 'UKNOWN_ERROR has incorrect value.');
  assertEqual(1, error.PERMISSION_DENIED,
              'PERMISSION_DENIED has incorrect value.');
  assertEqual(2, error.POSITION_UNAVAILABLE,
              'POSITION_UNAVAILABLE has incorrect value.');
  assertEqual(3, error.TIMEOUT, 'TIMEOUT has incorrect value.');
  assertEqual(expectedCode, error.code, 'Error code has incorrect value.');
  assertEqual(expectedMessage, error.message,
              'Error message has incorrect value.');
}

// Tests the parsing of the options passed to Geolocation.GetCurrentPosition and
// Geolocation.WatchPosition.
function testParseOptions() {
  if (isUsingCCTests) {
    var dummyFunction = function() {};

    // Intialise to default values.
    var defaultLocationProviderUrl = 'http://www.google.com/loc/json'
    var correctOptions = {
      enableHighAccuracy: false,
      maximumAge: 0,
      timeout: -1,
      gearsRequestAddress: false,
      gearsAddressLanguage: '',
      gearsLocationProviderUrls: [defaultLocationProviderUrl]
    };

    var parsedOptions;

    // No options.
    parsedOptions = internalTests.testParseGeolocationOptions(dummyFunction);
    assertObjectEqual(correctOptions, parsedOptions);

    // Empty options.
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction, dummyFunction, {});
    assertObjectEqual(correctOptions, parsedOptions);

    // Empty provider URLs.
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction, dummyFunction, {gearsLocationProviderUrls: []});
    correctOptions.gearsLocationProviderUrls = [];
    assertObjectEqual(correctOptions, parsedOptions);

    // Null provider URLs.
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction, dummyFunction, {gearsLocationProviderUrls: null});
    assertObjectEqual(correctOptions, parsedOptions);

    // All properties explicitly set to default values where possible.
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction,
        dummyFunction,
        {enableHighAccuracy: false, maximumAge: 0, gearsRequestAddress: false});
    correctOptions.gearsLocationProviderUrls = [defaultLocationProviderUrl];
    assertObjectEqual(correctOptions, parsedOptions);

    // Test infinite maximum age.
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction, dummyFunction, {maximumAge: Infinity});
    correctOptions.maximumAge = -1;
    assertObjectEqual(correctOptions, parsedOptions);

    // All properties set to non-default values.
    var options = {
      enableHighAccuracy: true,
      maximumAge: 1,
      timeout: 0,
      gearsRequestAddress: true,
      gearsAddressLanguage: 'test',
      gearsLocationProviderUrls: ['url1', 'url2']
    };
    parsedOptions = internalTests.testParseGeolocationOptions(
        dummyFunction, dummyFunction, options);
    assertObjectEqual(options, parsedOptions);
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
    var error;

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
    error = internalTests.testGeolocationGetLocationFromResponse(
        false,  // HttpPost result
        0,      // status code
        '',     // response body
        0,      // timestamp
        dummy_server);
    assertErrorEqual(error.POSITION_UNAVAILABLE,
                     'No response from network provider at ' +
                     dummy_server +
                     '.',
                     error);

    // Test bad response.
    error = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        400,    // status code
        '',     // response body
        0,      // timestamp
        dummy_server);
    assertErrorEqual(error.POSITION_UNAVAILABLE,
                     'Network provider at ' +
                     dummy_server +
                     ' returned error code 400.',
                     error);

    // Test good response with malformed body.
    error = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        'malformed response body',
        0,      // timestamp
        dummy_server);
    assertErrorEqual(error.POSITION_UNAVAILABLE, malformedResponseError, error);

    // Test good response with empty body.
    error = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        '',     // response body
        0,      // timestamp
        dummy_server);
    assertErrorEqual(error.POSITION_UNAVAILABLE, malformedResponseError, error);

    // Test good response where body is not an object.
    error = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        '"a string"',
        0,      // timestamp
        dummy_server);
    assertErrorEqual(error.POSITION_UNAVAILABLE, malformedResponseError, error);

    // Test good response with unknown position.
    error = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        '{}',   // response body
        0,      // timestamp
        dummy_server);
    assertErrorEqual(error.POSITION_UNAVAILABLE, noGoodFixError, error);

    // Test good response with explicit unknown position.
    error = internalTests.testGeolocationGetLocationFromResponse(
        true,   // HttpPost result
        200,    // status code
        '{"position": null}',
        0,      // timestamp
        dummy_server);
    assertErrorEqual(error.POSITION_UNAVAILABLE, noGoodFixError, error);
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
          function(error) {
            assert(false, 'makeSuccessfulRequest failed: ' + error.message);
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
          function(position) {
            assert(false, 'makeUnsuccessfulRequest succeeded: (' +
                          position.latitude + ', ' + position.longitude + ')');
          },
          noPositionErrorCallback,
          {gearsLocationProviderUrls: [mockNetworkLocationProvider]});
    }

    function makeMalformedRequest() {
      internalTests.configureGeolocationWifiDataProviderForTest(
          {mac_address: '00-00-00-00-00-00'});
      internalTests.configureGeolocationRadioDataProviderForTest({cell_id: 88});
      geolocation.getCurrentPosition(
          function(position) {
            assert(false, 'makeMalformedRequest succeeded: (' +
                          position.latitude + ', ' + position.longitude + ')');
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
      assertEqual(error.POSITION_UNAVAILABLE, error.code);
      assert(error.message.search('did not provide a good position fix') > 0,
             'Unexpected error message');
      makeMalformedRequest();
    };

    function malformedRequestErrorCallback(error) {
      assertEqual(error.POSITION_UNAVAILABLE, error.code);
      assert(error.message.search('returned error code 400') > 0,
             'Unexpected error message');
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

function testTimeout() {
  // A zero timeout is tested in testZeroTimeout() in geolocation_tests.js. Here
  // we test that the success callback is invoked if a non-zero timeout is used.
  // We use the mock location provider to do this.
  if (isUsingCCTests) {
    var options = {
      timeout: 1000,  // Should be plenty of time for the mock provider to act.
      gearsLocationProviderUrls: []
    };
    var mockPosition = {
      latitude: 51.0,
      longitude: -0.1,
      accuracy: 100.1
    };
    internalTests.configureGeolocationMockLocationProviderForTest(mockPosition);
    function locationAvailable(position) {
      delete position.timestamp;
      assertObjectEqual(mockPosition, position);
      internalTests.removeGeolocationMockLocationProvider();
      completeAsync();
    };
    var geolocation = google.gears.factory.create('beta.geolocation');
    startAsync();
    geolocation.getCurrentPosition(locationAvailable, null, options);
  }
}

function testWatchTimeout() {
  // Here we test that in the case of a watch, we get a success callback if a
  // position fix is obtained within the timeout, and an error callback if it is
  // not. We use the mock location provider to do this.
  if (isUsingCCTests) {
    var options = {
      timeout: 200,
      gearsLocationProviderUrls: []
    };
    var mockPosition1 = {
      latitude: 51.0,
      longitude: -0.1,
      accuracy: 100.1
    };
    var mockPosition2 = {
      latitude: 52.0,
      longitude: -1.1,
      accuracy: 101.1
    };
    var state = 0;
    function successCallback(position) {
      assert(state == 0 || state == 1,
             'Success callback should be called first.');
      delete position.timestamp;
      if (state == 0) {
        state = 1;
        assertObjectEqual(mockPosition1, position);
        // Report movement then schedule a position update within the timeout
        // period.
        internalTests.reportMovementInMockLocationProvider();
        timer.setTimeout(function() {
          internalTests.configureGeolocationMockLocationProviderForTest(
              mockPosition2);
        }, 100);
      } else {
        state = 2;
        assertObjectEqual(mockPosition2, position);
        // Report movement but do not follow with a position update.
        internalTests.reportMovementInMockLocationProvider();
      }
    };
    function errorCallback(error) {
      assert(state == 2, 'Error callback should be called last.');
      assertErrorEqual(
          error.TIMEOUT,
          'A position fix was not obtained within the specified time limit.',
          error);
      geolocation.clearWatch(watchId);
      internalTests.removeGeolocationMockLocationProvider();
      completeAsync();
    };
    var geolocation = google.gears.factory.create('beta.geolocation');
    var timer = google.gears.factory.create('beta.timer');
    internalTests.configureGeolocationMockLocationProviderForTest(
        mockPosition1);
    startAsync();
    var watchId = geolocation.watchPosition(successCallback,
                                            errorCallback,
                                            options);
  }
}

// Helper function for PopulateCacheAndMakeRequest
function PopulateCachedPosition(geolocation, successFunction) {
  // Uses the mock location provider to populate the cached position with a set
  // value.
  //
  // Note that we can never guarantee that the cached position will be updated
  // by the position from by a given provider because logic in the arbitrator
  // only updates the cached position if the new position has a better accuracy.
  // To make the Pulse unit tests run reliably, the position we use here is the
  // most accurate used in the unit tests.
  //
  // To make sure that the cached position has the correct timestamp (as well as
  // the correct lat/lng), we force two updates.
  var intermediatePositionToCache = {
    latitude: -80.0,
    longitude: 0.0,
    accuracy: 1.0
  };
  function SetIntermediatePosition() {
    internalTests.configureGeolocationMockLocationProviderForTest(
        intermediatePositionToCache);
    geolocation.getCurrentPosition(
        function(position) { SetPosition(); },
        null,
        {gearsLocationProviderUrls: null});
  }
  function SetPosition() {
    internalTests.configureGeolocationMockLocationProviderForTest(
        positionToCache);
    geolocation.getCurrentPosition(
        function(position) {
          assert(geolocation.lastPosition.timestamp >= Date.parse(startTime),
                 'Last position too old in PopulateCachedPosition.');
          var lastPosition = geolocation.lastPosition;
          delete lastPosition.timestamp;
          assertObjectEqual(
              positionToCache, lastPosition,
              'Last position incorrect in PopulateCachedPosition.');
          internalTests.removeGeolocationMockLocationProvider();
          successFunction();
        },
        null,
        {gearsLocationProviderUrls: null});
  }
  var startTime = Date();
  SetIntermediatePosition();
}

// Needs to be global to be used in tests.
var positionToCache = {
  latitude: 51.0,
  longitude: -0.1,
  accuracy: 1.0
};

// Helper function for testCachedPositionXXX tests
function PopulateCacheAndMakeRequest(waitTime, successCallback, errorCallback,
                                     options) {
  var geolocation = google.gears.factory.create('beta.geolocation');
  PopulateCachedPosition(geolocation, function() {
    var timer = google.gears.factory.create('beta.timer');
    timer.setTimeout(function() {
      geolocation.getCurrentPosition(successCallback, errorCallback, options);
    }, waitTime);
  });
}

function testCachedPositionWithinMaximumAge() {
  // Test that a request with a non-zero maximumAge immediately calls the
  // success callback with the cached position if we have a suitable one.
  startAsync();
  PopulateCacheAndMakeRequest(
      0,  // Wait for less than maximumAge.
      function(position) {
        delete position.timestamp;
        assertObjectEqual(positionToCache, position,
                          'Position incorrect in TestMaximumAge.');
        completeAsync();
      },
      null,
      {maximumAge: 1000});
}

function testCachedPositionOutsideMaximumAgeNoProviders() {
  // Test that a request with a non-zero maximumAge and no location providers
  // immediately calls the error callback if we don't have a suitable cached
  // position.
  startAsync();
  PopulateCacheAndMakeRequest(
      10,  // Wait for longer than maximumAge.
      function() {},
      function(error) {
        assertErrorEqual(error.POSITION_UNAVAILABLE,
                         'No suitable cached position available.',
                         error);
        completeAsync();
      },
      {maximumAge: 1, gearsLocationProviderUrls: null});
}

function testCachedPositionOutsideMaximumAge() {
  // Test that a request with a non-zero maximumAge and location providers
  // does not immediately call the error callback if we don't have a suitable
  // cached position. We use a non-existant URL for the location provider to
  // force an error.
  startAsync();
  PopulateCacheAndMakeRequest(
      10,  // Wait for longer than maximumAge.
      function() {},
      function(error) {
        assert(error.message.search('returned error code 404.') != -1);
        error.message = null;
        assertErrorEqual(error.POSITION_UNAVAILABLE, null, error);
        completeAsync();
      },
      {maximumAge: 1, gearsLocationProviderUrls: ['non_existant_url']});
}
