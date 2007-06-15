// Copyright 2007, Google Inc.
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

#include <vector>
#include "gears/base/common/paths.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/file.h"


//------------------------------------------------------------------------------
// TestFileUtils
//------------------------------------------------------------------------------
bool TestFileUtils() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestFileUtils - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

  // Create a new empty directory work with
  std::string16 temp_dir;
  TEST_ASSERT(File::CreateNewTempDirectory(&temp_dir));
  TEST_ASSERT(File::DirectoryExists(temp_dir.c_str()));
  TEST_ASSERT(File::GetDirectoryFileCount(temp_dir.c_str()) == 0);

  const char16 *kFileName = STRING16(L"File.ext");
  std::string16 filepath(temp_dir);
  filepath += kPathSeparator;
  filepath += kFileName;

  // Get the files extension
  std::string16 ext(File::GetFileExtension(filepath.c_str()));
  TEST_ASSERT(ext == STRING16(L".ext"));

  // Create a file, test existence, and make sure we fail if we try to
  // recreate an already existing file
  TEST_ASSERT(!File::Exists(filepath.c_str()));
  TEST_ASSERT(File::CreateNewFile(filepath.c_str()));
  TEST_ASSERT(File::Exists(filepath.c_str()));
  TEST_ASSERT(!File::CreateNewFile(filepath.c_str()));

  // See that the directory contains one item
  TEST_ASSERT(File::GetDirectoryFileCount(temp_dir.c_str()) == 1);

  // Write some data, then read it back and compare
  uint8 stack_fuzz[4096];
  std::vector<uint8> data_orig;
  std::vector<uint8> data_read;
  data_orig.assign(stack_fuzz, stack_fuzz + ARRAYSIZE(stack_fuzz));
  TEST_ASSERT(File::WriteVectorToFile(filepath.c_str(), &data_orig));
  TEST_ASSERT(File::ReadFileToVector(filepath.c_str(), &data_read));
  TEST_ASSERT(data_orig.size() == data_read.size());
  TEST_ASSERT(memcmp(&data_orig[0], &data_read[0], data_orig.size()) == 0);

  // Overwrite what was written above
  data_orig.assign(1024, 0xAA);
  data_read.clear();
  TEST_ASSERT(File::WriteVectorToFile(filepath.c_str(), &data_orig));
  TEST_ASSERT(File::ReadFileToVector(filepath.c_str(), &data_read));
  TEST_ASSERT(data_orig.size() == data_read.size());
  TEST_ASSERT(memcmp(&data_orig[0], &data_read[0], data_orig.size()) == 0);

  // Overwrite again with a zero length file
  data_orig.clear();
  data_read.clear();
  TEST_ASSERT(File::WriteVectorToFile(filepath.c_str(), &data_orig));
  TEST_ASSERT(File::ReadFileToVector(filepath.c_str(), &data_read));
  TEST_ASSERT(data_orig.size() == data_read.size());
  TEST_ASSERT(memcmp(&data_orig[0], &data_read[0], data_orig.size()) == 0);

  // Delete the file, test that it is deleted, and make sure we fail if we try
  // to delete a file that doesn't exist
  TEST_ASSERT(File::Delete(filepath.c_str()));
  TEST_ASSERT(!File::Exists(filepath.c_str()));
  TEST_ASSERT(!File::Delete(filepath.c_str()));

  // See the the directory now contains no items
  TEST_ASSERT(File::GetDirectoryFileCount(temp_dir.c_str()) == 0);

  // We should not be able to read or write a file that doesn't exist
  TEST_ASSERT(!File::WriteVectorToFile(filepath.c_str(), &data_orig));
  TEST_ASSERT(!File::ReadFileToVector(filepath.c_str(), &data_read));

  // Create a sub-directory
  std::string16 sub_dir(temp_dir);
  sub_dir += kPathSeparator;
  sub_dir += STRING16(L"sub");
  TEST_ASSERT(!File::DirectoryExists(sub_dir.c_str()));
  TEST_ASSERT(File::GetDirectoryFileCount(sub_dir.c_str()) == 0);
  TEST_ASSERT(File::RecursivelyCreateDir(sub_dir.c_str()));
  TEST_ASSERT(File::DirectoryExists(sub_dir.c_str()));
  TEST_ASSERT(File::GetDirectoryFileCount(temp_dir.c_str()) == 1);

  // Remove the entire tmp_dir incuding the sub-dir it contains
  TEST_ASSERT(File::DeleteRecursively(temp_dir.c_str()));
  TEST_ASSERT(!File::DirectoryExists(temp_dir.c_str()));

  LOG(("TestFileUtils - passed\n"));
  return true;
}

