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
#include "gears/base/common/common.h"
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string_utils.h"
#ifdef WINCE
#include "gears/base/common/wince_compatibility.h"
#endif


//------------------------------------------------------------------------------
// TestFileUtils
//------------------------------------------------------------------------------
bool TestCollapsePathSeparators();  // friend of file.h
static bool TestGetBaseName();
static bool TestLongPaths();
static bool CheckDirectoryCreation(const char16 *dir);

bool TestFileUtils() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestFileUtils - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

  //Run tests for individual functions
  TEST_ASSERT(TestGetBaseName());
  TEST_ASSERT(TestSplitPath());
  TEST_ASSERT(TestLongPaths());

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
  TEST_ASSERT(data_orig.size() == 0);

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
  TEST_ASSERT(CheckDirectoryCreation(sub_dir.c_str()));
  TEST_ASSERT(File::GetDirectoryFileCount(temp_dir.c_str()) == 1);

  // Remove the entire tmp_dir incuding the sub-dir it contains
  TEST_ASSERT(File::DeleteRecursively(temp_dir.c_str()));
  TEST_ASSERT(!File::DirectoryExists(temp_dir.c_str()));

#ifdef WINCE
   // Test the directory creation on Windows Mobile
  const char16 *kValidDirNames[] = {
      STRING16(L"\\Dir1"),
      STRING16(L"\\Dir1\\Dir2"),
      STRING16(L"\\Dir1\\Dir2\\"),
      STRING16(L"/Dir1"),
      STRING16(L"/Dir1/Dir2/"),
      STRING16(L"/Dir1\\Dir2/"),      
      STRING16("\\Dir1/Dir2/"),      
      STRING16(L".\\Dir"),
      STRING16(L"..\\Dir.Dir2.Dir3/Dir4/")
  };
  const char16 *kInvalidDirNames[] = {      
      STRING16(L"//Dir1/////Dir2/"),
      STRING16(L"\\\\Dir1\\Dir2\\"),
      STRING16(L"\\Dir1\\:Dir2"),
      STRING16(L"&.<>:\"/\\|?*"),
      STRING16(L""),      
      STRING16(L"?")
  };

  for (int i = 0; i < ARRAYSIZE(kValidDirNames); ++i) {
    TEST_ASSERT(CheckDirectoryCreation(kValidDirNames[i]));
    TEST_ASSERT(File::DeleteRecursively(kValidDirNames[i]));
  }

  for (int i = 0; i < ARRAYSIZE(kInvalidDirNames); ++i) {
    TEST_ASSERT(!(File::RecursivelyCreateDir(kInvalidDirNames[i])));
  }    
#endif
  LOG(("TestFileUtils - passed\n"));
  return true;
}

