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

function testTimeout() {
  var success = false;
  var timerFired = false;

  timer.setTimeout(function() {
    // Set success to true on the first firing, false if it fires again.
    success = !timerFired;
    timerFired = true;
  }, 20);

  scheduleCallback(function() {
    assert(timerFired, 'Timer did not fire');
    assert(success, 'Timer fired more than once');
  }, 100);
}

function testInterval() {
  var timerId = timer.setInterval(intervalHandler, 20);
  var targetIntervals = 3;
  var intervals = 0;

  function intervalHandler() {
    if (++intervals == targetIntervals) {
      timer.clearInterval(timerId);
    }
  }

  scheduleCallback(function() {
    assertEqual(3, targetIntervals);
  }, 100); // Give a little leeway
}


testScriptTimeout.fired = false;
testScriptTimeout.succeeded = false;

function testScriptTimeout() {
  timer.setTimeout('testScriptTimeout.succeeded = !testScriptTimeout.fired;' +
                   'testScriptTimeout.fired = true;',
                   20);

  scheduleCallback(function() {
    assert(testScriptTimeout.fired, 'Timer did not fire');
    assert(testScriptTimeout.succeeded, 'Timer fired more than once');
  }, 100);
}


testScriptInterval.intervals = 0;
testScriptInterval.targetIntervals = 3;

function testScriptInterval() {
  testScriptInterval.timerId = timer.setInterval(
        'testScriptInterval.intervals++;' +
        'if (testScriptInterval.intervals == ' +
        '    testScriptInterval.targetIntervals) {' +
        '  timer.clearInterval(testScriptInterval.timerId);' +
        '}',
        20);

  scheduleCallback(function() {
    assertEqual(3, testScriptInterval.targetIntervals);
  }, 100);
}
