// Copyright 2007, Google Inc.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "WinResrc.h"
#include "ui/ie/ui_resources.h"

//-----------------------------------------------------------------------------
// HTML
// Note: We break with convention on the naming of these resources so that
// they can be addressed the same way on all platforms.
// TODO(aa): What effect (if any) does the HTML tag on these have? Should I be
// using something else? It seems to work as is.
//-----------------------------------------------------------------------------

button_row_background.gif  HTML  "ui/common/button_row_background.gif"
html_dialog.css            HTML  "ui/common/html_dialog.css"
html_dialog.js             HTML  "ui/common/html_dialog.js"
json_noeval.js             HTML  "third_party/jsonjs/json_noeval.js"
permissions_dialog.html    HTML  "common/genfiles/permissions_dialog.html"
settings_dialog.html       HTML  "common/genfiles/settings_dialog.html"

//-----------------------------------------------------------------------------
// Dialogs
//-----------------------------------------------------------------------------

IDD_GENERIC_HTML DIALOGEX 0, 0, 500, 500
STYLE DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | DS_CENTER | WS_POPUP |
    WS_CAPTION | WS_SYSMENU
CAPTION ""
FONT 8, "MS Shell Dlg", 400, 0, 0x1
BEGIN
    // Note: This is the GUID of Microsoft's reusable browser control:
    // http://msdn2.microsoft.com/en-us/library/aa752040.aspx
    CONTROL         "",IDC_GENERIC_HTML,"{8856F961-340A-11D0-A96B-00C04FD705A2}",WS_TABSTOP,7,7,486,486
END

//-----------------------------------------------------------------------------
// Strings
//-----------------------------------------------------------------------------

// The string resources must follow the format below for i18n.
// * If a string resource is preceded by one or more comment lines, those
//   comments will be associated with the string for translation purposes.
// * To add a comment not tied to any particular string, follow it with
//   a blank line.
// * It is also permissible to list string resources consecutively, without
//   any comments or blank lines in-between.

// Leave commented out until we have a real entry; table cannot be empty.
//STRINGTABLE
//BEGIN
//
//  // Example:
//  // IDS_CAPABILITIES_HELP_URL  "http://www.google.com/"
//
//END
