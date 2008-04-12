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
  <title>Configuración de PRODUCT_FRIENDLY_NAME_UQ</title>
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
  <div id="head">
    <table width="100%" cellpadding="0" cellspacing="0" border="0">
      <tr>
        <td align="left" valign="middle">
          <img id="icon" src="icon_32x32.png" width="32" height="32">
        </td>
        <td width="100%" align="left" valign="middle">
          <h1>PRODUCT_FRIENDLY_NAME_UQ Settings</h1>
        </td>
      </tr>
    </table>
  </div>
  <div id="content">
    <h2>
      Sitios permitidos
    </h2>
    <p>
      Estos sitios siempre pueden acceder a PRODUCT_FRIENDLY_NAME_UQ.
    </p>
    <div id='div-allowed'>
    </div>
    <br>
    <br>
    <h2>
      Sitios denegados
    </h2>
    <p>
      Estos sitios no pueden acceder nunca a PRODUCT_FRIENDLY_NAME_UQ.
    </p>
    <div id='div-denied'>
    </div>
    <br>
    <br>
    <div id="version">
      PRODUCT_FRIENDLY_NAME_UQ versión PRODUCT_VERSION
    </div>
  </div>
  <div id="foot">
    <div id="text-buttons" style="display:none">
      <div id="text-cancel">
        Cancelar
      </div>
      <div id="text-save">
        Guardar
      </div>
      <div id="text-remove">
        Quitar
      </div>
      <div id="text-noallowed">
        No hay sitios denegados.
      </div>
      <div id="text-nodenied">
        No hay sitios denegados.
      </div>
    </div>
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
            <!--
            Fancy buttons
            Note: Weird line breaks are on purpose to avoid extra space between
            buttons.
            Note: Outer element is <a> because we want it to focusable and
            behave like an anchor. Inner elements should theoretically be able
            to be <span>, but IE renders incorrectly in this case.
            -->
            <a href="#" accesskey="S" id="confirm-button" 
                onclick="saveAndClose(g_dialogResult); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box"
                  ><span class="accesskey">G</span>uardar</div></div></a>
            <!--
            Note: There must be whitespace here or Firefox messes up the
            rendering.
            TODO(aa): This results in inconsistent spacing in IE vs Firefox
            between the buttons, but I am reluctant to hack things even further
            to fix that.
            -->
            <a href="#" accesskey="C" id="cancel-button"
                onclick="saveAndClose(null); return false;"
                class="inline-block custom-button">
              <div class="inline-block custom-button-outer-box">
                <div class="inline-block custom-button-inner-box"
                  ><span class="accesskey">C</span>ancelar</div></div></a>
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
m4_include(ui/common/html_dialog.js)
</script>
<script>
  var g_dialogResult = {"removeSites": []};
  var allowedSites;
  var deniedSites;
  var ALLOWED = 1;
  var DENIED = 2;

  initDialog();
  if (!isPIE) {
    initCustomLayout(layoutSettings);
  } else {
    setButtonLabel("text-cancel", "cancel-button");
    setButtonLabel("text-save", "confirm-button");
    var saveText = getElementById("text-save");
    if (saveText) {
      window.pie_dialog.SetButton(saveText.innerText, 
        "saveAndClose(g_dialogResult);");
    }
    var cancelText = getElementById("text-cancel");
    if (cancelText) {
      window.pie_dialog.SetCancelButton(cancelText.innerText);
    }
    window.pie_dialog.SetButtonEnabled(true);
  }
  initSettings();

  function cancelButton() {
    saveAndClose(null);
  }

  function confirmButton() {
    saveAndClose(g_dialogResult);
  }

  function initSettings() {
    var args = getArguments();
    allowedSites = args.allowed;
    deniedSites = args.denied;
    initList("div-allowed", args.allowed, ALLOWED);
    initList("div-denied", args.denied, DENIED);
  }

  function initList(tableId, sites, kind) {
    var table = getElementById(tableId);

    var content = "";
    if (!sites.length) {
      content = "<tr><td class=\"left\"><em>";
      if (kind == ALLOWED) {
        var allowedText = getElementById("text-noallowed");
        if (allowedText) {
          if (isDefined(typeof allowedText.innerText)) {
            content += allowedText.innerText;
          } else {
            content += allowedText.textContent;
          }
        }
      } else if (kind == DENIED) {
        var deniedText = getElementById("text-nodenied");
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
    var removeText = getElementById("text-remove");
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
  }

  function layoutSettings(contentHeight) {
    var content = getElementById("content");

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
