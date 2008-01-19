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
 * Creates globals to simplify browser checking
 */

var isIE = false;
var isPIE = false;
var isFF = false;

if (document.pie_dialog) {
  isPIE = true;
} else {
  isIE = Boolean(window.external &&
                 typeof window.external.GetDialogArguments != "undefined");
  isFF = Boolean(window.arguments);
}

/**
 * Initialize the base functionality of the dialog.
 */
function initDialog() {
  if (!isPIE) {
    addEvent(document, "keyup", handleKeyUp);
  }
}

/**
 * Provides a cross-browser way of getting an element by its id
 */
function getElementById(id) {
  if (document.getElementById) {
    return document.getElementById(id);
  } else {
    return document.all[id];
  }
}

/**
 * Allows a dialog to do custom layout when the window changes sizes. The
 * provided function will be called with the height of the content area when the
 * dialog should relayout.
 */
function initCustomLayout(layoutFunction) {
  function doLayout() {
    layoutFunction(getContentHeight());
  }

  doLayout();

  // We do an additional layout in onload because sometimes things aren't
  // stabilized when the first doLayout() is called above.
  addEvent(window, "load", doLayout);
  addEvent(window, "resize", doLayout);

  // Mozilla doesn't fire continuous events during resize, so if we want to get
  // somewhat smooth resizing, we need to run our own timer loop. This still
  // doesn't look perfect, because the timer goes off out of sync with the
  // browser's internal reflow, but I think it's better than nothing.
  // TODO(aa): Keep looking for a way to get an event for each reflow, like IE.
  if (navigator.product == "Gecko") {
    var lastHeight = -1;

    function maybeDoLayout() {
      var currentHeight = getWindowInnerHeight();
      if (currentHeight != lastHeight) {
        lastHeight = currentHeight;
        doLayout();
      }
    }

    window.setInterval(maybeDoLayout, 30);
  }
}

/**
 * Get the JSON-formatted arguments that were passed by the caller.
 */
function getArguments() {
  var argsString;
  if (isIE) {
    // IE
    argsString = window.external.GetDialogArguments();
  } else if (isPIE) {
    // PIE
    argsString = pie_dialog.GetDialogArguments();
  } else if (isFF) {
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
  if (isIE) {
    // IE
    window.external.CloseDialog(resultString);
  } else if (isPIE) {
    // PIE
    pie_dialog.CloseDialog(resultString);
  } else if (isFF) {
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
 * Returns the height of the content area of the dialog.
 */
function getContentHeight() {
  var head = getElementById("head");
  var foot = getElementById("foot");
  return getWindowInnerHeight() - head.offsetHeight - foot.offsetHeight;
}

/**
 * Returns the height of the inside of the window.
 */
function getWindowInnerHeight() {
  if (typeof window.innerHeight != 'undefined') { // Firefox
    return window.innerHeight;
  } else if (typeof document.body.offsetHeight != 'undefined') { // IE
    return document.body.offsetHeight;
  }

  throw new Error("Could not get windowInnerHeight.");
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

/**
 * Disables one of our fancy custom buttons.
 */
function disableButton(buttonElm) {
  if (isPIE) {
    buttonElm.style.textDecoration = "none";
    buttonElm.style.color = "gray";
  } else {
    var classes = buttonElm.className.split(" ");
    for (var i = 0, className; className = classes[i]; i++) {
      if (className == "custom-button-disabled") {
        // already disabled
        return;
      }
    }
    buttonElm.className += " custom-button-disabled";
  }
}

/**
 * Enables one of our fancy custom buttons.
 */
function enableButton(buttonElm) {
  if (isPIE) {
    buttonElm.style.textDecoration = "underline";
    buttonElm.style.color = "blue";
  } else {
    var classes = buttonElm.className.split(" ");
    for (var i = 0, className; className = classes[i]; i++) {
      if (className == "custom-button-disabled") {
        classes.splice(i, 1);
        buttonElm.className = classes.join(" ");
        return;
      }
    }
  }
}
