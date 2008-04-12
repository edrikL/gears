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
  <title>PRODUCT_FRIENDLY_NAME_UQ - loo töölaua otsetee</title>
  <link rel="stylesheet" href="button.css">
  <link rel="stylesheet" href="html_dialog.css">
  <style type="text/css">
    #icon {
      margin-right:0.5em;
    }

    #content {
      margin:0 1em;
    }

    #scroll {
      overflow-y:auto;
      overflow-x:hidden;
      /* initially display:none so that we can figure out the height of the
      borders and margins #content adds to the outside of scroll. Once we get
      this, we remove display:none in layoutShortcuts(). */
      display:none;
    }

    #scroll td {
      padding-top:0;
    }

    #scroll img {
      margin-right:1em;
    }

    #highlightbox {
      /* for pseudo-rounded corners, also see highlightbox-inner */
      margin:0 1px;
      border:solid #e8e8e8;
      border-width:1px 0;
      background:#f8f8f8;
    }

    #highlightbox-inner {
      margin:0 -1px;
      border:solid #e8e8e8;
      border-width:0 1px;
      padding:1em;
    }
    
    #shortcut-name {
      margin-bottom:2px;
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
      display:none;
    }
~,~~)
  </style>
</head>
<body>
  <div id="head">
    <table width="100%" cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td align="left" valign="top">
          <img id="icon" src="icon_32x32.png" width="32" height="32">
        </td>
        <td width="100%" align="left" valign="middle">
          <span id="header-desktop" style="display:none">
          Veebisait soovib teie töölauale otseteed luua. Kas lubate seda?
          </span>
          <span id="header-wince" style="display:none">
          Veebisait soovib teie programmiloendisse luua otsetee. Kas lubate seda?
          </span>
        </td>
      </tr>
    </table>
  </div>

  <div id="content">
    <div id="highlightbox">
      <div id="highlightbox-inner">
        <div id="scroll">
        </div>
      </div>
    </div>
  </div>

  <div id="foot">
    <!-- We use these divs to store the text for our buttons in a way that can
    be translated. We copy the text to the buttons in JavaScript. -->
    <div style="display:none">
      <div id="allow-text">
        <span class="accesskey">J</span>ah
      </div>
      <div id="deny-text">
        <span class="accesskey">E</span>i
      </div>
      <div id="deny-permanently-text">
        Ära luba seda otseteed kunagi
      </div>
      <div id="deny-permanently-text-short">
        Ära luba kunagi
      </div>
    </div>

m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
    <!-- On SmartPhone, we don't use the regular buttons. We just use this link,
    and softkeys for the other buttons. -->
    <div id="button-row-smartphone">
      <table cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td align="left" valign="middle">
          <a href="#" id="deny-permanently-link" onclick="denyShortcutPermanently(); return false;"></a>
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
            <input type="BUTTON" id="deny-permanently-button" onclick="denyShortcutPermanently(); return false;"></input>
          </td>

          <div id="div-buttons">
            <td width="50%" align="right" valign="middle">
              <input disabled type="BUTTON" id="allow-button" onclick="allowShortcutsTemporarily(); return false;"></input>
              <input type="BUTTON" id="deny-button" onclick="denyShortcutsTemporarily(); return false;"></input>
            </td>
          </div>
~,~
          <td width="100%" align="left" valign="middle">
            <a href="#" onclick="denyShortcutPermanently(); return false;" id="deny-permanently-link"></a>
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
            <a href="#" accesskey="Y" id="allow-button" 
                onclick="allowShortcutsTemporarily(); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box" id="allow-button-contents"
                  >&nbsp;&nbsp;<span class="accesskey">J</span>ah&nbsp;&nbsp;</div></div></a>
            <a href="#" accesskey="N" id="deny-button"
                onclick="denyShortcutsTemporarily(); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box" id="deny-button-contents"
                  >&nbsp;&nbsp;&nbsp;<span class="accesskey">E</span>i&nbsp;&nbsp;&nbsp;</div></div></a></td>~)
        </tr>
      </table>
    </div>
  </div>
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
m4_include(third_party/jsonjs/json_noeval.js)
m4_include(ui/common/html_dialog.js)
</script>

