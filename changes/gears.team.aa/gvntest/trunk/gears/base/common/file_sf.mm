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

// The methods of the File class implemented for use in Safari.
// Some method implementations are browser neutral and can be found
// in file.cc.

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syslimits.h>

#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/safari/scoped_cf.h"
#include "gears/base/common/paths_sf_more.h"

class CloseFileDescriptorFunctor {
public:
  void operator()(int x) const {
    if (x != -1) { close(x); }
  }
};

class CloseDIRFunctor {
public:
  void operator()(DIR *x) const {
    if (x != NULL) { closedir(x); }
  }
};

// file descriptor
typedef scoped_token<int, CloseFileDescriptorFunctor> scoped_file_descriptor;

// DIR object
typedef scoped_token<DIR *, CloseDIRFunctor> scoped_DIR;


static bool String16ToFilePath(const char16 *full_filepath, 
                               char *buffer, int len) {
  scoped_CFString path_str(CFStringCreateWithString16(full_filepath));

  return CFStringGetFileSystemRepresentation(path_str.get(), buffer, len - 1);
}


bool File::CreateNewFile(const char16 *full_filepath) {
  // Create a new file, if a file already exists this will fail
  char path[PATH_MAX];
  
  if (!String16ToFilePath(full_filepath, path, PATH_MAX))
    return false;
  
  scoped_file_descriptor 
    safe_file_descriptor(open(path, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR));
  
  if (safe_file_descriptor.get() == -1)
    return false;
  
  return true;
}


bool File::Delete(const char16 *full_filepath) {
  char path[PATH_MAX];
  
  if (!String16ToFilePath(full_filepath, path, PATH_MAX))
    return false;
  
  return unlink(path) != -1 ? true : false;
}


bool File::Exists(const char16 *full_filepath) {
  char path[PATH_MAX];
  
  if (!String16ToFilePath(full_filepath, path, PATH_MAX))
    return false;

  struct stat path_stat;

  if (stat(path, &path_stat) == -1)
    return false;
  
  // Regular file only -- no pipe, dir, socket, block
  return (path_stat.st_mode & S_IFREG);
}


bool File::DirectoryExists(const char16 *full_dirpath) {
  char path[PATH_MAX];
  
  if (!String16ToFilePath(full_dirpath, path, PATH_MAX))
    return false;

  struct stat path_stat;
  
  if (stat(path, &path_stat) == -1)
    return false;
  
  return (path_stat.st_mode & S_IFDIR);
}


bool File::ReadFileToVector(const char16 *full_filepath,
                            std::vector<uint8> *data) {
  char path[PATH_MAX];
  
  if (!String16ToFilePath(full_filepath, path, PATH_MAX))
    return false;
  
  scoped_file_descriptor safe_file_descriptor(open(path, O_RDONLY, 0));

  struct stat path_stat;
  
  if (fstat(safe_file_descriptor.get(), &path_stat) == -1)
    return false;

  // 64bit integer
  off_t file_size = path_stat.st_size; 

  // Resize our buffer to fit the size of the file.
  // TODO(michaeln): support large files here, where len > maxInt
  data->resize(file_size);
  if (data->size() != file_size) {
    return false;
  }

  if (file_size > 0) {
    // Read its contents into memory.
    if (file_size != read(safe_file_descriptor.get(), &(*data)[0],
                          file_size)) {
      data->clear();
      return false;
    }
  }

  return true;
}


bool File::WriteVectorToFile(const char16 *full_filepath,
                             const std::vector<uint8> *data) {
  char path[PATH_MAX];
  
  if (!String16ToFilePath(full_filepath, path, PATH_MAX))
    return false;
  
  // Open the file for writing.
  scoped_file_descriptor
    safe_file_descriptor(open(path, O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR));
  
  if (safe_file_descriptor.get() == -1)
    return false;

  // Write the file.
  // TODO(michaeln): support large files here, where len > maxInt
  size_t data_size = data->size();
  unsigned char nothing;
  
  if (data_size != (size_t)write(safe_file_descriptor.get(), 
                                 (data_size > 0) ? &(*data)[0] : &nothing,
                                 data_size)) {
    return false;
  }

  return true;
}


int File::GetDirectoryFileCount(const char16 *full_dirpath) {
  if (!File::DirectoryExists(full_dirpath))
    return 0;
  
  char path[PATH_MAX];
  
  if (!String16ToFilePath(full_dirpath, path, PATH_MAX))
    return 0;
  
  scoped_DIR safe_dir(opendir(path));
  
  if (!safe_dir.get())
    return 0;
  
  struct dirent *entry;
  int count = 0;
  
  while ((entry = readdir(safe_dir.get()))) {
    if (!strcmp(entry->d_name, ".") || (!strcmp(entry->d_name, "..")))
      continue;
    ++count;
  }

  return count;
}


bool File::CreateNewTempFile(std::string16 *path) {
  static const char *kTempFilePrefix = PRODUCT_SHORT_NAME_ASCII;
  char temp_path[PATH_MAX];
  
  snprintf(temp_path, PATH_MAX - 1, "/tmp/%sXXXXXX", kTempFilePrefix);
  scoped_file_descriptor safe_file_descriptor(mkstemp(temp_path));
  
  if (safe_file_descriptor.get() == -1)
    return false;

  UTF8ToString16(temp_path, path);
  return true;
} 


bool File::CreateNewTempDirectory(std::string16 *path) {
  static const char *kTempFilePrefix = PRODUCT_SHORT_NAME_ASCII;
  char temp_path[PATH_MAX];
  
  snprintf(temp_path, PATH_MAX - 1, "/tmp/%sXXXXXX", kTempFilePrefix);
  
  if (!mkdtemp(temp_path))
    return false;

  UTF8ToString16(temp_path, path);
  return true;
}


bool File::RecursivelyCreateDir(const char16 *full_dirpath) {
  NSString *path = [NSString stringWithString16:full_dirpath];
  return [GearsPathUtilities ensureDirectoryPathExists:path];
}


bool File::DeleteRecursively(const char16 *full_dirpath) {
  NSString *path = [NSString stringWithString16:full_dirpath];
  return [[NSFileManager defaultManager] removeFileAtPath:path handler:nil];
}

