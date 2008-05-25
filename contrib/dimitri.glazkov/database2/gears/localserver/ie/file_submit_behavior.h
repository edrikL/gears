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

#ifndef GEARS_LOCALSERVER_IE_FILE_SUBMIT_BEHAVIOR_H__
#define GEARS_LOCALSERVER_IE_FILE_SUBMIT_BEHAVIOR_H__

#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/localserver/common/resource_store.h"


//------------------------------------------------------------------------------
// SubmitFileBehavior
//
// A "binary behavior" used to support the submission of files in our local
// store as part of form submissions. GearsFileSubmitter interacts with
// this behavior object. This class exposes two properties on the hosting
// HTMLElement:
//   - name, the form field name to submit with the file data
//   - capturedUrl, the key of the file to submit in the local store
// When a form containing an element having this behavior is submitted,
// the name / file pair is uploaded as part of the resulting POST.
//
// Caveat: The behavior does not work when attached to <input> elements which
// ironically are exactly the type of elements we wanted to augment. Apparently
// the IElementBehaviorSubmit.GetSubmitInfo method is not called when the
// hosting element is an <input> element.
//
// @see GearsFileSubmitter
//------------------------------------------------------------------------------
class ATL_NO_VTABLE SubmitFileBehavior
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<SubmitFileBehavior>,
      public IElementBehavior,
      public IElementBehaviorSubmit,
      public IObjectSafetyImpl<SubmitFileBehavior,
                               INTERFACESAFE_FOR_UNTRUSTED_CALLER +
                               INTERFACESAFE_FOR_UNTRUSTED_DATA>,
      public IDispatchImpl<IDispatch> {
 public:
  DECLARE_NOT_AGGREGATABLE(SubmitFileBehavior)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  BEGIN_COM_MAP(SubmitFileBehavior)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IElementBehaviorSubmit)
  END_COM_MAP()

  // IElementBehavior
  STDMETHOD(Detach)(void);
  STDMETHOD(Init)(IElementBehaviorSite *pBehaviorSite);
  STDMETHOD(Notify)(long event, VARIANT *pVar);

  // IElementBehaviorSubmit
  STDMETHOD(GetSubmitInfo)(IHTMLSubmitData *submit_data);
  STDMETHOD(Reset)(void);

  // IDispatch overrides
  STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *names, UINT num_names,
                           LCID lcid, DISPID *dispid);
  STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                    DISPPARAMS* params, VARIANT *result, EXCEPINFO *exceptinfo,
                    unsigned int *argerr);

  static const wchar_t *GetCapturedUrlPropertyName() {
    return kCapturedUrl_DispName;
  }

  static const wchar_t *GetNamePropertyName() {
    return kName_DispName;
  }

 private:
  void InitPageOrigin(const SecurityOrigin &origin);

  HRESULT SetCapturedUrl(BSTR url);

  // Returns the IHTMLElement we're bound to
  HRESULT GetHTMLElement(IHTMLElement **element_out) {
    ATLASSERT(behavior_site_);
    return behavior_site_->GetElement(element_out);
  }

  // Returns the html attribute value of the HTMLElement we're bound to.
  HRESULT GetHTMLElementAttributeValue(const wchar_t *name, VARIANT *value) {
    CComPtr<IHTMLElement> element;
    HRESULT hr = GetHTMLElement(&element);
    if (FAILED(hr)) return hr;
    return ActiveXUtils::GetHTMLElementAttributeValue(element, name, value);
  }

  ResourceStore store_;
  CComBSTR name_;
  CComBSTR captured_url_;
  SecurityOrigin page_origin_;
  std::string16 temp_folder_;
  std::string16 temp_file_;
  CComPtr<IElementBehaviorSite> behavior_site_;

  // Name and dispatch ids for the property exposed by this behavior.
  static const wchar_t *kName_DispName;
  static const DISPID kName_DispId = 1;
  static const wchar_t *kCapturedUrl_DispName;
  static const DISPID kCapturedUrl_DispId = 2;

  friend class GearsFileSubmitter;
};

#endif  // GEARS_LOCALSERVER_IE_FILE_SUBMIT_BEHAVIOR_H__
