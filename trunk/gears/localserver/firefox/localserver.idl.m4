// Copyright 2005, Google Inc.
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

//#include "nsISupports.idl"
#include "base_interface_ff.idl" // XPIDL doesn't like slashes in #includes

interface nsIVariant;

//------------------------------------------------------------------------------
// ResourceCaptureCompletionHandler
//------------------------------------------------------------------------------
[scriptable, function, uuid(4995172C-1842-4720-9465-98033A5A4E66)]
interface ResourceCaptureCompletionHandler : nsISupports {
  void invoke(in AString url, in boolean success, in long captureId);
};

//------------------------------------------------------------------------------
// GearsResourceStoreInterface
//------------------------------------------------------------------------------
[scriptable, uuid(03BE747B-25B0-43ed-B78C-09093F50EEF8)]
interface GearsResourceStoreInterface : GearsBaseClassInterface {
  // Identifying properties
  readonly attribute AString name;
  readonly attribute AString requiredCookie;

  // Enable/disable local serving
  attribute boolean enabled;

  long capture(in nsIVariant urls,
               in ResourceCaptureCompletionHandler completion_callback);

  void abortCapture(in long capture_id);

  boolean isCaptured(in AString url);

  void remove(in AString url);

  void rename(in AString src_url, in AString dst_url);

  void copy(in AString src_url, in AString dst_url);

  AString getHeader(in AString url, in AString header);

  AString getAllHeaders(in AString url);

  void captureBlob(// in ModuleImplBaseClass blob,
                   // in AString url
                  );

  void captureFile(in nsISupports file_input_element, in AString url);

  AString getCapturedFileName(in AString url);

  nsISupports createFileSubmitter(); 
};

//------------------------------------------------------------------------------
// GearsLocalServerInterface
//------------------------------------------------------------------------------
[scriptable, uuid(37C3FB6E-40A0-40f1-8A98-5FFF1F3BAB3A)]
interface GearsLocalServerInterface : GearsBaseClassInterface {
  boolean canServeLocally();
      // in AString url

  nsISupports createManagedStore();
      // in AString name
      // [optional] in AString requiredCookie

  nsISupports openManagedStore();
      // in AString name
      // [optional] in AString requiredCookie

  void removeManagedStore();
      // in AString name
      // [optional] in AString requiredCookie

  GearsResourceStoreInterface createStore();
      // in AString name
      // [optional] in AString requiredCookie

  GearsResourceStoreInterface openStore();
      // in AString name
      // [optional] in AString requiredCookie

  void removeStore();
      // in AString name
      // [optional] in AString requiredCookie
};

//------------------------------------------------------------------------------
// SpecialHttpRequestInterface
//------------------------------------------------------------------------------
interface FFHttpRequest; // defined in C++
[ptr] native FFHttpRequestPtr(FFHttpRequest);

[noscript, uuid(c1aa6650-21da-4b68-bde4-a2f8cf5cc091)]
interface SpecialHttpRequestInterface : nsISupports {
  // This is an interface for use by CacheIntercept.  We QueryInterface
  // for it to see whether a given request came from Gears.
  [noscript] FFHttpRequestPtr getNativeHttpRequest();
};
