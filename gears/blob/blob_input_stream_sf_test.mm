// Copyright 2008 Google Inc. All Rights Reserved.
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

#include <cstring>

#import "gears/blob/blob_input_stream_sf.h"
#include "gears/blob/buffer_blob.h"

#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestBlobInputStreamSf - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

namespace {
  // Amount of data that we create (8 MB).
  const std::vector<uint8>::size_type data_size(1024 * 1024 * 8);
}

// This function assumes that the stream passed in has |data_size|
// bytes available for reading.
bool TestBlobInputStreamSfReadFullStream(NSInputStream *stream) {
  uint8_t buffer[data_size];

  BOOL rslt = [stream hasBytesAvailable];
  TEST_ASSERT(rslt == YES);

  memset(buffer, 0, sizeof(buffer));
  NSInteger numRead = [stream read:buffer maxLength:8];
  TEST_ASSERT(numRead == 8);
  TEST_ASSERT(buffer[0] == 79);
  TEST_ASSERT(buffer[7] == 79);
  TEST_ASSERT(buffer[8] == 0);

  memset(buffer, 0, sizeof(buffer));
  numRead = [stream read:buffer maxLength:sizeof(buffer)];
  TEST_ASSERT(numRead == data_size - 8);
  TEST_ASSERT(buffer[0] == 79);
  TEST_ASSERT(buffer[data_size - 9] == 79);
  TEST_ASSERT(buffer[data_size - 8] == 0);

  rslt = [stream hasBytesAvailable];
  TEST_ASSERT(rslt == NO);

  return true;
}

bool TestBlobInputStreamSf() {
  // Some data we can use to initialize our blobs.
  std::vector<uint8> data(data_size, 79);
  scoped_refptr<BlobInterface> blob(new BufferBlob(&data));
  bool ok = true;

  {
    BlobInputStream *stream;

    // Test BlobInputStream::initFromBlob
    stream = [[[BlobInputStream alloc] initFromBlob:blob.get()] autorelease];

    uint8_t *bufferPointer;
    NSUInteger len;
    BOOL rslt = [stream getBuffer:&bufferPointer length:&len];
    TEST_ASSERT(rslt == NO);

    ok &= TestBlobInputStreamSfReadFullStream(stream);
  }

  {
    // Test a BlobInputStream that has not been initialized with a blob.
    BlobInputStream *blobStream = [[[BlobInputStream alloc] init] autorelease];
  
    BOOL rslt = [blobStream hasBytesAvailable];
    TEST_ASSERT(rslt == NO);
    ok &= (rslt == NO);

    uint8_t buffer[data_size];
    memset(buffer, 0, sizeof(buffer));
    NSInteger numRead = [blobStream read:buffer maxLength:sizeof(buffer)];
    TEST_ASSERT(numRead == 0);
    TEST_ASSERT(buffer[0] == 0);

    // Hand a blob to the BlobInputStream.
    [blobStream reset:blob.get()];

    ok &= TestBlobInputStreamSfReadFullStream(blobStream);

    // reset a BlobInputStream that already has a blob assigned.
    [blobStream reset:blob.get()];

    ok &= TestBlobInputStreamSfReadFullStream(blobStream);
  }

  return ok;
}
