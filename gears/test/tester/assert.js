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

// This file defines all the builtins that can be used inside Gears unit tests.

/**
 * Whether the installed Gears extension is a debug build.
 */
var isDebug = google.gears.factory.getBuildInfo().indexOf('dbg') > -1;

/**
 * Whether the installed Gears extension is an official build.
 */
var isOfficial = google.gears.factory.getBuildInfo().indexOf('official') > -1;

/**
 * Whether the installed Gears extension is for Windows Mobile
 */
var isWince = google.gears.factory.getBuildInfo().indexOf('wince') > -1;

/**
 * A shared timer tests can use.
 */
var timer = google.gears.factory.create('beta.timer');

/**
 * Assert that something is true and throw an error if not.
 *
 * @param expr The expression to test. This will be coerced to bool if it isn't
 * already.
 * @param optMessage The message to display if expr is not true, or a function
 * which will return the message.
 */
function assert(expr, optMessage) {
  if (!expr) {
    if (isFunction(optMessage)) {
      throw new Error(optMessage());
    } else if (isString(optMessage)) {
      throw new Error(optMessage);
    } else {
      throw new Error('Assertion failed');
    }
  }
}

/**
 * Format error message with optional error message.
 *
 * @param message The original error message.
 * @param optDescription An optional description of what went wrong.
 */
function formatErrorMessage(message, optDescription) {
  if (optDescription) {
    return optDescription + ' - ' + message;
  } else {
    return message;
  }
}

/**
 * Assert that two values are equal. A strict equality test is used; 4 and "4"
 * are not equal.
 *
 * @param expected The expected value.
 * @param actual The actual value.
 * @param optDescription An optional description of what went wrong.
 */
function assertEqual(expected, actual, optDescription) {
  assert(expected === actual, function() {
    var message = 'Expected: %s (%s), actual: %s (%s)'.subs(
        expected, typeof expected, actual, typeof actual);
    return formatErrorMessage(message, optDescription);
  });
}


/**
 * Assert that two values are equal. This function will inspect object
 * object properties and array members. Is a function is encountered the only
 * check that is performed is to assert that they expected and actual are of
 * type function. Otherwise, a strict equality test is used; 4 and "4" are not
 * equal.
 *
 * @param expected Any type containing the expected value.
 * @param actual Any type containing the actual value.
 * @param optDescription An optional description of what went wrong.
 */
function assertEqualAnyType(expected, actual, optDescription) {
  if (isArray(expected) && isArray(actual)) {
    assertArrayEqual(expected, actual, optDescription);
  } else if (isObject(expected) && isObject(actual)) {
    assertObjectEqual(expected, actual, optDescription);
  } else if (isFunction(expected) && isFunction(actual)) {
    // functions are only tested for type
  } else {
    assertEqual(expected, actual, optDescription);
  }
}

/**
 * Assert that all values in two arrays are equal. A strict equality test is
 * used; 4 and "4" are not equal.
 *
 * @param expected The array containing expected values.
 * @param actual The array containing actual values.
 * @param optDescription An optional description of what went wrong.
 */
function assertArrayEqual(expected, actual, optDescription) {
  function notArrayErrorMessage(array, optDescription) {
    return function() {
      var message = 'Expected array, actual: %s (%s)'.subs(array, typeof array);
      return formatErrorMessage(message, optDescription);
    };
  }

  assert(isArray(expected), notArrayErrorMessage(expected, optDescription));
  assert(isArray(actual), notArrayErrorMessage(actual, optDescription));
  assert(expected.length == actual.length, function() {
    var message = 'Expected array length: %s actual length: %s'.subs(
        expected.length, actual.length);
    formatErrorMessage(message, optDescription);
  });

  for (var i = 0; i < expected.length; ++expected) {
    assertEqualAnyType(expected[i], actual[i], function() {
      var message = 'Expected element in array at %s: %s (%s) actual: %s (%s)'
          .subs(i, expected[i], typeof expected[i], actual[i],
          typeof actual[i]);
      formatErrorMessage(message, optDescription);
    });
  }
}

/**
 * Assert that all properties in two objects are equal. A strict equality test
 * is used; 4 and "4" are not equal.
 *
 * @param expected The object containing expected values.
 * @param actual The object containing actual values.
 * @param optDescription An optional description of what went wrong.
 */
function assertObjectEqual(expected, actual, optDescription) {
  function notObjectErrorMessage(array, optDescription) {
    return function() {
      var message = 'Expected object, actual: %s (%s)'.subs(
          array, typeof array);
      return formatErrorMessage(message, optDescription);
    };
  }

  assert(isObject(expected), notObjectErrorMessage(expected, optDescription));
  assert(isObject(actual), notObjectErrorMessage(actual, optDescription));

  for (property in expected) {
    assertEqualAnyType(expected[property], actual[property], optDescription);
  }
}