bool TestLongPaths() {
// Only Win32 has issues with long pathnames, ignore WinCE for now.
#if defined(WIN32) && !defined(WINCE)

  // Constants
  const std::string16 kDrv(STRING16(L"c:"));
  const std::string16 kSep(&kPathSeparator, 1);
  const std::string16 kFoo(STRING16(L"kFoo"));

  // Create a new kEmpty directory to work with
  std::string16 temp_dir;
  TEST_ASSERT(File::CreateNewTempDirectory(&temp_dir));
  TEST_ASSERT(File::DirectoryExists(temp_dir.c_str()));
  TEST_ASSERT(File::GetDirectoryFileCount(temp_dir.c_str()) == 0);

  std::string16 long_file_name;  // longest legal filename
  for (size_t i = 0; i < File::kMaxPathComponentChars; ++i) {
    long_file_name += L'A';
  }

  //------------ RecursivelyCreateDir ------------
  // Creating c: & c:\ should succeed
  TEST_ASSERT(File::RecursivelyCreateDir(kDrv.c_str()));

  // The previous implementation of RecursivelyCreateDir fails this test, as 
  // SHCreateDirectoryEx() returns an error when trying to create c:\. We
  // do not emulate this behavior.
  TEST_ASSERT(File::RecursivelyCreateDir((kDrv + kSep).c_str()));

  // Check that creating a directory with the same name as a file fails.
  {
    std::string16 file_path = temp_dir + kSep + kFoo;
    TEST_ASSERT(File::CreateNewFile(file_path.c_str()));
    TEST_ASSERT(File::Exists(file_path.c_str()));
    
    TEST_ASSERT(!File::RecursivelyCreateDir(file_path.c_str()));

    // Make sure file wasn't touched...
    TEST_ASSERT(File::Exists(file_path.c_str()));
    
    // then delete is.
    TEST_ASSERT(File::Delete(file_path.c_str()));
    TEST_ASSERT(!File::Exists(file_path.c_str()));
  }

  // Check Reading & Writing data from a file with a long path
  {
    std::string16 parent_dir = temp_dir + kSep;

    // Make sure that the directory this file is in is already longer than
    // MAX_PATH.
    while (parent_dir.length() < MAX_PATH) {
      parent_dir += long_file_name + kSep;
    }

    //long filename
    const std::string16 kFilenameSuffix = STRING16(L"_data_test.txt");
    const std::string16 kFilePath  = parent_dir + 
        long_file_name.substr(0, File::kMaxPathComponentChars - 
                           kFilenameSuffix.length()) + 
         kFilenameSuffix;

    uint8 write_data = 0xAA;
    std::vector<uint8> write_vector;
    std::vector<uint8> read_data;
    
    TEST_ASSERT(!File::DirectoryExists(parent_dir.c_str()));
    TEST_ASSERT(File::RecursivelyCreateDir(parent_dir.c_str()));
    TEST_ASSERT(File::DirectoryExists(parent_dir.c_str()));

    TEST_ASSERT(!File::Exists(kFilePath.c_str()));
    TEST_ASSERT(!File::ReadFileToVector(kFilePath.c_str(), &read_data));

    TEST_ASSERT(File::CreateNewFile(kFilePath.c_str()));
    TEST_ASSERT(File::WriteBytesToFile(kFilePath.c_str(), &write_data, 1));
    TEST_ASSERT(File::ReadFileToVector(kFilePath.c_str(), &read_data));
    TEST_ASSERT(read_data.size() == 1 && read_data[0] == write_data);

    TEST_ASSERT(File::Delete(kFilePath.c_str()));
    TEST_ASSERT(!File::Exists(kFilePath.c_str()));

    write_vector.push_back(write_data);
    TEST_ASSERT(File::CreateNewFile(kFilePath.c_str()));
    TEST_ASSERT(File::Exists(kFilePath.c_str()));
    TEST_ASSERT(File::WriteVectorToFile(kFilePath.c_str(), &write_vector));
    TEST_ASSERT(File::ReadFileToVector(kFilePath.c_str(), &read_data));
    TEST_ASSERT(read_data.size() == 1 && read_data[0] == write_data);

    TEST_ASSERT(File::Delete(kFilePath.c_str()));
    TEST_ASSERT(!File::Exists(kFilePath.c_str()));

    TEST_ASSERT(File::DeleteRecursively(parent_dir.c_str()));
    TEST_ASSERT(!File::DirectoryExists(parent_dir.c_str()));
  }

  // Check that creating a long directory path works
  // DeleteRecursively
  {
    const std::string16 kTopOfTestDir = temp_dir + kSep + long_file_name + kSep;
    std::string16 file_path = kTopOfTestDir;

    while (file_path.length() < MAX_PATH) {
      file_path += long_file_name + kSep;
    }

    TEST_ASSERT(!File::DirectoryExists(file_path.c_str()));

    TEST_ASSERT(File::RecursivelyCreateDir(file_path.c_str()));
    TEST_ASSERT(File::RecursivelyCreateDir(file_path.c_str()));

    TEST_ASSERT(File::DirectoryExists(file_path.c_str()));
    TEST_ASSERT(File::DeleteRecursively(kTopOfTestDir.c_str()));
    TEST_ASSERT(!File::DirectoryExists(kTopOfTestDir.c_str()));
  }
  
  // DirectoryExists, Exists, CreateNewFile, Delete, GetDirectoryFileCount
  {
    std::string16 parent_dir = temp_dir + kSep;

    // Make sure that the directory this file is in is already longer than
    // MAX_PATH.
    while (parent_dir.length() < MAX_PATH) {
      parent_dir += long_file_name + kSep;
    }

    std::string16 file_path = parent_dir + long_file_name;
    std::string16 other_file_path = parent_dir + STRING16(L"a.txt");


    TEST_ASSERT(!File::DirectoryExists(file_path.c_str()));
    TEST_ASSERT(!File::Exists(file_path.c_str()));
    
    TEST_ASSERT(File::RecursivelyCreateDir(parent_dir.c_str()));
    TEST_ASSERT(File::CreateNewFile(file_path.c_str()));
    TEST_ASSERT(File::Exists(file_path.c_str()));

    TEST_ASSERT(!File::CreateNewFile(file_path.c_str()));
    TEST_ASSERT(File::Exists(file_path.c_str()));

    TEST_ASSERT(File::GetDirectoryFileCount(parent_dir.c_str()) == 1);

    TEST_ASSERT(File::CreateNewFile(other_file_path.c_str()));
    TEST_ASSERT(File::Exists(other_file_path.c_str()));

    TEST_ASSERT(File::GetDirectoryFileCount(parent_dir.c_str()) == 2);
    
    TEST_ASSERT(File::Delete(file_path.c_str()));
    TEST_ASSERT(!File::Delete(file_path.c_str()));

    TEST_ASSERT(File::Delete(other_file_path.c_str()));

    TEST_ASSERT(File::GetDirectoryFileCount(parent_dir.c_str()) == 0);

    TEST_ASSERT(!File::DirectoryExists(file_path.c_str()));
    TEST_ASSERT(!File::Exists(file_path.c_str()));

    TEST_ASSERT(File::DeleteRecursively(parent_dir.c_str()));

  }
#endif  // defined(WIN32) && !defined(WINCE)
  return true;
}

