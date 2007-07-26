// Copyright 2006, Google Inc.
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

#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/scoped_win32_handles.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"
#include "gears/localserver/ie/file_submit_behavior.h"

static const wchar_t *kMissingFilename = L"Unknown";

const wchar_t *SubmitFileBehavior::kName_DispName = L"name";
const wchar_t *SubmitFileBehavior::kCapturedUrl_DispName = L"capturedUrl";


//------------------------------------------------------------------------------
// InitPageOrigin
//------------------------------------------------------------------------------
void SubmitFileBehavior::InitPageOrigin(const SecurityOrigin &origin) {
  page_origin_ = origin;
}


//------------------------------------------------------------------------------
// IElementBehavior::Init
//------------------------------------------------------------------------------
HRESULT SubmitFileBehavior::Init(IElementBehaviorSite *behavior_site) {
  behavior_site_ = behavior_site;

  // Don't allow this behavior to be attached to <input> elements. IE does
  // not call GetSubmitInfo() for behaviors attached to them.
  CComPtr<IHTMLElement> element;
  HRESULT hr = GetHTMLElement(&element);
  if (FAILED(hr)) {
    return hr;
  }
  CComQIPtr<IHTMLInputElement> input(element);
  if (input) {
    ATLTRACE(L"SubmitFileBehavior::Init"
             L" - cannot attach to an <input> element\n");
    return E_FAIL;
  }

  return S_OK;
}


//------------------------------------------------------------------------------
// IElementBehavior::Detach
//------------------------------------------------------------------------------
HRESULT SubmitFileBehavior::Detach(void) {
  Reset();
  behavior_site_.Release();
  return S_OK;
}


//------------------------------------------------------------------------------
// IElementBehavior::Notify
//------------------------------------------------------------------------------
HRESULT SubmitFileBehavior::Notify(long event, VARIANT *var) {
  if (!behavior_site_) {
    return E_UNEXPECTED;
  }

  switch (event) {
    // End tag of element has been parsed (we can get at attributes)
    case BEHAVIOREVENT_CONTENTREADY: {
      CComVariant name_value;
      HRESULT hr = GetHTMLElementAttributeValue(kName_DispName,
                                                &name_value);
      if (SUCCEEDED(hr) && (name_value.vt == VT_BSTR)) {
        name_ = name_value.bstrVal;
      }
      CComVariant captured_url_value;
      hr = GetHTMLElementAttributeValue(kCapturedUrl_DispName,
                                        &captured_url_value);
      if (SUCCEEDED(hr) && (captured_url_value.vt == VT_BSTR)) {
        SetCapturedUrl(captured_url_value.bstrVal);
      }
      return S_OK;
    }

    // HTML document has been parsed (we can get at the document object model)
    case BEHAVIOREVENT_DOCUMENTREADY:
      return S_OK;

    default:
      return S_OK;
  }
}


//------------------------------------------------------------------------------
// IElementBehaviorSubmit::Reset
//------------------------------------------------------------------------------
HRESULT SubmitFileBehavior::Reset(void) {
  captured_url_.Empty();
  if (!temp_file_.empty()) {
    DeleteFile(temp_file_.c_str());
    temp_file_.clear();
  }
  if (!temp_folder_.empty()) {
    RemoveDirectory(temp_folder_.c_str());
    temp_folder_.clear();
  }

  return S_OK;
}


//------------------------------------------------------------------------------
// IElementBehaviorSubmit::GetSubmitInfo
//------------------------------------------------------------------------------
HRESULT SubmitFileBehavior::GetSubmitInfo(IHTMLSubmitData *submit_data) {
  ATLTRACE(_T("SubmitFileBehavior::GetSubmitInfo\n"));
  if (!name_ || temp_file_.empty()) {
    return S_OK;
  }
  CComBSTR filepath(temp_file_.c_str());
  HRESULT hr = submit_data->appendNameFilePair(name_, filepath);
  return hr;
}


