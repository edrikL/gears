m4_changequote(`~',`~')m4_dnl
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
  <style type="text/css">
    .text-alignment {
      /* This is modified via a script for rtl languages. */
      text-align:left;
    }

m4_include(ui/common/button.css)
m4_include(ui/common/html_dialog.css)
    #content {
      margin:0 1em;
    }

    #yellowbox-right {
      padding-left:1em;
      padding-right:1em;
    }

m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
    /* 
     * On Windows Mobile, we hide the div containing the buttons
     * by default, to only show the correct one (ie smartphone or not)
     */

    #button-row {
      display:none;
    }

    #button-row-smartphone {
      margin-left:2em;
      display:none;
    }
~,~~)

    #checkbox-row {
      margin-top:1em;
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
      margin-bottom:0.5em;
~,
~
      margin-bottom:1em;
~)
    }

    #icon {
      margin-right:0.5em;
    }

    #custom-icon {
      display:none;
      margin-left:1em;
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
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
      margin-top:2px; 
~,
~
      margin-top:6px;
~)
      display:none;
    }

    #origin-only {
      font-weight:bold;
      font-size:1.1em;
      display:none;
    }

    #yellowbox {
      margin:4px 0 4px 0;
      border:solid #f1cc1d;
      border-width:1px 0;
      background:#faefb8;
    }

    #yellowbox-inner {
      /* pseudo-rounded corners */
      margin:0 -1px;
      border:solid #f1cc1d;
      border-width:0 1px;
m4_ifelse(PRODUCT_OS,~wince~,~~,m4_dnl
~
      padding:1em 0;
~)
    }

    #yellowbox p {
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
      padding:0 0;
~,
~
      padding:0 1em;
~)
    }

    #permissions-help {
      margin:4px;
      display:none;
    }
  </style>
</head>
<body>
  <div id="strings" style="display:none">
    <div id="string-allow"><TRANS_BLOCK desc="Button user can press to allow the use of Gears.">Allow</TRANS_BLOCK></div>
    <div id="string-allow-accesskey"><TRANS_BLOCK desc="Access key for the allow button.">A</TRANS_BLOCK></div>
    <div id="string-deny"><TRANS_BLOCK desc="Button user can press to disallow the use of Gears.">Deny</TRANS_BLOCK></div>
    <div id="string-deny-accesskey"><TRANS_BLOCK desc="Access key for the deny button.">D</TRANS_BLOCK></div>
    <div id="string-cancel"><TRANS_BLOCK desc="Button user can press to cancel the dialog.">Cancel</TRANS_BLOCK></div>
    <div id="string-never-allow"><TRANS_BLOCK desc="Link that disallows Gears on this site.">Never allow it</TRANS_BLOCK></div>
  </div>

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
     <TRANS_BLOCK desc="Header for basic help section.">
     PRODUCT_FRIENDLY_NAME_UQ is an open source browser extension that enables
     web applications to provide offline functionality using the following
     JavaScript APIs:
     </TRANS_BLOCK>
   </p>
   <ul>
     <li><TRANS_BLOCK desc="Short explanation of localserver">Store and serve application resources locally</TRANS_BLOCK></li>
     <li><TRANS_BLOCK desc="Short explanation of database">Store data locally in a fully-searchable relational database</TRANS_BLOCK></li>
     <li><TRANS_BLOCK desc="Short explanation of workerpool">Run asynchronous JavaScript to improve application responsiveness</TRANS_BLOCK></li>
   </ul>
   <a href="#" onclick="showHelp(false); return false;">Go back</a>
  </div>

