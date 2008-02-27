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
//
// The methods of the File class implemented for use in Windows CE.
//
// Note that this file completely ignores long pathnames, an alternate solution
// for this needs to be found.
//
// Some methods implementations are browser neutral and can be found
// in file.cc.

#ifdef WINCE
#include <assert.h>
#include <windows.h>
#include <shlobj.h>
#include "common/genfiles/product_constants.h"  // from OUTDIR
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/scoped_win32_handles.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/wince_compatibility.h"

bool File::CreateNewFile(const char16 *full_filepath) {
  // Create a new file, if a file already exists this will fail
  SAFE_HANDLE safe_file_handle(::CreateFileW(full_filepath,
                                             GENERIC_WRITE,
                                             0,
                                             NULL,
                                             CREATE_NEW,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL));
  if (safe_file_handle.get() == INVALID_HANDLE_VALUE) {
    SetLastFileError(kCreateFileFailedMessage,
                     full_filepath,
                     GetLastError());
    return false;
  }
  return true;
}


bool File::Delete(const char16 *full_filepath) {
  return ::DeleteFileW(full_filepath) ? true : false;
}


bool File::Exists(const char16 *full_filepath) {
  DWORD attrs = GetFileAttributesW(full_filepath);
  return (attrs != INVALID_FILE_ATTRIBUTES) &&
         ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
}