<script>
  var scrollBordersHeight = -1;
  var iconHeight = 32;
  var iconWidth = 32;

  initDialog();

  // Set the button and link labels.
  if (isPIE) {
    // For touchscreen devices (window.pie_dialog.IsSmartPhone() == false)
    setButtonLabel("allow-text", "allow-button");
    setButtonLabel("deny-text", "deny-button");
    setButtonLabel("deny-permanently-text-short", "deny-permanently-button");
    // For softkey UI devices (window.pie_dialog.IsSmartPhone() == true)
    window.pie_dialog.SetButton(getElementById("allow-text").innerText,
                                "allowShortcutsTemporarily();");
    window.pie_dialog.SetCancelButton(getElementById("deny-text").innerText);
    setElementContents("deny-permanently-text-short", "deny-permanently-link");
  } else {
    setElementContents("deny-permanently-text", "deny-permanently-link");
  }

  // PIE can't do scrollable divs, so there's no need to do the height
  // calculations.
  if (isPIE) {
    getElementById("scroll").style.display = "block";
  } else {
    initCustomLayout(layoutShortcuts);
  }
  initShortcuts();

  /**
   * Populate the shortcuts UI based on the data passed in from C++.
   */
  function initShortcuts() {
    var args = getArguments();

    // Handy for debugging layout:
    // var args = {
    //   name: "My Application",
    //   link: "http://www.google.com/",
    //   description: "This application does things does things!",
    //   // description: "This application does things does things that do things that do things that do things that do things that do things that do things that do things that do things that do things that do things that do things that do things that do things that do things that do things that do things that do things that do things.",
    //   icon16x16: "http://google-gears.googlecode.com/svn/trunk/gears/test/manual/shortcuts/16.png",
    //   icon32x32: "http://google-gears.googlecode.com/svn/trunk/gears/test/manual/shortcuts/32.png",
    //   icon48x48: "http://google-gears.googlecode.com/svn/trunk/gears/test/manual/shortcuts/48.png",
    //   icon128x128: "http://google-gears.googlecode.com/svn/trunk/gears/test/manual/shortcuts/128.png"
    // };

    // Populate the layout
    var content = createShortcutRow(args);
    getElementById("scroll").innerHTML =
        "<table cellpadding='0' cellspacing='0' border='0'><tbody>" +
        content +
        "</tbody></table>";

    preloadIcons(args);

    if (isPIE) {
      getElementById("header-wince").style.display = "block";
      // On PIE, the allow button is disabled by default.
      enableButton(getElementById("allow-button"));
    } else {
      getElementById("header-desktop").style.display = "block";
    }

    // Focus deny by default
    getElementById("deny-button").focus();
  }

  /**
   * Helper function. Creates a row for the shortcut list from the specified
   * data object.
   */
  function createShortcutRow(shortcutData) {
    var content = "<tr>";
    content += "<td valign='top'><img width='" + iconWidth + "px' " +
               "height='" + iconHeight + "px' " +
               "src='" + pickIconToRender(shortcutData) + "'>";
    content += "</td>";
    content += "<td align='left' width='100%' valign='middle'>";
    content += "<div id='shortcut-name'><b>";
    content += shortcutData.name;
    content += "</b></div>";
    if (isDefined(typeof shortcutData.description)) {
      content += "<div>" + shortcutData.description + "</div>";
    }
    content += "</td>";
    content += "</tr>";
    return content;
  }

  /**
   * Picks the best icon to render in the UI.
   */
  function pickIconToRender(shortcutData) {
    if (shortcutData.icon32x32) { // ideal size
      return shortcutData.icon32x32;
    } else if (shortcutData.icon48x48) { // better to pick one that is too big
      return shortcutData.icon48x48;
    } else if (shortcutData.icon128x128) {
      return shortcutData.icon128x128;
    } else {
      return shortcutData.icon16x16; // pick too small icon last
    }
  }

  /**
   * Preloads the icons for a shortcut so that later when they are requested
   * from C++, they will be fast.
   */
  function preloadIcons(shortcutData) {
    for (var prop in shortcutData) {
      if (prop.indexOf("icon") == 0) {
        var img = new Image();
        img.src = shortcutData[prop];
      }
    }
  }

  /**
   * Custom layout for this dialog. Allow the scroll region and its borders to
   * grow until they fill all available height, but no more.
   */
  function layoutShortcuts(contentHeight) {
    var scroll = getElementById("scroll");

    // Initialize on first run
    if (scrollBordersHeight == -1) {
      var content = getElementById("content");
      scrollBordersHeight = content.offsetHeight;
      scroll.style.display = "block";
    }

    var scrollHeight = contentHeight - scrollBordersHeight;
    scrollHeight = Math.min(scrollHeight, scroll.scrollHeight);
    scrollHeight = Math.max(scrollHeight, 0);

    scroll.style.height = scrollHeight + "px";

    // If there is a scrollbar visible add extra padding to the left of it.
    if (scrollHeight < scroll.scrollHeight) {
      scroll.style.paddingRight = "1em";
    } else {
      scroll.style.paddingHeight = "";
    }
  }

  /**
   * Called when the user clicks the allow button.
   */
  function allowShortcutsTemporarily() {
    saveAndClose({
      allow: true
    });
  }

  /**
   * Called when the user clicks the no button.
   */
  function denyShortcutsTemporarily() {
    saveAndClose(null); // default behavior is to deny all
  }
  
  /**
   * Called when the user clicks the "Never allow this shortcut" link.
   */
  function denyShortcutPermanently() {
    saveAndClose({
      allow: false
    });
  }

  /**
   * Set the contents of the element specified by destID from from that of the
   * element specified by sourceID.
   */
  function setElementContents(sourceID, destID) {
    var sourceElem = getElementById(sourceID);
    var destElem = getElementById(destID);
    if (isDefined(typeof sourceElem) && isDefined(typeof destElem)) {
      destElem.innerHTML = sourceElem.innerHTML;
    }
  }
</script>
</html>
