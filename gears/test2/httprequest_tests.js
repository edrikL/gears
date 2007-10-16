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

function testGet200() {
  doRequest(
      '../test_file_1.txt', 'GET', null, null, // url, method, data, reqHeaders[]
      200, '1', null);  // expected status, responseText, responseHeaders[]
}

function testPost200() {
  var data = 'hello';
  var headers = [["Name1", "Value1"],
                 ["Name2", "Value2"]];
  var expectedHeaders = getExpectedEchoHeaders(headers);
  doRequest('../echo_request.php', 'POST', data, headers, 200, data,
            expectedHeaders); 
}

function testPost302_200() {
  // A POST that gets redirected should GET the new location
  var data = 'hello';
  var expectedHeaders = [["echo-Method", "GET"]];
  doRequest('../server_redirect.php?location=echo_request.php', 'POST', data,
            null, 200, null, expectedHeaders); 
}

function testGet404() {
  doRequest('../nosuchfile___', 'GET', null, null, 404, null, null);
}

function testGet302_200() {
  doRequest('../server_redirect.php?location=test_file_1.txt', 'GET', null,
            null, 200, '1', null);
}

function testGet302_404() {
  doRequest('../server_redirect.php?location=nosuchfile___', 'GET', null, null,
            404, null, null);
}  

function testGetNoCrossOrigin() {
  assertError(function() {
    doRequest('http://www.google.com/', 'GET', null, null, 0, null, null);
  });
}

function testGet302NoCrossOrigin() {
  var headers = [["location", "http://www.google.com/"]];
  doRequest('../server_redirect.php?location=http://www.google.com/', 'GET',
            null, null, 302, "", headers);
}

function testRequestDisallowedHeaders() {
  var headers = [["Referer", "http://somewhere.else.com/"]];
  assertError(function() {
    doRequest('../should_fail', 'GET', null, headers, null, null, null);
  });
}

function testRequestReuse() {
  var reusedRequest = google.gears.factory.create('beta.httprequest', '1.0');
  var numGot = 0;
  var numToGet = 2;

  var errorMessage = null;
  var status;

  getOne();
  
  function getOne() {          
    if (numGot >= numToGet) {
      return;
    }  
    var url = '../echo_request.php?' + numGot++;
    reusedRequest.onreadystatechange = function() {
      if (reusedRequest.readyState == 4) {
        try {
          status = reusedRequest.status;
          getOne();
        } catch (e) {
          errorMessage = e.message;
        }
      }
    };
    reusedRequest.open('GET', url, true);
    reusedRequest.send(null);
  }    

  scheduleCallback(function() {
    assertEqual(numToGet, numGot, 'Did not expected number of requests');
    assertNull(errorMessage, 'Unexpected error');
    assertEqual(200, status, 'Unexpected status value');
  }, 400);
}

function testGetCapturedResource() {
  var myLocalServer = google.gears.factory.create('beta.localserver', '1.0');
  // We don't delete and recreate the store or captured url to avoid
  // interfering with this same test running in the other thread. 
  var storeName = 'testGet_CapturedResource';
  myLocalServer.removeStore(storeName);
  var myStore = myLocalServer.createStore(storeName);
  var url = '../echo_request.php?httprequest_a_captured_url';
  var captureSuccess;

  myStore.capture(url, function(url, success, id) {
    captureSuccess = success;
  });

  scheduleCallback(function() {
    doRequest(url, 'GET', null, null, 200, null, null);
  }, 100);
}
  
function testGet_BinaryResponse() {
  // TODO(michaeln): do something reasonable with binary responses
}

function testNullOnReadyStateChange() {
  var nullHandlerRequest = google.gears.factory.create('beta.httprequest',
                                                       '1.0');
  nullHandlerRequest.onreadystatechange = function() {};
  nullHandlerRequest.onreadystatechange = null;
  nullHandlerRequest.open('GET', 'nosuchfile___');
  nullHandlerRequest.send();

  var unsetHandlerRequest = google.gears.factory.create('beta.httprequest',
                                                        '1.0');
  unsetHandlerRequest.open('GET', 'nosuchfile___');
  unsetHandlerRequest.send();
}

// Generates header name value pairs echo_requests.php will respond
// with for the given request headers.
function getExpectedEchoHeaders(requestHeaders) {
  var echoHeaders = [];
  for (var i = 0; i < requestHeaders.length; ++i) {
    var name = 'echo-' + requestHeaders[i][0];
    var value = requestHeaders[i][1];
    echoHeaders.push([name, value]);
  }
  return echoHeaders;
}

// A helper that initiates a request and examines the response.
function doRequest(url, method, data, requestHeaders, expectedStatus,
                   expectedResponse, expectedHeaders) {
  var request = google.gears.factory.create('beta.httprequest', '1.0');
  var isComplete = false;
  var status;
  var statusText;
  var headers;
  var calledTooManyTimes = false;
  var responseText;

  request.onreadystatechange = function() {
    var state = request.readyState;
    if (state < 0 || state > 4) {
      throw new Error('Invalid readyState value, ' + state);
    }
    if (request.readyState >= 3) {
      // fetch all of the values we can fetch
      status = request.status;
      statusText = request.statusText;
      headers = request.getAllResponseHeaders();

      if (request.readyState == 4) {
        if (isComplete) {
          calledTooManyTimes = true;
          return;
        }
        isComplete = true;
        responseText = request.responseText;
      }
    }
  };
  request.open(method, url, true);
  if (requestHeaders) {
    for (var i = 0; i < requestHeaders.length; ++i) {
      request.setRequestHeader(requestHeaders[i][0],
                               requestHeaders[i][1]);
    }
  }
  request.send(data);

  scheduleCallback(function() {
    // see if we got what we expected to get
    var msg = '';
    if (expectedStatus != null) {
      assertEqual(expectedStatus, status,
                  'Wrong value for status property');
    }
    if (expectedHeaders != null) {
      for (var i = 0; i < expectedHeaders.length; ++i) {
        var name = expectedHeaders[i][0];
        var expectedValue = expectedHeaders[i][1];
        var actualValue = request.getResponseHeader(name);
        assertEqual(expectedValue, actualValue,
                    'Wrong value for header "%s"'.subs(name));
      }
    }
    if (isComplete && expectedResponse != null) {
      assertEqual(expectedResponse, responseText, 'Wrong responseText');
    }
  }, 200);
}
