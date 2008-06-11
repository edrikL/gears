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

#ifdef USING_CCTESTS

#include <cstring>
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/blob/blob_builder.h"
#include "gears/blob/buffer_blob.h"
#include "gears/blob/file_blob.h"
#include "gears/blob/join_blob.h"
#include "gears/blob/slice_blob.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

bool TestBufferBlob(std::string16 *error) {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    printf("TestBufferBlob - failed (%d)\n", __LINE__); \
    assert(error); \
    *error += STRING16(L"TestBufferBlob - failed. "); \
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

bool TestFileBlob(std::string16 *error) {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    printf("TestFileBlob - failed (%d)\n", __LINE__); \
    assert(error); \
    *error += STRING16(L"TestFileBlob - failed. "); \
    return false; \
  } \
}
  uint8 buffer[64];
  const uint8 vec_contents[] = "abcdef";
  scoped_ptr<File> file(File::CreateNewTempFile());
  file->Write(vec_contents, sizeof(vec_contents) - 1);
  file->Flush();

  // Test FileBlob construction given a File*.
  scoped_refptr<FileBlob> blob(new FileBlob(file.release()));
  TEST_ASSERT(blob->Length() == 6);
  memset(buffer, 0, sizeof(buffer));
  TEST_ASSERT(6 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(memcmp(buffer, "abcdef", 7) == 0);

  // Create a FileBlob from a nonexistent file.
  blob.reset(new FileBlob(STRING16(L"/A/B/C/Z/doesnotexist")));
  TEST_ASSERT(blob->Length() == -1);
  TEST_ASSERT(-1 == blob->Read(buffer, 0, sizeof(buffer)));

  // Test an empty FileBlob.
  std::string16 temp_dir;
  File::CreateNewTempDirectory(&temp_dir);
  std::string16 filepath(temp_dir + kPathSeparator +
                         STRING16(L"TestFileBlob.ext"));
  File::CreateNewFile(filepath.c_str());
  blob.reset(new FileBlob(filepath));
  TEST_ASSERT(blob->Length() == 0);
  TEST_ASSERT(0 == blob->Read(buffer, 0, sizeof(buffer)));
  // This will close filepath, which is necessary before writing to it on win32.
  blob.reset();

  // Test FileBlob construction given a file name.
  File::WriteBytesToFile(filepath.c_str(), vec_contents, 6);
  blob.reset(new FileBlob(filepath));
  TEST_ASSERT(blob->Length() == 6);
  TEST_ASSERT(6 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, "abcdef", 7));

  // Cleanup
  File::DeleteRecursively(temp_dir.c_str());

  return true;
}


bool TestSliceBlob(std::string16 *error) {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    printf("TestSliceBlob - failed (%d)\n", __LINE__); \
    assert(error); \
    *error += STRING16(L"TestSliceBlob - failed. "); \
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

bool TestJoinBlob(std::string16 *error) {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    printf("TestJoinBlob - failed (%d)\n", __LINE__); \
    assert(error); \
    *error += STRING16(L"TestJoinBlob - failed. "); \
    return false; \
  } \
}
  uint8 buffer[64];
  BlobBuilder builder;
  scoped_refptr<BlobInterface> blob;

  // JoinBlob with no content.
  builder.CreateBlob(&blob);
  TEST_ASSERT(blob->Length() == 0);
  TEST_ASSERT(0 == blob->Read(buffer, 0, sizeof(buffer)));

  // JoinBlob containing one BufferBlob.
  const char data1[] = "nom";
  scoped_refptr<BlobInterface> blob1(new BufferBlob(data1, strlen(data1)));
  builder.AddBlob(blob1.get());
  builder.CreateBlob(&blob);
  TEST_ASSERT(blob->Length() == strlen(data1));
  TEST_ASSERT(3 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, data1, strlen(data1)));

  // JoinBlob containing one BufferBlob, one string, and one data buffer.
  builder.AddBlob(blob1.get());
  builder.AddString(STRING16(L"burp"));
  builder.AddData("ahh", 3);
  builder.CreateBlob(&blob);
  TEST_ASSERT(blob->Length() == 10);
  TEST_ASSERT(10 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, "nomburpahh", 10));
  TEST_ASSERT(0 == blob->Read(buffer, 12, sizeof(buffer)));
  TEST_ASSERT(3 == blob->Read(buffer, 1, 3));
  TEST_ASSERT(0 == memcmp(buffer, "omb", 3));

  // JoinBlob containing data, blob, then more data.
  builder.AddData("ahh", 3);
  builder.AddBlob(blob1.get());
  builder.AddData("ahh", 3);
  builder.CreateBlob(&blob);
  TEST_ASSERT(blob->Length() == 9);
  TEST_ASSERT(9 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, "ahhnomahh", 9));

  // JoinBlob containing EmptyBlobs and other empty data.
  builder.AddBlob(new EmptyBlob);
  builder.AddBlob(blob1.get());
  builder.AddString(STRING16(L""));
  builder.AddData("", 0);
  builder.AddBlob(new EmptyBlob);
  builder.CreateBlob(&blob);
  TEST_ASSERT(blob->Length() == strlen(data1));
  TEST_ASSERT(3 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, data1, strlen(data1)));

  // JoinBlob containing a string with non-ascii data.
  builder.AddString(STRING16(L"\xFFFF\x0080"));
  builder.CreateBlob(&blob);
  TEST_ASSERT(blob->Length() == 5);
  TEST_ASSERT(5 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, "\xef\xbf\xbf\xc2\x80", 5));

  // JoinBlob containing the same BufferBlob multiple times.
  for (unsigned i = 0; i < 3; ++i) {
    builder.AddBlob(blob1.get());
  }
  builder.CreateBlob(&blob);
  TEST_ASSERT(blob->Length() == 9);
  TEST_ASSERT(9 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, "nomnomnom", 7));

  // SliceBlob containing a JoinBlob.
  // NOTE - assumes that blob currently contains "nomnomnom".
  scoped_refptr<BlobInterface> blob2 = new SliceBlob(blob.get(), 2, 5);
  TEST_ASSERT(blob2->Length() == 5);
  TEST_ASSERT(5 == blob2->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, "mnomn", 5));

  // JoinBlob containing a SliceBlob.
  builder.AddBlob(blob1.get());
  builder.AddBlob(blob2.get());
  builder.AddString(STRING16(L"burp"));
  builder.CreateBlob(&blob);
  TEST_ASSERT(blob->Length() == 12);
  TEST_ASSERT(12 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, "nommnomnburp", 12));

  // Create JoinBlob from strings surrounding a FileBlob.
  const uint8 vec_contents[] = "abcdef";
  scoped_ptr<File> file(File::CreateNewTempFile());
  file->Write(vec_contents, sizeof(vec_contents) - 1);
  file->Flush();
  scoped_refptr<FileBlob> blob3(new FileBlob(file.release()));
  builder.AddString(STRING16(L"one"));
  builder.AddString(STRING16(L"two"));
  builder.AddBlob(blob3.get());
  builder.AddString(STRING16(L"three"));
  builder.CreateBlob(&blob);
  TEST_ASSERT(blob->Length() == 17);
  TEST_ASSERT(17 == blob->Read(buffer, 0, sizeof(buffer)));
  TEST_ASSERT(0 == memcmp(buffer, "onetwoabcdefthree", 17));

  return true;
}

#endif  // USING_CCTESTS