m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~ <div id="permissions-settings">~)
  <div id="head">
    <table width="100%" cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td class="text-alignment" valign="top">
          <img id="icon" src="icon_32x32.png" width="32" height="32">
          <!-- Some browsers automatically focus the first focusable item. We
          don't want anything focused, so we add this fake item. -->
          <a href="#" id="focus-thief"></a>
        </td>
        <td width="100%" class="text-alignment" valign="middle">
          <TRANS_BLOCK desc="Asks the user if they want to let the site use Gears">
          The website below wants to use PRODUCT_FRIENDLY_NAME_UQ. This site 
          will be able to store and access information on your computer.
          </TRANS_BLOCK>
        </td>
      </tr>
    </table>
  </div>

  <div id="content">
    <div id="yellowbox">
      <div id="yellowbox-inner">
        <table width="100%" cellpadding="0" cellspacing="0" border="0">
          <tr>
            <td id="yellowbox-left" class="text-alignment" valign="top">
              <img id="custom-icon" width="0" height="0">
            </td>
            <td id="yellowbox-right" width="100%" class="text-alignment" valign="middle">
              <div id="custom-name"></div>
              <div id="origin"></div>
              <div id="origin-only"></div>
              <div id="custom-message"></div>
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
          <TRANS_BLOCK desc="Indicates the user lets the site use Gears.">
              &nbsp;I <span class="accesskey">t</span>rust this site. Allow
              it to use PRODUCT_FRIENDLY_NAME_UQ.
          </TRANS_BLOCK>
            </label>
          </td>
        </tr>
      </table>
    </p>
    m4_ifelse(PRODUCT_OS,~wince~,~~,~<br>~)
  </div>

  <div id="foot">
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
    <!--
    On Windows Mobile, we use the softkey bar in addition to HTML buttons.
    On Windows Mobile smartphone, we only display this div and hide the
    button-row containing the HTML buttons (as screen estate is expensive
    and we already have the buttons through the softkey bar)
    -->

    <div id="button-row-smartphone">
      <table cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td class="text-alignment" valign="middle">
          <a href="#" onclick="denyAccessPermanently(); return false;">
            <TRANS_BLOCK desc="Link that disallows Gears on this site.">
              Never allow this site
            </TRANS_BLOCK>
          </a>
        </td>
      </tr>
      </table>
    </div>
~,~~)
    <div id="button-row">
      <table cellpadding="0" cellspacing="0" border="0">
        <tr>
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~         <!-- 
          We use form input buttons instead of buttons elements as PIE
          does not support them.
          -->

          <td width="50%" class="text-alignment" valign="middle">
            <input type="BUTTON" id="never-allow-button" onclick="denyAccessPermanently(); return false;"></input>
          </td>

          <div id="div-buttons">
            <td width="50%" align="right" valign="middle">
              <input disabled type="BUTTON" id="allow-button" onclick="allowAccessPermanently();"></input>
              <input type="BUTTON" id="deny-button" onclick="denyAccessTemporarily(); return false;"></input>
            </td>
          </div>
~,~           
          <td width="100%" class="text-alignment" valign="middle">
            <a href="#" onclick="denyAccessPermanently(); return false;">
            <TRANS_BLOCK desc="Link that disallows Gears on this site.">
              Never allow this site
            </TRANS_BLOCK>
            </a>
          </td>
          <td nowrap="true" align="right" valign="middle">
            <button id="allow-button" class="custom"
              onclick="allowAccessPermanently(); return false;"></button
            ><button id="deny-button" class="custom"
              onclick="denyAccessTemporarily(); return false;"></button>
          </td>~)
        </tr>
      </table>
    </div>
  </div>
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~ </div>~)
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~<object style="display:none;" classid="clsid:134AB400-1A81-4fc8-85DD-29CD51E9D6DE" id="pie_dialog">
</object>~)
</body>
<!--
We include all files through m4 as the HTML dialog implementation on
PocketIE does not support callbacks for loading external JavaScript files.
TODO: find a better way to include scripts for PIE
-->
<script>
m4_include(../third_party/jsonjs/json_noeval.js)
m4_include(ui/common/base.js)
m4_include(ui/common/dom.js)
m4_include(ui/common/html_dialog.js)
m4_include(ui/common/button.js)
</script>