//------------------------------------------------------------------------------
// IDispatch::GetIDsOfNames
// Implemented to look up the IDs of the properties exposed by this behavior.
//------------------------------------------------------------------------------
HRESULT SubmitFileBehavior::GetIDsOfNames(REFIID riid,
                                          LPOLESTR *names,
                                          UINT num_names,
                                          LCID lcid,
                                          DISPID *dispids) {
  if (!names || !dispids) {
    return E_POINTER;
  }

  if (num_names == 0) {
    return E_INVALIDARG;  // An odd case
  }

  HRESULT hr = S_OK;

  if (_wcsicmp(*names, kName_DispName) == 0) {
    *dispids++ = kName_DispId;
    hr = (num_names == 1) ? S_OK : DISP_E_UNKNOWNNAME;
  } else if (_wcsicmp(*names, kCapturedUrl_DispName) == 0) {
    *dispids++ = kCapturedUrl_DispId;
    hr = (num_names == 1) ? S_OK : DISP_E_UNKNOWNNAME;
  } else {
    *dispids++ = DISPID_UNKNOWN;
    hr = DISP_E_UNKNOWNNAME;
  }

  for (UINT i = 1; i < num_names; ++i) {
    *dispids++ = DISPID_UNKNOWN;
  }

  return hr;
}

//------------------------------------------------------------------------------
// IDispatch::Invoke
// Implemented to get/set the properties exposed by this behavior.
//------------------------------------------------------------------------------
HRESULT SubmitFileBehavior::Invoke(DISPID dispid,
                                   REFIID riid,
                                   LCID lcid,
                                   WORD flags,
                                   DISPPARAMS *params,
                                   VARIANT* result,
                                   EXCEPINFO* exceptinfo,
                                   unsigned int *argerr) {
  // Property Put
  if (flags & DISPATCH_PROPERTYPUT) {
    if (!params) return E_POINTER;
    if ((params->cArgs != 1)
        || !params->rgvarg
        || (VT_BSTR != params->rgvarg->vt)) {
      return E_INVALIDARG;
    }

    if (dispid == kName_DispId) {
      name_ = params->rgvarg->bstrVal;
      return S_OK;
    } else if (dispid == kCapturedUrl_DispId) {
      return SetCapturedUrl(params->rgvarg->bstrVal);
    } else {
      return DISP_E_MEMBERNOTFOUND;
    }

  // Property Get
  } else if (flags & DISPATCH_PROPERTYGET) {
    if (!result) return E_POINTER;
    VariantInit(result);

    if (dispid == kName_DispId) {
      return name_.CopyTo(result);
    } else if (dispid == kCapturedUrl_DispId) {
      return captured_url_.CopyTo(result);
    } else {
      return DISP_E_MEMBERNOTFOUND;
    }

  // Caller is trying to invoke our property as a method
  } else {
    return DISP_E_MEMBERNOTFOUND;
  }
}

//------------------------------------------------------------------------------
// SetCapturedUrl
//------------------------------------------------------------------------------
HRESULT SubmitFileBehavior::SetCapturedUrl(BSTR full_url) {
  // Clear out any existing tempfile and url data we have
  Reset();

  // Null or empty strings clear the property value, no file will be submitted.
  if (!full_url || (wcslen(full_url) == 0)) {
    return S_OK;
  }

  if (!page_origin_.IsSameOriginAsUrl(full_url)) {
    return E_FAIL;
  }

  // Read data from our local store
  ResourceStore::Item item;
  if (!store_.GetItem(full_url, &item)) {
    return E_FAIL;
  }

  // TODO(michaeln): Now that we're storing files on disk, we don't need to
  // create temp file for this. Just open the file for shared reading to
  // prevent it from being removed prior to the form submission and close
  // the file in our destructor. Ditto for Firefox.

  // Copy to a temp file, we do this here rather than in GetSubmitInfo to more
  // closely match the behavior in FF.
  //
  // The server will see the local filename, so we try to use the original
  // filename, using a temp directory per class instance to avoid name
  // conflicts.
  if (!File::CreateNewTempDirectory(&temp_folder_)) {
    return E_FAIL;
  }

  temp_file_ = temp_folder_;

  std::string16 filename;
  item.payload.GetHeader(HttpConstants::kXCapturedFilenameHeader, &filename);
  if (filename.empty()) {
    // This handles the case where the URL didn't get into the database
    // using the captureFile API. It's arguable whether we should support this.
    filename = kMissingFilename;
  }

  temp_file_ += kPathSeparator;  // PathAppend() and basic_string don't mix
  temp_file_ += filename;

  size_t data_len = 0;
  const uint8 *data = NULL;
  if (item.payload.data.get()) {
    data_len = item.payload.data->size();
    data = &(item.payload.data->at(0));
  }

  if (!File::CreateNewFile(temp_file_.c_str()) ||
      !File::WriteVectorToFile(temp_file_.c_str(), item.payload.data.get())) {
    Reset();
    return E_FAIL;
  }

  captured_url_ = full_url;

  return S_OK;
}

