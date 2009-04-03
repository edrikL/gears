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

#ifndef GEARS_BLOB_BLOB_BUILDER_MODULE_H__
#define GEARS_BLOB_BLOB_BUILDER_MODULE_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"

class BlobBuilder;

class GearsBlobBuilder : public ModuleImplBaseClass {
 public:
  static const std::string kModuleName;

  GearsBlobBuilder();
  ~GearsBlobBuilder();

  // IN: int|string|GearsBlob|array appendee
  // OUT: -
  void Append(JsCallContext *context);

  // IN: -
  // OUT: GearsBlob
  void GetAsBlob(JsCallContext *context);

 private:
  scoped_ptr<BlobBuilder> builder_;

  bool Append(
      BlobBuilder *builder,
      const JsToken &token,
      JsContextPtr js_context,
      AbstractJsTokenVector *array_stack);

  DISALLOW_EVIL_CONSTRUCTORS(GearsBlobBuilder);
};

#endif  // GEARS_BLOB_BLOB_BUILDER_MODULE_H__
