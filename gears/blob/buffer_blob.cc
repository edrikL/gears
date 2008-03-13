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
#include "gears/blob/buffer_blob.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

BufferBlob::BufferBlob(std::vector<uint8> *buffer) {
  buffer_.swap(*buffer);
  delete buffer;
  writable_ = false;
}


int BufferBlob::Append(const void *source, int num_bytes) {
  MutexLock lock(&mutex_);
  if (!writable_ ||
      num_bytes < 0 ||
      // Don't try to support BufferBlobs over 2GB:
      static_cast<int64>(buffer_.size()) + num_bytes > kint32max) {
    return 0;
  }
  int original_size = buffer_.size();
  const uint8* bytes = static_cast<const uint8*>(source);
  buffer_.insert(buffer_.end(), bytes, bytes + num_bytes);
  return buffer_.size() - original_size;
}


void BufferBlob::Finalize() {
  MutexLock lock(&mutex_);
  writable_ = false;
}


int BufferBlob::Read(uint8 *destination, int64 offset, int max_bytes) const {
  {
    MutexLock lock(&mutex_);
    if (writable_) {
      return 0;
    }
    // By this point, we've established that the blob will not change so we
    // don't need the mutex lock any more.
  }
  if (offset >= buffer_.size() || offset < 0 || max_bytes < 0) {
    return 0;
  }
  assert(offset <= kint32max);  // Enforced by Append()

  int offset_as_int = static_cast<int>(offset);
  int actual = buffer_.size() - offset_as_int;
  if (actual > max_bytes) {
    actual = max_bytes;
  }
  memcpy(destination, &(buffer_[offset_as_int]), actual);
  return actual;
}


int64 BufferBlob::Length() const {
  MutexLock lock(&mutex_);
  return buffer_.size();
}

#endif  // not OFFICIAL_BUILD
