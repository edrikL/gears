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
//
// Sets up google.gears.factory to point to an instance of 
// GearsFactory.
//
// google.gears.factory is *the only* supported way to
// access GearsFactory. We reserve the right to change the 
// way Scour is exposed in the browser at anytime, but we
// will always support the google.gears.factory object.

if (typeof google == 'undefined') {
  var google = {};
}

if (typeof google.gears == 'undefined') {
  google.gears = {};
}

if (typeof google.gears.factory == 'undefined') {
  google.gears.factory = (function() {
    // Firefox
    if (typeof GearsFactory != 'undefined') {
      return new GearsFactory();
    }

    // IE
    try {
      return new ActiveXObject('Gears.Factory');
    } catch (e) {}

    // Safari
    if (navigator.mimeTypes["application/x-googlegears"]) {
      var factory = document.createElement("object");
      factory.style.display = "none";
      factory.width = "0";
      factory.height = "0";
      factory.type = "application/x-googlegears";
      document.documentElement.appendChild(factory);
      return factory;
    }

    return null;
  })();
}
