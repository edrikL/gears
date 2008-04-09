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

#ifdef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#else

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/blob/blob.h"
#include "gears/blob/slice_blob.h"

DECLARE_GEARS_WRAPPER(GearsBlob);

template<>
void Dispatcher<GearsBlob>::Init() {
  RegisterMethod("slice", &GearsBlob::Slice);
  RegisterProperty("length", &GearsBlob::GetLength, NULL);
}

const std::string GearsBlob::kModuleName("GearsBlob");

void GearsBlob::GetLength(JsCallContext *context) {
  // A GearsBlob should never be let out in the JS world unless it has been
  // Initialize()d with valid contents_.
  assert(contents_.get());

  int64 length = contents_->Length();
  if ((length < JS_INT_MIN) || (length > JS_INT_MAX)) {
    context->SetException(STRING16(L"length is out of range."));
    return;
  }

  // If length (which is 64-bit) fits inside 32 bits, then return it as a
  // 32-bit int, otherwise return it as a double.
  if ((length < INT_MIN) || (length > INT_MAX)) {
    double length_as_double = static_cast<double>(length);
    context->SetReturnValue(JSPARAM_DOUBLE, &length_as_double);
  } else {
    int length_as_int = static_cast<int>(length);
    context->SetReturnValue(JSPARAM_INT, &length_as_int);
  }
}

void GearsBlob::Slice(JsCallContext *context) {
  int64 offset = 0;
  int64 length = 0;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT64, &offset },
    { JSPARAM_OPTIONAL, JSPARAM_INT64, &length },
  };
  int argc = context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  if (offset < 0) {
    context->SetException(STRING16(L"Offset must be a non-negative integer."));
    return;
  }
  if (argc != 2) {
    int64 blob_size = contents_->Length();
    length = (blob_size > offset) ? blob_size - offset : 0;
  }
  if (length < 0) {
    context->SetException(STRING16(L"Length must be a non-negative integer."));
    return;
  }

  // Slice the blob.
  scoped_refptr<BlobInterface> sliced(new SliceBlob(contents_.get(), offset,
                                                    length));

  scoped_refptr<GearsBlob> gears_blob;
  CreateModule<GearsBlob>(GetJsRunner(), &gears_blob);
  if (!gears_blob->InitBaseFromSibling(this)) {
    context->SetException(STRING16(L"Initializing base class failed."));
    return;
  }
  gears_blob->Reset(sliced.get());
  context->SetReturnValue(JSPARAM_DISPATCHER_MODULE, gears_blob.get());
  ReleaseNewObjectToScript(gears_blob.get());
}

#endif  // not OFFICIAL_BUILD
