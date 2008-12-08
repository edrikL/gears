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

#include "gears/desktop/drag_and_drop_utils_common.h"

#include "gears/base/common/base_class.h"
#include "gears/base/common/file.h"
#include "gears/base/common/mime_detect.h"
#include "gears/desktop/file_dialog.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

static bool AppendStringsToJsArray(const std::set<std::string16> &strings,
                                   JsArray *array) {
  int length = 0;
  if (!array || !array->GetLength(&length)) {
    return false;
  }
  for (std::set<std::string16>::const_iterator i = strings.begin();
       i != strings.end(); ++i) {
    if (!array->SetElementString(length++, *i)) {
      return false;
    }
  }
  return true;
}

FileDragAndDropMetaData::FileDragAndDropMetaData() {
  Reset();
}

bool FileDragAndDropMetaData::IsEmpty() {
  return filenames_.empty();
}

void FileDragAndDropMetaData::Reset() {
  filenames_.clear();
  extensions_.clear();
  mime_types_.clear();
  total_bytes_ = 0;
}

void FileDragAndDropMetaData::SetFilenames(
      std::vector<std::string16> &filenames) {
  assert(IsEmpty());
  filenames_.swap(filenames);
  for (std::vector<std::string16>::iterator i = filenames_.begin();
       i != filenames_.end(); ++i) {
    std::string16 &filename = *i;
    // TODO(nigeltao): Should we also keep an array of per-file MIME types,
    // not just the overall set of MIME types? If so, the mimeType should
    // probably be a property of the file JavaScript object (i.e. the thing
    // with a name and blob property), not a separate array to the files array.
    mime_types_.insert(DetectMimeTypeOfFile(filename));
    // TODO(nigeltao): Decide whether we should insert ".txt" or "txt" -
    // that is, does the file extension include the dot at the start.
    // We should do whatever desktop.openFiles does.
    std::string16 extension(File::GetFileExtension(filename.c_str()));
    if (!extension.empty()) {
      extensions_.insert(extension);
    }
    int64 bytes = File::GetFileSize(filename.c_str());
    if (bytes != File::kInvalidSize) {
      total_bytes_ += bytes;
    }
  }
}

bool FileDragAndDropMetaData::ToJsObject(
    ModuleEnvironment *module_environment,
    bool is_in_a_drop,
    JsObject *object_out,
    std::string16 *error_out) {
  // TODO(nigeltao): Error checking. We should return empty (or 0) instead of
  // partial results, in case of failure.

  object_out->SetPropertyInt(STRING16(L"fileCount"), filenames_.size());
  object_out->SetPropertyDouble(
      STRING16(L"fileTotalBytes"),
      static_cast<double>(total_bytes_));

  scoped_ptr<JsArray> extensions_array(
      module_environment->js_runner_->NewArray());
  AppendStringsToJsArray(extensions_, extensions_array.get());
  object_out->SetPropertyArray(STRING16(L"fileExtensions"),
      extensions_array.get());

  scoped_ptr<JsArray> mime_types_array(
      module_environment->js_runner_->NewArray());
  AppendStringsToJsArray(mime_types_, mime_types_array.get());
  object_out->SetPropertyArray(STRING16(L"fileMimeTypes"),
      mime_types_array.get());

  if (is_in_a_drop) {
    scoped_ptr<JsArray> file_array(
        module_environment->js_runner_->NewArray());
    if (!FileDialog::FilesToJsObjectArray(
            filenames_,
            module_environment,
            file_array.get(),
            error_out)) {
      assert(!error_out->empty());
      return false;
    }
    object_out->SetPropertyArray(STRING16(L"files"), file_array.get());
  }
  return true;
}
