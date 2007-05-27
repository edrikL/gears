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

// JavaScript for go_offline.html
//*****************************************
//  Edit the following two variables to match your 
//  project's naming needs.
//
//  Provide a filename to save your application's info 
//  on the user's local computer.
var STORE_FILENAME = "my_offline_docset";

//  Provide your Manifest file name below.
var MANIFEST_FILENAME = "manifest.json";
//*****************************************

// -----------Begin API calls ------

// ManagedResourceStore create, update, and erase
function createManagedStore(storeName, manifestFileName) {
  var localServer = google.gears.factory.create("beta.localserver","1.0");
  var store = localServer.createManagedStore(storeName);
  store.manifestUrl = manifestFileName;
  store.checkForUpdate();
  
  var timerId = window.setInterval(function() {
    // When the currentVersion property has a value, all of the resources
    // listed in the manifest file for that version are captured. There is
    // an open bug to surface this state change as an event.
    if (store.currentVersion) {
      window.clearInterval(timerId);
      textOut("The documents are now available offline.\n" + 
              "Refresh the browser to see the locally served version. " +
              "The version stored is: " + store.currentVersion);
    }
  }, 500);  
}
// Call this function whenever you want to check if the static files you 
// have stored locally are up-to-date or if they need to be updated.
// If old files are stored locally, this function will update them.
// 
// (Assumes same name for manifest file; however, it is OK to
//  use a new manifest file.)
function updateManagedStore(storeName, manifestFileName) { 
  var localServer = google.gears.factory.create("beta.localserver","1.0");
  var store = localServer.openManagedStore(storeName);
  store.manifestUrl = manifestFileName;
  store.checkForUpdate();
  
  var timerId = window.setInterval(function() {
    // There is an open bug to surface this state change as an event.  
    if (store.currentVersion != "whatever") {
      window.clearInterval(timerId);
      textOut("Update is complete. Reload browser to see version: " +
           store.currentVersion);
    }
  }, 500);
}

function removeManagedStore(storeName) { 
  var localServer = google.gears.factory.create("beta.localserver","1.0");
  localServer.removeManagedStore(storeName);
  textOut("Done. Store has been removed." +
          "Press reload to see the online version again.");
}
//------------ End API calls -----------

//Button Event Handlers
function doCapture() {
  // Makes files available offline.
  createManagedStore(STORE_FILENAME, MANIFEST_FILENAME);	
}

function doUpdate() {
  // Assumes the store has previously been captured, and updates it.
  updateManagedStore(STORE_FILENAME, MANIFEST_FILENAME);
}

function doErase() {
  removeManagedStore(STORE_FILENAME);
}

// Utilities
function textOut(s) {
 var elm = document.getElementById("textOut");
  while (elm.firstChild) {
    elm.removeChild(elm.firstChild);
  } 
  elm.appendChild(document.createTextNode(s));
}


window.onload = function() {
  //This tests if Google Gears is installed or not.
  if (!google.gears.factory) {
    textOut("NOTE:  You must install Google Gears first.");
  } else {
    textOut("Yeay, Google Gears is already installed.");
  }
}
