var localServer = google.gears.factory.create('beta.localserver');

function testCaptureLongUrl() {
  var storeName = 'maximalUserSuppliedStoreNameWhichCanGetVeryLongAsYouSeeaaaaaaaaa';
  var invalidStoreName = 'invalidResourceStoreNameWhichIsMuchLongerThanShouldBeAllowedByAnySelfRespectingComputerProgram';
  
  // test createStore
  assertError(function() {
    localServer.createStore(invalidStoreName);
    }, false, 
    "LocalServer should fail when creating ResourceStores with long names.");
  
  // test openStore
  assertError(function() {
    localServer.openStore(invalidStoreName);
    }, false, 
    "LocalServer should fail when opening ResourceStores with long names.");
  
  // A resource with a long name - 85 characters in the filename and a 
  // 6 character extension.
  // LocalServer should be truncating the name internally and therefore
  // should succeed in capturing this resource.  Resources with long names
  // can and will exist on the web and we should capture them correctly.
  var captureUri = '/testcases/testaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txtfoo';
  
  if (localServer.openStore(storeName)) {
    localServer.removeStore(storeName);
  }
  var resourceStore = localServer.createStore(storeName);

  startAsync();
  resourceStore.capture(captureUri, function(url, success, id) {
    assert(success, 'Retrieval should have succeeded');
    assert(resourceStore.isCaptured(captureUri),
           'Resource should have been captured');
           
    assert(localServer.canServeLocally(captureUri),
           'File should be servable');
    
    httpGet(captureUri, function(content) {
      assert(content == 'foo',
             'Unexpected content in long resource');
    });
           
    completeAsync();      
  });
}

function testRequiredCookie() {
  var STORE_NAME = 'RequiredCookieTestStore';
  var REQUIRED_COOKE = 'a=1';
  var CAPTURE_URI = '/testcases/test_file_1024.txt';
  var RENAME_URI = '/testcases/renamed_not_on_server.txt';
  var EXPECTED_CONTENT = '1111111111111111111111111111111111111111111111';

  localServer.removeStore(STORE_NAME, REQUIRED_COOKE);
  var resourceStore = localServer.createStore(STORE_NAME, REQUIRED_COOKE);

  createTestCookie('a', '1');
  startAsync();
  resourceStore.capture(CAPTURE_URI, function(url, success, id) {
    assert(success, 'Failed to capture');
    assert(resourceStore.isCaptured(CAPTURE_URI),
           'Original should have been captured');

    // Rename it to something not on the server
    resourceStore.rename(CAPTURE_URI, RENAME_URI);
    assert(resourceStore.isCaptured(RENAME_URI),
           'Renamed item should be captured');
    assert(!resourceStore.isCaptured(CAPTURE_URI),
           'Original should not be captured after rename');
    assert(localServer.canServeLocally(RENAME_URI),
           'Should be servable with the right cookie');

    httpGet(RENAME_URI, function(content) {
      assert(content,
             'Should be http fetchable with the right cookie');
      assert(content.startsWith(EXPECTED_CONTENT),
             'Unexpected content in resource');

      createTestCookie('a', '2');
      assert(!localServer.canServeLocally(RENAME_URI),
             'Should not servable with the wrong cookie');

      createTestCookie('a', '1');
      assert(localServer.canServeLocally(RENAME_URI),
             'Should be on again');

      eraseTestCookie('a');
      assert(!localServer.canServeLocally(RENAME_URI),
             'Should not be servable with no cookie');

      localServer.removeStore(STORE_NAME, REQUIRED_COOKE);

      completeAsync();
    });
  });
}

function testRequiredCookieNONE() {
  var STORE_NAME = 'RequiredCookieTestStore';
  var REQUIRED_COOKE = 'a=;NONE;';
  var CAPTURE_URI = '/testcases/test_file_1024.txt';
  var RENAME_URI = '/testcases/renamed_not_on_server_no_cookie.txt';
  var EXPECTED_CONTENT = '1111111111111111111111111111111111111111111111';

  localServer.removeStore(STORE_NAME, REQUIRED_COOKE);
  var resourceStore = localServer.createStore(STORE_NAME, REQUIRED_COOKE);

  eraseTestCookie('a');
  startAsync();
  resourceStore.capture(CAPTURE_URI, function(url, success, id) {
    assert(success, 'Failed to capture');
    assert(resourceStore.isCaptured(CAPTURE_URI),
           'Original should have been captured');

    // Rename it to something not on the server
    resourceStore.rename(CAPTURE_URI, RENAME_URI);
    assert(resourceStore.isCaptured(RENAME_URI),
           'Renamed item should be captured');
    assert(!resourceStore.isCaptured(CAPTURE_URI),
           'Original should not be captured after rename');
    assert(localServer.canServeLocally(RENAME_URI),
           'Should be servable with no cookie');

    httpGet(RENAME_URI, function(content) {
      assert(content,
             'Should be http fetchable with no cookie');
      assert(content.startsWith(EXPECTED_CONTENT),
             'Unexpected content in resource');

      createTestCookie('a', '1');
      assert(!localServer.canServeLocally(RENAME_URI),
             'Should not servable with a cookie');

      eraseTestCookie('a');
      assert(localServer.canServeLocally(RENAME_URI),
             'Should be servable with no cookie (2)');

      localServer.removeStore(STORE_NAME, REQUIRED_COOKE);

      completeAsync();
    });
  });
}
