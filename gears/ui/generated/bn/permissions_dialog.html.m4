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
  <title>PRODUCT_FRIENDLY_NAME_UQ সুরক্ষা সতর্কতা</title>
  <link rel="stylesheet" href="button.css">
  <link rel="stylesheet" href="html_dialog.css">
  <style type="text/css">
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
     PRODUCT_FRIENDLY_NAME_UQ হ'ল ব্রাউজার এক্সটেনশনের এক মুক্ত উত্স যা ওয়েব প্রয়োগগুলিকে নিম্নলিখিত জাভাস্ক্রিপ্ট API গুলি ব্যবহার করে অফলাইন কার্যকারিতা সরবরাহ করতে সক্ষম করে:
   </p>
   <ul>
     <li>প্রয়োগ সম্পদ স্থানীয়ভানে সঞ্চয় এবং পরিবেশন করুন</li>
     <li>একটি পূর্ণ-সন্ধানযোগ্য সম্পর্কযুক্ত ডেটাবেসে স্থানীয়ভাবে ডেটা সঞ্চয় করুন</li>
     <li>প্রয়োগ প্রতিক্রিয়া উন্নত করতে অ্যাসিঙ্ক্রোনাস জাভাস্ক্রিপ্ট রান করুন</li>
   </ul>
   <a href="#" onclick="showHelp(false); return false;">Go back</a>
  </div>

m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~ <div id="permissions-settings">~)
  <div id="head">
    <table width="100%" cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td align="left" valign="top">
          <img id="icon" src="icon_32x32.png" width="32" height="32">
        </td>
        <td width="100%" align="left" valign="middle">
          নীচের ওয়েবসাইটটি PRODUCT_FRIENDLY_NAME_UQ ব্যবহার করতে চায়৷ এই সাইটটি আপনার কম্পিউটারে তথ্য সঞ্চয় এবং অ্যাক্সেস করতে সক্ষম হবে৷
          <a href="#" onclick="showHelp(true); return false;">
          এটি&nbsp;কী&nbsp;?
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
            <td id="yellowbox-left" align="left" valign="top">
              <img id="custom-icon" width="0" height="0">
            </td>
            <td id="yellowbox-right" width="100%" align="left" valign="middle">
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
          &nbsp;আমি <span class="accesskey" এই সাইটটি >বি</span>শ্বাস করি৷ এটিকে PRODUCT_FRIENDLY_NAME_UQ ব্যবহারের অনুমতি দিন৷
            </label>
          </td>
        </tr>
      </table>
    </p>
    m4_ifelse(PRODUCT_OS,~wince~,~~,~<br>~)
  </div>

  <div id="foot">
    <div id="text-buttons" style="display:none">
      <div id="text-never-allow">
        এটিকে কখনই অনুমতি দেয় না
      </div>
      <div id="text-allow">
        মঞ্জুরি দিন
      </div>
      <div id="text-deny">
        অস্বীকার
      </div>
      <div id="text-cancel">
        বাতিল
      </div>
    </div>
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
        <td align="left" valign="middle">
          <a href="#" onclick="denyAccessPermanently(); return false;">
            কখনই এই সাইটটির মঞ্জুরি দেবেন না
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

          <td width="50%" align="left" valign="middle">
            <input type="BUTTON" id="never-allow-button" onclick="denyAccessPermanently(); return false;"></input>
          </td>

          <div id="div-buttons">
            <td width="50%" align="right" valign="middle">
              <input disabled type="BUTTON" id="allow-button" onclick="allowAccessPermanently();"></input>
              <input type="BUTTON" id="deny-button" onclick="denyAccessTemporarily(); return false;"></input>
            </td>
          </div>
~,~           
          <td width="100%" align="left" valign="middle">
            <a href="#" onclick="denyAccessPermanently(); return false;">
            কখনই এই সাইটটির মঞ্জুরি দেবেন না
            </a>
          </td>
          <td nowrap="true" align="right" valign="middle">
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
                onclick="allowAccessPermanently(); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box"
                  ><span class="accesskey">ম</span>ঞ্জুরি</div></div></a>
            <a href="#" accesskey="C" id="deny-button"
                onclick="denyAccessTemporarily(); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box"
                  ><span class="accesskey">বা</span>তিল</div></div></a></td>~)
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
m4_include(ui/common/html_dialog.js)
</script>

<script>
  initDialog();
  if (isPIE) {
    setButtonLabel("text-never-allow", "never-allow-button");
    setButtonLabel("text-allow", "allow-button");
    setButtonLabel("text-deny", "deny-button");
    var allowText = getElementById("text-allow");
    if (allowText) {
      window.pie_dialog.SetButton(allowText.innerText, "allowAccessPermanently();");
    }
    var cancelText = getElementById("text-cancel");
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
    if (isPIE) {
      var elemSettings = getElementById("permissions-settings"); 
      var elemHelp = getElementById("permissions-help"); 
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
    // The arguments to this dialog are a single string, see PermissionsDialog
    var args = getArguments();

    var origin = args['origin'];  // required parameter
    var customIcon = args['customIcon'];
    var customName = args['customName'];
    var customMessage = args['customMessage'];

    var elem;

    if (!customName) {
      elem = getElementById("origin-only");
      elem.style.display = "block";
      if (isPIE) {
        elem.innerHTML = wrapDomain(origin);
      } else {
        setTextContent(elem, origin);
      }

      // When all we have is the origin, we lay it out centered because that
      // looks nicer. This is also what the original dialog did, which did not
      // support the extra name, icon, or message.
      if (!isPIE && !customIcon && !customMessage) {
        elem.setAttribute("align", "center");
      }
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
    } else { 
      window.pie_dialog.ResizeDialog();
    } 
  }


  function updateAllowButtonEnabledState() {
    var allowButton = getElementById("allow-button");
    var unlockCheckbox = getElementById("unlock");

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
