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

#include "gears/blob/blob_interface.h"

// NewBufferBlob does not assume ownership of buffer's memory, as it will make
// a private copy of the given buffer.  Returns NULL on failure.
// The result is suitable to place inside a scoped_ptr<BlobInterface>.
BlobInterface *NewBufferBlob(const void *buffer, int64 size);

class BufferBlob : public BlobInterface {
 public:
  ~BufferBlob();

  int Read(uint8 *destination, int max_bytes, int64 position) const;
  int64 Length() const;

 private:
  friend BlobInterface *NewBufferBlob(const void *buffer, int64 size);

  // The BufferBlob will assume ownership of buffer's memory.
  BufferBlob(const uint8 *buffer, int64 length);

  const uint8 *buffer_;
  const int64 length_;

  DISALLOW_EVIL_CONSTRUCTORS(BufferBlob);
};

#endif  // GEARS_BLOB_BUFFER_BLOB_H__
