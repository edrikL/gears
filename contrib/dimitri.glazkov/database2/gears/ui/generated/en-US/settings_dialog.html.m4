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
  <title>PRODUCT_FRIENDLY_NAME_UQ Settings</title>
  <link rel="stylesheet" href="button.css">
  <link rel="stylesheet" href="html_dialog.css">
  <style type="text/css">
    h1 {
      font-size:1.2em;
      margin:0;
    }

    h2 {
      font-size:1.1em;
      margin:0;
    }

    /* TODO(aa): Move into standard stylesheet if used in all dialogs? */
    #icon {
      margin-right:0.5em;
    }

    #content {
      overflow-x:hidden;
      overflow-y:auto;
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
      margin:0 4px;
~,
~
      margin:0 1em;
~)
    }

    #content table {
      border:1px #ccc;
      border-style:none solid;
      border-collapse:collapse;
      /* A bug in Mozilla with border-collapse:collapse along with the overflow
      settings we have on the scroll element causes the left border to be
      placed 1px to the left, making the element invisible. */
      margin-left:1px;
    }

    #content td {
      border:1px #ccc;
      border-style:solid none;
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
      padding:4px;
~,
~
      padding:0.5em; 
~)
    }

    #version {
      font-size:0.8em;
      font-style:italic;
    }

    .left {
      width:100%;
    }

    .app-icon {
      width: 30px;
    }

    .app-name {
      width: 150px;
    }

    .app-origin {
      width: 70px;
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
  <div id="strings" style="display:none;">
    <div id="string-cancel"><TRANS_BLOCK desc="Button user can press to cancel the dialog.">Cancel</TRANS_BLOCK></div>
    <div id="string-cancel-accesskey"><TRANS_BLOCK desc="Access key for cancel button.">C</TRANS_BLOCK></div>
    <div id="string-save"><TRANS_BLOCK desc="Button user can press to save changes.">Save</TRANS_BLOCK></div>
    <div id="string-save-accesskey"><TRANS_BLOCK desc="Access key for save button.">S</TRANS_BLOCK></div>
    <div id="string-remove"><TRANS_BLOCK desc="Button user can press to remove a site from the list.">Remove</TRANS_BLOCK></div>
    <div id="string-noallowed"><TRANS_BLOCK desc="States that there are no allowed sites.">No allowed sites.</TRANS_BLOCK></div>
    <div id="string-nodenied"><TRANS_BLOCK desc="States that there are no denied sites.">No denied sites.</TRANS_BLOCK></div>
  </div>
  
  <div id="head">
    <table width="100%" cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td align="left" valign="middle">
          <img id="icon" src="icon_32x32.png" width="32" height="32">
          <!-- Some browsers automatically focus the first focusable item. We
          don't want anything focused, so we add this fake item. -->
          <a href="#" id="focus-thief"></a>
        </td>
        <td width="100%" align="left" valign="middle">
          <h1>PRODUCT_FRIENDLY_NAME_UQ Settings</h1>
        </td>
      </tr>
    </table>
  </div>
  <div id="content">
    <h2>
      <TRANS_BLOCK desc="Header for allowed sites.">
      Allowed Sites
      </TRANS_BLOCK>
    </h2>
    <p>
      <TRANS_BLOCK desc="Description of allowed sites.">
      These sites are always allowed to access PRODUCT_FRIENDLY_NAME_UQ.
      </TRANS_BLOCK>
    </p>
    <div id='div-allowed'>
    </div>
    <br>
    <br>
    <h2>
      <TRANS_BLOCK desc="Header for denied sites.">
      Denied Sites
      </TRANS_BLOCK>
    </h2>
    <p>
      <TRANS_BLOCK desc="Description of denied sites.">
      These sites are never allowed to access PRODUCT_FRIENDLY_NAME_UQ.
      </TRANS_BLOCK>
    </p>
    <div id='div-denied'>
    </div>
    <br>
    <br>
    <div id="version">
      <TRANS_BLOCK desc="Version string.">
      PRODUCT_FRIENDLY_NAME_UQ version PRODUCT_VERSION
      </TRANS_BLOCK>
    </div>
  </div>
  <div id="foot">
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
    <!-- On SmartPhone, we don't use the regular buttons. We just use this link,
    and softkeys for the other buttons. -->
    <div id="button-row-smartphone">
    </div>
~,~~)
    <div id="button-row">
      <table width="100%" cellpadding="0" cellspacing="0" border="0">
        <tr>
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~
          <div id="div-buttons">
            <td width="50%" align="left" valign="middle">
              <input type="BUTTON" id="cancel-button"
               onclick="saveAndClose(null); return false;"></input>
            </td>
            <td width="50%" align="right" valign="middle">
              <input type="BUTTON" id="confirm-button"
               onclick="saveAndClose(g_dialogResult); return false;"></input>
            </td>
          </div>
~,m4_dnl
~
          <td nowrap="true" align="right" valign="middle">
            <button id="confirm-button" class="custom"
                onclick="saveAndClose(g_dialogResult); return false;"
              ></button
            ><button id="cancel-button" accesskey="C" class="custom"
                onclick="saveAndClose(null); return false;"
              ></button>
          </td>
