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
// File picker is not ready for official builds
#else

#ifndef GEARS_DESKTOP_FILE_DIALOG_H__
#define GEARS_DESKTOP_FILE_DIALOG_H__

#include <vector>

#include "gears/base/common/common.h"

class ModuleImplBaseClass;

// An interface to file dialogs on multiple platforms.
class FileDialog {
 public:
  // Used for constructing dialogs.
  // This is intended to allow for different types of file pickers in future.
  // For example, FILES_AND_DIRECTORIES, for a custom dialog that allows the
  // user to select files and directories at the same time.
  enum Mode {
    MULTIPLE_FILES  // one or more files
  };

  struct Filter {
    // The text the user sees. For example "Images".
    std::string16 description;

    // A semi-colon separated list of filters. For example "*.jpg;*.gif;*.png".
    std::string16 filter;
  };

  virtual ~FileDialog();

  // Displays a file dialog.
  // Returns: false on failure (user selecting cancel is not failure)
  // Parameters:
  //   filters - in - A vector of filters to add to the dialog. This must not be
  //     empty. file_dialog_utils.h has helper functions to create this from
  //     a JsArray.
  //
  //   files - out - An array of files selected by the user in placed here.
  //     If the user canceled the dialog this will be an empty array.
  //
  //   error - out - The error message if the function returned false.
  //
  virtual bool OpenDialog(const std::vector<Filter>& filters,
                          std::vector<std::string16>* selected_files,
                          std::string16* error) = 0;

 protected:
  FileDialog();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(FileDialog);
};

FileDialog* NewFileDialog(const FileDialog::Mode mode,
                          const ModuleImplBaseClass& module);

#endif  // GEARS_DESKTOP_FILE_DIALOG_H__

#endif  // OFFICIAL_BUILD
