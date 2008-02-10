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

#include <gecko_sdk/include/nspr.h>  // for PR_*
#include <gecko_sdk/include/nsServiceManagerUtils.h>  // for NS_IMPL_* and NS_INTERFACE_*
#include <gecko_sdk/include/nsCOMPtr.h>
#include <gecko_internal/jsapi.h>
#include <gecko_internal/nsIDOMClassInfo.h>

#include "gears/blob/blob_interface.h"
#include "gears/blob/blob_ff.h"

NS_IMPL_THREADSAFE_ADDREF(GearsBlob)
NS_IMPL_THREADSAFE_RELEASE(GearsBlob)
NS_INTERFACE_MAP_BEGIN(GearsBlob)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsBlobInterface)
  NS_INTERFACE_MAP_ENTRY(GearsBlobPvtInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsBlobInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsBlob)
NS_INTERFACE_MAP_END

const char *kGearsBlobClassName = "GearsBlob";
const nsCID kGearsBlobClassId = {0x3d32d95c, 0xac6d, 0x11dc, {0x83, 0x14,
                                 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66}};
                                 // {3D32D95C-AC6D-11DC-8314-0800200C9A66}


NS_IMETHODIMP GearsBlob::GetLength(PRInt64 *retval) {
  // A GearsBlob should never be let out in the JS world unless it has been
  // Initialize()d with valid contents_.
  assert(contents_.get());

  int64 length = contents_->Length();
  if ((length < JS_INT_MIN) || (length > JS_INT_MAX)) {
    RETURN_EXCEPTION(STRING16(L"length is out of range."));
  }
  *retval = length;
  return NS_OK;
}


NS_IMETHODIMP GearsBlob::GetContents(BlobInterface** retval) {
  *retval = contents_.get();
  return NS_OK;
}
