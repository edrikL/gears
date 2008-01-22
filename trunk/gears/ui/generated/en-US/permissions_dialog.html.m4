m4_changequote(`^',`^')m4_dnl
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

    #permissions-help {
      margin:4px;
      display:none;
    }
  </style>
</head>
<body>

  <!--
   PIE only works with one window, and we are in a modal dialog.
   Using window.open(this.href) replaces the content of the current dialog,
   which is annoying when no back button is displayed...
   The workaround is to embed directly a short explanation text, and
   hide/show the div container for the help and the settings dialog.
  -->

  <div id="permissions-help">
   <h2>Information</h2>
   <p>
     PRODUCT_FRIENDLY_NAME_UQ is an open source browser extension that enables
     web applications to provide offline functionality using the following
     JavaScript APIs:
   </p>
   <ul>
     <li>Store and serve application resources locally</li>
     <li>Store data locally in a fully-searchable relational database</li>
     <li>Run asynchronous Javascript to improve application responsiveness</li>
   </ul>
   <a href="#" onclick="showHelp(false); return false;">Go back</a>
  </div>

m4_ifelse(PRODUCT_OS,^wince^,m4_dnl
^ <div id="permissions-settings">^)
  <div id="head">
    <table width="100%" cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td align="left" valign="top">
          <img id="icon" src="icon_32x32.png" width="32" height="32">
        </td>
        <td width="100%" align="left" valign="middle">
          <?cs #TC_MSG_BREAK desc: Asks the user if they want to let the site use gears ?>
          The website below wants to use PRODUCT_FRIENDLY_NAME_UQ. This site 
          will be able to store and access information on your computer.&nbsp;
          <?cs #TC_MSG_BREAK ?>
          <a href="#" onclick="showHelp(true); return false;">
          <?cs #TC_MSG_BREAK desc: Help link displayed in the installation dialog. ?>
          What is this?
          <?cs #TC_MSG_BREAK ?>
          </a>
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
            <input type="checkbox" id="unlock" accesskey="T"
                   onclick="updateAllowButtonEnabledState()">
          </td>
          <td valign="middle">
            <label for="unlock">
          <?cs #TC_MSG_BREAK desc: Indicates the user lets the site use Gears ?>
              &nbsp;I <span class="accesskey">t</span>rust this site. Allow
              it to use PRODUCT_FRIENDLY_NAME_UQ.
          <?cs #TC_MSG_BREAK ?>
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
            <a href="#" onclick="denyAccessPermanently(); return false;">
            <?cs #TC_MSG_BREAK desc: Link that disallows Gears on this site. ?>
              Never allow this site
            <?cs #TC_MSG_BREAK ?>
            </a>
          </td>
