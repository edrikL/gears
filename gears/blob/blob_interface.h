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

#ifndef GEARS_BLOB_BLOB_INTERFACE_H__
#define GEARS_BLOB_BLOB_INTERFACE_H__

#include "gears/base/common/common.h"
#include "gears/base/common/scoped_refptr.h"

class BlobInterface : public RefCounted {
 public:
  // Reads up to max_bytes from the blob beginning at the absolute position
  // indicated by offset, and writes the data into destination.  Multiple Reads
  // are unrelated.  Returns the number of bytes successfully read, or -1 on
  // error.  A null destination or negative offset or max_bytes will result in
  // an error.  An offset beyond the end of the stream will succeed and return 0
  // bytes.
  virtual int64 Read(uint8 *destination, int64 offset,
                     int64 max_bytes) const = 0;

  // Note that Length can be volatile, e.g. a file-backed Blob can have that
  // file's size change underneath it.
  virtual int64 Length() const = 0;

 protected:
  BlobInterface() {}
  virtual ~BlobInterface() {}

 private:
  DISALLOW_EVIL_CONSTRUCTORS(BlobInterface);
};

class EmptyBlob : public BlobInterface {
 public:
  EmptyBlob() {}

  int64 Read(uint8 *destination, int64 offset, int64 max_bytes) const {
    if (!destination || (offset < 0) || (max_bytes < 0))
      return -1;
    return 0;
  }

  int64 Length() const {
    return 0;
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(EmptyBlob);
};

#endif  // GEARS_BLOB_BLOB_INTERFACE_H__
