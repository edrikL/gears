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
  var timerFired = false;

  startAsync();
  timer.setTimeout(function() {
    assert(!timerFired, 'Timer fired more than once');
    completeAsync();
  }, 20);
}

function testInterval() {
  var targetIntervals = 3;
  var intervals = 0;

  startAsync();
  var timerId = timer.setInterval(intervalHandler, 20);

  function intervalHandler() {
    ++intervals;

    if (intervals == targetIntervals) {
      timer.clearInterval(timerId);
      completeAsync();
    } else if (intervals > targetIntervals) {
      assert(false, 'Timer fired too many times');
    }
  }
}

testScriptTimeout.fired = false;

function testScriptTimeout() {
  startAsync();
  timer.setTimeout(
    'assert(!testScriptTimeout.fired);' +
    'testScriptTimeout.fired = true;' +
    'completeAsync();',
    20);
}

testScriptInterval.intervals = 0;
testScriptInterval.targetIntervals = 3;

function testScriptInterval() {
  startAsync();
  testScriptInterval.timerId = timer.setInterval(
        'testScriptInterval.intervals++;' +
        'if (testScriptInterval.intervals == ' +
        '    testScriptInterval.targetIntervals) {' +
        '  timer.clearInterval(testScriptInterval.timerId);' +
        '  completeAsync();' +
        '} else if (testScriptInterval.intervals > ' +
        '           testScriptInterval.targetIntervals) {' +
        '  assert(false, "Timer fired too many times");' +
        '}',
        20);
}