/**
 * Assert that two values are not equal. A strict equality test is used; 4 and
 * "4" are not equal.
 *
 * @param unexpected The unexpected value.
 * @param actual The actual value.
 * @param optDescription An optional description of what went wrong.
 */
function assertNotEqual(unexpected, actual, optDescription) {
  assert(unexpected !== actual, function() {
    var message = 'Expected value other than "%s" (%s)'.subs(
       unexpected, typeof unexpected);
    return formatErrorMessage(message, optDescription);
  });
}

/**
 * Assert a value is null. This tests for strict equality to null, no coercion
 * is done.
 *
 * @param val The value expected to be null.
 * @param optDescription An optional description of what went wrong.
 */
function assertNull(val, optDescription) {
  assert(val === null, function() {
    var message = "Expected null value.";
    return formatErrorMessage(message, optDescription);
  });
}

/**
 * Assert a value is not null. This tests for strict equality to null, no
 * coercion is done.
 *
 * @param val The value expected to be non-null.
 * @param optDescription An optional description of what went wrong.
 */
function assertNotNull(val, optDescription) {
  assert(val !== null, function() {
    var message = "Unexpected null value.";
    return formatErrorMessage(message, optDescription);
  });
}

/**
 * Assert that a function produces an error.
 *
 * @param fn This function will be run. If it produces an error, the assert
 * succeeds. Otherwise, it fails.
 * @param optExpectedError An optional error message that is expected. If the
 * message that results from running fn contains this substring, the assert
 * succeeds. Otherwise, it fails.
 * @param optDescription An optional description of what went wrong.
 */
function assertError(fn, optExpectedError, optDescription) {
  try {
    fn();
  } catch (e) {
    if (!optExpectedError) {
      return;
    } else {
      var actualError = e.message || e.toString();
      if (actualError.indexOf(optExpectedError) > -1) {
        return;
      }
    }
  }

  var message = 'Did not receive expected error';

  if (optExpectedError) {
    message += ': "' + optExpectedError + '"';
  }

  throw new Error(formatErrorMessage(message, optDescription));
}

/**
 * Starts the current test running asynchronously. The next test will not be
 * started until completeAsync() is called.
 */
function startAsync() {
  Harness.current_.startAsync();
}

/**
 * Marks the currently running asynchronous test as successful and starts the
 * next test.
 */
function completeAsync() {
  Harness.current_.completeAsync();
}

/**
 * Wait for one or more global errors to occur before starting the next test. If
 * the errors do not occur, occur out of order, or if some other error occurs,
 * the test is marked failed.
 *
 * @param errorMessages Array of expected error substrings. When an error occurs
 * the first item in this array is removed and compared to the full error text.
 * If it occurs as a substring, the expected error is considered found.
 * Otherwise the test is marked failed.
 */
function waitForGlobalErrors(errorMessages) {
  Harness.current_.waitForGlobalErrors(errorMessages);
}

/**
 * Process a resultset and ensure it is closed, even if there is an error.
 * @param rs The resultset to process.
 * @param fn A function that will receive the resultset as an argument.
 */
function handleResult(rs, fn) {
  try {
    fn(rs);
  } finally {
    rs.close();
  }
}


/**
 * Utility to send an HTTP request.
 * @param url The url to fetch
 * @param method The http method (ie. 'GET' or 'POST')
 * @param data The data to send, may be null
 * @param callback Will be called with contents of URL, or null if the request
 * was unsuccessful.
 */
function sendHttpRequest(url, method, data, callback) {
  var req = google.gears.factory.create('beta.httprequest');
  req.onreadystatechange = function() {
    if (req.readyState == 4) {
      if (req.status == 200) {
        callback(req.responseText);
      } else {
        callback(null);
      }
    }
  };

  req.open(method, url, true);  // async
  req.send(data);
}

/**
 * Utility to asynchronously get a URL and return the content.
 * @param url The url to fetch
 * @param callback Will be called with contents of URL, or null if the request
 * was unsuccessful.
 */
function httpGet(url, callback) {
  sendHttpRequest(url, 'GET', null, callback);
}

/**
 * Utility to asynchronously POST to a URL and return the content.
 * @param url The url to fetch
 * @param data The data to post
 * @param callback Will be called with contents of URL, or null if the request
 * was unsuccessful.
 */
function httpPost(url, data, callback) {
  sendHttpRequest(url, 'POST', data, callback);
}
