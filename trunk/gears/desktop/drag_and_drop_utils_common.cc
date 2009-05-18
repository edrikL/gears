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

#if GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM

#include "gears/base/common/base_class.h"
#include "gears/base/common/file.h"
#include "gears/base/common/mime_detect.h"
#include "gears/desktop/file_dialog.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

static bool ToJsArray(std::set<std::string16> const &s, JsArray *array) {
  if (!array) return false;

  std::set<std::string16>::const_iterator it = s.begin();
  for (int index = 0; it != s.end(); ++it) {
    if (!it->empty() && !array->SetElementString(index++, *it))
      return false;
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
  has_files_ = false;
}

void FileDragAndDropMetaData::SetFilenames(std::vector<std::string16> &names) {
  assert(IsEmpty());

  has_files_ = true;
  filenames_.swap(names);
  for (std::vector<std::string16>::iterator i = filenames_.begin();
       i != filenames_.end(); ++i) {
    const std::string16 &filename = *i;
    mime_types_.insert(DetectMimeTypeOfFile(filename));
    extensions_.insert(File::GetFileExtension(filename.c_str()));
    int64 bytes = File::GetFileSize(filename.c_str());
    if (bytes != File::kInvalidSize) {
      total_bytes_ += bytes;
    }
  }
}

bool FileDragAndDropMetaData::ToJsObject(ModuleEnvironment *module_environment,
                                         bool is_in_a_drop_event,
                                         JsObject *object_out,
                                         std::string16 *error_out) {
  JsRunnerInterface *runner = module_environment->js_runner_;
  if (!has_files_)
    return false;

  static const std::string16 kCount(STRING16(L"count"));
  if (!object_out->SetPropertyInt(kCount, filenames_.size()))
    return false;

  static const std::string16 kBytes(STRING16(L"totalBytes"));
  double total_bytes = static_cast<double>(total_bytes_);
  if (!object_out->SetPropertyDouble(kBytes, total_bytes))
    return false;

  static const std::string16 kExtensions(STRING16(L"extensions"));
  scoped_ptr<JsArray> extensions_array(runner->NewArray());
  if (!ToJsArray(extensions_, extensions_array.get()))
    return false;
  if (!object_out->SetPropertyArray(kExtensions, extensions_array.get()))
    return false;

  static const std::string16 kMimes(STRING16(L"mimeTypes"));
  scoped_ptr<JsArray> mime_array(runner->NewArray());
  if (!ToJsArray(mime_types_, mime_array.get()))
    return false;
  if (!object_out->SetPropertyArray(kMimes, mime_array.get()))
    return false;

  if (!is_in_a_drop_event)
    return true;

  scoped_ptr<JsArray> file_array(runner->NewArray());
  if (!file_array.get())
    return false;

  if (!FileDialog::FilesToJsObjectArray(
          filenames_, module_environment, file_array.get(), error_out)) {
    assert(!error_out->empty());
    return false;
  }

  static const std::string16 kFiles(STRING16(L"files"));
  return object_out->SetPropertyArray(kFiles, file_array.get());
}

#endif  // GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM

