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

#ifndef GEARS_BASE_COMMON_FILE_H__
#define GEARS_BASE_COMMON_FILE_H__

#include <vector>
#include "gears/base/common/int_types.h"
#include "gears/base/common/string16.h"

class SecurityOrigin;  // base/common/security_model.h

class File {
 public:
  // Creates a new file. If the file already exists, returns false.
  // Returns true if a new file has been created.
  static bool CreateNewFile(const char16 *full_filepath);

  // Deletes a file. If the file does not exist, returns false.
  // Returns true if the file was deleted.
  static bool Delete(const char16 *full_filepath);

  // Returns true if the file exists.
  static bool Exists(const char16 *full_filepath);

  // Returns true if the directory exists.
  static bool DirectoryExists(const char16 *full_dirpath);

  // Reads the contents of the file into memory. If the file does not exist,
  // returns false. Returns true on success
  static bool ReadFileToVector(const char16 *full_filepath,
                               std::vector<uint8> *data);

  // Writes the contents of the file. If the file does not exist, returns false.
  // Existing contents are overwritten. Returns true on success
  static bool WriteVectorToFile(const char16 *full_filepath,
                                const std::vector<uint8> *data);

  // Returns the number of files and directories contained in the given
  // directory. If the directory does not exist, returns 0.
  static int GetDirectoryFileCount(const char16 *full_dirpath);

  // Returns a pointer to the last '.' within 'filename' if an extension is
  // found, or a pointer to the trailing NULL otherwise.
  static const char16 *GetFileExtension(const char16 *filename);

  // Creates a unique file in the system temporary directory.  Returns the
  // full path of the new file in 'path'.
  // Returns true if the function succeeds.  'path' is unmodified on failure.
  static bool CreateNewTempFile(std::string16 *full_filepath);

  // Creates a unique directory under the system temporary directory.  Returns
  // the full path of the new directory in 'path'.
  // Returns true if the function succeeds.  'path' is unmodified on failure.
  static bool CreateNewTempDirectory(std::string16 *full_filepath);

  // Ensures all directories along the specified path exist.  Any directories
  // that do not exist will be created. Returns true if the function succeeds.
  static bool RecursivelyCreateDir(const char16 *full_dirpath);

  // Removes the directory and all of its children. If the directory does
  // not exist, returns false. Returns true if the function succeeds.
  static bool DeleteRecursively(const char16 *full_dirpath);
  
// TODO(aa): Implement these on other platforms as needed
#if BROWSER_FF
  // Writes raw data to a file.
  static bool WriteBytesToFile(const char16 *full_filepath, const uint8 *data,
                               int length);

  // Moves a directory to a new location. Returns true if the function succeeds.
  static bool MoveDirectory(const char16 *src_path, const char16 *dest_path);
#endif

  // Clears the last file error for the current thread
  static void ClearLastFileError();

  // Gets the last file error that occurred on the current thread. If
  // there was no error, returns false and error_out is cleared.
  // Note: CreateNewFile is the only method that sets an error message
  // at this time.
  static bool GetLastFileError(std::string16 *error_out);

  struct IconData {
    int width;
    int height;
    std::vector<uint8> bytes;
  };

  // Creates a shortcut that opens a URL in a specific browser.
  //
  // link_name is a string that will be used to create the user-visible
  // shortcut name. Do not append ".lnk" or pass a path.
  //
  // launch_url is a relative URL that the browser should open when the user
  // invokes the shortcut.
  //
  // icon_data is a list of icons of different sizes to be used to generate the
  // desktop shortcut.
  static bool CreateDesktopShortcut(const std::string16 &link_name,
                                    const std::string16 &launch_url,
                                    const std::vector<IconData *> &icons);

 private:
  static void SetLastFileError(const char16 *message,
                               const char16 *filepath,
                               int error_code);

  static const char16 *kCreateFileFailedMessage;

  File() {}

  // TODO(miket): someone fix common.h so that it doesn't require a browser
  // flag to be defined! Or better yet, someone shoot common.h in the head!
  //  DISALLOW_EVIL_CONSTRUCTORS(File);
};


#endif  // GEARS_BASE_COMMON_FILE_H__
