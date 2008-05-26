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

#include "gears/desktop/file_dialog_utils.h"

#include "gears/base/common/base_class.h"
#include "gears/base/common/file.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/blob/blob.h"
#include "gears/blob/file_blob.h"
#include "third_party/linked_ptr/linked_ptr.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

bool FileDialogUtils::FiltersToVector(const JsArray& filters,
                                      std::vector<FileDialog::Filter>* out,
                                      std::string16* error) {
  assert(out);
  assert(error);
  // check array length
  int length = 0;
  if (!filters.GetLength(&length)) {
    *error = STRING16(L"Failed to get filters array length.");
    return false;
  }
  if (length < 0 || length % 2 != 0) {
    *error = STRING16(L"Array length must be a factor of two.");
    return false;
  }

  // add filters to vector
  FileDialog::Filter filter;
  for (int i = 0; i + 1 < length; i += 2) {
    if (!filters.GetElementAsString(i, &filter.description) ||
        !filters.GetElementAsString(i + 1, &filter.filter)) {
      *error = STRING16(L"Failed to retrieve strings from filters array.");
      return false;
    }

    out->push_back(filter);
  }

  return true;
}

bool FileDialogUtils::FilesToJsObjectArray(
    const std::vector<std::string16>& selected_files,
    const ModuleImplBaseClass& module,
    JsArray* files,
    std::string16* error) {
  // selected_files, blobs and base_names are aligned
  const int size = selected_files.size();
  std::vector<std::string16> base_names;
  base_names.reserve(size);
  std::vector< scoped_refptr<BlobInterface> > blobs;
  blobs.reserve(size);

  // create all blobs and base names
  for (std::vector<std::string16>::const_iterator it = selected_files.begin();
      it != selected_files.end(); ++it) {
    std::string16 base_name;
    if (File::GetBaseName(*it, &base_name)) {
      base_names.push_back(base_name);
      blobs.push_back(new FileBlob(it->c_str()));
    } else {
      *error = STRING16(L"Failed to create blob.");
      return false;
    }
  }

  // create javascript objects and place in javascript array
  for (int i = 0; i < size; ++i) {
    JsRunnerInterface* js_runner = module.GetJsRunner();
    if (!js_runner) {
      *error = STRING16(L"Failed to get JsRunnerInterface.");
      return false;
    }
    scoped_ptr<JsObject> obj(js_runner->NewObject());
    if (!obj.get()) {
      *error = STRING16(L"Failed to create javascript object.");
      return false;
    }

    if (!obj->SetPropertyString(STRING16(L"name"), base_names[i])) {
      *error = STRING16(L"Failed to set name property on File.");
      return false;
    }

#ifdef OFFICIAL_BUILD
  // Blob support is not ready for prime time yet
#else
    scoped_refptr<GearsBlob> gears_blob;
    CreateModule<GearsBlob>(module.GetJsRunner(), &gears_blob);
    if (!gears_blob->InitBaseFromSibling(&module)) {
      *error = STRING16(L"Initializing base class failed.");
      return false;
    }
    if (!obj->SetPropertyDispatcherModule(STRING16(L"blob"),
                                          gears_blob.get())) {
      *error = STRING16(L"Failed to set blob property on File.");
      return false;
    }
    gears_blob->Reset(blobs[i].get());
#endif  // OFFICIAL_BUILD

    // JsArray takes the javascript token out of JsObject. Hence, the JsObject
    // wrapper around the token still needs to be deleted. This is why obj.get()
    // is used instead of release().
    if (!files->SetElementObject(i, obj.get())) {
      *error = STRING16(L"Failed to add File to array.");
      return false;
    }
  }

  return true;
}

#endif  // OFFICIAL_BUILD
