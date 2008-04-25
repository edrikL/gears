// Copyright 2007, Google Inc.
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

if (isDebug) {
  var internalTests = google.gears.factory.create('beta.test');
}

function testInternal() {
  if (isDebug) {
    assert(internalTests.runTests(),
           'Internal tests failed.');
  }
}


// test functions for passing arguments
function testPassArguments() {
  if (isDebug) {
    // Have to do this because Gears methods don't support apply() in IE.
    function callTest(args) {
      internalTests.testPassArguments(args[0], args[1], args[2], args[3],
                                      args[4]);
    }
    
    var good_bool = true;
    var good_int = 42;
    var good_int64 = Math.pow(2, 42);
    var good_double = 88.8;
    var good_string = "hotdog";
    
    var bad_bool = false;
    var bad_int = 43;
    var bad_int64 = Math.pow(2, 43);
    var bad_double = 88.9;
    var bad_string = "hotdof";
    
    // Test a good one
    var good_vals = [good_bool, good_int, good_int64, good_double, good_string];
    callTest(good_vals);
    
    // Test passing wrong values
    var bad_vals = [bad_bool, bad_int, bad_int64, bad_double, bad_string];
    for (var i = 0; i < good_vals.length; i++) {
      var test_vals = [].concat(good_vals);
      test_vals[i] = bad_vals[i];
      assertError(function() { callTest(test_vals) },
                  "Incorrect value for parameter " + (i + 1));
    }

    // Test not passing required args
    var required_argument_error = "Required argument 5 is missing.";
    assertError(function() {
      internalTests.testPassArguments(good_bool, good_int, good_int64,
                                      good_double, null); },
      required_argument_error);
    assertError(function() {
      internalTests.testPassArguments(good_bool, good_int, good_int64,
                                      good_double, undefined); },
      required_argument_error);
    assertError(function() {
      internalTests.testPassArguments(good_bool, good_int, good_int64,
                                      good_double); },
      required_argument_error);

    // Test passing wrong type
    for (var i = 0; i < good_vals.length; i++) {
      for (var j = 0; j < good_vals.length; j++) {
        if (i == j) continue;

        var expect_conversion_allowed = true;

        // We implicitly convert int to int64, so we don't expect errors in
        // that case.
        if (good_vals[i] == good_int64 && good_vals[j] == good_int) {
          expect_conversion_allowed = false;
        }
        
        // Same for converting int and int64 to double.
        if (good_vals[i] == good_double) {
          if (good_vals[j] == good_int || good_vals[j] == good_int64) {
            expect_conversion_allowed = false;
          }
        }
        
        var test_vals = [].concat(good_vals);
        test_vals[i] = test_vals[j];

        var expected_error = expect_conversion_allowed ?
            "Argument " + (i + 1) + " has invalid type or is outside allowed " +
            "range" :
            "Incorrect value for parameter " + (i + 1);
        
        assertError(function() { callTest(test_vals); }, expected_error);
      }
    }
  }
}

function testPassArgumentsOptional() {
  if (isDebug) {
    internalTests.testPassArgumentsOptional(42, 42, 42);
    internalTests.testPassArgumentsOptional(42, 42);
    internalTests.testPassArgumentsOptional(42);
    
    assertError(
      function() { internalTests.testPassArgumentsOptional(42, "hello"); },
      "Argument 2 has invalid type or is outside allowed range");
      
    assertError(
      function() { internalTests.testPassArgumentsOptional(42, 43); },
      "Incorrect value for parameter 2");
  }
}



// JsObject test functions

// Test passing an object with different types of properties to C++.
function testPassObject() {
  if (isDebug) {
    internalTests.testPassObject(new TestObject());
  }
}

// Test creating a javascript object in C++ and returning it.
function testCreateObject() {
  if (isDebug) {
    // TODO(cdevries): Enable this test on FF in worker pools when
    //                 SetReturnValue() has been implemented.
    var isFirefox = google.gears.factory.getBuildInfo().indexOf("firefox") > -1;
    var isFirefoxWorker = isFirefox && google.gears.workerPool;

    if (!isFirefoxWorker) {
      var testObject = new TestObject();
      var createdObject = internalTests.testCreateObject(createTestFunction());
      // assertObjectEqual does not compare function results
      assertObjectEqual(testObject, createdObject);
      assertEqual(testObject.array_many_types[6](),
          createdObject.array_many_types[6]());
      assertEqual(testObject.func(), createdObject.func());
    }
  }

}

