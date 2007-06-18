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

function setupSample() {
  // Make sure we have Gears. If not, tell the user.
  if (!window.google || !google.gears) {
    if (confirm("This demo requires Gears to be installed. Install now?")) {
      var spliceStart = location.href.indexOf("/samples");
      location.href =
        location.href.substring(0, spliceStart) + "/install.html";
      return;
    }
  }

  var viewSourceElem = document.getElementById("view-source");
  if (!viewSourceElem) {
    return;
  }
  var elm;
  if (navigator.product == "Gecko") {
    // If we're gecko, we can show the source of the application with the
    // view-source protocol.
    elm = document.createElement("a");
    elm.href = "view-source:" + location.href;
    elm.innerHTML = "View Demo Source";
  } else {
    // Otherwise, just tell users how to do it manually.
    elm = document.createElement("em");
    elm.innerHTML = "To see how this works, use the <strong>view "+
      "source</strong> feature of your browser";
  }
  viewSourceElem.appendChild(elm);
}

function checkProtocol() {
  if (location.protocol.indexOf('http') != 0) {
    setError('This sample must be hosted on an HTTP server');
    return false;
  } else {
    return true;
  }
}

function addStatus(s, opt_class) {
  var elm = document.getElementById('status');
  if (!elm) return;
  var node = document.createTextNode(s);
  if (opt_class) {
    var span = document.createElement('span');
    span.className = opt_class;
    span.appendChild(node);
    node = span;
  }
  elm.appendChild(node);
  elm.appendChild(document.createElement('br'));
}

function clearStatus() {
  var elm = document.getElementById('status');
  while (elm && elm.firstChild) {
    elm.removeChild(elm.firstChild);
  }
}

function setError(s) {
  clearStatus();
  addStatus(s, 'error');
}

setupSample();
