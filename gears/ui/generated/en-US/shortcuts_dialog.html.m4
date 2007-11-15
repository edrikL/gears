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
  <title>PRODUCT_FRIENDLY_NAME_UQ - Create Desktop Shortcut</title>
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
      padding-top:0.75em;
    }

    #scroll tr.first td {
      padding-top:0;
    }

    #scroll img {
      margin-right:1em;
    }

    #scroll input {
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

    #measure-checkbox {
      /* So that we don't affect any of our other layout */
      position:absolute;
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
        <td width="100%" align="left" valign="middle">
          <span id="header-singular">This website wants to create a shortcut
          on your desktop. Do you want to allow this?</span>
          <span id="header-plural">This website wants
          to create the shortcuts listed below on your desktop. Do you want to
          allow this?</span>
        </td>
      </tr>
    </table>
  </div>

  <div id="content">
    <div id="highlightbox">
      <div id="highlightbox-inner">
        <div id="scroll">
          <table cellpadding="0" cellspacing="0" border="0">
            <tr id="template">
              <td valign="top">
                <input type="checkbox" checked="true"
                  onclick="resetDisabledState()">
              </td>
              <td valign="top"><img width="32" height="32"></td>
              <td align="left" width="100%" valign="top">
                <b><a target="_blank"></a></b><br>
                <span></span>
              </td>
            </tr>
          </table>
        </div>
      </div>
    </div>
  </div>

  <div id="foot">
    <!-- This checkbox is used to measure how tall checkboxes are on this
    platform. We need to know this so that we can correctly center them next to
    the shortcut icons. -->
    <input id="measure-checkbox" type="checkbox">

    <div id="button-row">
      <table width="100%" cellpadding="0" cellspacing="0" border="0">
        <tr>
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
                onclick="allowShortcuts(); return false;"
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
                onclick="denyShortcuts(); return false;"
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
  var scrollBordersHeight = -1;
  var checkboxHeight = -1;
  var iconHeight = 32;
  var iconWidth = 32;
  var isDisabled = false;

  initDialog();
  initCustomLayout(layoutShortcuts);
  initShortcuts();

  /**
   * Populate the shortcuts UI based on the data passed in from C++.
   */
  function initShortcuts() {
    var args = getArguments();
    var template = document.getElementById("template");
    var parent = template.parentNode;
    parent.removeChild(template);

    // Populate all the rows of the table.
    var showingMultiple = args.length > 1;
    for (var i = 0; i < args.length; i++) {
      var shortcutRow = createShortcutRow(args[i], showingMultiple, template);

      if (i == 0) {
        shortcutRow.className = "first";
      }

      parent.appendChild(shortcutRow);
    }

    // Show the right heading, depending on whether we are showing multiple
    // shortcuts.
    var headerSingular = document.getElementById("header-singular");
    var headerPlural = document.getElementById("header-plural");

    if (showingMultiple) {
      headerSingular.style.display = "none";
      headerPlural.style.display = "block";
      resetDisabledState();
    } else {
      headerSingular.style.display = "block";
      headerPlural.style.display = "none";
    }

    // Focus deny by default
    document.getElementById("deny-button").focus();
  }

  /**
   * Helper function. Creates a row for the shortcut list from the specified
   * data object.
   */
  function createShortcutRow(shortcutData, includeCheckbox, templateNode) {
    var shortcutRow = templateNode.cloneNode(true);

    // No need to have the new row have an ID.
    shortcutRow.removeAttribute("id");

    var checkbox = shortcutRow.getElementsByTagName("input")[0];
    var link = shortcutRow.getElementsByTagName("a")[0];
    var icon = shortcutRow.getElementsByTagName("img")[0];
    var span = shortcutRow.getElementsByTagName("span")[0];

    // If we are showing checkboxes, position the checkbox so that it is
    // vertically centered with the icon to it's right. Otherwise, hide it.
    if (includeCheckbox) {
      checkbox.style.marginTop = checkboxTopMargin + "px";
    } else {
      checkbox.parentNode.style.display = "none";
    }

    link.href = shortcutData.link;
    link.appendChild(document.createTextNode(shortcutData.name));
    icon.src = shortcutData.icon;
    icon.height = iconHeight;
    icon.width = iconWidth;
    span.appendChild(document.createTextNode(shortcutData.description));

    return shortcutRow;
  }

  /**
   * Resets the disabled state of the allow button depending on whether there is
   * at least one of the shortcuts is checked.
   */
  function resetDisabledState() {
    var checkboxen = document.getElementsByTagName("input");
    var allowButton = document.getElementById("allow-button");

    for (var i = 0, checkbox; checkbox = checkboxen[i]; i++) {
      if (checkbox.checked) {
        // Found at least one checked checkbox. Make sure allow button is
        // enabled.
        enableButton(allowButton);
        disabled = false;
        return;
      }
    }

    // No checkboxes were checked. Disable allow button.
    disableButton(allowButton);
    disabled = true;
  }

  /**
   * Custom layout for this dialog. Allow the scroll region and its borders to
   * grow until they fill all available height, but no more.
   */
  function layoutShortcuts(contentHeight) {
    var scroll = document.getElementById("scroll");

    // Initialize on first run
    if (scrollBordersHeight == -1) {
      var content = document.getElementById("content");
      scrollBordersHeight = content.offsetHeight;
      scroll.style.display = "block";

      var measureCheckbox = document.getElementById("measure-checkbox");
      var checkboxHeight = measureCheckbox.offsetHeight;
      checkboxTopMargin = Math.floor((iconHeight - checkboxHeight) / 2);
      measureCheckbox.parentNode.removeChild(measureCheckbox);
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
  function allowShortcuts() {
    if (!disabled) {
      var checkboxen = document.getElementsByTagName("input");
      var result = [];

      for (var i = 0, checbox; checkbox = checkboxen[i]; i++) {
        result.push(checkbox.checked);
      }

      saveAndClose(result);
    }
  }

  /**
   * Called when the user clicks the cancel button.
   */
  function denyShortcuts() {
    saveAndClose(null); // default behavior is to deny all
  }
</script>
</html>
