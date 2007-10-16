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
// Please feel free to add new ones as the mood strikes. Here are some ideas:
// - assertObjectsAreEqual()
// - assertArraysContainSameElements()
// ... etc ...

/**
 * Whether the installed Gears extension is a debug build.
 */
var isDebug = google.gears.factory.getBuildInfo().indexOf("dbg") > -1;

/**
 * A shared timer tests can use.
 */
var timer = google.gears.factory.create('beta.timer', '1.0');

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

    if (optDescription) {
      message = optDescription + ' - ' + message;
    }

    return message;
  });
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

    if (optDescription) {
      message = optDescription + ' - ' + message;
    }

    return message;
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
    if (optDescription) {
      message += " " + optDescription;
    }
    return message;
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
    if (optDescription) {
      message += " " + optDescription;
    }
    return message;
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

  if (optDescription) {
    message = optDescription + ' - ' + message;
  }

  throw new Error(message);
}

/**
 * Schedules a callback to continue a test at a later time.
 *
 * @param fn The function to call to resume the test.
 * @param timeoutMsec The amount of time to wait before resuming.
 */
function scheduleCallback(fn, timeoutMsec) {
  return Harness.current_.scheduleCallback(fn, timeoutMsec);
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
 * Utility to asynchronously get a URL and return the content.
 * @param url The url to fetch
 * @param callback Will be called with contents of URL, or null if the request
 * was unsuccessful.
 */
function httpGet(url, callback) {
  var req = google.gears.factory.create('beta.httprequest', '1.0');
  req.onreadystatechange = function() {
    if (req.readyState == 4) {
      if (req.status == 200) {
        callback(req.responseText);
      } else {
        callback(null);
      }
    }
  };

  req.open('GET', url, true); // async
  req.send(null);
}

/**
 * This class helps test for errors that are expected to be unhandled and
 * bubble up to the global handler.
 */
function GlobalErrorHandler() {
  bindMethods(this);

  this.expectedErrors_ = [];
  this.receivedErrors_ = [];

  // Global errors are reported by a different object in workers than in the
  // DOM.
  var globalErrorSource = google.gears.workerPool || window;
  globalErrorSource.onerror = this.handleError_;
}

/**
 * List of error substrings that are expected.
 */
GlobalErrorHandler.prototype.expectedErrors_ = null;

/**
 * List of error messages that have been received.
 */
GlobalErrorHandler.prototype.receivedErrors_ = null;

/**
 * Handle a top-level errors. Ir the error was expected, remember that it was
 * received for later checking. Otherwise, re-throw it.
 *
 * @param errorMessage The error to handle.
 */
GlobalErrorHandler.prototype.handleError_ = function(errorMessage) {
  // In workers, the errorMessage is actually an object with various properties,
  // one of which is the message.
  errorMessage = google.gears.workerPool ? errorMessage.message : errorMessage;

  for (var i = 0; i < this.expectedErrors_.length; i++) {
    if (errorMessage.indexOf(this.expectedErrors_[i]) > -1) {
      // This is an error we were expecting. Note down that we got it and
      // prevent the browser UI.
      this.receivedErrors_.push(errorMessage);

      // Remove the error from the expected list so that we minimize the chance
      // we will remove other errors accidentally.
      this.expectedErrors_.splice(i, 1);
      return true;
    }
  }

  // This is some other error. Fall through to the regular browser UI.
  return false;
};

/**
 * Add an error to the list of expected ones.
 * @param A substring of the error message expected.
 */
GlobalErrorHandler.prototype.expectError = function(errorMessageSubstring) {
  this.expectedErrors_.push(errorMessageSubstring);
};

/**
 * Check to see if an expected error was in fact received.
 * @param errorMessageSubstring A substring of the error to search for.
 */
GlobalErrorHandler.prototype.wasErrorReceived =
    function(errorMessageSubstring) {
  for (var i = 0; i < this.receivedErrors_.length; i++) {
    if (this.receivedErrors_[i].indexOf(errorMessageSubstring) > -1) {
      return true;
    }
  }
  return false;
};
