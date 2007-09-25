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
    throw new Error(opt_message || 'Assertion failed');
  }
}

/**
 * Assert that two values are equal.
 *
 * @param expected The expected value.
 * @param actual The actual value.
 * @param opt_message An optional message to display if the values aren't equal.
 */
function assertEqual(expected, actual, opt_message) {
  assert(expected == actual,
         opt_message || 'Expected: ' + expected + ', actual: ' + actual);
}

/**
 * Assert that a function produces an error.
 *
 * @param fn This function will be run. If it produces an error, the assert
 * succeeds. Otherwise, it fails.
 * @param opt_expected_error An optional error message that is expected. If the
 * message that results from running fn contains this substring, the assert
 * succeeds. Otherwise, it fails.
 */
function assertError(fn, opt_expected_error) {
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