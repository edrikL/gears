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

/**
 * This class loads and runs the tests in a given URL in the current context.
 *
 * Typical usage:
 *
 * var harness = new Harness();
 * harness.onTestsLoad = function(success, errorMessage) {
 *   ...
 * };
 * harness.onTestComplete = function(name, success, errorMessage) {
 *   ...
 * };
 * harness.onAsyncTestStart = function(name) {
 *   ...
 * };
 * harness.load('mytests.js');
 *
 * @constructor
 */
function Harness() {
  bindMethods(this);
  Harness.current_ = this;
  this.testNames_ = [];
}
Harness.inherits(RunnerBase);

/**
 * The current harness active in this context.
 */
Harness.current_ = null;

/**
 * The latest GearsHttpRequest instance used.
 */
Harness.prototype.request_ = null;

/**
 * Whether the last test called scheduleCallback().
 */
Harness.prototype.scheduledCallback_ = false;

/**
 * The name of current running test.
 */
Harness.prototype.currentTestName_ = null;

/**
 * The url of the test to load.
 */
Harness.prototype.testUrl_ = null;

/**
 * The global scope to use for finding test names. In IE this is a special
 * 'RuntimeObject' that allows lookup in the global scope by name.
 */
Harness.prototype.globalScope_ = null;

/**
 * An array of all tests found in this context.
 */
Harness.prototype.testNames_ = null;

/**
 * The index of the current running test in testNames_.
 */
Harness.prototype.currentTestIndex_ = -1;

/**
 * Load and run a test file.
 * @param url The url of a file containing the tests to run.
 */
Harness.prototype.load = function(testUrl) {
  this.request_ = google.gears.factory.create('beta.httprequest', '1.0');
  this.request_.onreadystatechange = this.handleRequestReadyStateChange_;
  this.testUrl_ = testUrl;

  try {
    this.request_.open('GET', testUrl, false);
    this.request_.send(null);
  } catch (e) {
    this.onTestsLoaded(false, 'Error loading tests: ' + e.message);
  }
};

/**
 * Handles the onreadystatechange event when loading a test file.
 */
Harness.prototype.handleRequestReadyStateChange_ = function() {
  if (this.request_.readyState == 4) {
    if (this.request_.status == 0 || this.request_.status == 200) {
      // Have to use this hack to eval in the global scope in IE workers.
      var timer = google.gears.factory.create('beta.timer', '1.0');
      timer.setTimeout(
        '\n' + // This whitespace is required. For an explanation of why, see:
               // http://code.google.com/p/google-gears/issues/detail?id=265
        this.request_.responseText +
        '\nHarness.current_.handleTestsLoaded_(true)',
        0);
    } else {
      this.onTestsLoaded(
        false,
        'Could not load tests: ' + this.testUrl_ + '. Status: ' +
            this.request_.status + ' ' + this.request_.statusText);
    }
  }
};

/**
 * Called when a test file has been loaded. Evaluate it and run the tests.
 */
Harness.prototype.handleTestsLoaded_ = function(content) {
  this.onTestsLoaded(true);
  this.runTests_();
};

/**
 * Runs all the tests found in the current context.
 */
Harness.prototype.runTests_ = function() {
  // IE has a bug where you can't iterate the objects in the global scope, but
  // it luckily has this hack you can use to get around it.
  this.globalScope_ = global.RuntimeObject ? global.RuntimeObject('test*')
                                           : global;

  // Find all the test names
  for (var name in this.globalScope_) {
    if (name.substring(0, 4) == 'test') {
      this.testNames_.push(name);
    }
  }

  this.runNextTest_();
};

/**
 * Starts the next test, or ends the test run if there are no more tests.
 */
Harness.prototype.runNextTest_ = function() {
  this.currentTestIndex_++;

  if (this.currentTestIndex_ == this.testNames_.length) {
    // We're done!
    this.onAllTestsComplete();
    return;
  }

  // Start the next test
  this.currentTestName_ = this.testNames_[this.currentTestIndex_];
  this.onBeforeTestStart(this.currentTestName_);
  this.startOrResumeCurrentTest_(this.globalScope_[this.currentTestName_]);
};

/**
 * Starts or resumes the current test using the supplied function.
 */
Harness.prototype.startOrResumeCurrentTest_ = function(testFunction) {
  this.scheduledCallback_ = false;

  var success = true;
  try {
    testFunction();
  } catch (e) {
    success = false;
    this.onTestComplete(this.currentTestName_, false,
                        e.message || e); // sometimes the error is just a string
  }

  if (!this.scheduledCallback_) {
    if (success) {
      this.onTestComplete(this.currentTestName_, true);
    }

    this.runNextTest_();
  }
};

/**
 * Schedules a callback for the current test. This method is exported into the
 * global scope when running tests.
 * @param callback The function to call to check results later.
 * @param delayMs The length of time to wait to call callback.
 *
 */
Harness.prototype.scheduleCallback = function(callback, delayMs) {
  if (this.scheduledCallback_) {
    throw new Error('There is already a callback scheduled for this test');
  }

  this.onAsyncTestStart(this.currentTestName_);

  this.scheduledCallback_ = true;
  timer.setTimeout(partial(this.startOrResumeCurrentTest_, callback), delayMs);
};