<script>
  var debug = false;
  initDialog();

  setButtonLabel("string-allow", "allow-button", "string-allow-accesskey");
  setButtonLabel("string-deny", "deny-button", "string-deny-accesskey");

  if (!browser.ie_mobile) {
    CustomButton.initializeAll();
  }

  if (browser.ie_mobile) {
    setButtonLabel("string-never-allow", "never-allow-button");
    var allowText = dom.getElementById("string-allow");
    if (allowText) {
      window.pie_dialog.SetButton(allowText.innerText, "allowAccessPermanently();");
    }
    var cancelText = dom.getElementById("string-cancel");
    if (cancelText) {
      window.pie_dialog.SetCancelButton(cancelText.innerText);
    }
    updateAllowButtonEnabledState();
  }
  initWarning();

  var allowButtonUnlocked = false;

  function setTextContent(elem, content) {
    if (isDefined(typeof document.createTextNode)) {
      elem.appendChild(document.createTextNode(content)); 
    } else {
      elem.innerText = content;
    }
  }

  function showHelp(show) {
    if (browser.ie_mobile) {
      var elemSettings = dom.getElementById("permissions-settings"); 
      var elemHelp = dom.getElementById("permissions-help"); 
      if (show) {
        elemSettings.style.display = 'none';
        elemHelp.style.display = 'block';
        window.pie_dialog.SetButton("Back", "showHelp(false);");
        window.pie_dialog.SetButtonEnabled(true);
      } else {
        elemSettings.style.display = 'block';
        elemHelp.style.display = 'none';
        window.pie_dialog.SetButton("Allow", "allowAccessPermanently();");
        updateAllowButtonEnabledState();
      }
      window.pie_dialog.ResizeDialog();
    } else {
      window.open('http://gears.google.com/?action=help');
    }
  }

  function initWarning() {
    var args;
    if (debug) {
      // Handy for debugging layout:
      args = {
        origin: "http://www.google.com",
        customIcon: "http://google-gears.googlecode.com/svn/trunk/gears/test/manual/shortcuts/32.png",
        customName: "My Application",
        customMessage: "Press the button to enable my application to run offline!"
      };
    } else {
      // The arguments to this dialog are a single string, see PermissionsDialog
      args = getArguments();
    }

    var origin = args['origin'];  // required parameter
    var customIcon = args['customIcon'];
    var customName = args['customName'];
    var customMessage = args['customMessage'];

    var elem;

    if (!customName) {
      elem = dom.getElementById("origin-only");
      elem.style.display = "block";
      if (browser.ie_mobile) {
        elem.innerHTML = wrapDomain(origin);
      } else {
        setTextContent(elem, origin);
      }

      // When all we have is the origin, we lay it out centered because that
      // looks nicer. This is also what the original dialog did, which did not
      // support the extra name, icon, or message.
      if (!browser.ie_mobile && !customIcon && !customMessage) {
        elem.setAttribute("align", "center");
      }
    } else {
      elem = dom.getElementById("origin");
      elem.style.display = "block";
      setTextContent(elem, origin);
    }

    if (customIcon) {
      elem = dom.getElementById("custom-icon");
      elem.style.display = "inline";
      elem.src = customIcon;
      elem.height = 32;
      elem.width = 32;
    }

    if (customName) {
      elem = dom.getElementById("custom-name");
      elem.style.display = "block";
      setTextContent(elem, customName);
    }

    if (customMessage) {
      elem = dom.getElementById("custom-message");
      elem.style.display = "block";
      setTextContent(elem, customMessage);
    }

    // This does not work on PIE...
    if (!browser.ie_mobile) {
      // Set up the checkbox to toggle the enabledness of the Allow button.
      dom.getElementById("unlock").onclick = updateAllowButtonEnabledState;
    }
    updateAllowButtonEnabledState();
    resizeDialogToFitContent();
  }


  function updateAllowButtonEnabledState() {
    var allowButton = dom.getElementById("allow-button");
    var unlockCheckbox = dom.getElementById("unlock");

    allowButtonUnlocked = unlockCheckbox.checked;

    if (allowButtonUnlocked) {
      enableButton(allowButton);
    } else {
      disableButton(allowButton);
    }
  }

  // Note: The structure that the following functions pass is coupled to the
  // code in PermissionsDialog that processes it.
  function allowAccessPermanently() {
    if (allowButtonUnlocked) {
      saveAndClose({"allow": true, "permanently": true});
    }
  }

  function denyAccessTemporarily() {
    saveAndClose({"allow": false, "permanently": false});
  }

  function denyAccessPermanently() {
    saveAndClose({"allow": false, "permanently": true });
  }
</script>
</html>
