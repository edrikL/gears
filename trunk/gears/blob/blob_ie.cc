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

#ifdef WINCE
// No BLOB support on WINCE yet
#else
#ifdef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#else

#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"

#include "gears/blob/blob_ie.h"


STDMETHODIMP GearsBlob::get_length(VARIANT *retval) {
  // A GearsBlob should never be let out in the JS world unless it has been
  // Initialize()d with valid contents_.
  assert(contents_.get());

  int64 length = contents_->Length();
  if ((length < JS_INT_MIN) || (length > JS_INT_MAX)) {
    RETURN_EXCEPTION(STRING16(L"length is out of range."));
  }
  retval->vt = VT_R8;
  retval->dblVal = static_cast<DOUBLE>(length);
  RETURN_NORMAL();
}


STDMETHODIMP GearsBlob::get_contents(VARIANT *retval) {
  // We pack the pointer into the byref field of a VARIANT.
  retval->vt = VT_BYREF;
  retval->byref =
      const_cast<void*>(reinterpret_cast<const void*>(contents_.get()));
  RETURN_NORMAL();
}

#endif  // not OFFICIAL_BUILD
#endif  // WINCE
