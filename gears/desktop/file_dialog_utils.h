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

#ifndef GEARS_DESKTOP_FILE_DIALOG_UTILS_H__
#define GEARS_DESKTOP_FILE_DIALOG_UTILS_H__

#include "gears/desktop/file_dialog.h"  // for struct Filter

class JsArray;

class ModuleImplBaseClass;

namespace FileDialogUtils {
  // Validates and places filters in a vector.
  // Returns: false if input is invalid
  // Parameters:
  //  filters - in - An array of pairs of strings (description,filter).
  //  out - out - The filters are placed in here.
  //  error - out - error message is placed in here
  bool FiltersToVector(const JsArray& filters,
                       std::vector<FileDialog::Filter>* out,
                       std::string16* error);

  // Creates an array of javascript objects from files.
  // Each javascript object has the following properties.
  //  name - the filename without path
  //  blob - the blob representing the contents of the file
  // Returns: false on failure
  // Parameters:
  //  selected_files - in - the list of files to process
  //  module - in - required for constructing new objects
  //  files - out - the constructed javascript array is placed in here
  //  error - out - error message is placed in here
  bool FilesToJsObjectArray(const std::vector<std::string16>& selected_files,
                            const ModuleImplBaseClass& module,
                            JsArray* files,
                            std::string16* error);
}  // namespace FileDialogUtils

#endif  // GEARS_DESKTOP_FILE_DIALOG_UTILS_H__