m4_ifelse(PRODUCT_OS,^wince^,m4_dnl
^          <!-- 
           We use simple links instead of buttons as PIE does not support them
           -->

           <td width="100%" align="right" valign="middle">
            <a href="#" accesskey="A" id="allow-button"
              onclick="allowAccess(); return true;">
              <?cs #TC_MSG_BREAK desc: Button user can press to allow the use of gears ?><span class="accesskey">A</span>llow<?cs #TC_MSG_BREAK ?></a>
            <a href="#" accesskey="C" id="deny-button"
              onclick="denyAccess(); return false;">
              <?cs #TC_MSG_BREAK desc: Button user can press to disallow the use of gears ?><span class="accesskey">C</span>ancel<?cs #TC_MSG_BREAK ?></a>
          </tr>^,m4_dnl
^          <td nowrap="true" align="right" valign="middle">
            <!--
            Fancy buttons
            Note: Weird line breaks are on purpose to avoid extra space between
            buttons.
            Note: Outer element is <a> because we want it to focusable and
            behave like an anchor. Inner elements should theoretically be able
            to be <span>, but IE renders incorrectly in this case.
             
            Note: The whitespace in this section is very delicate.  The lack of
            space between the tags and the space between the buttons both
            are important to ensure proper rendering.
            TODO(aa): This results in inconsistent spacing in IE vs Firefox
            between the buttons, but I am reluctant to hack things even further
            to fix that.
            -->
            <a href="#" accesskey="A" id="allow-button" 
                onclick="allowAccess(); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box"
                  ><?cs #TC_MSG_BREAK desc: Button user can press to allow the use of gears ?><span class="accesskey">A</span>llow<?cs #TC_MSG_BREAK ?></div></div></a>
            <a href="#" accesskey="C" id="deny-button"
                onclick="denyAccess(); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box"
                  ><?cs #TC_MSG_BREAK desc: Button user can press to disallow the use of gears ?><span class="accesskey">C</span>ancel<?cs #TC_MSG_BREAK ?></div></div></a>^)
          </td>
        </tr>
      </table>
    </div>
  </div>
m4_ifelse(PRODUCT_OS,^wince^,m4_dnl
^ </div>^)
m4_ifelse(PRODUCT_OS,^wince^,m4_dnl
^<object classid="clsid:134AB400-1A81-4fc8-85DD-29CD51E9D6DE" id="pie_dialog">
</object>^)
</body>
<!--
 We include all files through m4 as the HTML dialog implementation
 on PocketIE does not support callbacks for loading external javascript files
 TODO: find a better way to include scripts for PIE
-->
<script>
m4_include(third_party/jsonjs/json_noeval.js)
m4_include(ui/common/html_dialog.js)
</script>

<script>
  initDialog();
  initWarning();

  var disabled = true;

  function setTextContent(elem, content) {
    if (document.createTextNode) {
      elem.appendChild(document.createTextNode(content)); 
    } else {
      elem.innerText = content;
    }
  }

  function showHelp(show) {
    if (isPIE) {
      var elemSettings = getElementById("permissions-settings"); 
      var elemHelp = getElementById("permissions-help"); 
      if (show) {
        elemSettings.style.display = 'none';
        elemHelp.style.display = 'block';
      } else {
        elemSettings.style.display = 'block';
        elemHelp.style.display = 'none';
      }
    } else {
      window.open('http://gears.google.com/?action=help');
    }
  }

  function initWarning() {
    // The arguments to this dialog are a single string, see PermissionsDialog
    var args = getArguments();

    var origin = args['origin'];  // required parameter
    var customIcon = args['customIcon'];
    var customName = args['customName'];
    var customMessage = args['customMessage'];

    var elem;

    if (!customIcon && !customName && !customMessage) {
       elem = getElementById("origin-only");
       elem.style.display = "block";
       setTextContent(elem, origin);
    } else {
       elem = getElementById("origin");
       elem.style.display = "block";
       setTextContent(elem, origin);
    }

    if (customIcon) {
      elem = getElementById("custom-icon");
      elem.style.display = "inline";
      elem.src = customIcon;
      elem.height = 32;
      elem.width = 32;
    }

    if (customName) {
      elem = getElementById("custom-name");
      elem.style.display = "block";
      setTextContent(elem, customName);
    }

    if (customMessage) {
      elem = getElementById("custom-message");
      elem.style.display = "block";
      setTextContent(elem, customMessage);
    }

    // Focus deny by default
    getElementById("deny-button").focus();

    // This does not work on PIE...
    if (!isPIE) {
      // Set up the checkbox to toggle the enabledness of the Allow button.
      getElementById("unlock").onclick = updateAllowButtonEnabledState;
    }
    updateAllowButtonEnabledState();

    // This does not work on PIE (no height measurement)
    if (!isPIE) {
      // Resize the window to fit
      var contentDiv = getElementById("content");
      var contentHeightProvided = getContentHeight();
      var contentHeightDesired = contentDiv.offsetHeight;
      if (contentHeightDesired != contentHeightProvided) {
        var dy = contentHeightDesired - contentHeightProvided;
        window.resizeBy(0, dy);
      }
    }  
  }


  function updateAllowButtonEnabledState() {
    var allowButton = getElementById("allow-button");
    var checkbox = getElementById("unlock");

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
