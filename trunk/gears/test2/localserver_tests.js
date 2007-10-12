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

var localServer = google.gears.factory.create('beta.localserver', '1.1');
var STORE_NAME = 'unit_test';
var UPDATE_STATUS = {
  ok: 0,
  checking: 1,
  updating: 2,
  failure: 3
};

// TODO(michaeln): Some of the tests want to open the same
// store rather than deleting then recreating a new one.
// These tests are not quite correct yet.
function getFreshStore() {
  if (localServer.openStore(STORE_NAME)) {
    localServer.removeStore(STORE_NAME);
  }
  return localServer.createStore(STORE_NAME);
}

function getFreshManagedStore() {
  if (localServer.openManagedStore(STORE_NAME)) {
    localServer.removeManagedStore(STORE_NAME);
  }
  return localServer.createManagedStore(STORE_NAME);
}

function updateManagedStore(url, callback) {
  var managedStore = getFreshManagedStore();
  managedStore.manifestUrl = url;
  managedStore.checkForUpdate();

  // Wait 1 second for the update check to complete
  scheduleCallback(partial(callback, managedStore),
                   200);
}

function testEmptyParams() {
  var shouldFail = [
    'localServer.createStore()',
    'localServer.createStore("")',
    'localServer.createStore(null)',
    'localServer.createStore(undefined)',
    'localServer.createManagedStore()',
    'localServer.createManagedStore("")',
    'localServer.createManagedStore(null)',
    'localServer.createManagedStore(undefined)'
  ];
  
  for (var i = 0, str; str = shouldFail[i]; i++) {
    assertError(function() {
      eval(str)
    }, null, 'Incorrectly allowed call "%s"'.subs(str));
  }
}

function testInternal() {
  if (isDebug) {
    assert(localServer.canServeLocally('test:webcache'),
           'Internal tests failed.');
  }
}

function testCaptureUrl() {
  var captureUri = '../localserver_tests.js';
  var renameUri = 'renamed.js';
  var expectedCaptureContent = '// Copyright 2007, Google Inc.';
  var resourceStore = getFreshStore();

  assert(!resourceStore.isCaptured(captureUri),
         'test file should not be captured');

  var originalSuccess;
  var originalCaptured;
  var copyCaptured;
  var originalCapturedAfterCopy;
  var renameCaptured;
  var originalCapturedAfterRename;
  var renameCanBeServed;
  var renameContent;
  var canServeAfterDisable;
  var disabledContent;
  var redirectedContent;
  var renameIsCapturedAfterRemove;
  var copyIsCapturedAfterRemove;

  resourceStore.capture(captureUri, function(url, success, id) {
    originalSuccess = success;
    originalCaptured = resourceStore.isCaptured(captureUri);

    // Make a copy of it
    var copyUri = "Copy_of_" + captureUri;
    resourceStore.copy(captureUri, copyUri);
    copyCaptured = resourceStore.isCaptured(copyUri);
    originalCapturedAfterCopy = resourceStore.isCaptured(captureUri);

    // Rename it
    resourceStore.rename(captureUri, renameUri);
    renameCaptured = resourceStore.isCaptured(renameUri);
    originalCapturedAfterRename = resourceStore.isCaptured(captureUri);

    // Verify the local server claims to be able to serve renameUri now
    renameCanBeServed =  localServer.canServeLocally(renameUri);

    // Fetch the contents of renameUri and see if its what we expect
    httpGet(renameUri, function(content) {
      renameContent = content;

      // Disable our store and verify we can no longer fetch renameUri
      // note: depends on renameUri not being available on the server)
      resourceStore.enabled = false;
      canServeAfterDisable = localServer.canServeLocally(renameUri);

      // Fetch and make sure disabled
      httpGet(renameUri, function(content) {
        disabledContent = content;

        // Now re-enable and try to redirect back into cache
        resourceStore.enabled = true;
        httpGet("../server_redirect.php?location=runner/" + renameUri,
          function(content) {
            redirectedContent = content;

            // Now remove the uris, and verify isCaptured() returns false.
            resourceStore.remove(renameUri);
            resourceStore.remove(copyUri);
            renameIsCapturedAfterRemove = resourceStore.isCaptured(renameUri);
            copyIsCapturedAfterRemove = resourceStore.isCaptured(copyUri);
          }
        );
      });
    });
  });

  scheduleCallback(function() {
    assert(originalSuccess, 'Original should have succeeded');
    assert(originalCaptured, 'Original should have been captured');
    assert(copyCaptured, 'Copy should have been captured');
    assert(originalCapturedAfterCopy,
           'Original should have been captured after copy');
    assert(renameCaptured, 'Rename should have been captured');
    assert(!originalCapturedAfterRename,
           'Original should not have been captured after rename');
    assert(renameCanBeServed, 'Rename should have been servable');
    assert(renameContent.startsWith(expectedCaptureContent),
           'Unexpected content in renamed resource');
    assert(!canServeAfterDisable,
           'Should not have been able to serve after disable');
    assertNull(disabledContent, 'Should not have served disabled content');
    assertEqual(renameContent, redirectedContent, 
                'Redirected content should match');
    assert(!renameIsCapturedAfterRemove,
           'Rename should not have been captured after remove');
    assert(!copyIsCapturedAfterRemove,
           'Copy should not have been captured after remove');
  }, 200);
}

