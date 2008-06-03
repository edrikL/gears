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

// This file contains information about all the Gears unit tests. It is used by
// the test runners to find tests to run.

/**
 * Represents a suite of tests that are logically related.
 * @constructor
 */
function TestSuite(name) {
  this.name = name;
  this.files = [];
}

/**
 * Adds an individual file to a test suite. Files can group related tests, or
 * can be used to separate tests with different configuration requirements.
 * @param relativePath The path of the file to add to the suite.
 * @param config An object containing flags to control how the test runs.
 * Currently, support useWorker and useIFrame.
 */
TestSuite.prototype.addFile = function(relativePath, config) {
  this.files.push({
    relativePath: relativePath,
    config: config
  });
};

// Lists test configuration below...

var TEST_TIMEOUT_SECONDS = 4 * 60 * 1000;
var suites = [];

if (!isWince && !isOfficial) {
  var consoleSuite = new TestSuite('Console');
  consoleSuite.addFile('../testcases/console_tests.js',
                        {useWorker: true, useIFrame: true});
  suites.push(consoleSuite);
}

var databaseSuite = new TestSuite('Database');
databaseSuite.addFile('../testcases/database_tests.js',
                      {useWorker: true, useIFrame: true});
databaseSuite.addFile('../testcases/database_noworker_tests.js',
                      {useWorker: false, useIFrame: true});
databaseSuite.addFile('../testcases/database_fts_tests.js',
                      {useWorker: true, useIFrame: true});
suites.push(databaseSuite);

var database2Suite = new TestSuite('Database2');
database2Suite.addFile('../testcases/database2_tests.js',
                       {useWorker: true, useIFrame: true});
suites.push(database2Suite);

var factorySuite = new TestSuite('Factory');
factorySuite.addFile('../testcases/factory_tests.js',
                     {useWorker: true, useIFrame: true});
factorySuite.addFile('../testcases/factory_siting_tests.js',
                     {useWorker: false, useIFrame: true});
suites.push(factorySuite);

var httpRequestSuite = new TestSuite('HttpRequest');
httpRequestSuite.addFile('../testcases/httprequest_tests.js',
                         {useWorker: true, useIFrame: true});
suites.push(httpRequestSuite);

if (!isSafari & !isWince) {
  var imagingSuite = new TestSuite('Imaging');
  imagingSuite.addFile('../testcases/imaging_tests.js',
      {useWorker: false, useIFrame: true});
  suites.push(imagingSuite);
}

var localServerSuite = new TestSuite('LocalServer');
localServerSuite.addFile('../testcases/localserver_tests.js',
                         {useWorker: true, useIFrame: true});
localServerSuite.addFile('../testcases/localserver_noworker_tests.js',
                         {useWorker: false, useIFrame: true});
suites.push(localServerSuite);

var timerSuite = new TestSuite('Timer');
timerSuite.addFile('../testcases/timer_tests.js', 
                   {useWorker: true, useIFrame: true});
suites.push(timerSuite);

if (!isSafari && !isWince) {
  var audioSuite = new TestSuite('Audio');
  audioSuite.addFile('../testcases/audio_tests.js', 
                     {useWorker: true, useIFrame: true});
  suites.push(audioSuite);
}

var internalTestSuite = new TestSuite('Internal_Tests');
internalTestSuite.addFile('../testcases/internal_tests.js',
                          {useWorker: true, useIFrame: true});
internalTestSuite.addFile('../testcases/internal_coercion_tests.js', 
                          {useWorker: true, useIFrame: true});
suites.push(internalTestSuite);

var workerPoolSuite = new TestSuite('WorkerPool');
workerPoolSuite.addFile('../testcases/workerpool_tests.js',
                        {useWorker: true, useIFrame: true});
workerPoolSuite.addFile('../testcases/workerpool_onerror_tests.js',
                        {useWorker: true, useIFrame: true});
workerPoolSuite.addFile(
                        '../testcases/workerpool_createworkerfromurl_tests.js',
                        {useWorker: false, useIFrame: true});
if (!isSafari) {
  workerPoolSuite.addFile('../testcases/workerpool_message_body_tests.js',
                          {useWorker: true, useIFrame: true});
}
suites.push(workerPoolSuite);