// Tests the bogus browser cache entries used on WinCE to prevent the 'Cannot
// Connect' popup when serving content from the LocalServer when offline.
// TODO(steveblock): Add more tests once test harness is fully working for
// WinCE.
// - Managed resource store.
// - Test that existing browser cache entires are not over-written.
function testBrowserCache() {
  if (isDebug && google.gears.factory.getBuildInfo().indexOf("wince") > -1) {
    // Clear the browser cache.
    var urls = ['/testcases/test_file_1.txt',
                '/testcases/test_file_1024.txt'];
    internalTests.removeEntriesFromBrowserCache(urls);
    // Create a resource store and capture some resources.
    var localServer = google.gears.factory.create('beta.localserver');
    var storeName = 'testBrowserCacheStore';
    var store = localServer.createStore(storeName);
    var responses = 0;
    startAsync();
    store.capture(urls, function(url, success, captureId) {
      ++responses;
      assert(success, 'Capture for ' + url + ' failed.');
      if (responses == urls.length) {
        // Check that these resources are in the browser cache.
        internalTests.testEntriesPresentInBrowserCache(urls, true, true);
        // Remove a URL.
        store.remove(urls[0]);
        internalTests.testEntriesPresentInBrowserCache([urls[0]], false);
        internalTests.testEntriesPresentInBrowserCache([urls[1]], true, true);
        // Remove the store.
        localServer.removeStore(storeName);
        internalTests.testEntriesPresentInBrowserCache(urls, false);
        completeAsync();
      }
    });
  }
}

// Tests the parsing of the options passed to Geolocation.GetCurrentPosition and
// Geolocation.WatchPosition.
function testParseGeolocationOptions() {
  if (isDebug && !isOfficial) {
    var dummyFunction = function() {};
    // All good.
    internalTests.testParseGeolocationOptions(dummyFunction);
    // Test correct types.
    // Missing callback function.
    assertError(function() { internalTests.testParseGeolocationOptions() });
    // Wrong type for callback function.
    assertError(function() { internalTests.testParseGeolocationOptions(42) });
    // Wrong type for options.
    assertError(
        function() { internalTests.testParseGeolocationOptions(
            dummyFunction, 42) });
    // Wrong type for enableHighAccuracy.
    assertError(
        function() { internalTests.testParseGeolocationOptions(
            dummyFunction, { enableHighAccuracy: 42 }) },
        'options.enableHighAccuracy should be a boolean.');
    // Wrong type for requestAddress.
    assertError(
        function() { internalTests.testParseGeolocationOptions(
            dummyFunction, { requestAddress: 42 }) },
        'options.requestAddress should be a boolean.');
    // Wrong type for gearsLocationProviderUrls.
    assertError(
        function() { internalTests.testParseGeolocationOptions(
            dummyFunction, { gearsLocationProviderUrls: 42 }) },
        'options.gearsLocationProviderUrls should be null or an array of ' +
        'strings');
    assertError(
        function() { internalTests.testParseGeolocationOptions(
            dummyFunction, { gearsLocationProviderUrls: [42] }) },
        'options.gearsLocationProviderUrls should be null or an array of ' +
        'strings');
    // Test correct parsing.
    var defaultUrlArray = ['http://www.google.com/'];
    var urls = ['url1', 'url2'];
    var parsed_options;
    // No options.
    parsed_options = internalTests.testParseGeolocationOptions(dummyFunction);
    assertEqual(false, parsed_options.repeats);
    assertEqual(false, parsed_options.enableHighAccuracy);
    assertEqual(false, parsed_options.requestAddress);
    assertArrayEqual(defaultUrlArray,
                     parsed_options.gearsLocationProviderUrls);
    // Empty options.
    parsed_options = internalTests.testParseGeolocationOptions(dummyFunction,
                                                               {});
    assertEqual(false, parsed_options.repeats);
    assertEqual(false, parsed_options.enableHighAccuracy);
    assertEqual(false, parsed_options.requestAddress);
    assertArrayEqual(defaultUrlArray,
                     parsed_options.gearsLocationProviderUrls);
    // Empty provider URLs.
    parsed_options = internalTests.testParseGeolocationOptions(
        dummyFunction,
        { gearsLocationProviderUrls: [] });
    assertEqual(false, parsed_options.repeats);
    assertEqual(false, parsed_options.enableHighAccuracy);
    assertEqual(false, parsed_options.requestAddress);
    assertArrayEqual([], parsed_options.gearsLocationProviderUrls);
    // Null provider URLs.
    parsed_options = internalTests.testParseGeolocationOptions(
        dummyFunction,
        { gearsLocationProviderUrls: null });
    assertEqual(false, parsed_options.repeats);
    assertEqual(false, parsed_options.enableHighAccuracy);
    assertEqual(false, parsed_options.requestAddress);
    assertArrayEqual([], parsed_options.gearsLocationProviderUrls);
    // All properties false, provider URLs set.
    parsed_options = internalTests.testParseGeolocationOptions(
        dummyFunction,
        { enableHighAccuracy: false,
          requestAddress: false,
          gearsLocationProviderUrls: urls });
    assertEqual(false, parsed_options.repeats);
    assertEqual(false, parsed_options.enableHighAccuracy);
    assertEqual(false, parsed_options.requestAddress);
    assertArrayEqual(urls, parsed_options.gearsLocationProviderUrls);
    // All properties true, provider URLs set.
    parsed_options = internalTests.testParseGeolocationOptions(
        dummyFunction,
        { enableHighAccuracy: true,
          requestAddress: true,
          gearsLocationProviderUrls: urls });
    assertEqual(false, parsed_options.repeats);
    assertEqual(true, parsed_options.enableHighAccuracy);
    assertEqual(true, parsed_options.requestAddress);
    assertArrayEqual(urls, parsed_options.gearsLocationProviderUrls);
  }
}
  
