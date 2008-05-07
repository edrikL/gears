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
// Implementation of the File class for use on POSIX compliant platforms.
// Some methods implementations are browser neutral and can be found
// in file.cc.

#ifdef WIN32
// Currently all non-win32 gears targets are POSIX compliant.
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
#include "gears/base/common/basictypes.h"
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/string_utils.h"
#include "genfiles/product_constants.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

// paths.h defines a char16 version of this, but we use UTF8 internally here
// and POSIX systems are consistent in this regard.
static const char kPathSeparatorUTF8 = '/';

// Wraps filename, "is file a directory?" pair.
class DirEntry : public std::pair<std::string, bool> {
 public:
  DirEntry(const std::string &filename, bool is_dir) :
      std::pair<std::string,bool>(filename, is_dir) {}

  const std::string &Filename() const { return first; }
  bool IsDirectory() const { return second; }
};
typedef std::vector<DirEntry> DirContentsVector;
typedef DirContentsVector::const_iterator DirContentsVectorConstIterator;

File *File::Open(const char16 *full_filepath, OpenAccessMode access_mode,
                 OpenExistsMode exists_mode) {
  scoped_ptr<File> file(new File());
  std::string file_path_utf8_;
  if (!String16ToUTF8(full_filepath, &file_path_utf8_)) {
    return false;
  }

  const char *mode = NULL;
  switch (exists_mode) {
    case NEVER_FAIL:
      break;
    case FAIL_IF_NOT_EXISTS:
      if (!File::Exists(full_filepath)) {
        return NULL;
      }
      break;
    case FAIL_IF_EXISTS:
      if (File::Exists(full_filepath)) {
        return NULL;
      }
      break;
  }
  switch (access_mode) {
    case READ:
      mode = "rb";
      break;
    case WRITE:
      // NOTE: Read will fail when opened for WRITE
    case READ_WRITE:
      // NOTE: rb+ does not create inexistant files, and w+ truncates them
      mode = File::Exists(full_filepath) ? "rb+" : "w+";
      break;
  }
  file->mode_ = access_mode;
  file->handle_.reset(fopen(file_path_utf8_.c_str(), mode));
  if (file->handle_.get() == NULL) {
    return NULL;
  }
  return file.release();
}

// Places the direct contents of the 'path' directory into results.  This
// method is not recursive.
// 'results' may not be NULL.
//
// Returns false on error in which case 'results' isn't modified.
static bool ReadDir(const std::string16 &path, DirContentsVector *results) {
  std::string path_utf8;
  if (!String16ToUTF8(path.c_str(), &path_utf8)) {
    return false;
  }
  DIR *the_dir = opendir(path_utf8.c_str());
  if (the_dir == NULL) {
    return false;
  }

  DirContentsVector local_dir_contents;
  bool error_reading_dir_contents = false;

  // Zero errno - as a NULL return value from readdir() can mean we're done
  // reading a directory OR that an error has occured, so our only way of
  // knowing the difference is via errno.
  // The need to do this arose from a unit test failing - readdir() does not
  // zero errno itself (at least under OSX).
  //
  // To clarify, the error case we're solving here, is when we reach this point
  // with errno != 0.  Then when readdir() returns NULL inside the loop to
  // signify that it's finished reading the directory - when we check errno we
  // will find a non-zero value and mistakenly think that readdir() has failed.
  //
  // We do not need to move this inside the loop, because no change will be
  // made to errno inside the loop if there are no errors, and if there are
  // we want to bail immediately.
  errno = 0;
  while (true) {
    struct dirent *dir_info = readdir(the_dir);
    if (dir_info == NULL) {
      if (errno != 0) {
        error_reading_dir_contents = true;
      }
      break;  // Reached end of directory contents.
    }
    if ((strcmp(dir_info->d_name, "..") == 0) ||
        (strcmp(dir_info->d_name, ".") == 0)) {
      continue;  // Skip parent and current directories.
    }

    std::string filename(dir_info->d_name);
    bool is_dir = (dir_info->d_type == DT_DIR);
    local_dir_contents.push_back(DirEntry(filename, is_dir));
  }

  if (closedir(the_dir) != 0) {
    return false;
  }

  if (error_reading_dir_contents) {
    return false;
  }
  results->swap(local_dir_contents);
  return true;
}

