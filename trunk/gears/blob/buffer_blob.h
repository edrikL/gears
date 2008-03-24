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

#ifndef GEARS_BLOB_BUFFER_BLOB_H__
#define GEARS_BLOB_BUFFER_BLOB_H__

#ifdef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#else

#include <vector>
#include "gears/base/common/mutex.h"
#include "gears/blob/blob_interface.h"

// Because BufferBlobs store their contents in a vector, they are restricted in
// size by the maximum value of int.
class BufferBlob : public BlobInterface {
 public:
  // Initializes an empty, write-only BufferBlob.
  BufferBlob() : buffer_(), writable_(true) {}

  // Takes ownership of buffer and initializes a read-only BufferBlob with its
  // contents.  buffer must have been created on the heap with new.
  BufferBlob(std::vector<uint8> *buffer);

  // Returns -1 and does nothing if this is read-only.  Otherwise, attempts to
  // write num_bytes of source to the end of this and returns the number of
  // bytes actually written.
  int64 Append(const void *source, int64 num_bytes);

  // Indicates the blob's contents will not change further (makes it
  // read-only).  Must be called once after all updates are complete, before
  // any reads are attempted.
  void Finalize();

  // Returns -1 if this is write-only.  Otherwise, copies up to max_bytes at
  // offset position to destination.  Returns the number of bytes actually
  // read.
  int64 Read(uint8 *destination, int64 offset, int64 max_bytes) const;

  int64 Length() const;

 private:
  typedef std::vector<uint8> BufferType;
  typedef BufferType::size_type size_type;
  BufferType buffer_;

  // true means write-only, false means read-only.  BufferBlobs are initialized
  // as write-only, and once set read-only, stay read-only forever.
  bool writable_;

  mutable Mutex mutex_;

  DISALLOW_EVIL_CONSTRUCTORS(BufferBlob);
};

#endif  // not OFFICIAL_BUILD

#endif  // GEARS_BLOB_BUFFER_BLOB_H__