bool File::DirectoryExists(const char16 *full_dirpath) {
  DWORD attrs = GetFileAttributesW(full_dirpath);
  return (attrs != INVALID_FILE_ATTRIBUTES) &&
         ((attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
}


int64 File::GetFileSize(const char16 *full_filepath) {
  return 0;  // Stubbed out for now
  // TODO: Implement this function (check the following code).
#if 0
  SAFE_HANDLE safe_file_handle(::CreateFileW(full_filepath,
                                             GENERIC_READ,
                                             FILE_SHARE_READ,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL));
  if (safe_file_handle.get() == INVALID_HANDLE_VALUE) {
    return 0;
  }
  return static_cast<int64>(::GetFileSize(safe_file_handle.get(), NULL));
#endif
}


int File::ReadFileSegmentToBuffer(const char16 *full_filepath,
                                  uint8* destination,
                                  int64 position,
                                  int max_bytes) {
  return 0;  // Stubbed out for now
  // TODO: Implement this function (check the following code).
#if 0
  if (max_bytes <= 0 || position < 0) {
    return 0;
  }

  SAFE_HANDLE safe_file_handle(::CreateFileW(full_filepath,
                                             GENERIC_READ,
                                             FILE_SHARE_READ,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL));
  if (safe_file_handle.get() == INVALID_HANDLE_VALUE) {
    return 0;
  }

  if (::SetFilePointer(safe_file_handle.get(), static_cast<LONG>(position),
                       NULL, FILE_BEGIN) == 0xFFFFFFFF) {
    return 0;
  }

  // Read its contents into memory.
  DWORD bytes_read;
  if (!::ReadFile(safe_file_handle.get(), destination,
                  max_bytes, &bytes_read, NULL)) {
    return 0;
  }

  return static_cast<int>(bytes_read);
#endif
}


bool File::ReadFileToVector(const char16 *full_filepath,
                            std::vector<uint8> *data) {
  // Open the file for reading.
  SAFE_HANDLE safe_file_handle(::CreateFileW(full_filepath,
                                             GENERIC_READ,
                                             FILE_SHARE_READ,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL));
  if (safe_file_handle.get() == INVALID_HANDLE_VALUE) {
    return false;
  }
  // Resize our buffer to fit the size of the file.
  // TODO(michaeln): support large files here, where len > maxInt
  DWORD file_size = ::GetFileSize(safe_file_handle.get(), NULL);
  if (file_size == INVALID_FILE_SIZE) {
    return false;
  }
  data->resize(file_size);
  if (data->size() != file_size) {
    return false;
  }

  if (file_size > 0) {
    // Read its contents into memory.
    DWORD bytes_read;
    if (!::ReadFile(safe_file_handle.get(), &(*data)[0],
                    file_size, &bytes_read, NULL)
        || (bytes_read != bytes_read)) {
      data->clear();
      return false;
    }
  }
  return true;
}


bool File::WriteVectorToFile(const char16 *full_filepath,
                             const std::vector<uint8> *data) {
  const uint8 *first_byte = data->size() ? &(data->at(0)) : NULL;
  return WriteBytesToFile(full_filepath, first_byte, data->size());
}


bool File::WriteBytesToFile(const char16 *full_filepath, const uint8 *buf,
                            int length) {
  // Open the file for writing.
  SAFE_HANDLE safe_file_handle(::CreateFileW(full_filepath,
                                             GENERIC_WRITE,
                                             0,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL));
  if (safe_file_handle.get() == INVALID_HANDLE_VALUE) {
    return false;
  }
  // Write the file.
  // TODO(michaeln): support large files here, where len > maxInt
  size_t data_size = length;
  DWORD bytes_written;
  unsigned char nothing;
  if (!::WriteFile(safe_file_handle.get(),
                   (data_size > 0) ? buf : &nothing,
                   data_size, &bytes_written, NULL)
      || (bytes_written != data_size)) {
    return false;
  }
  // Explicitly set EOF to truncate pre-existing content beyond the end of
  // the newly written content
  return SetEndOfFile(safe_file_handle.get()) ? true : false;
}


int File::GetDirectoryFileCount(const char16 *full_dirpath) {
  std::string16 find_spec(full_dirpath);
  find_spec += L"\\*";
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = FindFirstFile(find_spec.c_str(), &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    return 0;  // expected if the directory does not exist
  }
  int count = 0;
  do {
    if ((wcscmp(find_data.cFileName, L"..") == 0) ||
        (wcscmp(find_data.cFileName, L".") == 0)) {
      continue;  // don't count parent and current directories
    }
    ++count;
  } while (FindNextFile(find_handle, &find_data) != 0);
  FindClose(find_handle);
  return count;
}


bool File::CreateNewTempFile(std::string16 *path) {
  static const char16 *kTempFilePrefix = STRING16(PRODUCT_SHORT_NAME);

  // Get the system temp directory.
  wchar_t root[MAX_PATH];
  DWORD chars = GetTempPathW(MAX_PATH, root);
  if (chars >= MAX_PATH) {
    return false;
  }

  // Create a uniquely named temp file in that directory.
  // Note: GetTempFileName() uses 3 chars max of the suggested prefix
  wchar_t file[MAX_PATH];
  UINT id = GetTempFileNameW(root, kTempFilePrefix, 0, file);
  if (0 == id) {
    return false;
  }
  (*path) = file;
  return true;
}


bool File::CreateNewTempDirectory(std::string16 *path) {
  std::string16 temp;  // to avoid modifying 'path' if something fails
  if (!CreateNewTempFile(&temp)) {
    return false;
  }

  // Delete that file, and create a directory with the same name,
  // now that we know it's unique.
  if (0 == ::DeleteFileW(temp.c_str())) {
    return false;
  }
  if (0 == ::CreateDirectoryW(temp.c_str(), NULL)) {
    return false;
  }
  (*path) = temp;
  return true;
}


bool File::RecursivelyCreateDir(const char16 *full_dirpath) {
  // Note: SHCreateDirectoryEx is available in shell32.dll version 5.0+,
  // which means Win2K/XP and higher, plus WinME.
  // For Windows Mobile, we implement it in wince_compatibility.cc.
  int r = SHCreateDirectoryEx(NULL,   // parent HWND, if UI desired
                              full_dirpath,
                              NULL);  // security attributes for new folders
  if (r != ERROR_SUCCESS &&
      r != ERROR_FILE_EXISTS && r != ERROR_ALREADY_EXISTS) {
    return false;
  } else if (r == ERROR_ALREADY_EXISTS && File::Exists(full_dirpath)) {
    return false;
  }

  return true;
}

bool File::DeleteRecursively(const char16 *full_dirpath) {
  std::string16 delete_op_path(full_dirpath);
  delete_op_path += L'\0';  // SHFileOperation needs double null termination

  SHFILEOPSTRUCTW fileop = {0};
  fileop.wFunc = FO_DELETE;
  fileop.pFrom = delete_op_path.c_str();
  fileop.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;
#ifdef WINCE
  // FOF_NOERRORUI is not defined in Windows Mobile.
#else
  fileop.fFlags |= FOF_NOERRORUI;
#endif
  return (SHFileOperationW(&fileop) == 0);
}
#endif  // WINCE
