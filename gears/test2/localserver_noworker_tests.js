var localServer = google.gears.factory.create('beta.localserver', '1.1');
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
  }, 'Invalid file input parameter.');
}

function testCaptureFilenameString() {
  assertError(function() {
    resourceStore.captureFile('c:\\autoexec.bat', 'bar');
  });
}
