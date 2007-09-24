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
  <title>Security Warning</title>
  <link rel="stylesheet" href="button.css">
  <link rel="stylesheet" href="html_dialog.css">
  <style type="text/css">
    #button-row {
      /* 
      Override the background in html_dialog.css
      TODO(aa): Remove this when the settings dialog is redesigned and
      doesn't need the background anymore.
       */
      background:none;
      margin:0 0.5em;
    }

    #checkbox-row {
      margin-top:1em;
    }

    #icon {
      margin-right:0.5em;
    }

    #sitename {
      text-align:center;
      font-weight:bold;
      font-size:1.1em;
      background:white;
      border:solid #f1cc1d;
      border-width:1px 0;
      margin:1em 0.5em 0;
    }

    #sitename-inner {
      border:solid #f1cc1d;
      border-width:0 1px;
      margin:0 -1px;
      padding:0.5em;
    }

    #yellowbox {
      margin:1em 0;
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
  <div id="scroll">
    <div id="content">
      <table width="100%" cellpadding="0" cellspacing="0" border="0">
        <tr>
          <td align="left" valign="middle">
            <img id="icon" src="icon_32x32.png" width="32" height="32">
          </td>
          <td width="100%" align="left" valign="middle">The website below
          wants to use PRODUCT_FRIENDLY_NAME_UQ.</td>
        </tr>
      </table>

      <div id="yellowbox">
        <div id="yellowbox-inner">
          <p>This site will be able to store and access information on your
          computer.
          <!-- 
          [naming] - Not using short_name here because not sure if url always
          changes when name does.

          HACK: What I wanted here was target="_blank" to open this URL in a new
          window. However, when I did that Firefox opened the new window, but
          did not actually navigate it anywhere. This is probably due to
          modality issues (the code that would do the navigate is blocked behind
          this modal dialog). Using onclick fixes, for some reason.
          -->
          &nbsp;<a href="http://gears.google.com/?action=help"
            onclick="window.open(this.href); return false;">What is
            this?</a></p>

          <div id="sitename"><div id="sitename-inner"></div></div>
        </div>
      </div>
      <p id="checkbox-row">
        <table cellspacing="0" cellpadding="0" border="0">
          <tr>
            <td valign="middle">
              <input type="checkbox" id="unlock" accesskey="A">
            </td>
            <td valign="middle">
              <label for="unlock">
                &nbsp;I trust this site. <span class="accesskey">A</span>llow
                it to use PRODUCT_FRIENDLY_NAME_UQ.
              </label>
            </td>
          </tr>
        </table>
      </p>
    </div>
  </div>
  <div id="button-row">
    <table cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td width="100%" align="left" valign="middle">
          <a href="#" onclick="denyAccessPermanently()">Never allow this
          site</a>
        </td>
        <td nowrap="true" align="right" valign="middle">
          <!--
          Fancy buttons
          Note: Weird line breaks are on purpose to avoid extra space between
          buttons.
          Note: Outer element is <a> because we want it to focusable and behave
          like an anchor. Inner elements should theoretically be able to be
          <span>, but IE renders incorrectly in this case.
          -->
          <a href="#" accesskey="A" id="allow-button" onclick="allowAccess()"
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
          <a href="#" accesskey="C" id="deny-button" onclick="denyAccess()"
              class="inline-block custom-button">
            <div class="inline-block custom-button-outer-box">
              <div class="inline-block custom-button-inner-box"
                ><span class="accesskey">C</span>ancel</div></div></a>
        </td>
      </tr>
    </table>
  </div>
</body>
<script src="json_noeval.js"></script>
<script src="html_dialog.js"></script>
<script>
  initDialog();
  initWarning();

  var disabled = true;

  function initWarning() {
    // The arguments to this dialog are a single string. See CapabilitiesDB.
    document.getElementById("sitename-inner").appendChild(
      document.createTextNode(getArguments()));

    // Focus deny by default
    document.getElementById("deny-button").focus();

    // Set up the checkbox to toggle the enabledness of the Allow button.
    document.getElementById("unlock").onclick = updateAllowButtonEnabledState;
    updateAllowButtonEnabledState();
  }

  function updateAllowButtonEnabledState() {
    var button = document.getElementById("allow-button");
    var classes = button.className.split(" ");

    if (disabled) {
      classes.pop();
    } else {
      classes.push("custom-button-disabled");
    }

    button.className = classes.join(" ");
    disabled = !disabled;
  }

  // Note: The structure that the following functions pass are coupled to the
  // code in CapabilitiesDB that processes it.
  function allowAccess() {
    saveAndClose({
      allow: true,
      remember: true
    });
  }

  function denyAccess() {
    saveAndClose(null); // default behavior is to deny temporarily
  }

  function denyAccessPermanently() {
    saveAndClose({
      allow: false,
      remember: true
    });
  }
</script>
</html>
