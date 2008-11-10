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

// NOTE: This assumes that we are running on port 8001!
if (location.port != 8001) {
  throw new Error("Test must be run from port 8001");
}

var currentOrigin = location.protocol + '//' + location.hostname + ':8001';
var differentOrigin = location.protocol + '//' + location.hostname + ':8002';

// This origin is never granted access to Gears by the unit tests.
var differentOrigin2 = location.protocol + '//' + location.hostname + ':8003';

// This worker expects to be run in-origin, and does not call
// allowCrossOrigin().
var sameOriginWorkerPath = '/testcases/test_worker_same_origin.worker.js';

// This worker expects to be run out-of-origin, and calls allowCrossOrigin().
var crossOriginWorkerPath = '/testcases/test_worker_cross_origin.worker.js';

// This worker gets served with the wrong content type for cross-origin access
// since it doesn't end in '.worker.js'. It gets rejected by Gears before even
// being executed.
var crossOriginWorkerBadContentType =
    '/testcases/test_worker_bad_content_type.js';

var redirectPath = '/testcases/cgi/server_redirect.py?location=';

function testCreateWorkerFromUrl1() {
  startAsync();

  var wp = google.gears.factory.create('beta.workerpool');
  wp.onmessage = function(text, sender, m) {
    completeAsync();
  }
  var childId = wp.createWorkerFromUrl(sameOriginWorkerPath);
  wp.sendMessage('PING1', childId);
}

function testCreateWorkerFromUrl2() {
  startAsync();

  // Cleanup any local DB before starting test.  
  var db = google.gears.factory.create('beta.database');
  db.open('worker_js');
  db.execute('drop table if exists PING2').close();
  db.close();

  var wp = google.gears.factory.create('beta.workerpool');
  wp.onmessage = function(text, sender, m) {
    // Worker database SHOULD exist in parent origin.
    var db = google.gears.factory.create('beta.database');
    db.open('worker_js');
    var rs = db.execute('select * from sqlite_master where name = ? limit 1',
                        ['PING2']);
    handleResult(rs, function(rs) {
      assert(rs.isValidRow(), 'PING2 table should have been created');
    });
    db.close();
    completeAsync();
  };

  var childId = wp.createWorkerFromUrl(currentOrigin + sameOriginWorkerPath);
  wp.sendMessage('PING2', childId);
}

function testCreateWorkerFromUrl3() {
  startAsync();

  // Cleanup any local DB before starting test.  
  var db = google.gears.factory.create('beta.database');
  db.open('worker_js');
  db.execute('drop table if exists PING3').close();
  db.close();

  var wp = google.gears.factory.create('beta.workerpool');
  wp.onmessage = function(text, sender, m) {
    // Worker database should NOT exist in parent origin.
    var db = google.gears.factory.create('beta.database');
    db.open('worker_js');
    var rs = db.execute('select * from sqlite_master where name = ? limit 1',
                        ['PING3']);
    handleResult(rs, function(rs) {
      assert(!rs.isValidRow(), 'PING3 table should not have been created');
    });
    db.close();
    completeAsync();
  };

  // TODO(cprince): In dbg builds, add a 2nd param to createWorkerFromUrl()
  // so callers can simulate a different origin without being online.
  //if (!gIsDebugBuild) {
  var childId = wp.createWorkerFromUrl(differentOrigin + crossOriginWorkerPath);
  //} else {
  //  var childId = wp.createWorkerFromUrl(
  //      currentOrigin + crossOriginWorkerPath, differentOrigin);
  //}
  wp.sendMessage('PING3', childId);
}

// Test fails intermittently
// TODO(ace): Uncomment or remove this test case upon resolution of Issue 388
//function testCreateWorkerFromUrl4() {
//  var workerUrl = '/non-existent-file.js';
//
//  waitForGlobalErrors([workerUrl]);
//
//  var wp = google.gears.factory.create('beta.workerpool');
//  wp.createWorkerFromUrl(workerUrl);
//}

function testCreateWorkerFromUrl5() {
  var expectedError = 'Page does not have permission to use Google Gears';
  
  var wp = google.gears.factory.create('beta.workerpool');
  // Have to keep a reference to the workerpool otherwise, sometimes the message
  // never gets processed!
  // TODO(aa): Investigate why this happens -- ThreadInfo objects are
  // AddRef()'ing the workerpool, so I would assume this shouldn't be possible.
  testCreateWorkerFromUrl5.wp = wp;

  waitForGlobalErrors([expectedError]);

  var childId = wp.createWorkerFromUrl(differentOrigin + sameOriginWorkerPath);
  // TODO(cprince): Could add debug-only origin override here too.
  wp.sendMessage('PING5', childId);
}

function testOneShotWorkerFromUrl() {
  // Not having a global reference to wp or any callbacks causes this
  // GearsWorkerPool instance to get GC'd before page unload. This found a bug
  // where the HttpRequest used to load from url was getting destroyed from a
  // different thread than it was created on.
  var wp = google.gears.factory.create('beta.workerpool');
  wp.createWorkerFromUrl(sameOriginWorkerPath);
}

// The following two tests test redirects for HttpRequest/AsyncTask where the
// redirect behaviour is FOLLOW_ALL.
function testRedirectToCrossOrigin() {
  // Tests loading a worker where the URL results in a cross-origin redirect.
  // Gears will not grant permission to the redirected origin, so the worker
  // will throw an exception when it tries to create a new Gears object. This
  // test relies on the redirected origin not already having local storage
  // permissions.
  var wp = google.gears.factory.create('beta.workerpool');
  var expectedError = 'Error in worker 1 at line 21. Page does not have ' +
      'permission to use Google Gears.';

  var workerUrl = differentOrigin2 + crossOriginWorkerPath;
  var childId = wp.createWorkerFromUrl(redirectPath + workerUrl);

  wp.sendMessage('cross origin ping', childId);
  waitForGlobalErrors([expectedError]);
}

function testCrossOriginToRedirect() {
  // Tests loading a worker where the URL is cross-origin and results in a
  // redirect.
  var wp = google.gears.factory.create('beta.workerpool');
  wp.onmessage = function(text, sender, m) {
    completeAsync();
  };
  var redirectUrl = differentOrigin + redirectPath;
  var childId = wp.createWorkerFromUrl(redirectUrl + crossOriginWorkerPath);

  startAsync();
  wp.sendMessage('cross origin ping', childId);
}

// Test that cross-domain workers will not load unless served with the correct
// content-type.
function testCrossOriginWorkerWithBadContentType() {
  var expectedError = "Error in worker 0. Cross-origin worker has invalid " +
                      "content-type header.";
  var wp = google.gears.factory.create('beta.workerpool');
  wp.createWorkerFromUrl(differentOrigin + crossOriginWorkerBadContentType);
  waitForGlobalErrors([expectedError]);
}
