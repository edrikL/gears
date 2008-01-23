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
// Implmenetation of the File class for use on POSIX compliant platforms.
// Some methods implementations are browser neutral and can be found
// in file.cc.

#ifdef WIN32
// Currently all non-win32 gears targets are POSIX compliant.
#else
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/string_utils.h"

// paths.h defines a char16 version of this, but we use UTF8 internally here
// and POSIX systems are consistent in this regard.
static const char kPathSeparatorUTF8 = '/';

// Wraps filename, "is file a directory?" pair.
class DirEntry : public std::pair<std::string,bool> {
 public:
  DirEntry(const std::string &filename, bool is_dir) :
      std::pair<std::string,bool>(filename, is_dir) {}
      
  const std::string &Filename() const { return first; }
  bool IsDirectory() const { return second; }
};
typedef std::vector<DirEntry> DirContentsVector;
typedef DirContentsVector::const_iterator DirContentsVectorConstIterator;

static bool ReadDir(const std::string16 &path, DirContentsVector *results) {
  std::string path_utf8;
  String16ToUTF8(path.c_str(), &path_utf8);
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
  errno = 0;
  while (true) {
    struct dirent *dir_info = readdir(the_dir);
    if (dir_info == NULL) {
      if (errno != 0) {
        error_reading_dir_contents = true;
        break;
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

// Scoped file.
class CloseFileFunctor {
 public:
  void operator()(FILE *f) const {
    if (f) { fclose(f); }
  }
};
typedef scoped_token<FILE *, CloseFileFunctor> ScopedFile;

bool File::CreateNewFile(const char16 *path) {
  std::string path_utf8;
  String16ToUTF8(path, &path_utf8);
  
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

// Retrieves the stat() struct for the given file.
static bool StatFile(const char16 *path, struct stat *stat_data) {
  std::string path_utf8;
  String16ToUTF8(path, &path_utf8);
  
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


int64 File::GetFileSize(const char16 *full_filepath) {
  struct stat stat_data;
  if (!StatFile(full_filepath, &stat_data)) {
    return 0;
  }
  return static_cast<int64>(stat_data.st_size);
}


int File::ReadFileSegmentToBuffer(const char16 *full_filepath,
                                  uint8* destination,
                                  int max_bytes,
                                  int64 position) {
  if (max_bytes <= 0 || position < 0) {
    return 0;
  }

  std::string file_path_utf8;
  String16ToUTF8(full_filepath, &file_path_utf8);
  ScopedFile scoped_file(fopen(file_path_utf8.c_str(), "rb"));
  if (scoped_file.get() == NULL) {
    return 0;
  }

  if (position != 0 && fseek(scoped_file.get(), position, SEEK_SET) != 0) {
    return 0;
  }
  
  size_t bytes_read = fread(destination, 1, max_bytes, scoped_file.get());
  
  if (ferror(scoped_file.get()) || fclose(scoped_file.release()) != 0) {
    return 0;
  }
  
  return bytes_read;
}


bool File::ReadFileToVector(const char16 *full_filepath,
                            std::vector<uint8> *data) {                      
  // Get file size.
  struct stat stat_data;
  if (!StatFile(full_filepath, &stat_data)) {
    return false;
  }
  size_t file_size = stat_data.st_size;

  std::string file_path_utf8;
  String16ToUTF8(full_filepath, &file_path_utf8);  
  ScopedFile scoped_file(fopen(file_path_utf8.c_str(), "rb"));
  if (scoped_file.get() == NULL) {
    return false;
  }
  
  std::vector<uint8> local_data;
  local_data.resize(file_size);
  if (local_data.size() != file_size) {
    return false;
  }
  
  // Read Data.
  size_t bytes_read = fread(&(local_data)[0], 1, file_size, scoped_file.get());
  
  if (bytes_read < file_size) {
    return false;
  }else if (ferror(scoped_file.get())) {
    return false;
  }
  
  if (fclose(scoped_file.release()) != 0) {
    return false;
  }
  
  data->swap(local_data);
  return true;
}

bool File::WriteVectorToFile(const char16 *full_filepath,
                             const std::vector<uint8> *data) {
  const uint8 *first_byte = data->size() ? &(data->at(0)) : NULL;
  return WriteBytesToFile(full_filepath, first_byte, data->size());
}


bool File::WriteBytesToFile(const char16 *full_filepath, const uint8 *buf,
                            int length) {
  std::string file_path_utf8;
  String16ToUTF8(full_filepath, &file_path_utf8);
  
  // Don't create a file if it doesn't exist already.
  ScopedFile scoped_file(fopen(file_path_utf8.c_str(), "rb+"));
  if (scoped_file.get() == NULL) {
    return false;
  }
  
  // Erase current contents of file.
  if (ftruncate(fileno(scoped_file.get()), 0) != 0) {
    return false;
  }
  
  // Behave the same as other platforms - don't fail on write of 0 bytes
  // if file exists.
  if (length > 0 ) {
    if (fwrite(buf, 1, length, scoped_file.get()) != 
        static_cast<size_t>(length)) {
      return false;
    }
  }
  
  if (fclose(scoped_file.release()) != 0) {
    return false;
  }
  
  return true;
}

bool File::Delete(const char16 *full_filepath) {
  std::string path_utf8;
  String16ToUTF8(full_filepath, &path_utf8);
  
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
  
  UTF8ToString16(temp_dir_template, full_filepath);
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
    String16ToUTF8(long_path.c_str(), &path_utf8);
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
static bool DeleteRecursiveImp(const std::string &del_path) {
  DirContentsVector dir_contents;
  std::string16 del_path_utf16;
  UTF8ToString16(del_path.c_str(), &del_path_utf16);
  if (!ReadDir(del_path_utf16, &dir_contents)) {
    return false;
  }

  for (DirContentsVectorConstIterator it = dir_contents.begin(); 
       it != dir_contents.end(); 
       ++it) {
       
    std::string path_component_to_delete = del_path + kPathSeparatorUTF8 + 
                                           it->Filename();
    if (it->IsDirectory()) {
      if (!DeleteRecursiveImp(path_component_to_delete)) {
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
  String16ToUTF8(full_dirpath, &dir_to_delete);

  // We can only operate on a directory.
  if(!File::DirectoryExists(full_dirpath)) {
    return false;
  }
  
  // Cut off trailing slash from directory name if any.
  std::string path_sep(&kPathSeparatorUTF8, 1);
  if (EndsWith(dir_to_delete, path_sep)) {
    dir_to_delete.erase(dir_to_delete.end()-1);
  }

  return DeleteRecursiveImp(dir_to_delete);
}

#endif  // !WIN32
