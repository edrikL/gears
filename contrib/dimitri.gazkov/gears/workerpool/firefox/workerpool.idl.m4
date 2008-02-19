// Copyright 2006, Google Inc.
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

interface nsIVariant;

//
// GearsWorkerPoolInterface
//
[scriptable, function, uuid(074EB1A4-80BE-4fce-85DA-19C9EFAEC2FD)]
interface GearsWorkerPoolInterface : GearsBaseClassInterface {
  long createWorker(//in AString full_script
                   );
  long createWorkerFromUrl(//in AString url
                          );
  void allowCrossOrigin();
  void sendMessage(//in AString message_string
                   //in long dest_worker_id
                  );
  
  // nsIVariant is used for these two arbitrarily. We get the params using
  // JsParamFetcher. IDL requires us to provide a type for properties, but this
  // could be anything.
  attribute nsIVariant onmessage;
  attribute nsIVariant onerror;

m4_changequote(`^',`^')m4_dnl
m4_ifdef(^DEBUG^,^m4_dnl
  void forceGC();
^)
};