// Helper functions

function createTestArray(length) {
  var array = [];
  for (var i = 0; i < length; ++i) {
    array.push(i);
  }
  return array;
}

function ChildTestObject() {
  this.bool_true = true;
  this.int_0 = 0;
  this.double_0 = 0.01;
  this.string_0 = '';
  this.array_0 = createTestArray(0);
}

function createTestFunction() {
  return function() { return 'i am a function'; };
}

function TestObject() {
  // boolean
  this.bool_true = true;
  this.bool_false = false;
  this.bool_array = [this.bool_true, this.bool_false];

  // integer (assumed to be tagged 32 bit signed integer,
  //          30 bits magnitude, 1 bit sign, 1 bit tag)
  this.int_0 = 0;
  this.int_1 = 1;
  this.int_large = 1073741823; // 2**30 - 1
  this.int_negative_1 = -1;
  this.int_negative_large = -1073741823; // -(2**30 - 1)
  this.int_array = [this.int_0, this.int_1, this.int_large,
    this.int_negative_1, this.int_negative_large];

  // double (assumed to be 64-bit IEEE double precision float point)
  this.double_0 = 0.01;  // 0.0 becomes an int
  this.double_1 = 1.01;
  this.double_large = Number.MAX_VALUE;
  this.double_negative_1 = -1.01;
  this.double_negative_large = Number.MIN_VALUE;
  this.double_array = [this.double_0, this.double_1, this.double_large,
    this.double_negative_1, this.double_negative_large];

  // string
  this.string_0 = '';
  this.string_1 = 'a';
  this.string_many = 'asdjh1)!(@#*h38ind89!03234bnmd831%%%*&*jdlwnd8893asd1233';
  this.string_array = [this.string_0, this.string_1, this.string_many];

  // array
  this.array_0 = createTestArray(0);
  this.array_1 = createTestArray(1);
  this.array_8 = createTestArray(8);
  this.array_10000 = createTestArray(10000);
  this.array_many_types = [false, true, 2, '3', 4.01,
    [0, 1, 2, 3, 4],
    createTestFunction()];

  // object
  this.obj = new ChildTestObject();

  // function
  this.func = createTestFunction();
}

function testGetSystemTime() {
  if (isDebug) {
    // Test system time increases.
    var start = internalTests.getSystemTime();
    var callback = function() {
      var end = internalTests.getSystemTime();
      assert(start < end, 'System time should increase.');
      completeAsync();
    };
    var timer = google.gears.factory.create('beta.timer');
    startAsync();
    // getSystemTime resolution is only 1s on WinCE.
    timer.setTimeout(callback, 2000);
  }
}

function testPerfTimer() {
  if (isDebug) {
    // Test zero tick delta.
    var ticks = internalTests.getTicks();
    var elapsed = internalTests.getTickDeltaMicros(ticks, ticks);
    assert(elapsed == 0, 'Time delta should be zero.');
    // Test non-zero tick delta.
    var callback = function() {
      elapsed =
          internalTests.getTickDeltaMicros(ticks, internalTests.getTicks());
      assert(elapsed > 0, 'Time delta should be greater than zero.');
      completeAsync();
    };
    var timer = google.gears.factory.create('beta.timer');
    startAsync();
    // Tick resolution should be at most 1us.
    timer.setTimeout(callback, 1);
  }
}