// friend of File class, so can't be static
bool TestSplitPath() {
  
  const std::string16 kSep(&kPathSeparator, 1);
  
  const std::string16 kFoo(STRING16(L"foo"));
  const std::string16 kBar(STRING16(L"bar"));
  const std::string16 kEmpty;
  File::PathComponents components;
  
  // '/foo' -> ['foo']
  File::SplitPath(kSep + kFoo, &components);
  TEST_ASSERT(components.size() == 1 && components[0] == kFoo);

  // '/foo/bar/' -> ['foo', 'bar']
  File::SplitPath(kSep + kFoo + kSep + kBar + kSep, &components);
  TEST_ASSERT(components.size() == 2 && 
      components[0] == kFoo &&
      components[1] == kBar);

#ifdef WIN32
  const std::string16 kDrv(STRING16(L"c:"));

  // 'c:/foo' -> ['c:\','foo']
  File::SplitPath(kDrv + kSep + kFoo, &components);
  TEST_ASSERT(components.size() == 2 && components[0] == kDrv &&
      components[1] == kFoo);
#endif  // WIN32

  return true;

}

static bool TestGetBaseName() {
  // GetBaseName
  const std::string16 kSep(&kPathSeparator, 1);
  const std::string16 kDrvSep(STRING16(L"c:"));
  const std::string16 kA(STRING16(L"a"));
  const std::string16 kADotTxt(STRING16(L"a.txt"));
  const std::string16 kFoo(STRING16(L"foo"));
  const std::string16 kEmpty;
  std::string16 out;

  // '' -> ''
  File::GetBaseName(kEmpty, &out);
  TEST_ASSERT(out == kEmpty);
  
  // 'a'->'a'
  File::GetBaseName(kA, &out);
  TEST_ASSERT(out == kA);

  // 'a.txt'->'a.txt'
  File::GetBaseName(kADotTxt, &out);
  TEST_ASSERT(out == kADotTxt);

  // '\a'->'a'
  File::GetBaseName(kSep + kA, &out);
  TEST_ASSERT(out == kA);

  // '\\a'->'a'
  File::GetBaseName(kSep + kSep + kA, &out);
  TEST_ASSERT(out == kA);

  // '\foo\a.txt'->'a.txt'
  File::GetBaseName(kSep + kFoo + kSep + kADotTxt, &out);
  TEST_ASSERT(out == kADotTxt);

  // ------ Directories -------

  // '\' -> '\'
  File::GetBaseName(kSep, &out);
  TEST_ASSERT(out == kSep);

  // '\\' -> '\'
  File::GetBaseName(kSep + kSep, &out);
  TEST_ASSERT(out == kSep);

  // '\a\' -> 'a'
  File::GetBaseName(kSep + kA + kSep, &out);
  TEST_ASSERT(out == kA);

  // -------- Windows Only Tests -------
#ifdef WIN32
  // 'c:\a'->'a'
  File::GetBaseName(kDrvSep + kSep + kA, &out);
  TEST_ASSERT(out == kA);

  // 'c:\foo\a.txt'->'a.txt'
  File::GetBaseName(kDrvSep  + kSep + kFoo + kSep + kADotTxt, &out);
  TEST_ASSERT(out == kADotTxt);

  // 'c:\\foo\\a.txt'->'a.txt'
  File::GetBaseName(kDrvSep + kSep + kSep+ kFoo + kSep + kSep + kADotTxt, &out);
  TEST_ASSERT(out == kADotTxt);

  // 'c:\' -> 'c:'
  File::GetBaseName(kDrvSep + kSep, &out);
  TEST_ASSERT(out == kDrvSep);

  // 'c:\\' -> 'c:'
  File::GetBaseName(kDrvSep + kSep + kSep, &out);
  TEST_ASSERT(out == kDrvSep);

  // Paths starting with '\\\' are illegal in win32.
  TEST_ASSERT(!File::GetBaseName(kSep + kSep + kSep, &out));

#else
  // '\\\' -> '\'
  File::GetBaseName(kSep + kSep + kSep, &out);
  TEST_ASSERT(out == kSep);
#endif  // WIN32

  return true;
}

static bool CheckDirectoryCreation(const char16 *dir) {
  return File::RecursivelyCreateDir(dir) && File::DirectoryExists(dir);  
}
