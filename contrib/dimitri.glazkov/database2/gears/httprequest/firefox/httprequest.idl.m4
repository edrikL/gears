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

//------------------------------------------------------------------------------
// ReadyStateChangeEventListener
//------------------------------------------------------------------------------
[scriptable, function, uuid(07B50C3D-F17B-4a99-85A9-06E01FA050DE)]
interface ReadyStateChangeEventListener : nsISupports {
  void invoke();
};

//------------------------------------------------------------------------------
// GearsHttpRequest
//------------------------------------------------------------------------------
[scriptable, uuid(D2C033DE-B7EF-450f-BBD6-32CDC3572E3B)]
interface GearsHttpRequestInterface : GearsBaseClassInterface {
  // event handler
  attribute ReadyStateChangeEventListener onreadystatechange;

  // state
  readonly attribute long readyState;

  // request
  void open(
    // [in] AString method,
    // [in] AString url,
    // [in] Boolean async
    );
  void setRequestHeader(
    // [in] AString headerName,
    // [in] AString headerValue
    );
  void send(
    // [in, optional] nsIVariant data
    );
  void abort();

  // response
  AString getAllResponseHeaders();
  AString getResponseHeader(
    // [in] AString headerName
    );
  readonly attribute AString responseText;

m4_changequote(`^',`^')m4_dnl
m4_ifdef(^OFFICIAL_BUILD^,m4_dnl
  ^^, m4_dnl Do not declare anything for OFFICIAL_BUILDs - Blobs are not ready
  ^m4_dnl Else:
  readonly attribute nsISupports responseBlob;
^)

  readonly attribute long status;
  readonly attribute AString statusText;
};
