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
  <link rel="stylesheet" href="html_dialog.css">
  <style type="text/css">
    table {
      border:1px #ccc;
      border-style:none solid;
      border-collapse:collapse;
      cell-spacing:0;
    }

    td {
      border:1px #ccc;
      border-style:solid none;
      padding:0.5em;
    }

    .left {
      width:100%;
    }
  </style>
</head>
<body>
  <div id="scroll">
    <div id="content">
      <h1>Allowed Sites</h1>
      <p>These sites are always allowed to access PRODUCT_FRIENDLY_NAME_UQ.</p>
      <div>
        <table>
          <tbody id="allowed-list">
            <tr>
              <td class="left"><em>No allowed sites.</em></td>
              <td></td>
            </tr>
          </tbody>
        </table>
      </div>
      <br>
      <br>
      <h1>Denied Sites</h1>
      <p>These sites are never allowed to access PRODUCT_FRIENDLY_NAME_UQ.</p>
      <div>
        <table>
          <tbody id="denied-list">
            <tr>
              <td class="left"><em>No denied sites.</em></td>
              <td></td>
            </tr>
          </tbody>
        </table>
      </div>
      <br>
      <br>
      <div id="version">
        PRODUCT_FRIENDLY_NAME_UQ version PRODUCT_VERSION
      </div>
    </div>
  </div>
  <div id="button-row">
    <!-- 
    Note: Weird line breaks are on purpose to avoid extra space between or
    inside buttons.
    -->
    <button id="confirm-button" onclick="saveAndClose(g_dialogResult)" 
      disabled="true" accesskey="S"
      ><span class="accesskey">S</span>ave</button
    ><button id="cancel-button" onclick="saveAndClose(null)" accesskey="C"
      ><span class="accesskey">C</span>ancel</button>
  </div>
</body>
<script src="json_noeval.js"></script>
<script src="html_dialog.js"></script>
<script>
  var g_dialogResult = [];

  initDialog();
  initSettings();
  
  function initSettings() {
    var args = getArguments();
    initList("allowed-list", args.allowed);
    initList("denied-list", args.denied);
  }
  
  function initList(tableId, sites) {
    var table = document.getElementById(tableId);
    if (!sites.length) {
      return;
    }
    
    // Hide the empty message
    table.rows[0].style.display = "none";
    
    // Add rows for each child    
    for (var site, i = 0; site = sites[i]; i++) {
      initSite(table, site);
    }
  }
  
  function initSite(table, siteName) {
    var row = document.createElement("TR");
    var left = document.createElement("TD");
    var right = document.createElement("TD");
    var link = document.createElement("A");
    left.className = "left";
    right.className = "right";
    left.appendChild(document.createTextNode(siteName));
    link.appendChild(document.createTextNode("Remove"));
    link.onclick = handleRemoveClick;
    link.row = row;
    link.siteName = siteName;
    link.href = "#";
    right.appendChild(link);
    row.appendChild(left);
    row.appendChild(right);
    table.appendChild(row);
  }
  
  function handleRemoveClick() {
    // Add to the list of things to be removed from database.
    g_dialogResult.push(this.siteName);

    // Remove the row visually.
    var table = this.row.parentNode;
    table.removeChild(this.row);
    
    // If we have removed all the rows, show the empty message again.
    if (table.rows.length == 1) {
      table.rows[0].style.display = "";
    }
    
    // Enable the save button since we have now made a change.
    document.getElementById("confirm-button").disabled = false;

    // Return false to prevent the default link action (scrolling up to top of
    // page in this case).    
    return false;
  }
</script>
</html>
