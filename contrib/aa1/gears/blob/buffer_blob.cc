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

#include <assert.h>
#include "gears/blob/buffer_blob.h"


BlobInterface *NewBufferBlob(const void *buffer, int64 size) {
  // Although Blobs generally can have sizes of over 2GB (think files on
  // disk), let's not try to allocate an in-memory buffer of that size.
  if (size > kint32max || size < 0) {
    return NULL;
  }
  int size_as_int = static_cast<int>(size);

  // Make a copy of the buffer's contents, to simplify memory ownership issues.
  uint8* copied_buffer = new uint8[size_as_int];
  if (!copied_buffer) {
    return NULL;
  }
  memcpy(copied_buffer, buffer, size_as_int);

  return new BufferBlob(copied_buffer, size_as_int);
}


BufferBlob::BufferBlob(const uint8 *buffer, int64 length)
    : BlobInterface(),
      buffer_(buffer),
      length_(length) {
  assert(length >= 0);
}


BufferBlob::~BufferBlob() {
  delete buffer_;
}


int BufferBlob::Read(
    uint8 *destination,
    int max_bytes,
    int64 position) const {
  if (position >= length_ || position < 0 || max_bytes < 0) {
    return 0;
  }
  int64 actual = length_ - position;
  if (actual > max_bytes) {
    actual = max_bytes;
  }
  assert(actual >= 0);
  int actual_as_int = static_cast<int>(actual);
  memcpy(destination, buffer_ + position, actual_as_int);
  return actual_as_int;
}


int64 BufferBlob::Length() const {
  return length_;
}
