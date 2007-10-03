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
  this.instanceId_ = Harness.instances_.length;
  Harness.instances_[this.instanceId_] = this;
}
Harness.inherits(RunnerBase);

/**
 * A list of instances of Harness.
 */
Harness.instances_ = [];

/**
 * Identifies this instance in Harness.instances_.
 */
Harness.prototype.instanceId_ = -1;

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
        '\nHarness.instances_[' + this.instanceId_ + '].' +
        'handleTestsLoaded_(true)',
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
  global.scheduleCallback = this.scheduleCallback;

  // IE has a bug where you can't iterate the objects in the global scope, but
  // it luckily has this hack you can use to get around it.
  var globalScope = global.RuntimeObject ? global.RuntimeObject('test*')
                                         : global;
  var testSucceeded;

  for (var name in globalScope) {
    testSucceeded = true;

    this.scheduledCallback_ = false;
    this.currentTestName_ = name;

    if (name.substring(0, 4) == 'test') {
      try {
        globalScope[name]();
      } catch (e) {
        testSucceeded = false;
        this.onTestComplete(name, false, e.message);
      }

      if (testSucceeded && !this.scheduledCallback_) {
        this.onTestComplete(name, true);
      }
    }
  }

  this.onAllSyncTestsComplete();
};

/**
 * Schedules a callback for the current test. This method is exported into the
 * global scope when running tests.
 * @param callback The function to call to check results later.
 * @param delayMs The length of time to wait to call callback.
 *
 */
Harness.prototype.scheduleCallback = function(callback, delayMs) {
  this.onAsyncTestStart(this.currentTestName_);

  this.scheduledCallback_ = true;
  var timer = google.gears.factory.create('beta.timer', '1.0');
  timer.setTimeout(partial(this.fireCallback_, this.currentTestName_, callback),
                   delayMs);
};

/**
 * Fires a callback that was scheduled with scheduleCallback.
 * @param name The name of the test to complete by calling callback.
 * @param callback A function to call to complete a test previously scheduled
 * with scheduleCallback_().
 */
Harness.prototype.fireCallback_ = function(name, callback) {
  try {
    callback();
  } catch (e) {
    this.onTestComplete(name, false, e.message);
    return;
  }

  this.onTestComplete(name, true);
};
