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

var geolocation = google.gears.factory.create('beta.geolocation');
var dummyFunction = function() {};

function testArguments() {
  // All good.
  var goodOptions = {
    enableHighAccuracy: false,
    timeout: 0,
    gearsRequestAddress: false,
    gearsAddressLanguage: 'test',
    gearsLocationProviderUrls: ['test']
  };
  geolocation.getCurrentPosition(dummyFunction);
  geolocation.getCurrentPosition(dummyFunction, dummyFunction);
  geolocation.getCurrentPosition(dummyFunction, null);
  geolocation.getCurrentPosition(dummyFunction, null, {});
  geolocation.getCurrentPosition(dummyFunction, null, goodOptions);
  geolocation.getCurrentPosition(dummyFunction, dummyFunction, {});
  geolocation.getCurrentPosition(dummyFunction, dummyFunction, goodOptions);

  // Test correct types.
  // Missing success callback.
  assertError(function() {
    geolocation.getCurrentPosition();
  });
  // Wrong type for success callback.
  assertError(function() {
    geolocation.getCurrentPosition(42);
  });
  // Wrong type for error callback.
  assertError(function() {
    geolocation.getCurrentPosition(dummyFunction, 42);
  });
  // Wrong type for options.
  assertError(function() {
    geolocation.getCurrentPosition(dummyFunction, dummyFunction, 42);
  });
  // Wrong type for enableHighAccuracy.
  assertError(function() {
    geolocation.getCurrentPosition(dummyFunction,
                                   dummyFunction,
                                   {enableHighAccuracy: 42});
  }, 'options.enableHighAccuracy should be a boolean.');
  // Wrong type for timeout.
  assertError(function() {
    geolocation.getCurrentPosition(dummyFunction,
                                   dummyFunction,
                                   {timeout: 42.9});
  }, 'options.timeout should be a non-negative 32 bit signed integer.');
  assertError(function() {
    geolocation.getCurrentPosition(dummyFunction,
                                   dummyFunction,
                                   {timeout: -1});
  }, 'options.timeout should be a non-negative 32 bit signed integer.');
  assertError(function() {
    geolocation.getCurrentPosition(dummyFunction,
                                   dummyFunction,
                                   {timeout: 2147483648});  // 2^31
  }, 'options.timeout should be a non-negative 32 bit signed integer.');
  // Wrong type for gearsRequestAddress.
  assertError(
      function() {
        geolocation.getCurrentPosition(dummyFunction,
                                       dummyFunction,
                                       {gearsRequestAddress: 42});
      },
      'options.gearsRequestAddress should be a boolean.');
  // Wrong type for gearsAddressLanguage.
  assertError(
      function() {
        geolocation.getCurrentPosition(dummyFunction,
                                       dummyFunction,
                                       {gearsAddressLanguage: 42});
      },
      'options.gearsAddressLanguage should be a string.');
  // Wrong type for gearsLocationProviderUrls.
  assertError(
      function() {
        geolocation.getCurrentPosition(dummyFunction,
                                       dummyFunction,
                                       {gearsLocationProviderUrls: 42});
      },
      'options.gearsLocationProviderUrls should be null or an array of ' +
      'strings');
  assertError(
      function() {
        geolocation.getCurrentPosition(dummyFunction,
                                       dummyFunction,
                                       {gearsLocationProviderUrls: [42]});
      },
      'options.gearsLocationProviderUrls should be null or an array of ' +
      'strings');
}

function testNoProviders() {
  assertError(
      function() {
        geolocation.getCurrentPosition(
            dummyFunction,
            dummyFunction,
            {gearsLocationProviderUrls: [], enableHighAccuracy: false});
      },
      'Fix request has no location providers.',
      'Calling getCurrentPosition() should fail if no location providers are ' +
      'specified.');
}

function testZeroTimeout() {
  // A request with a zero timeout should call the error callback immediately.
  function errorCallback(error) {
    assertEqual(error.TIMEOUT, error.code,
                'Error callback should be called with code TIMEOUT.');
    completeAsync();
  }
  startAsync();
  geolocation.getCurrentPosition(dummyFunction, errorCallback, {timeout: 0});
}
