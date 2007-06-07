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

#include "gears/base/ie/activex_utils.h"
#include "gears/localserver/ie/file_submitter_ie.h"
#include "gears/localserver/ie/file_submit_behavior.h"


//------------------------------------------------------------------------------
// setFileInputElement
//------------------------------------------------------------------------------
STDMETHODIMP GearsFileSubmitter::setFileInputElement(
    IDispatch *file_input_element,
    const BSTR captured_url_key) {
  ATLTRACE(_T("GearsFileSubmitter::setFileInputElement\n"));

  // The SubmitFileBehavior expects a full url so we do the resolution and
  // test for same-origin here. Also, we want to return reasonble error messages
  // to the caller if either of those fail. The SubmitFileBehavior independently
  // checks for same-origin to defend against script directly setting the
  // 'capturedUrl' property (undocumented API) without using the
  // FileSubmitter object.
  std::string16 full_url;
  if (!ResolveRelativeUrl(EnvPageLocationUrl().c_str(), captured_url_key,
                          &full_url)) {
    RETURN_EXCEPTION(STRING16(L"Failed to resolve url."));
  }
  if (!EnvPageSecurityOrigin().IsSameOriginAsUrl(full_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Url is not from the same origin"));
  }

  // This class delegates most of the work to the SubmitFileBehavior class.
  // Here we pass the captured_url_key to an instance of the behavior attached
  // to file_input_element. If our behavior is not yet attached to that element,
  // we attach an new instance and then pass the url to it.

  // TODO(michaeln): We can't attach our behavior to <input> elements, as we
  // would like, due to limitations in IE. For now the DHTML developer has 
  // to use GearsFileSubmitter with some element other than an <input> element
  // that is nested in a <form> element for IE.  The TODO is to allow developers
  // to pass in <input type=file> elements, and to dynamically create a new
  // element of some type with our behavior attached to it (or to dynamically
  // re-write the input element to be a different type).

  // Pass the captured_url_key param to the behavior via the property
  // our behavior class exposes on its IHTMLElement thru IDispatch.
  const WCHAR *name = SubmitFileBehavior::GetCapturedUrlPropertyName();
  CComVariant value(full_url.c_str());
  HRESULT hr = ActiveXUtils::SetDispatchProperty(file_input_element,
                                                 name, &value);
  if (hr == DISP_E_UNKNOWNNAME) {
    // Our behavior is not attached to this element, attach it
    CComQIPtr<IElementBehaviorFactory> factory(this);
    CComQIPtr<IHTMLElement2> html_element2(file_input_element);
    if (!factory || !html_element2) {
      return E_NOINTERFACE;
    }
    CComBSTR behavior_url(L"#Google" PRODUCT_SHORT_NAME L"#SubmitFile");
    CComVariant factory_var(static_cast<IUnknown*>(factory));
    LONG cookie = 0;
    hr = html_element2->addBehavior(behavior_url, &factory_var, &cookie);
    if (FAILED(hr)) {
      return hr;
    }
    // And try again to set the property
    hr = ActiveXUtils::SetDispatchProperty(file_input_element, name, &value);
  }
  return hr;
}

//------------------------------------------------------------------------------
// FindBehavior
//------------------------------------------------------------------------------
HRESULT GearsFileSubmitter::FindBehavior(BSTR name, BSTR url,
                                         IElementBehaviorSite* behavior_site,
                                         IElementBehavior** behavior_out) {
  // We only know how to create one kind of behavior
  if (!name || (_wcsicmp(name, L"SubmitFile") != 0)) {
    return E_FAIL;
  }
  CComObject<SubmitFileBehavior>* behavior = NULL;
  HRESULT hr = CComObject<SubmitFileBehavior>::CreateInstance(&behavior);
  if (FAILED(hr)) {
    return hr;
  }

  CComPtr< CComObject<SubmitFileBehavior> >  reference_adder(behavior);
  if (!behavior->store_.Clone(&store_)) {
    return E_FAIL;
  }
  behavior->InitPageOrigin(EnvPageSecurityOrigin());

  return behavior->QueryInterface(behavior_out);
}

