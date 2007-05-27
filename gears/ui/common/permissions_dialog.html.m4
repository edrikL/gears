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
  <link rel="stylesheet" href="html_dialog.css">
  <style type="text/css">
    #checkbox-row {
      margin-top:1em;
    }

    #sitename {
      text-align:center;
      font-weight:bold;
      background:#e0ecff;
      padding:0.5em 0;
    }
  </style>
</head>
<body>
  <div id="scroll">
    <div id="content">
      <p>The website below wants to use PRODUCT_FRIENDLY_NAME_UQ.</p>
      <p id="sitename"></p>
      <p>
        This will let the website store data on your computer. Do you
        want to allow this?
        <!-- 
        [naming] - Not using short_name here because not sure if url always
        changes when name does.

        HACK: What I wanted here was target="_blank" to open this URL in a new
        window. However, when I did that Firefox opened the new window, but
        did not actually navigate it anywhere. This is probably due to modality
        issues (the code that would do the navigate is blocked behind this
        modal dialog). Using onclick fixes, for some reason.
        -->
        &nbsp;(<a href="http://gears.google.com/?help"
          onclick="window.open(this.href); return false;">What is this?</a>)
      </p>
      <p id="checkbox-row">
        <table cellspacing="0" cellpadding="0" border="0">
          <tr>
            <td valign="middle">
              <input type="checkbox" id="remember" accesskey="R">
            </td>
            <td valign="middle">
              <label for="remember">
                &nbsp;Remember my decision for this site
              </label>
            </td>
          </tr>
        </table>
      </p>
    </div>
  </div>
  <div id="button-row">
    <!-- 
    Note: Weird line breaks are on purpose to avoid extra space between buttons.
    -->
    <button id="allow-button" accesskey="A" onclick="finishWarning(true)"
      ><span class="accesskey">A</span>llow</button
    ><button id="deny-button" accesskey="D" onclick="finishWarning(false)"
      ><span class="accesskey">D</span>eny</button>
  </div>
</body>
<script src="json_noeval.js"></script>
<script src="html_dialog.js"></script>
<script>
  initDialog();
  initWarning();

  function initWarning() {
    // The arguments to this dialog are a single string. See CapabilitiesDB.
    document.getElementById("sitename").appendChild(
      document.createTextNode(getArguments()));

    // Focus deny by default
    document.getElementById("deny-button").focus();
  }
  
  function finishWarning(allow) {
    // This is coupled to the results that CapabilitiesDB is expecting.
    saveAndClose({
      allow: allow,
      remember: document.getElementById("remember").checked
    });
  }
</script>
</html>
