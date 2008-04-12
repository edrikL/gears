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

#if BROWSER_FF || BROWSER_IE // blobs not implemented for npapi yet

#ifdef DEBUG

#include <cstring>
#include "gears/blob/buffer_blob.h"
#include "gears/blob/slice_blob.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

bool TestBufferBlob() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    printf("TestBufferBlob - failed (%d)\n", __LINE__); \
    return false; \
  } \
}

  int64 num_bytes = 0;
  uint8 buffer[64];
  memset(buffer, 0, sizeof(buffer));
  scoped_refptr<BufferBlob> blob;

  // Initialize a blob with an empty vector.
  std::vector<uint8> vec;
  blob.reset(new BufferBlob(&vec));
  TEST_ASSERT(blob->Length() == 0);
  num_bytes = blob->Read(buffer, 0, 64);
  TEST_ASSERT(num_bytes == 0);
  TEST_ASSERT(buffer[0] == 0);  

  // Initialize a blob with a typical vector.
  vec.push_back('a');
  vec.push_back('b');
  vec.push_back('c');
  blob.reset(new BufferBlob(&vec));
  TEST_ASSERT(vec.empty());
  TEST_ASSERT(blob->Length() == 3);
  num_bytes = blob->Read(buffer, 0, 64);
  TEST_ASSERT(num_bytes == 3);
  TEST_ASSERT(buffer[0] == 'a');
  TEST_ASSERT(buffer[1] == 'b');
  TEST_ASSERT(buffer[2] == 'c');
  TEST_ASSERT(buffer[3] == 0);

  // Initialize buffer blob from an array.
  const uint8 data[] = "abcdefghijklmnopqrstuvwxyz";
  blob.reset(new BufferBlob(data, 9));
  TEST_ASSERT(blob->Length() == 9);

  // Ensure that Read obeys the max_length parameter.
  memset(buffer, 1, sizeof(buffer));
  num_bytes = blob->Read(buffer, 0, 3);
  TEST_ASSERT(num_bytes == 3);
  TEST_ASSERT(buffer[0] == 'a');
  TEST_ASSERT(buffer[1] == 'b');
  TEST_ASSERT(buffer[2] == 'c');
  TEST_ASSERT(buffer[3] == 1);

  // Read with a non-zero starting offset.
  memset(buffer, 1, sizeof(buffer));
  num_bytes = blob->Read(buffer, 3, 3);
  TEST_ASSERT(num_bytes == 3);
  TEST_ASSERT(buffer[0] == 'd');
  TEST_ASSERT(buffer[1] == 'e');
  TEST_ASSERT(buffer[2] == 'f');
  TEST_ASSERT(buffer[3] == 1);

  // Read with an offset beyond the end of the blob.
  memset(buffer, 1, sizeof(buffer));
  num_bytes = blob->Read(buffer, 20, 3);
  TEST_ASSERT(num_bytes == 0);
  TEST_ASSERT(buffer[0] == 1);

  return true;
}

// TODO(bpm): TestFileBlob

bool TestSliceBlob() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    printf("TestSliceBlob - failed (%d)\n", __LINE__); \
    return false; \
  } \
}

  uint8 buffer[64];

  // Create a BufferBlob and verify its contents.
  const uint8 vec_contents[] = "abcdef";
  std::vector<uint8> vec(vec_contents, vec_contents + sizeof(vec_contents) - 1);
  scoped_refptr<BufferBlob> blob_buffer(new BufferBlob(&vec));
  TEST_ASSERT(1 == blob_buffer->GetRef());

  TEST_ASSERT(blob_buffer->Length() == 6);
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(6 == blob_buffer->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "abcdef", 7) == 0);

  // Slice the BufferBlob and verify the slice.  abcdef -> cde
  scoped_refptr<BlobInterface> blob1(new SliceBlob(blob_buffer.get(), 2, 3));
  TEST_ASSERT(2 == blob_buffer->GetRef());
  TEST_ASSERT(1 == blob1->GetRef());
  TEST_ASSERT(blob1->Length() == 3);
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(3 == blob1->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "cde", 4) == 0);

  // A positive offset combines with the slice offset.
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(1 == blob1->Read(buffer, 2, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "e", 2) == 0);

  // A negative offset fails (even though the combined offsets are valid).
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(-1 == blob1->Read(buffer, -1, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "", 1) == 0);

  // A too-large offset returns no bytes.
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(0 == blob1->Read(buffer, 4, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "", 1) == 0);

  // Slice the existing slice.  cde -> d
  scoped_refptr<BlobInterface> blob2(new SliceBlob(blob1.get(), 1, 1));
  TEST_ASSERT(2 == blob_buffer->GetRef());
  TEST_ASSERT(2 == blob1->GetRef());
  TEST_ASSERT(1 == blob2->GetRef());
  TEST_ASSERT(blob2->Length() == 1);
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(1 == blob2->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "d", 2) == 0);

  // Verify that the original slice is unaffected.
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(3 == blob1->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "cde", 4) == 0);

  // The caller is responsible for ensuring that offset and length are
  // non-negative.
  // blob1 = new SliceBlob(blob_buffer, -1, 1));
  // blob1 = new SliceBlob(blob_buffer, 1, -1));

  return true;
}

#endif  // DEBUG
#endif  // BROWSER_FF || BROWSER_IE
#endif  // not OFFICIAL_BUILD
