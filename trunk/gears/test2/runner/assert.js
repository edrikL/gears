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
 * Assert that something is true and throw an error if not.
 *
 * @param expr The expression to test. This will be coerced to bool if it isn't
 * already.
 * @param opt_message The message to display if expr is not true.
 */
function assert(expr, opt_message) {
  if (!expr) {
    if (isFunction(opt_message)) {
      throw new Error(opt_message());
    } else if (isString(opt_message)) {
      throw new Error(opt_message);
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
 * @param opt_description An optional description of what went wrong, or a
 * function which can be called to provide the description.
 */
function assertEqual(expected, actual, opt_description) {
  assert(expected === actual, function() {
    var message = 'Expected: %s (%s), actual: %s (%s)'.subs(
       expected, typeof expected, actual, typeof actual);

    if (opt_description) {
      message = opt_description + ' - ' + message;
    }

    return message;
  });
}

/**
 * Assert that a function produces an error.
 *
 * @param fn This function will be run. If it produces an error, the assert
 * succeeds. Otherwise, it fails.
 * @param opt_expected_error An optional error message that is expected. If the
 * message that results from running fn contains this substring, the assert
 * succeeds. Otherwise, it fails.
 * @param opt_description An optional description of what went wrong.
 */
function assertError(fn, opt_expected_error, opt_description) {
  try {
    fn();
  } catch (e) {
    if (!opt_expected_error) {
      return;
    } else if (e.message.indexOf(opt_expected_error) > -1) {
      return;
    }
  }

  var message = 'Did not receive expected error';

  if (opt_expected_error) {
    message += ': "' + opt_expected_error + '"';
  }

  if (opt_description) {
    message = opt_description + ' - ' + message;
  }

  throw new Error(message);
}

/**
 * Schedule the test runner to invoke callback to finish the test later.
 * @param callback Callback to invoke to finish test.
 * @param delayMs Length of time to wait before calling callback.
 */
function scheduleCallback(callback, delayMs) {
  // NOTE: This function is implemented by the Harness class when it is running
  // tests.
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

