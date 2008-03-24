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
// The methods of the File class with a Windows-specific implementation.

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented
#if defined(WIN32) && !defined(WINCE)
#include <assert.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>
#include <wininet.h>

#include <limits>

#include "common/genfiles/product_constants.h"  // from OUTDIR
#include "gears/base/common/common.h"
#include "gears/base/common/file.h"
#include "gears/base/common/int_types.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/png_utils.h"
#include "gears/base/common/scoped_win32_handles.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/localserver/common/http_constants.h"

// Long Path Prefix - '\\?\' used for handling paths longer than MAX_PATH.
static const std::string16 kLongPathPrefix(STRING16(L"\\\\?\\"));

// Long Path Prefix -  '\\?\UNC\' used for network shares.
static const std::string16 kLongPathPrefixUNC(STRING16(L"\\\\?\\UNC\\"));

// '\\' Prefix for network shares.
static const std::string16 kSharePrefix(STRING16(L"\\\\"));

// Prepend long path prefix onto pathname so we can handle long filenames.
// This function does nothing if the prefix already exists.
static std::string16 ToLongPath(const std::string16 &path) {
  if (StartsWith(path, kLongPathPrefix) || 
      StartsWith(path, kLongPathPrefixUNC)) {
    return path;
  }

  if (StartsWith(path, kSharePrefix)) {
    return kLongPathPrefixUNC + path.substr(kSharePrefix.length());
  } else {
    return kLongPathPrefix + path;
  }
}

// Joins 2 paths together.
static std::string16 JoinPath(const std::string16 &path1, 
                              const std::string16 &path2) {
  static std::string16 path_sep(&kPathSeparator, 1);
  return path1 + path_sep + path2;
}

// Is this path a file share?
static bool IsFileShare(const std::string16 &path) {
  if (StartsWith(path, kLongPathPrefixUNC)) {
    return true;
 } else if (!StartsWith(path, kLongPathPrefix) && 
            StartsWith(path, kSharePrefix)) {
    return true;
 }
 return false;
}

// Given a file share this function modifies the original path by removing
// the server name.
//
// Input: Full path of windows share passed in, in path parameter e.g.
// Output: server_name - '\\server' part of share path.
//         share_path - '\\server' prefix is stripped off.
static void SplitServerNameOffShare(std::string16 *share_path, 
                                    std::string16 *server_name) {
  assert(share_path);
  assert(server_name);
  assert(share_path->length() > (kSharePrefix.length() + 2));

  if (StartsWith(*share_path, kLongPathPrefixUNC)) {
    // Paths of the form '\\?\UNC\server_name\rest_of_path'.
    // Strip off everything before server_name.
    *share_path = share_path->substr(kLongPathPrefixUNC.length());
  } else {
    // Paths of the form '\\server_name\rest_of_path'
    // Strip \\ off the '\\' prefix.
    *share_path = share_path->substr(kSharePrefix.length());
  }

  size_t end_of_servername_ofs = share_path->find(kPathSeparator);
  assert(end_of_servername_ofs != std::string16::npos);

  *server_name = kSharePrefix + share_path->substr(0, end_of_servername_ofs);
  share_path->erase(0, end_of_servername_ofs);
}

bool File::CreateNewFile(const char16 *full_filepath) {
  std::string16 long_path = ToLongPath(full_filepath);
  // Create a new file, if a file already exists this will fail
  SAFE_HANDLE safe_file_handle(::CreateFileW(long_path.c_str(),
                                             GENERIC_WRITE,
                                             0,
                                             NULL,
                                             CREATE_NEW,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL));
  if (safe_file_handle.get() == INVALID_HANDLE_VALUE) {
    SetLastFileError(kCreateFileFailedMessage, full_filepath, GetLastError());
    return false;
  }
  return true;
}


bool File::Delete(const char16 *full_filepath) {
  std::string16 long_path = ToLongPath(std::string16(full_filepath));
  return ::DeleteFileW(long_path.c_str()) ? true : false;
}


bool File::Exists(const char16 *full_filepath) {
  std::string16 long_path = ToLongPath(std::string16(full_filepath));
  DWORD attrs = ::GetFileAttributesW(long_path.c_str());
  return (attrs != INVALID_FILE_ATTRIBUTES) &&
         ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0);
}


