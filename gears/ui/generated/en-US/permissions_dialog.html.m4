<!DOCTYPE html>

<!--
Copyright 2007, Google Inc.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
 3. Neither the name of Google Inc. nor the names of its contributors may be
    used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

<html>
<head>
  <title>PRODUCT_FRIENDLY_NAME_UQ Security Warning</title>
  <link rel="stylesheet" href="button.css">
  <link rel="stylesheet" href="html_dialog.css">
  <style type="text/css">
    #content {
      margin:0 1em;
    }

    #checkbox-row {
      margin-top:1em;
    }

    #icon {
      margin-right:0.5em;
    }

    #custom-icon {
      margin-right:1em;
      margin-left:1em;
      display:none;
    }

    #custom-name {
      font-weight:bold;
      font-size:1.1em;
      display:none;
    }

    #origin {
      display:none;
    }

    #custom-message {
      margin-top:6px;
      display:none;
    }

    #origin-only {
      font-weight:bold;
      font-size:1.1em;
      display:none;
    }

    #yellowbox {
      margin:0 1px;
      border:solid #f1cc1d;
      border-width:1px 0;
      background:#faefb8;
    }

    #yellowbox-inner {
      /* pseudo-rounded corners */
      margin:0 -1px;
      border:solid #f1cc1d;
      border-width:0 1px;
      padding:1em 0;
    }

    #yellowbox p {
      padding:0 1em;
    }
  </style>
</head>
<body>
  <div id="head">
    <table width="100%" cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td align="left" valign="top">
          <img id="icon" src="icon_32x32.png" width="32" height="32">
        </td>
        <td width="100%" align="left" valign="middle">The website below
          wants to use PRODUCT_FRIENDLY_NAME_UQ. This site will be able to
          store and access information on your computer.&nbsp;
          <a href="http://gears.google.com/?action=help"
             onclick="window.open(this.href); return false;">
          What is this?</a>
        </td>
      </tr>
    </table>
  </div>

  <div id="content">
    <div id="yellowbox">
      <div id="yellowbox-inner">
        <table width="100%" cellpadding="0" cellspacing="0" border="0">
          <tr>
            <td align="left" valign="top">
              <img id="custom-icon" width="0" height="0">
            </td>
            <td width="100%" align="left" valign="middle">
              <div id="custom-name"></div>
              <div id="origin"></div>
              <div id="custom-message"></div>
              <div id="origin-only" align="center"></div>
            </td>
          </tr>
        </table>
      </div>
    </div>

    <p id="checkbox-row">
      <table cellspacing="0" cellpadding="0" border="0">
        <tr>
          <td valign="middle">
            <input type="checkbox" id="unlock" accesskey="T">
          </td>
          <td valign="middle">
            <label for="unlock">
              &nbsp;I <span class="accesskey">t</span>rust this site. Allow
              it to use PRODUCT_FRIENDLY_NAME_UQ.
            </label>
          </td>
        </tr>
      </table>
    </p>
    <br>
  </div>

  <div id="foot">
    <div id="button-row">
      <table cellpadding="0" cellspacing="0" border="0">
        <tr>
          <td width="100%" align="left" valign="middle">
            <a href="#" onclick="denyAccessPermanently(); return false;">Never
            allow this site</a>
          </td>
          <td nowrap="true" align="right" valign="middle">
            <!--
            Fancy buttons
            Note: Weird line breaks are on purpose to avoid extra space between
            buttons.
            Note: Outer element is <a> because we want it to focusable and
            behave like an anchor. Inner elements should theoretically be able
            to be <span>, but IE renders incorrectly in this case.
            -->
            <a href="#" accesskey="A" id="allow-button" 
                onclick="allowAccess(); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box"
                  ><span class="accesskey">A</span>llow</div></div></a>
            <!--
            Note: There must be whitespace here or Firefox messes up the
            rendering.
            TODO(aa): This results in inconsistent spacing in IE vs Firefox
            between the buttons, but I am reluctant to hack things even further
            to fix that.
            -->
            <a href="#" accesskey="C" id="deny-button"
                onclick="denyAccess(); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box"
                  ><span class="accesskey">C</span>ancel</div></div></a>
          </td>
        </tr>
      </table>
    </div>
  </div>
</body>
<script src="json_noeval.js"></script>
<script src="html_dialog.js"></script>
<script>
  initDialog();
  initWarning();

  var disabled = true;

  function initWarning() {
    // The arguments to this dialog are a single string, see PermissionsDialog
    var args = getArguments();

    var origin = args['origin'];  // required parameter
    var customIcon = args['customIcon'];
    var customName = args['customName'];
    var customMessage = args['customMessage'];

    var elem;

    if (!customIcon && !customName && !customMessage) {
       elem = document.getElementById("origin-only");
       elem.style.display = "block";
       elem.appendChild(document.createTextNode(origin));
    } else {
       elem = document.getElementById("origin");
       elem.style.display = "block";
       elem.appendChild(document.createTextNode(origin));
    }

    if (customIcon) {
      elem = document.getElementById("custom-icon");
      elem.style.display = "inline";
      elem.src = customIcon;
      elem.height = 32;
      elem.width = 32;
    }

    if (customName) {
      elem = document.getElementById("custom-name");
      elem.style.display = "block";
      elem.appendChild(document.createTextNode(customName));
    }

    if (customMessage) {
      elem = document.getElementById("custom-message");
      elem.style.display = "block";
      elem.appendChild(document.createTextNode(customMessage));
    }

    // Focus deny by default
    document.getElementById("deny-button").focus();

    // Set up the checkbox to toggle the enabledness of the Allow button.
    document.getElementById("unlock").onclick = updateAllowButtonEnabledState;
    updateAllowButtonEnabledState();

    // Resize the window to fit
    var contentDiv = document.getElementById("content");
    var contentHeightProvided = getContentHeight();
    var contentHeightDesired = contentDiv.offsetHeight;
    if (contentHeightDesired != contentHeightProvided) {
      var dy = contentHeightDesired - contentHeightProvided;
      window.resizeBy(0, dy);
    }
  }


  function updateAllowButtonEnabledState() {
    var allowButton = document.getElementById("allow-button");
    var checkbox = document.getElementById("unlock");

    disabled = !checkbox.checked;

    if (disabled) {
      disableButton(allowButton);
    } else {
      enableButton(allowButton);
    }
  }

  // Note: The structure that the following functions pass are coupled to the
  // code in PermissionsDialog that processes it.
  function allowAccess() {
    if (!disabled) {
      saveAndClose({
        allow: true
      });
    }
  }

  function denyAccess() {
    saveAndClose(null); // default behavior is to deny temporarily
  }

  function denyAccessPermanently() {
    saveAndClose({
      allow: false
    });
  }
</script>
</html>
