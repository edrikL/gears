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

/**
 * Initialize the base functionality of the dialog.
 */
function initDialog() {
  addEvent(document, "keyup", handleKeyUp);
  addEvent(window, "resize", layoutDialog);
  layoutDialog();
}

/**
 * Get the JSON-formatted arguments that were passed by the caller.
 */
function getArguments() {
  var argsString;
  if (window.external &&
      typeof window.external.GetDialogArguments != "undefined") {
    // IE
    argsString = window.external.GetDialogArguments();
  } else if (window.arguments && window.arguments[0]) {
    // Firefox
    argsString = getFirefoxArguments(window.arguments[0]);
  }

  if (typeof argsString == "string") {
    return JSON.parse(argsString);
  } else {
    return null;
  }
}

/**
 * Helper used by getArguments(). Getting the arguments in the right type is
 * more involved in Firefox.
 */
function getFirefoxArguments(windowArguments) {
  var Ci = Components.interfaces;
  windowArguments.QueryInterface(Ci.nsIProperties);
  var supportsString = 
    windowArguments.get("dialogArguments", Ci.nsISupportsString);
  return supportsString.data;
}

/**
 * Close the dialog, sending the specified result back to the caller.
 */
function saveAndClose(resultObject) {
  var resultString = JSON.stringify(resultObject);
  if (window.external &&
      typeof window.external.CloseDialog != "undefined") {
    // IE
    window.external.CloseDialog(resultString);
  } else if (window.arguments && window.arguments[0]) {
    // Firefox
    saveFirefoxResults(resultString);
    window.close();
  }
}

/**
 * Helper used by endDialog() for Firefox.
 */
function saveFirefoxResults(resultString) {
  var Cc = Components.classes;
  var Ci = Components.interfaces;
  
  var props = window.arguments[0].QueryInterface(Ci.nsIProperties);
  var supportsString = Cc["@mozilla.org/supports-string;1"]
      .createInstance(Ci.nsISupportsString);
      
  supportsString.data = resultString;
  props.set("dialogResult", supportsString);
}

/**
 * Workaround for the fact that CSS doesn't have any way to create
 * variable-height scrolling regions. We measure the dialog's inner height
 * manually and set the scrolling region's height based on that.
 */
function layoutDialog() {
  var buttonRow = document.getElementById("button-row");
  var scroll = document.getElementById("scroll");
  if (!buttonRow || !scroll) {
    throw new Error("button-row or main element not found.");
  }
  
  var clientHeight = 0;
  if (window.innerHeight) {
    // Firefox
    clientHeight = window.innerHeight;
  } else if (document.body.offsetHeight) {
    // IE
    clientHeight = document.body.offsetHeight;
  }
  
  if (!clientHeight) {
    throw new Error("Could not get clientHeight.");
  }
  
  scroll.style.height = (clientHeight - buttonRow.offsetHeight) + "px";
}

/**
 * For some reason ESC doesn't work on HTML dialogs in either Firefox or IE. So
 * we implement it manually.
 */
function handleKeyUp(e) {
  e = e || window.event;
  var ESC_KEY_CODE = 27;
  
  if (e.keyCode == ESC_KEY_CODE) {
    saveAndClose(null);
  }
}

/**
 * Utility to add an event listener cross-browser.
 */
function addEvent(element, eventName, handler) {
  if (element.addEventListener) {
    // Standards-compatible browsers
    element.addEventListener(eventName, handler, false);
  } else {
    // IE
    element.attachEvent("on" + eventName, handler);
  }
}
