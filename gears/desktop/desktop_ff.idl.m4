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

#include "nsISupports.idl"
#include "base_interface_ff.idl" // XPIDL doesn't like slashes in #includes

//
// GearsDesktopInterface
//
[scriptable, function, uuid(0273640F-FE6D-4a26-95C7-455DE83C6049)]
interface GearsDesktopInterface : GearsBaseClassInterface {

  // icons parameter is expected to be a javascript object with properties for
  // each available icon size. Valid property names are currently "16x16",
  // "32x32", "48x48", and "128x128".
  void createShortcut(//in string name,
                      //in string description,
                      //in string url,
                      //in object icons
                      );

m4_changequote(`^',`^')m4_dnl
m4_ifdef(^OFFICIAL_BUILD^,m4_dnl
  ^^, m4_dnl File pickers are not ready for OFFICIAL_BUILD
  ^m4_dnl Else:
  void getLocalFiles(//optional in string array filters
                     //file array retval
                     );
^)
};