bool File::CreateNewFile(const char16 *path) {
  // TODO(fry): implement using File in file.cc
  std::string path_utf8;
  if (!String16ToUTF8(path, &path_utf8)) {
    return false;
  }

  // Create new file with permission 0600, fail if the file already exists.
  int fd = open(path_utf8.c_str(), O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    return false;
  }

  if (close(fd) != 0) {
    return false;
  }
  return true;
}

static bool StatFile(const char16 *path, struct stat *stat_data) {
  std::string path_utf8;
  if (!String16ToUTF8(path, &path_utf8)) {
    return false;
  }
  struct stat tmp;
  if (stat(path_utf8.c_str(), &tmp) != 0) {
    return false;
  }
  *stat_data = tmp;
  return true;
}

bool File::Exists(const char16 *full_filepath) {
  struct stat stat_data;
  if (!StatFile(full_filepath, &stat_data)) {
    return false;
  }

  return S_ISREG(stat_data.st_mode);
}

bool File::DirectoryExists(const char16 *full_dirpath) {
  struct stat stat_data;
  if (!StatFile(full_dirpath, &stat_data)) {
    return false;
  }

  return S_ISDIR(stat_data.st_mode);
}

int64 File::Size() {
  struct stat stat_data;
  if (fstat(fileno(handle_.get()), &stat_data) != 0) {
    return false;
  }
  return static_cast<int64>(stat_data.st_size);
}

int64 File::Read(uint8* destination, int64 max_bytes) {
  if (mode_ == WRITE) {
    // NOTE: we may have opened the file with read-write access to avoid
    // truncating it, but we still want to refuse reads
    return -1;
  }
  if (!destination || max_bytes < 0) {
    return -1;
  }

  // Read its contents into memory.
  if (max_bytes > std::numeric_limits<size_t>::max()) {  // fread limit
    max_bytes = std::numeric_limits<size_t>::max();
  }
  size_t bytes_read = fread(destination, 1, static_cast<size_t>(max_bytes),
                            handle_.get());
  if (ferror(handle_.get()) && !feof(handle_.get())) {
    return -1;
  }
  return bytes_read;
}

bool File::Seek(int64 offset, SeekMethod seek_method) {
  int whence = 0;
  switch (seek_method) {
    case SEEK_FROM_START:
      whence = SEEK_SET;
      break;
    case SEEK_FROM_CURRENT:
      whence = SEEK_CUR;
      break;
    case SEEK_FROM_END:
      whence = SEEK_END;
      break;
  }

  // handle 64-bit seek for 32-bit off_t
  while (offset > std::numeric_limits<off_t>::max()) {
    if (fseeko(handle_.get(), std::numeric_limits<off_t>::max(), whence) != 0) {
      return false;
    }
    offset -= std::numeric_limits<off_t>::max();
    whence = SEEK_CUR;
  }
  while (offset < std::numeric_limits<off_t>::min()) {
    if (fseeko(handle_.get(), std::numeric_limits<off_t>::min(), whence) != 0) {
      return false;
    }
    offset -= std::numeric_limits<off_t>::min();
    whence = SEEK_CUR;
  }

  return (fseeko(handle_.get(), static_cast<long>(offset), whence) == 0);
}

int64 File::Tell() {
  return ftello(handle_.get());
}

bool File::Truncate(int64 length) {
  if (length < 0) {
    return false;
  }
  return ftruncate(fileno(handle_.get()), length) == 0;
}


int64 File::Write(const uint8 *source, int64 length) {
  if (mode_ == READ) {
    // NOTE: fwrite doesn't fail after opening in READ mode
    return -1;
  }
  if (!source || length < 0) {
    return -1;
  }
  // can't write more data than fwrite can handle
  assert(length <= std::numeric_limits<size_t>::max());

  size_t bytes_written = fwrite(source, 1, length, handle_.get());
  if (ferror(handle_.get())) {
    return -1;
  }
  return bytes_written;
}

