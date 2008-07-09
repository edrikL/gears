var localServer = google.gears.factory.create('beta.localserver');
var resourceStore = localServer.createStore('noworker_tests');

function testCreateFileSubmitter() {
  assertNotNull(resourceStore.createFileSubmitter(),
                'Could not create FileSubmitter');
}

function testCaptureEmptyFileElement() {
  var div = document.createElement('div');
  document.body.appendChild(div);
  div.innerHTML = '<input type="file">';
  var input = document.getElementsByTagName('input')[0];
  assertError(function() {
    resourceStore.captureFile(input, 'should_fail');
    }, 'File path is empty.',
    'Expected capturing empty file input element to fail');
}

function testCaptureSpoofedInputElement() {
  // QueryInterface is a Firefox-specific thing, but this test is also useful
  // for IE.
  var fileInputSpoofer = {
    type: "file",
    value: "c:\\autoexec.bat",
    QueryInterface: function(iid) {
      return this;
    }
  }

  assertError(function() {
    resourceStore.captureFile(fileInputSpoofer, 'foo');
  });
}

function testCaptureFilenameString() {
  assertError(function() {
    resourceStore.captureFile('c:\\autoexec.bat', 'bar');
  });
}

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

// SAFARI-TEMP - Disable tests that don't currently work on Safari.
testCaptureSpoofedInputElement._disable_in_safari = true;
testCreateFileSubmitter._disable_in_safari = true;
testCaptureEmptyFileElement._disable_in_safari = true;
