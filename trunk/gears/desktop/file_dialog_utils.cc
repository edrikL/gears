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
#include "gears/blob/file_blob.h"
#include "gears/third_party/linked_ptr/linked_ptr.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#if BROWSER_FF
  #include "gears/blob/blob_ff.h"
#elif BROWSER_IE
  #include "gears/blob/blob_ie.h"
#endif  // BROWSER_xyz

bool FileDialogUtils::FiltersToVector(const JsArray* filters,
                                      std::vector<FileDialog::Filter>* out,
                                      std::string16* error) {
  if (!filters) {
    // add default
    FileDialog::Filter filter = { STRING16(L"All Files"), STRING16(L"*.*") };
    out->push_back(filter);
    return true;
  }

  // check array length
  int length = 0;
  if (!filters->GetLength(&length)) {
    *error = STRING16(L"Failed to get filters array length.");
    return false;
  }
  if (length <= 0 || length % 2 != 0) {
    *error = STRING16(L"Array length must be a factor of two.");
    return false;
  }

  // add filters to vector
  FileDialog::Filter filter;
  for (int i = 0; i + 1 < length; i += 2) {
    if (!filters->GetElementAsString(i, &filter.description)
        || !filters->GetElementAsString(i + 1, &filter.filter)) {
      *error = STRING16(L"Failed to retrieve strings from filters array.");
      return false;
    }

    out->push_back(filter);
  }

  return true;
}

// Creates a new COM blob. This holds the internal BlobInterface.
// Returns: false on failure
// Parameters:
//  module - in - used for constructing objects
//  blob_internal - in - base acquires a reference to this blob
//  base - out - addrefed interface placed in here
//  error - out - error message is placed here
static bool CreateBlob(const ModuleImplBaseClass& module,
                       BlobInterface* blob_internal,
                       GearsBlobInterface** base,
                       std::string16* error) {
#if BROWSER_FF

    scoped_ptr<GearsBlob> gears_blob(new GearsBlob());
    if (!gears_blob.get()) {
      *error = STRING16(L"Failed to create blob.");
      return false;
    }

    if (!gears_blob->InitBaseFromSibling(&module)) {
      *error = STRING16(L"Initializing base class failed.");
      return false;
    }

    gears_blob->Reset(blob_internal);

    // disambiguate nsISupports
    *base = gears_blob.release();
    return true;

#elif BROWSER_IE

    CComObject<GearsBlob>* gears_blob_create = NULL;
    HRESULT hr = CComObject<GearsBlob>::CreateInstance(&gears_blob_create);
    if (FAILED(hr)) {
      *error = STRING16(L"Could not create GearsBlob.");
      return false;
    }
    // CComPtr will AddRef()
    CComPtr<CComObject<GearsBlob> > gears_blob(gears_blob_create);

    if (!gears_blob->InitBaseFromSibling(&module)) {
      *error = STRING16(L"Initializing base class failed.");
      return false;
    }

    gears_blob->Reset(blob_internal);

    // disambiguate IUnknown
    CComQIPtr<GearsBlobInterface> blob_external = gears_blob;
    if (!blob_external) {
      *error = STRING16(L"Could not get GearsBlob interface.");
      return false;
    }
    *base = blob_external.Detach();
    // CComPtr and CComQIPtr both dervie from CComPtrBase and therefore they
    // Release() when going out of scope. CComPtr d'tor will Release() but
    // CComQIPtr d'tor will not because it has been Detached. This leaves
    // a reference count of 1.
    return true;

#else

  return false;

#endif  // BROWSER_xyz
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
      blobs.push_back(scoped_refptr<BlobInterface>(new FileBlob(it->c_str())));
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
    scoped_ptr<JsObject> obj(js_runner->NewObject(NULL));
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
    GearsBlobInterface* base = NULL;
    if (!CreateBlob(module, blobs[i].get(), &base, error))
      return false;

#if BROWSER_IE
    assert(base->AddRef() == 2);
    assert(base->Release() == 1);
#endif

    if (!obj->SetPropertyModule(STRING16(L"blob"), base)) {
      *error = STRING16(L"Failed to set blob property on File.");
      base->Release();
      return false;
    }
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