bool File::Delete(const char16 *full_filepath) {
  std::string path_utf8;
  if (!String16ToUTF8(full_filepath, &path_utf8)) {
    return false;
  }

  return unlink(path_utf8.c_str()) == 0;
}

int File::GetDirectoryFileCount(const char16 *full_dirpath) {
  std::string16 the_dir(full_dirpath);
  DirContentsVector dir_contents;
  bool success = ReadDir(the_dir, &dir_contents);
  if (!success) {
    return 0;
  } else {
    return dir_contents.size();
  }
}

bool File::CreateNewTempDirectory(std::string16 *full_filepath) {
  char temp_dir_template[] = "/tmp/" PRODUCT_SHORT_NAME_ASCII "TempXXXXXX";

  // mkdtemp() creates the directory with permissions set to 0700.
  if (mkdtemp(temp_dir_template) == NULL) {
    return false;
  }

  if (!UTF8ToString16(temp_dir_template, full_filepath)) {
    return false;
  }
  return true;
}

bool File::RecursivelyCreateDir(const char16 *full_dirpath) {
   // If directory already exists, no need to do anything.
  if(File::DirectoryExists(full_dirpath)) {
    return true;
  }

  File::PathComponents path_components;
  File::SplitPath(std::string16(full_dirpath), &path_components);

  std::string16 long_path;
  // Recursively create directories.
  for (File::PathComponents::const_iterator it = path_components.begin();
       it != path_components.end();
       ++it) {
    // '//', '.' & '..' shouldn't be present in the path, but if they are fail
    // hard!
    if (it->empty() || *it == STRING16(L".") || *it == STRING16(L"..")) {
      assert("Badly formed pathname" == NULL);
      return false;
    }

    long_path = long_path + kPathSeparator + *it;

    std::string path_utf8;
    if (!String16ToUTF8(long_path.c_str(), &path_utf8)) {
      return false;
    }

    // Create directory with permissions set to 0700.
    if (mkdir(path_utf8.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0) {
      // Any error value other than "directory already exists" is considered
      // fatal.
      if (errno != EEXIST) {
        return false;
      } else if (!File::DirectoryExists(long_path.c_str())) {
        return false; // We've collided with a file having the same name.
      }
    }
  }

  return true;
}

// Recursive function for use by DeleteRecursively.
static bool DeleteRecursivelyImpl(const std::string &del_path) {
  std::string16 del_path_utf16;
  if (!UTF8ToString16(del_path.c_str(), &del_path_utf16)) {
    return false;
  }
  DirContentsVector dir_contents;
  if (!ReadDir(del_path_utf16, &dir_contents)) {
    return false;
  }

  for (DirContentsVectorConstIterator it = dir_contents.begin();
       it != dir_contents.end();
       ++it) {
    std::string path_component_to_delete = del_path + kPathSeparatorUTF8 +
                                           it->Filename();
    if (it->IsDirectory()) {
      if (!DeleteRecursivelyImpl(path_component_to_delete)) {
        return false;
      }
    } else {
      if (unlink(path_component_to_delete.c_str()) != 0) {
        return false;
      }
    }
  }

  if (rmdir(del_path.c_str()) != 0) {
    return false;
  }

  return true;
}

bool File::DeleteRecursively(const char16 *full_dirpath) {
  std::string dir_to_delete;
  if (!String16ToUTF8(full_dirpath, &dir_to_delete)) {
    return false;
  }

  // We can only operate on a directory.
  if(!File::DirectoryExists(full_dirpath)) {
    return false;
  }

  // Cut off trailing slash from directory name if any.
  std::string path_sep(&kPathSeparatorUTF8, 1);
  if (EndsWith(dir_to_delete, path_sep)) {
    dir_to_delete.erase(dir_to_delete.end()-1);
  }

  return DeleteRecursivelyImpl(dir_to_delete);
}

#endif  // !WIN32
