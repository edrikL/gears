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

#include "gears/base/common/common.h"
#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/ie/module_wrapper.h"

HRESULT ModuleWrapper::GetIDsOfNames(REFIID iid, OLECHAR FAR* FAR* names,
                                     unsigned int num_names, LCID lcid, 
                                     DISPID FAR* retval) {
  // JavaScript does not have named parameters
  assert(num_names == 1);

  std::string member_name_utf8;
  if (!String16ToUTF8(static_cast<char16 *>(*names), &member_name_utf8)) {
    assert(false);
    return DISP_E_UNKNOWNNAME;
  }

  DispatchId dispatch_id = dispatcher_->GetDispatchId(member_name_utf8);
  if (dispatch_id == NULL) {
    *retval = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
  } else {
    *retval = reinterpret_cast<DISPID>(dispatch_id);
    return S_OK;
  }
}

HRESULT ModuleWrapper::Invoke(DISPID member_id, REFIID iid, LCID lcid,
                              WORD flags, DISPPARAMS FAR* params,
                              VARIANT FAR* retval, EXCEPINFO FAR* exception,
                              unsigned int FAR* arg_error_index) {
  assert(dispatcher_.get());
  JsCallContext js_call_context(params, retval, exception);

  DispatchId dispatch_id = reinterpret_cast<DispatchId>(member_id);

  if (flags & DISPATCH_METHOD) {
    if (!dispatcher_->CallMethod(dispatch_id, &js_call_context)) {
      return DISP_E_MEMBERNOTFOUND;
    }
  } else if (flags & DISPATCH_PROPERTYGET) {
    if (!dispatcher_->GetProperty(dispatch_id, &js_call_context)) {
      return DISP_E_MEMBERNOTFOUND;
    }
  } else if (flags & DISPATCH_PROPERTYPUT) {
    if (!dispatcher_->SetProperty(dispatch_id, &js_call_context)) {
      return DISP_E_MEMBERNOTFOUND;
    }
  } else {
    return DISP_E_MEMBERNOTFOUND;
  }

  if (js_call_context.is_exception_set()) {
    return DISP_E_EXCEPTION;
  } else {
    return S_OK;
  }
}

STDMETHODIMP ModuleWrapper::get_moduleWrapper(VARIANT *retval) {
  // We pack the pointer into the byref field of a VARIANT.
  retval->vt = VT_BYREF;
  retval->byref = this;
  RETURN_NORMAL();
}
