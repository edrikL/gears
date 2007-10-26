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

  // Wait for the update to complete
  var timerId = timer.setInterval(function() {
    if (managedStore.currentVersion ||
        managedStore.updateStatus == UPDATE_STATUS.failure) {
      timer.clearInterval(timerId);
      callback(managedStore);
    }
  }, 50);
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
  var captureUri = '../test_file_1024.txt';
  var renameUri = 'renamed.txt';
  var expectedCaptureContent = '1111111111111111111111111111111111111111111111';
  var resourceStore = getFreshStore();

  assert(!resourceStore.isCaptured(captureUri),
         'test file should not be captured');

  startAsync();
  resourceStore.capture(captureUri, function(url, success, id) {
    assert(success, 'Original should have succeeded');
    assert(resourceStore.isCaptured(captureUri),
           'Original should have been captured');

    // Make a copy of it
    var copyUri = "copied.txt";
    resourceStore.copy(captureUri, copyUri);
    assert(resourceStore.isCaptured(copyUri), 'Copy should have been captured');
    assert(resourceStore.isCaptured(captureUri),
           'Original should have been captured after copy');

    // Rename it
    resourceStore.rename(captureUri, renameUri);
    assert(resourceStore.isCaptured(renameUri),
           'Rename should have been captured');
    assert(!resourceStore.isCaptured(captureUri),
           'Original should not have been captured after rename');

    // Verify the local server claims to be able to serve renameUri now
    assert(localServer.canServeLocally(renameUri),
           'Rename should have been servable');

    // Fetch the contents of renameUri and see if its what we expect
    httpGet(renameUri, function(content) {
      var renameContent = content;
      assert(renameContent.startsWith(expectedCaptureContent),
             'Unexpected content in renamed resource');

      // Disable our store and verify we can no longer fetch renameUri
      // note: depends on renameUri not being available on the server)
      resourceStore.enabled = false;
      assert(!localServer.canServeLocally(renameUri),
             'Should not have been able to serve after disable');

      // Fetch and make sure disabled
      httpGet(renameUri, function(content) {
        var disabledContent = content;
        assertNull(disabledContent, 'Should not have served disabled content');

        // Now re-enable and try to redirect back into cache
        resourceStore.enabled = true;
        httpGet("../server_redirect.php?location=runner/" + renameUri,
          function(content) {
            var redirectedContent = content;
            assertEqual(renameContent, redirectedContent, 
                        'Redirected content should match');

            // Now remove the uris, and verify isCaptured() returns false.
            resourceStore.remove(renameUri);
            resourceStore.remove(copyUri);
            assert(!resourceStore.isCaptured(renameUri),
                   'Rename should not have been captured after remove');
            assert(!resourceStore.isCaptured(copyUri),
                   'Copy should not have been captured after remove');

            completeAsync();
          }
        );
      });
    });
  });
}

function testCaptureFragment() {
  var baseUri = '../test_file_fragment';
  var resourceStore = getFreshStore();

  startAsync();
  resourceStore.capture(baseUri + '#foo', function(url, success, id) {
    assert(success, 'Capture should have succeeded');
    assert(resourceStore.isCaptured(baseUri),
           'baseUri without fragment should be captured');
    assert(resourceStore.isCaptured(baseUri + '#foo'),
           '#foo should be captured');
    assert(resourceStore.isCaptured(baseUri + '#bar'),
           '#bar should be captured');
    completeAsync();
  });
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
  var urlList = getObjectProps(urls);
  var results = {};

  startAsync();

  // Capture all the URLs
  resourceStore.capture(urlList, function(url, success, id) {
    assert(!(url in results),
           'Callback called more than once for url "%s"'.subs(url));
    results[url] = 1;

    if (urls[url] == -1) {
      assert(!success, 'Capture of "%s" should have failed'.subs(url));
    } else {
      assert(success, 'Capture of "%s" should have succeeded'.subs(url));
    }

    // Once they are all complete, fetch them all to make sure they were
    // captured correctly.
    ++captureCompleteCount;
    if (captureCompleteCount == urlList.length) {
      fetchNextUrl();
    }
  });

  function fetchNextUrl() {
    var url = urlList.shift();

    if (!url) {
      completeAsync();
      return;
    }

    httpGet(url, function(content) {
      if (urls[url] == -1) {
        assertNull(content,
                   'Should not have been able to fetch "%s"'.subs(url));
      } else {
        assertNotNull(content, 'Should have been able to fetch "%s"'.subs(url));
        assertEqual('text/plain', resourceStore.getHeader(url, "Content-Type"),
                    'Wrong contentType for url "%s"'.subs(url));
        assertEqual(urls[url], content.length,
                   'Wrong content length for url "%s"'.subs(url));
      }

      fetchNextUrl();
    });
  }
}

function testCaptureCrossDomain() {
  var resourceStore = getFreshStore();

  assertError(function() {
    resourceStore.capture('http://cross.domain.not/',
        function(url, success, id) {
      assert(false, 'Should not have fired callback');
    });
  }, null, 'Should have thrown error trying to capture cross-domain resource');
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
  startAsync();
  updateManagedStore("../manifest-good.txt", function(managedStore) {
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

    fetchNextTestUrl();

    function fetchNextTestUrl() {
      var nextUrl = testUrls.shift();
      if (!nextUrl) {
        // we're done!
        completeAsync();
        return;
      }

      httpGet(nextUrl, function(content) {
        assertEqual(expectedUrl1Content, content, 
                    'Incorrect content for url "%s"'.subs(nextUrl));
        fetchNextTestUrl();
      });
    }
  });
}

function testBadManifest() {
  startAsync();
  
  updateManagedStore("../manifest-bad.txt", function(managedStore) {
    assertEqual(UPDATE_STATUS.failure, managedStore.updateStatus,
                'updateStatus should be FAILED after bad manifest');

    var re = /^Download of '.+?url2.txt' returned response code 404$/;
    assert(re.test(managedStore.lastErrorMessage), 
           'Incorrect lastErrorMessage after bad manifest');

    completeAsync();
  });
}

function testInvalidManifest() {
  startAsync();

  updateManagedStore('../manifest-ugly.txt', function(managedStore) {
    assertEqual(UPDATE_STATUS.failure, managedStore.updateStatus,
                'updateStatus should be FAILED after ivalid manifest');

    assert(managedStore.lastErrorMessage.startsWith("Invalid manifest"),
           'Incorrect lastErrorMessage after invalid manifest');

    completeAsync();
  });
}

function testIllegalRedirectManifest() {
  startAsync();

  updateManagedStore('../manifest-illegal-redirect.txt',
    function(managedStore) {
      assertEqual(
        UPDATE_STATUS.failure, managedStore.updateStatus,
        'updateStatus should be FAILED after illegal-redirect manifest');

      assert(managedStore.lastErrorMessage.indexOf('302') > -1,
             'Incorrect lastErrorMessage after illegal-redirect manifest');

      completeAsync();
    }
  );
}
