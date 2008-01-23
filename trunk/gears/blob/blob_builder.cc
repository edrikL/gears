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

#include "gears/blob/buffer_blob.h"
#include "gears/blob/blob_builder.h"

const int BlobBuilder::kInitialCapacity = 1024;

BlobBuilder::BlobBuilder()
    : buffer_(new uint8[kInitialCapacity]),
      capacity_(kInitialCapacity),
      length_(0) {
}

int BlobBuilder::Append(const void *source, int num_bytes) {
  if (num_bytes < 0) {
    return 0;
  }
  if (!buffer_) {
    // We don't allow writing to BlobBuilders which have already been sent off
    // to Blobs
    return 0;
  }
  if (length_ + num_bytes < 0) {  // Overflow
    return 0;
  }

  if (length_ + num_bytes > capacity_) {
    int new_capacity = capacity_;
    // Find the smallest power of two big enough to hold the data.
    while (length_ + num_bytes > new_capacity) {
      new_capacity *= 2;
      if (new_capacity <= 0) {  // Overflow
        new_capacity = kint32max;
        // This is guaranteed to fit the data because of the overflow check
        // above
        break;
      }
    }

    uint8* new_buffer = new uint8[new_capacity];
    if (!new_buffer) {
      return 0;
    }
    memcpy(new_buffer, buffer_, length_);
    delete buffer_;
    buffer_ = new_buffer;
    capacity_ = new_capacity;
  }

  // Now copy the new data in
  memcpy(buffer_ + length_, source, num_bytes);
  length_ += num_bytes;
  return num_bytes;
}

BlobInterface *BlobBuilder::ToBlob() {
  if (buffer_) {
    BufferBlob *blob = new BufferBlob(buffer_, length_);
    buffer_ = NULL;
    return blob;
  } else {
    // We don't allow creating multiple Blobs from a single BlobBuilder
    return NULL;
  }
}

BlobBuilder::~BlobBuilder() {
  delete buffer_;
}
