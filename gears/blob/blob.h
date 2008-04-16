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

#ifndef GEARS_BLOB_BLOB_H__
#define GEARS_BLOB_BLOB_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/scoped_refptr.h"
#include "gears/blob/blob_interface.h"

#ifdef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#else

class GearsBlob : public ModuleImplBaseClassVirtual {
 public:
  static const std::string kModuleName;

  GearsBlob()
      : ModuleImplBaseClassVirtual(kModuleName),
        contents_(new EmptyBlob()) {}

  // IN: nothing
  // OUT: int64
  void GetLength(JsCallContext *context);

  // IN: int64 offset, optional int64 length
  // OUT: GearsBlob
  void Slice(JsCallContext *context);

  void GetContents(scoped_refptr<BlobInterface> *out) {
    *out = contents_;
  }

  void Reset(BlobInterface *blob) {
    contents_.reset(blob);
  }

 private:
  scoped_refptr<BlobInterface> contents_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsBlob);
};

#endif  // not OFFICIAL_BUILD

#endif  // GEARS_BLOB_BLOB_H__
