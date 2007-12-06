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

#include <assert.h>
#include "gears/base/common/string16.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#include "gears/base/ie/detect_version_collision.h"
#include "gears/channel/ie/channel.h"
#include "gears/database/ie/database.h"
#include "gears/desktop/desktop_ie.h"
#include "gears/factory/common/factory_utils.h"
#include "gears/factory/ie/factory.h"
#include "gears/httprequest/ie/httprequest_ie.h"
#include "gears/workerpool/ie/workerpool.h"
#include "gears/localserver/ie/localserver_ie.h"
#include "gears/timer/timer.h"

#ifdef DEBUG
#include "gears/cctests/test_ie.h"
#endif


GearsFactory::GearsFactory()
    : is_creation_suspended_(false),
      is_permission_granted_(false),
      is_permission_value_from_user_(false) {
  SetActiveUserFlag();
}


STDMETHODIMP GearsFactory::create(const BSTR object_name_bstr_in,
                                  const VARIANT *version_variant,
                                  IDispatch **retval) {
  HRESULT hr;

  const BSTR object_name_bstr = ActiveXUtils::SafeBSTR(object_name_bstr_in);

  ATLTRACE(_T("GearsFactory::create(%s)\n"), object_name_bstr);

  if (DetectedVersionCollision()) {
    if (!EnvIsWorker()) {
      MaybeNotifyUserOfVersionCollision();  // only notifies once per process
    }
    RETURN_EXCEPTION(kVersionCollisionErrorMessage);
  }

  // Make sure the user gives this origin permission to use Gears.

  if (!HasPermissionToUseGears(this, NULL, NULL, NULL)) {
    RETURN_EXCEPTION(STRING16(L"Page does not have permission to use "
                              PRODUCT_FRIENDLY_NAME L"."));
  }

  // Check the version string.

  std::string16 version;
  if (!ActiveXUtils::OptionalVariantIsPresent(version_variant)) {
    version = STRING16(L"1.0");  // default value for this optional param
  } else {
    if (version_variant->vt != VT_BSTR) {
      RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
    }
    version = version_variant->bstrVal;
  }

  if (version != kAllowedClassVersion) {
    RETURN_EXCEPTION(STRING16(L"Invalid version string. Must be 1.0."));
  }

  // Create an instance of the object.
  //
  // Do case-sensitive comparisons, which are always better in APIs. They make
  // code consistent across callers, and they are easier to support over time.

  std::string16 object_name(object_name_bstr);
  ModuleImplBaseClass *base_class = NULL;
  CComQIPtr<IDispatch> idispatch;

  hr = E_FAIL;
#ifdef WINCE
  // TODO(steveblock): Implement channel for WinCE.
  if (false) {
#else
  if (object_name == STRING16(L"beta.channel")) {
    CComObject<GearsChannel> *obj;
    hr = CComObject<GearsChannel>::CreateInstance(&obj);
    base_class = obj;
    idispatch = obj;
#endif
  } else if (object_name == STRING16(L"beta.database")) {
    CComObject<GearsDatabase> *obj;
    hr = CComObject<GearsDatabase>::CreateInstance(&obj);
    base_class = obj;
    idispatch = obj;
  } else if (object_name == STRING16(L"beta.httprequest")) {
    CComObject<GearsHttpRequest> *obj;
    hr = CComObject<GearsHttpRequest>::CreateInstance(&obj);
    base_class = obj;
    idispatch = obj;
  } else if (object_name == STRING16(L"beta.localserver")) {
    CComObject<GearsLocalServer> *obj;
    hr = CComObject<GearsLocalServer>::CreateInstance(&obj);
    base_class = obj;
    idispatch = obj;
  } else if (object_name == STRING16(L"beta.test")) {
#ifdef DEBUG
    CComObject<GearsTest> *obj;
    hr = CComObject<GearsTest>::CreateInstance(&obj);
    base_class = obj;
    idispatch = obj;
#else
    RETURN_EXCEPTION(STRING16(L"Object is only available in debug build."));
#endif
  } else if (object_name == STRING16(L"beta.timer")) {
    CComObject<GearsTimer> *obj;
    hr = CComObject<GearsTimer>::CreateInstance(&obj);
    base_class = obj;
    idispatch = obj;
  } else if (object_name == STRING16(L"beta.workerpool")) {
    CComObject<GearsWorkerPool> *obj;
    hr = CComObject<GearsWorkerPool>::CreateInstance(&obj);
    base_class = obj;
    idispatch = obj;
  } else {
    RETURN_EXCEPTION(STRING16(L"Unknown object."));
  }

  if (FAILED(hr) || !base_class || !idispatch) {
    RETURN_EXCEPTION(STRING16(L"Failed to create requested object."));
  }

  // setup the ModuleImplBaseClass (copy settings from this factory)
  if (!base_class->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing base class."));
  }

  // Our factory-created Gears objects no longer need to be sited, since we get
  // the domain and location_url from ModuleImplBaseClass.  But we may someday
  // later need an object to be sited, in which case we should enable this code.
  //
  //// SetSite any object that supports IObjectWithSite
  //if (SUCCEEDED(hr)) {
  //  CComQIPtr<IObjectWithSite> object_with_site = idispatch;
  //  if (object_with_site) {
  //    hr = object_with_site->SetSite(m_spUnkSite);
  //  }
  //}

  *retval = idispatch;
  (*retval)->AddRef();  // ~CComQIPtr will Release, so must AddRef here
  assert((*retval)->AddRef() == 3 &&
         (*retval)->Release() == 2);
  RETURN_NORMAL();
}


STDMETHODIMP GearsFactory::getBuildInfo(BSTR *retval) {
  ATLTRACE(_T("GearsFactory::getBuildInfo()\n"));
  std::string16 build_info;
  AppendBuildInfo(&build_info);
  if (DetectedVersionCollision()) {
    build_info += L" (pending browser restart)";
  }

  *retval = SysAllocString(build_info.c_str());
  if (*retval == NULL) {
    RETURN_EXCEPTION(STRING16(L"Internal error."));
  }
  RETURN_NORMAL();
}


STDMETHODIMP GearsFactory::get_version(BSTR *retval) {
  *retval = SysAllocString(STRING16(PRODUCT_VERSION_STRING));
  if (*retval == NULL) {
    RETURN_EXCEPTION(STRING16(L"Internal error."));
  }
  RETURN_NORMAL();
}


STDMETHODIMP GearsFactory::getPermission(const BSTR site_name_in,
                                         const BSTR image_url_in,
                                         const BSTR extra_message_in,
                                         VARIANT_BOOL *retval) {
  // Guard against NULL BSTRs.
  // TODO(cprince): Do this automatically in JsParamFetcher for IE.
  const BSTR site_name = ActiveXUtils::SafeBSTR(site_name_in);
  const BSTR image_url = ActiveXUtils::SafeBSTR(image_url_in);
  const BSTR extra_message = ActiveXUtils::SafeBSTR(extra_message_in);

  if (HasPermissionToUseGears(this, image_url,
                              site_name, extra_message)) {
    *retval = VARIANT_TRUE;
  } else {
    *retval = VARIANT_FALSE;
  }
  RETURN_NORMAL();
}


// Purposely ignores 'is_creation_suspended_'.  The 'hasPermission' property
// indicates whether USER opt-in is still required, not whether DEVELOPER
// methods have been called correctly (e.g. allowCrossOrigin).
STDMETHODIMP GearsFactory::get_hasPermission(VARIANT_BOOL *retval) {
  *retval = is_permission_granted_ ? VARIANT_TRUE : VARIANT_FALSE;
  RETURN_NORMAL();
}


// InitBaseFromDOM needs the object to be sited.  We override SetSite just to
// know when this happens, so we can do some one-time setup afterward.
STDMETHODIMP GearsFactory::SetSite(IUnknown *site) {
  HRESULT hr = IObjectWithSiteImpl<GearsFactory>::SetSite(site);
#ifdef WINCE
  // We are unable to get IWebBrowser2 from this pointer. Instead, the user must
  // call privateSetGlobalObject from JavaScript.
#else
  InitBaseFromDOM(m_spUnkSite);
#endif
  return hr;
}

#ifdef WINCE
STDMETHODIMP GearsFactory::privateSetGlobalObject(IDispatch *js_dispatch) {
  return InitBaseFromDOM(js_dispatch);
}
#endif

// TODO(cprince): See if we can use Suspend/Resume with the opt-in dialog too,
// rather than just the cross-origin worker case.  (Code re-use == good.)
void GearsFactory::SuspendObjectCreation() {
  is_creation_suspended_ = true;
}

void GearsFactory::ResumeObjectCreationAndUpdatePermissions() {
  // TODO(cprince): The transition from suspended to resumed is where we should
  // propagate cross-origin opt-in to the permissions DB.
  is_creation_suspended_ = false;
}
