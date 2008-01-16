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

var console = google.gears.factory.create('beta.console');
console.onlog = handleEvent;

// Determine if we are in a WorkerPool process or not (see handleEvent)
var inWorker = false;
if (google.gears.workerPool) {
  inWorker = true;
}

// Argument validation tests
function testNoParameters() {
  assertError(function() { console.log() }, null,
      'Calling console.log with no parameters should fail');
}
function testOneParameter() {
  assertError(function() { console.log('test'); }, null,
      'Calling console.log with only one parameter should fail');
}
function testTypeNull() {
  assertError(function() { console.log(null, 'test'); }, null,
      'Calling console.log with type null should fail');
}
function testMessageNull() {
  assertError(function() { console.log('test', null); }, null,
      'Calling console.log with message null should fail');
}
function testTypeEmptyString() {
  assertError(function() { console.log('', 'test'); }, null,
      'Calling console.log with type as empty string should fail');
}
function testMessageEmptyString() {
  assertError(function() { console.log('test', ''); }, null,
      'Calling console.log with message as empty string should fail');
}

// Test logging across Worker boundary by logging from a worker and
// handling the log event in the main script.
// NOTE: this test assumes that the test on the main page runs *before*
// the same test in the worker. Also this test is only valid if the
// worker test runs since it will always return success straight away
// for the main page.
function testCrossBoundaryLog() {
  if (inWorker) {
    startAsync();
    // Will only be caught by the handler in the main page
    console.log('Worker:false', 'testCrossBoundaryLog');
  }
}

// Helper callback for testing console.onlog and logging functionality
function handleEvent(log_event) {
  // Because these tests are also run in a WorkerPool process, this handler
  // is duplicated and registered twice (once in the WorkerPool process and
  // once in the main page). Console log events, however, cross worker
  // boundaries, so *both* handlers will be called whenever a message is
  // logged in a script. For most cases we want to ignore these duplicate
  // messages. We acheive this by using a custom event type - 'Worker:true'
  // for log events originating from the WorkerPool process and
  // 'Worker:false' otherwise. By setting the log type to 'Worker:ignore',
  // both handlers will be invoked for each log event.
  if (log_event.type == 'Worker:ignore') {
    // Called twice!

  } else if (inWorker && log_event.type == 'Worker:true' ||
            !inWorker && log_event.type == 'Worker:false') {
    // Only called for local log events

    // testCrossBoundaryLog
    if (log_event.message == 'testCrossBoundaryLog') {
      if (!inWorker) {
        // Got a log message from workerPool process, send one back to
        // complete test
        console.log('Worker:true', 'testCrossBoundaryLog');
      } else {
        // Received a log back from main script, done!
        completeAsync();
      }
    }
  }
}