function testCaptureFragment() {
  var baseUri = '../test_file_fragment';
  var resourceStore = getFreshStore();

  var captureSuccess;
  var baseIsCaptured;
  var fooIsCaptured;
  var barIsCaptured;

  resourceStore.capture(baseUri + '#foo', function(url, success, id) {
    captureSuccess = success;
    baseIsCaptured = resourceStore.isCaptured(baseUri);
    fooIsCaptured = resourceStore.isCaptured(baseUri + '#foo');
    barIsCaptured = resourceStore.isCaptured(baseUri + '#bar');
  });

  scheduleCallback(function() {
    assert(captureSuccess, 'Capture should have succeeded');
    assert(baseIsCaptured, 'baseUri without fragment should be captured');
    assert(fooIsCaptured, '#foo should be captured');
    assert(barIsCaptured, '#bar should be captured');
  }, 300);
}

function testCaptureMany() {
  var urls = {
    "../test_file_0.txt": 0,
    "../nonexistent_file": -1,  // should fail
    "../test_file_1.txt": 1,
    "../server_redirect.php?location=nonexistent_file": -1,
    "../test_file_1024.txt": 1024
  };

  var resourceStore = getFreshStore();
  var captureCompleteCount = 0;
  var results = {};

  resourceStore.capture(getObjectProps(urls), function(url, success, id) {
    results[url] = {success: success};

    httpGet(url, function(content) {
      results[url].content = content;
      if (content !== null) {
        results[url].contentType =
          resourceStore.getHeader(url, "Content-Type");
        results[url].length = content.length;
      }
    });
  });

  scheduleCallback(function() {
    for (var url in urls) {
      assert(Boolean(results[url]),
             'Capture of "%s" did not complete'.subs(url));
      
      if (urls[url] == -1) {
        assert(!results[url].success,
               'Capture of "%s" should have failed'.subs(url));
        assertNull(results[url].content,
                   'Should not have been able to fetch "%s"'.subs(url));
      } else {
        assert(results[url].success,
               'Capture of "%s" should have succeeded'.subs(url));
        assertNotNull(results[url].content,
                      'Should have been able to fetch "%s"'.subs(url));
        assertEqual('text/plain', results[url].contentType,
                    'Wrong contentType for url "%s"'.subs(url));
        assertEqual(urls[url], results[url].length,
                   'Wrong content length for url "%s"'.subs(url));
      }
    }
  }, 400);
}

function testCaptureCrossDomain() {
  var calledBack = false;
  var resourceStore = getFreshStore();

  assertError(function() {
    resourceStore.capture('http://cross.domain.not/',
        function(url, success, id) {
      calledBack = true;
    });
  }, null, 'Should have thrown error trying to capture cross-domain resource');

  scheduleCallback(function() {
    assert(!calledBack, 'Should not have fired callback');
  }, 10);
}

function testCaptureWithNullCallback() {
  var resourceStore = getFreshStore();
  resourceStore.capture('../nonexistent_file', null);
}

function testGoodManifest() {
  // First, fetch url1's contents for later comparison
  var expectedUrl1Content;
  httpGet("../manifest-url1.txt", function(content) {
    expectedUrl1Content = content;
  });

  // Then, capture a manifest containing many references to that URL
  scheduleCallback(function() {
    updateManagedStore("../manifest-good.txt",
                 partial(checkGoodManifest, expectedUrl1Content));
  }, 50);
}

function checkGoodManifest(expectedUrl1Content, managedStore) {
  assertEqual(UPDATE_STATUS.ok, managedStore.updateStatus,
              'updateStatus should be OK after good manifest');

  // TODO(aa): It would be cool if we could actually return null in this case
  assertEqual('', managedStore.lastErrorMessage,
              'lastErrorMessage should be empty string after good manifest');

  var testUrls = [
    '../manifest-url1.txt',
    '../manifest-url1.txt?query',
    '../alias-to-manifest-url1.txt',
    '../redirect-to-manifest-url1.txt',
    '../unicode?foo=bar'
  ];

  for (var i = 0; i < testUrls.length; i++) {
    assert(localServer.canServeLocally(testUrls[i]),
           'Should be able to serve "%s" locally'.subs(testUrls[i]));
  }

  var fetchResults = {};

  function fetchTestUrl(url) {
    httpGet(url, function(content) {
      fetchResults[url] = content;
    });
  }

  // Fetch all the URLs and make sure they  have the right content
  for (var i = 0; i < testUrls.length; i++) {
    fetchTestUrl(testUrls[i]);
  }

  // Wait for them to complete and then verify contents. They are captured, so
  // it should be fast.
  scheduleCallback(function() {
    for (var i = 0; i < testUrls.length; i++) {
      assertEqual(expectedUrl1Content, fetchResults[testUrls[i]],
                  'Incorrect content for url "%s"'.subs(testUrls[i]));
    }
  }, 100);
}

function testBadManifest() {
  updateManagedStore("../manifest-bad.txt", function(managedStore) {
    assertEqual(UPDATE_STATUS.failure, managedStore.updateStatus,
                'updateStatus should be FAILED after bad manifest');

    var re = /^Download of '.+?url2.txt' returned response code 404$/;
    assert(re.test(managedStore.lastErrorMessage), 
           'Incorrect lastErrorMessage after bad manifest');
  });
}

function testInvalidManifest() {
  updateManagedStore('../manifest-ugly.txt', function(managedStore) {
    assertEqual(UPDATE_STATUS.failure, managedStore.updateStatus,
                'updateStatus should be FAILED after ivalid manifest');

    assert(managedStore.lastErrorMessage.startsWith("Invalid manifest"),
           'Incorrect lastErrorMessage after invalid manifest');
  });
}

function testIllegalRedirectManifest() {
  updateManagedStore('../manifest-illegal-redirect.txt',
    function(managedStore) {
      assertEqual(
        UPDATE_STATUS.failure, managedStore.updateStatus,
        'updateStatus should be FAILED after illegal-redirect manifest');

      assert(managedStore.lastErrorMessage.indexOf('302') > -1,
             'Incorrect lastErrorMessage after illegal-redirect manifest');
    }
  );
}
