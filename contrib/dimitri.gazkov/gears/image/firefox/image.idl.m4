// Copyright 2008, Google Inc.
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

#include "base_interface_ff.idl" // XPIDL doesn't like slashes in #includes

m4_changequote(`^',`^')m4_dnl
m4_ifdef(^OFFICIAL_BUILD^,m4_dnl
  ^^, m4_dnl The Image API has not been finalized for official builds
  ^m4_dnl Else:
interface nsIVariant;

[scriptable, uuid(D11D1910-A2F3-11DC-A6AC-D18B55D89593)]
interface GearsImageInterface : GearsBaseClassInterface {
  void resize(//in PRInt32 width,
              //in PRInt32 height
             );
  void crop(//in PRInt32 x,
            //in PRInt32 y,
            //in PRInt32 width,
            //in PRInt32 height
           );
  void rotate(//in PRInt32 degrees
             );
  void flipHorizontal();
  void flipVertical();
  void drawImage(//in GearsImageInterface image,
                 //in PRInt32 x,
                 //in PRInt32 y
                );
  readonly attribute PRInt32 width;
  readonly attribute PRInt32 height;
  nsISupports toBlob(//in AString type
                    );
};

[scriptable, uuid(5D2E3E35-B6F1-4D8E-B47C-833008C902E7)]
interface GearsImageLoaderInterface : GearsBaseClassInterface {
  GearsImageInterface createImageFromBlob(in nsISupports blob);
};

interface Image; // defined in C++
[ptr] native ImagePtr(Image);

[/* not scriptable */uuid(636078ca-4000-4e81-a51d-d19eccc9d975)]
interface GearsImagePvtInterface : nsISupports {
  [noscript] readonly attribute ImagePtr contents;
};

^)