bool File::DirectoryExists(const char16 *full_dirpath) {
  std::string16 long_path = ToLongPath(std::string16(full_dirpath));
  DWORD attrs = ::GetFileAttributesW(long_path.c_str());
  return (attrs != INVALID_FILE_ATTRIBUTES) &&
         ((attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
}


int64 File::GetFileSize(const char16 *full_filepath) {
  std::string16 long_path = ToLongPath(std::string16(full_filepath));
  SAFE_HANDLE safe_file_handle(::CreateFileW(long_path.c_str(),
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
}


int64 File::ReadFileSegmentToBuffer(const char16 *full_filepath,
                                    uint8* destination,
                                    int64 position,
                                    int64 max_bytes) {
  if (max_bytes < 0 || position < 0) {
    return -1;
  }

  std::string16 long_path = ToLongPath(std::string16(full_filepath));
  SAFE_HANDLE safe_file_handle(::CreateFileW(long_path.c_str(),
                                             GENERIC_READ,
                                             FILE_SHARE_READ,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL));
  if (safe_file_handle.get() == INVALID_HANDLE_VALUE) {
    return -1;
  }

  LARGE_INTEGER li_pos;
  li_pos.QuadPart = position;
  if (!::SetFilePointerEx(safe_file_handle.get(), li_pos, NULL, FILE_BEGIN)) {
    return -1;
  }

  // Read its contents into memory.
  if (max_bytes > std::numeric_limits<DWORD>::max()) {  // ReadFile limit
    max_bytes = std::numeric_limits<DWORD>::max();
  }
  DWORD bytes_read;
  if (!::ReadFile(safe_file_handle.get(), destination,
                  static_cast<DWORD>(max_bytes), &bytes_read, NULL)) {
    return -1;
  }

  return bytes_read;
}


bool File::ReadFileToVector(const char16 *full_filepath,
                            std::vector<uint8> *data) {
  // Open the file for reading.
  std::string16 long_path = ToLongPath(std::string16(full_filepath));
  SAFE_HANDLE safe_file_handle(::CreateFileW(long_path.c_str(),
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
        || (bytes_read != file_size)) {
      data->clear();
      return false;
    }
  }
  return true;
}


bool File::WriteVectorToFile(const char16 *full_filepath,
                             const std::vector<uint8> *data) {
  const uint8 *first_byte = data->size() ? &(data->at(0)) : NULL;
  return WriteBytesToFile(full_filepath, first_byte, 
                          static_cast<int>(data->size()));
}


bool File::WriteBytesToFile(const char16 *full_filepath, const uint8 *buf,
                            int length) {
  // Open the file for writing.
  std::string16 long_path = ToLongPath(std::string16(full_filepath));
  SAFE_HANDLE safe_file_handle(::CreateFileW(long_path.c_str(),
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
  if (!::WriteFile(safe_file_handle.get(), (data_size > 0) ? buf : &nothing,
                   static_cast<DWORD>(data_size), &bytes_written, NULL)
      || (bytes_written != data_size)) {
    return false;
  }
  // Explicitly set EOF to truncate pre-existing content beyond the end of
  // the newly written content
  return SetEndOfFile(safe_file_handle.get()) ? true : false;
}


int File::GetDirectoryFileCount(const char16 *full_dirpath) {
  std::string16 find_spec = ToLongPath(full_dirpath);

  // If path ends with trailing '\' then sanitize it before passing into
  // FindFirstFile.
  size_t find_spec_length = find_spec.length();
  if (find_spec_length > 1 && 
      find_spec[find_spec_length - 1] == kPathSeparator) {
    find_spec.erase(find_spec_length - 1);
  }

  find_spec += L"\\*";
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = ::FindFirstFileW(find_spec.c_str(), &find_data);
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
  } while (::FindNextFile(find_handle, &find_data) != 0);
  ::FindClose(find_handle);
  return count;
}


bool File::CreateNewTempFile(std::string16 *path) {
  static const char16 *kTempFilePrefix = STRING16(PRODUCT_SHORT_NAME);

  // Get the system temp directory.
  wchar_t root[MAX_PATH];
  DWORD chars = ::GetTempPathW(MAX_PATH, root);
  if (chars >= MAX_PATH) {
    return false;
  }

  // Create a uniquely named temp file in that directory.
  // Note: GetTempFileName() uses 3 chars max of the suggested prefix
  wchar_t file[MAX_PATH];
  UINT id = ::GetTempFileNameW(root, kTempFilePrefix, 0, file);
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
  // In order to handle long path names reliably we have to do this ourselves
  // rather than call SHCreateDirectoryEx().

  // If directory already exists, no need to do anything.
  if(File::DirectoryExists(full_dirpath)) {
    return true;
  }

  // Init long path with the \\?\ prefix in case of file or \\?\UNC\server_name 
  // in case of a file share.  This so we can accumulate the path for the 
  // directories without prepending the prefix each time.
  std::string16 long_path;
  std::string16 full_path_to_split(full_dirpath);

  // Strip off server name from file share.
  if (IsFileShare(full_dirpath)) {
    std::string16 server_name;
    SplitServerNameOffShare(&full_path_to_split, &server_name);
    long_path = ToLongPath(server_name);
    // Long path is now \\?\UNC\server_name, and full_path_to_split is the
    // remainder of the original path (with the \\server prefix stripped off).
  } else {
    long_path = kLongPathPrefix;

    // Strip long file prefix if it exists.
    if (StartsWith(full_path_to_split, kLongPathPrefix)) {
      full_path_to_split.erase(0, kLongPathPrefix.length());
    }
  }

  File::PathComponents path_components;
  File::SplitPath(full_path_to_split, &path_components);

  // If first component in path is drive letter then remove that, as we
  // definitely don't need to create it.
  if (path_components.size() > 0 && path_components[0].length() == 2) {
    const std::string16 &drv_letter = path_components[0];

    if (iswalpha(drv_letter[0]) && drv_letter[1] == L':') {
      long_path += drv_letter;
      path_components.erase(path_components.begin());
    }
  }

  // Recursively create directories.
  for (File::PathComponents::const_iterator it = path_components.begin();
       it != path_components.end();
       ++it) {

    // Skip zero length components (If \\ was specified when creating path).
    if (it->empty()) {
      continue;
    }

    long_path = JoinPath(long_path, *it);

    if (!CreateDirectoryW(long_path.c_str(), NULL)) {
      DWORD err = GetLastError();

      // Any error value other than "directory already exists" is considered
      // fatal.
      if (err != ERROR_ALREADY_EXISTS) {
        return false;
      } else if (!File::DirectoryExists(long_path.c_str())) {
        return false; // We've collided with a file having the same name.
      }
    }
  }

  return true;
}

// Recursive internal function for use by DeleteRecursively.
static bool DeleteRecursiveImp(const std::string16 &del_path) {
  assert(StartsWith(del_path, kLongPathPrefix));

  // First recursively delete directory contents
  std::string16 find_spec = del_path + L"\\*";
  WIN32_FIND_DATA find_data;
  HANDLE find_handle = ::FindFirstFileW(find_spec.c_str(), 
                                        &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  do {
    if ((wcscmp(find_data.cFileName, L"..") == 0) ||
        (wcscmp(find_data.cFileName, L".") == 0)) {
      continue;  // nothing to do for parent or current directory
    }

    bool item_is_dir = (find_data.dwFileAttributes & 
      FILE_ATTRIBUTE_DIRECTORY) != 0;

    std::string16 child = JoinPath(del_path, find_data.cFileName);
    if (item_is_dir) {
      if (!DeleteRecursiveImp(child)) {
          return false;
      }
    } else {
      if (::DeleteFileW(child.c_str()) == 0) {
        return false;
      }
    }

  } while (::FindNextFile(find_handle, &find_data) != 0);
  if (::FindClose(find_handle) == 0) {
    return false;
  }

  if (::RemoveDirectoryW(del_path.c_str()) == 0) {
    return false;
  }
  return true;
}

bool File::DeleteRecursively(const char16 *full_dirpath) {
  std::string16 dir_to_delete(full_dirpath);
  dir_to_delete = ToLongPath(dir_to_delete);

  // Cut off trailing slashes.
  size_t path_length = dir_to_delete.length();
  while (path_length > 0 && dir_to_delete[path_length - 1] == kPathSeparator) {
    dir_to_delete.erase(path_length - 1);
    path_length -= 1;
  }

  DWORD attributes = ::GetFileAttributesW(dir_to_delete.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  bool is_dir = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  if (!is_dir) {  // We can only operate on a directory.
    return false;
  }

  return DeleteRecursiveImp(dir_to_delete);
}
#endif  // #if defined(WIN32) && !defined(WINCE)