~)
        </tr>
      </table>
    </div>
  </div>


</body>
m4_ifelse(PRODUCT_OS,~wince~,m4_dnl
~<object style="display:none;" classid="clsid:134AB400-1A81-4fc8-85DD-29CD51E9D6DE" id="pie_dialog">~m4_dnl
~</object>~)
<script>
m4_include(../third_party/jsonjs/json_noeval.js)
m4_include(ui/common/base.js)
m4_include(ui/common/dom.js)
m4_include(ui/common/html_dialog.js)
m4_include(ui/common/button.js)
</script>
<script>
  var g_dialogResult = {"removeSites": []};
  var allowedSites;
  var deniedSites;
  var ALLOWED = 1;
  var DENIED = 2;

  initDialog();

  setButtonLabel("string-cancel", "cancel-button", "string-cancel-accesskey");
  setButtonLabel("string-save", "confirm-button", "string-save-accesskey");

  if (!browser.ie_mobile) {
    CustomButton.initializeAll();
    initCustomLayout(layoutSettings);
  } else {
    var saveText = dom.getElementById("string-save");
    if (saveText) {
      window.pie_dialog.SetButton(saveText.innerText, 
        "saveAndClose(g_dialogResult);");
    }
    var cancelText = dom.getElementById("string-cancel");
    if (cancelText) {
      window.pie_dialog.SetCancelButton(cancelText.innerText);
    }
    window.pie_dialog.SetButtonEnabled(true);
  }

  initSettings();

  // Start out with only cancel enabled, just for clarity.
  disableButton(dom.getElementById("confirm-button"));

  function cancelButton() {
    saveAndClose(null);
  }

  function confirmButton() {
    saveAndClose(g_dialogResult);
  }

  function initSettings() {
    var args = getArguments();
    
    // Handy for debugging layout:
    // var args = {
    //   allowed: ["http://www.google.com", "http://aaronboodman.com"],
    //   denied: ["http://www.evil.org"]
    // };
    
    allowedSites = args.allowed;
    deniedSites = args.denied;
    initList("div-allowed", args.allowed, ALLOWED);
    initList("div-denied", args.denied, DENIED);
  }

  function initList(tableId, sites, kind) {
    var table = dom.getElementById(tableId);

    var content = "";
    if (!sites.length) {
      content = "<tr><td class=\"left\"><em>";
      if (kind == ALLOWED) {
        var allowedText = dom.getElementById("string-noallowed");
        if (allowedText) {
          if (isDefined(typeof allowedText.innerText)) {
            content += allowedText.innerText;
          } else {
            content += allowedText.textContent;
          }
        }
      } else if (kind == DENIED) {
        var deniedText = dom.getElementById("string-nodenied");
        if (deniedText) {
          if (isDefined(typeof deniedText.innerText)) {
            content += deniedText.innerText;
          } else {
            content += deniedText.textContent;
          }
        }
      }
      content += "</em></td><td></td></tr>";
    } else {
      for (var site, i = 0; site = sites[i]; i++) {
        var cont = initSite(table, site, i, kind);
        content += cont; 
      }
    } 
    table.innerHTML = "<table><tbody>" + content + "</tbody></table>";
  }

  function initSite(table, siteName, rowNumber, kind) {
    var content = "<tr><td class=\"left\">";
    content += wrapDomain(siteName);
    content += "</td>";
    content += "<td class=\"right\"><a href='#' onclick='handleRemoveClick(";
    content += rowNumber;
    content += ",\"" + siteName + "\"," + kind + ");'>";
    var removeText = dom.getElementById("string-remove");
    if (removeText) {
      if (isDefined(typeof removeText.innerText)) {
        content += removeText.innerText;
      } else {
        content += removeText.textContent;
      }
    }
    content += "</a></td></tr>";
    return content;
  }

  function handleRemoveClick(rowNumber, origin, kind) {
    removeOrigin(rowNumber, origin, kind);

    // Return false to prevent the default link action (scrolling up to top of
    // page in this case).
    return false;
  }

  function removeOrigin(row, origin, kind) {
    // Add to the list of things to be removed from database.
    g_dialogResult.removeSites.push(origin);

    if (kind == ALLOWED) {
      allowedSites = removeRow(row, allowedSites);
      initList("div-allowed", allowedSites, kind);
    } else if (kind == DENIED) {
      deniedSites = removeRow(row, deniedSites);
      initList("div-denied", deniedSites, kind);
    }

    enableButton(dom.getElementById("confirm-button"));
  }

  function layoutSettings(contentHeight) {
    var content = dom.getElementById("content");

    content.style.height = Math.max(contentHeight, 0) + "px";

    // If a scrollbar is going to be visible, then add some padding between it
    // and the inner right edge of the content. We don't want to have this in
    // all the time or else there will be double-padding when the scrollbar
    // isn't visible.
    if (content.scrollHeight > contentHeight) {
      content.style.paddingRight = "1em";
    } else {
      content.style.paddingRight = "";
    }
  }

  function removeRow(row, array) {
    var head = array.slice(0, row);
    var tail = array.slice(row + 1, array.length);
    return new Array().concat(head, tail);
  }

</script>
</html>
