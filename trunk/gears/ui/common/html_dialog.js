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

// TODO(aa): Implement in terms of userAgent and move to base.js.
var isIE = false;
var isPIE = false;
var isFF = false;
var isSafari = false;

if (isDefined(typeof window.pie_dialog)) {
  isPIE = true;
} else {
  // Note that we can't use isDefined() here because
  // window.external.GetDialogArguments has type 'unknown' on IE!
  isIE = isDefined(typeof window.external) &&
         typeof window.external.GetDialogArguments != 'undefined';
  isFF = isDefined(typeof window.arguments);
  isSafari = isDefined(typeof window.gears_dialogArguments);
}

/**
 * Initialize the base functionality of the dialog.
 */
function initDialog() {
  if (!isPIE) {
    dom.addEvent(document, "keyup", handleKeyUp);
  } else {
    var buttonRowElem = null;
    if (window.pie_dialog.IsSmartPhone()) {
      buttonRowElem = dom.getElementById("button-row-smartphone");
    } else {
      buttonRowElem = dom.getElementById("button-row");
    }
    if (buttonRowElem) {
      buttonRowElem.style.display = 'block';
    }
  }
  if (isPIE) {
    window.pie_dialog.SetScriptContext(window);
    window.pie_dialog.ResizeDialog();
  }
}

/**
 * Set the label of input button, an optionally, it's accesskey.
 */
function setButtonLabel(textID, elemID, accessKeyID) {
  var textElem = dom.getElementById(textID);
  var buttonElem = dom.getElementById(elemID);
  if (!isDefined(typeof textElem) || !isDefined(typeof buttonElem)) {
    return;
  }

  // This function works for two different kinds of buttons. Simple buttons
  // based on the <input type="button"> tag, and custom buttons based on a
  // <button> tag with the css class "custom".
  // Note that on Windows Mobile 5, the tagName property is not defined.
  // We can therefore safely assume that the converse is also true:
  // if tagName is not defined, then the platform must be Windows Mobile 5.  
  if (!isDefined(typeof buttonElem.tagName) ||
      buttonElem.tagName.toLowerCase() == "input") {
    buttonElem.value = dom.getTextContent(textElem).trim();
    if (isDefined(typeof accessKeyElem)) {
      buttonElem.accessKey = dom.getTextContent(accessKeyElem).trim();
    }
  } else if (buttonElem.tagName.toLowerCase() == "button") {
    var text = dom.getTextContent(textElem).trim();
    var textLength = text.length;

    if (isDefined(typeof accessKeyID)) {
      // Some browsers use the accessKey attribute of the the anchor tag.
      var accessKeyElem = dom.getElementById(accessKeyID);
      var accessKey = dom.getTextContent(accessKeyElem).trim();
      buttonElem.accessKey = accessKey;

      // Find the first matching character in the text and mark it.
      // Note: this form of String.replace() only replaces the first occurence.
      text = text.replace(accessKey,
                          "<span class='accesskey'>" + accessKey + "</span>");
    }

    buttonElem.innerHTML = text;
  } else {
    throw new Error("Unexpected button tag name: " + buttonElem.tagName);
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
  dom.addEvent(window, "load", doLayout);
  dom.addEvent(window, "resize", doLayout);

  // Mozilla doesn't fire continuous events during resize, so if we want to get
  // somewhat smooth resizing, we need to run our own timer loop. This still
  // doesn't look perfect, because the timer goes off out of sync with the
  // browser's internal reflow, but I think it's better than nothing.
  // TODO(aa): Keep looking for a way to get an event for each reflow, like IE.
  if (navigator.product == "Gecko") {
    var lastHeight = -1;

    function maybeDoLayout() {
      var currentHeight = dom.getWindowInnerHeight();
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
    argsString = window.pie_dialog.GetDialogArguments();
  } else if (isFF) {
    // Firefox
    argsString = getFirefoxArguments(window.arguments[0]);
  } else if (isSafari) {
    // Safari
    argsString = window.gears_dialogArguments;
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
    window.pie_dialog.CloseDialog(resultString);
  } else if (isFF) {
    // Firefox
    saveFirefoxResults(resultString);
    window.close();
  } else if (isSafari) {
    // Safari
    window.gears_dialog.setResults(resultString);
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
  var head = dom.getElementById("head");
  var foot = dom.getElementById("foot");
  return dom.getWindowInnerHeight() - head.offsetHeight - foot.offsetHeight;
}

/**
 * For some reason ESC doesn't work on HTML dialogs in either Firefox or IE. So
 * we implement it manually.
 */
function handleKeyUp(e) {
  var ESC_KEY_CODE = 27;
  
  if (e.keyCode == ESC_KEY_CODE) {
    saveAndClose(null);
  }
}

/**
 * Disables a button in the right way, whether it is normal or custom.
 */
function disableButton(buttonElem) {
  buttonElem.disabled = true;

  if (isPIE) {
    window.pie_dialog.SetButtonEnabled(false);
  }

  if (!isDefined(typeof buttonElem.tagName) || 
      buttonElem.tagName.toLowerCase() == "input") {
    buttonElem.style.color = "gray";
  } else if (buttonElem.tagName.toLowerCase() == "button") {
    dom.addClass(buttonElem, "disabled");
  } else {
    throw new Error("Unexpected tag name: " + buttonElem.tagName);
  }
}

/**
 * Enables a button in the right way, whether it is normal or custom.
 */
function enableButton(buttonElem) {
  buttonElem.disabled = false;

  if (isPIE) {
    window.pie_dialog.SetButtonEnabled(true);
  }
  
  if (!isDefined(typeof buttonElem.tagName) ||
      buttonElem.tagName.toLowerCase() == "input") {
    buttonElem.style.color = "black";
  } else if (buttonElem.tagName.toLowerCase() == "button") {
    dom.removeClass(buttonElem, "disabled");
  } else {
    throw new Error("Unexpected tag name: " + buttonElem.tagName);
  }
}

/**
 * Returns a wrapped domain (useful for small screens dialogs, 
 * e.g. windows mobile devices)
 */
function wrapDomain(str) {
  if (isPIE) {
    var max = 20;
    var url;
    var scheme_start = str.indexOf("://");
    var scheme = "";

    if (scheme_start != -1) {
      scheme = str.substring(0, scheme_start);
      scheme += "://";
      // there's automatically an hyphenation
      // point used by the browser after http://
      // so we only have to hyphenate the
      // rest of the string
      url = str.substring(scheme.length);
    } else {
      url = str;
    }

    // We hyphenate the string on every dot
    var components = url.split(".");
    if (components.length < 1) {
      return str;
    }

    var content = components[0];
    var len = content.length;
    for (var i=1; i < components.length; i++) {
      var elem = components[i];
      content += ".";
      len++;
      if (len + elem.length > max) {
        content += "<br>";
        len = 0;
      }
      content += elem;
      len += elem.length;
    }
    return scheme + content;
  }
  return str;
}
