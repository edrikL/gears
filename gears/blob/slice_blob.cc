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

#include <assert.h>

#include "gears/blob/slice_blob.h"

SliceBlob::SliceBlob(BlobInterface *source, int64 offset, int64 length)
    : blob_(source), offset_(offset), length_(length) {
  // We explicitly allow offset and length to indicate a slice that extends
  // beyond the end of the source blob, although Read will return no data for
  // that portion.
  assert(offset_ > 0);
  assert(length_ > 0);
}

int SliceBlob::Read(uint8 *destination, int64 offset, int max_bytes) const {
  if (offset < 0) {
    return 0;
  }
  int64 remaining = (offset < length_) ? length_ - offset : 0;
  if (remaining < max_bytes) {
    max_bytes = static_cast<int>(remaining);
  }
  return blob_->Read(destination, offset_ + offset, max_bytes);
}

int64 SliceBlob::Length() const {
  return length_;
}

BlobInterface *SliceBlob::Clone() const {
  if (BlobInterface *cloned = blob_->Clone()) {
    return new SliceBlob(cloned, offset_, length_);
  }
  return NULL;
}

#endif  // not OFFICIAL_BUILD
