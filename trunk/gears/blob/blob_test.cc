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

#include "gears/blob/buffer_blob.h"
#include "gears/blob/slice_blob.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

bool TestBufferBlob() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    printf("TestBufferBlob - failed (%d)\n", __LINE__); \
    return false; \
  } \
}
  int num_bytes = 0;
  uint8 buffer[64] = { 0 };

  // Empty blob tests
  BufferBlob blob1;
  TEST_ASSERT(blob1.Length() == 0);
  // A non-finalized blob is not readable.
  num_bytes = blob1.Read(buffer, 0, 64);
  TEST_ASSERT(num_bytes == 0);
  blob1.Finalize();
  TEST_ASSERT(blob1.Length() == 0);
  // An empty blob returns no bytes.
  num_bytes = blob1.Read(buffer, 0, 64);
  TEST_ASSERT(num_bytes == 0);
  // A finalized blob is not writable.
  num_bytes = blob1.Append("abc", 3);
  TEST_ASSERT(num_bytes == 0);

  memset(buffer, 0, sizeof(buffer));

  // Typical blob operation
  BufferBlob blob2;
  num_bytes = blob2.Append("abc", 3);
  TEST_ASSERT(num_bytes == 3);
  TEST_ASSERT(blob2.Length() == 3);
  num_bytes = blob2.Append("de", 2);
  TEST_ASSERT(num_bytes == 2);
  TEST_ASSERT(blob2.Length() == 5);
  // A non-finalized blob is not readable.
  num_bytes = blob2.Read(buffer, 0, 64);
  TEST_ASSERT(num_bytes == 0);
  blob2.Finalize();
  TEST_ASSERT(blob2.Length() == 5);
  num_bytes = blob2.Read(buffer, 0, 64);
  TEST_ASSERT(num_bytes == 5);
  // A finalized blob is not writable.
  num_bytes = blob2.Append("fgh", 3);
  TEST_ASSERT(num_bytes == 0);
  TEST_ASSERT(buffer[0] == 'a');
  TEST_ASSERT(buffer[1] == 'b');
  TEST_ASSERT(buffer[2] == 'c');
  TEST_ASSERT(buffer[3] == 'd');
  TEST_ASSERT(buffer[4] == 'e');
  TEST_ASSERT(buffer[5] == '\0');

  // Ensure that Read obeys the max_length parameter.
  num_bytes = blob2.Read(buffer, 2, 3);
  TEST_ASSERT(num_bytes == 3);
  TEST_ASSERT(buffer[0] == 'c');
  TEST_ASSERT(buffer[1] == 'd');
  TEST_ASSERT(buffer[2] == 'e');
  TEST_ASSERT(buffer[3] == 'd');
  TEST_ASSERT(buffer[4] == 'e');

  // Initialize a blob with an empty vector.
  std::vector<uint8> *vec3 = new std::vector<uint8>;
  BufferBlob blob3(vec3);
  TEST_ASSERT(blob1.Length() == 0);
  num_bytes = blob1.Read(buffer, 0, 64);
  TEST_ASSERT(num_bytes == 0);

  memset(buffer, 0, sizeof(buffer));

  // Initialize a blob with a typical vector.
  std::vector<uint8> *vec4 = new std::vector<uint8>;
  vec4->push_back('a');
  vec4->push_back('b');
  vec4->push_back('c');
  BufferBlob blob4(vec4);
  TEST_ASSERT(blob4.Length() == 3);
  num_bytes = blob4.Read(buffer, 0, 64);
  TEST_ASSERT(num_bytes == 3);
  TEST_ASSERT(buffer[0] == 'a');
  TEST_ASSERT(buffer[1] == 'b');
  TEST_ASSERT(buffer[2] == 'c');  

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

  BufferBlob blob_buffer;
  TEST_ASSERT(6 == blob_buffer.Append("abcdef", 6));
  blob_buffer.Finalize();

  // Clone the BufferBlob and verify it's contents.
  scoped_ptr<BlobInterface> blob1(blob_buffer.Clone());
  TEST_ASSERT(blob1.get());
  TEST_ASSERT(blob1->Length() == 6);
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(6 == blob1->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "abcdef", 7) == 0);

  // Slice the BufferBlob and verify the slice.
  blob1.reset(new SliceBlob(blob1.release(), 2, 3));  // abcdef -> cde
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
  TEST_ASSERT(0 == blob1->Read(buffer, -1, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "", 1) == 0);

  // A too-large offset returns no bytes.
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(0 == blob1->Read(buffer, 4, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "", 1) == 0);

  // Clone the slice and re-slice it.
  scoped_ptr<BlobInterface> blob2(blob1->Clone());
  TEST_ASSERT(blob2.get());
  blob2.reset(new SliceBlob(blob2.release(), 1, 1));  // cde -> d
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
  // blob1.reset(new SliceBlob(blob_buffer.Clone(), -1, 1));
  // blob1.reset(new SliceBlob(blob_buffer.Clone(), 1, -1));

  return true;
}

#endif  // DEBUG
#endif  // BROWSER_FF || BROWSER_IE
#endif  // not OFFICIAL_BUILD
