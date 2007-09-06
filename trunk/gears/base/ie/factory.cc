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
#include "gears/base/common/factory_utils.h"
#include "gears/base/common/string16.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#include "gears/base/ie/detect_version_collision.h"
#include "gears/base/ie/factory.h"
#include "gears/database/ie/database.h"
#include "gears/httprequest/ie/httprequest_ie.h"
#include "gears/workerpool/ie/workerpool.h"
#include "gears/localserver/ie/localserver_ie.h"
#include "gears/timer/ie/timer.h"


GearsFactory::GearsFactory()
    : is_creation_suspended_(false),
      is_permission_granted_(false),
      is_permission_value_from_user_(false) {
  SetActiveUserFlag();
}


STDMETHODIMP GearsFactory::create(const BSTR object_name_bstr_in,
                                  const BSTR version_bstr_in,
                                  IDispatch **retval) {
  const BSTR object_name_bstr = ActiveXUtils::SafeBSTR(object_name_bstr_in);
  const BSTR version_bstr = ActiveXUtils::SafeBSTR(version_bstr_in);
  HRESULT hr;

  ATLTRACE(_T("GearsFactory::create(%s, %s)\n"),
           object_name_bstr, version_bstr);

  if (DetectedVersionCollision()) {
    if (!EnvIsWorker()) {
      MaybeNotifyUserOfVersionCollision();  // only notifies once per process
    }
    RETURN_EXCEPTION(kVersionCollisionErrorMessage);
  }

  // Make sure the user gives this origin permission to use Gears.

  if (!HasPermissionToUseGears(this)) {
    RETURN_EXCEPTION(STRING16(L"Page does not have permission to use "
                              PRODUCT_FRIENDLY_NAME L"."));
  }

  // Parse the version string.

  int major_version_desired;
  int minor_version_desired;
  if (!ParseMajorMinorVersion(version_bstr,
			      &major_version_desired,
			      &minor_version_desired)) {
    RETURN_EXCEPTION(STRING16(L"Invalid version string."));
  }

  // Create an instance of the object.
  //
  // Do case-sensitive comparisons, which are always better in APIs. They make
  // code consistent across callers, and they are easier to support over time.

  std::string16 object_name(object_name_bstr);
  GearsBaseClass *base_class = NULL;
  CComQIPtr<IDispatch> idispatch;

  hr = E_FAIL;
  if (object_name == STRING16(L"beta.database")) {
    if (major_version_desired == kGearsDatabaseVersionMajor &&
        minor_version_desired <= kGearsDatabaseVersionMinor) {
      CComObject<GearsDatabase> *obj;
      hr = CComObject<GearsDatabase>::CreateInstance(&obj);
      base_class = obj;
      idispatch = obj;
    }
  } else if (object_name == STRING16(L"beta.httprequest")) {
    if (major_version_desired == kGearsHttpRequestVersionMajor &&
        minor_version_desired <= kGearsHttpRequestVersionMinor) {
      CComObject<GearsHttpRequest> *obj;
      hr = CComObject<GearsHttpRequest>::CreateInstance(&obj);
      base_class = obj;
      idispatch = obj;
    }
  } else if (object_name == STRING16(L"beta.localserver")) {
    if (major_version_desired == kGearsLocalServerVersionMajor &&
        minor_version_desired <= kGearsLocalServerVersionMinor) {
      CComObject<GearsLocalServer> *obj;
      hr = CComObject<GearsLocalServer>::CreateInstance(&obj);
      base_class = obj;
      idispatch = obj;
    }
  } else if (object_name == STRING16(L"beta.timer")) {
    if (major_version_desired == kGearsTimerVersionMajor &&
        minor_version_desired <= kGearsTimerVersionMinor) {
      CComObject<GearsTimer> *obj;
      hr = CComObject<GearsTimer>::CreateInstance(&obj);
      base_class = obj;
      idispatch = obj;
    }
  } else if (object_name == STRING16(L"beta.workerpool")) {
    if (major_version_desired == kGearsWorkerPoolVersionMajor &&
        minor_version_desired <= kGearsWorkerPoolVersionMinor) {
      CComObject<GearsWorkerPool> *obj;
      hr = CComObject<GearsWorkerPool>::CreateInstance(&obj);
      base_class = obj;
      idispatch = obj;
    }
  } else {
    RETURN_EXCEPTION(STRING16(L"Unknown object."));
  }

  if (FAILED(hr) || !base_class || !idispatch) {
    RETURN_EXCEPTION(STRING16(L"Failed to create requested object."));
  }

  // setup the GearsBaseClass (copy settings from this factory)
  if (!base_class->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Error initializing base class."));
  }

  // Our factory-created Gears objects no longer need to be sited, since we get
  // the domain and location_url from GearsBaseClass.  But we may someday later
  // need an object to be sited, in which case we should enable this code.
  //
  //// SetSite any object that supports IObjectWithSite
  //if (SUCCEEDED(hr)) {
  //  CComQIPtr<IObjectWithSite> object_with_site = idispatch;
  //  if (object_with_site) {
  //    hr = object_with_site->SetSite(m_spUnkSite);
  //  }
  //}

  *retval = idispatch;
  (*retval)->AddRef(); // ~CComQIPtr will Release, so must AddRef here
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
  if (*retval != NULL) {
    return S_OK;
  } else {
    return E_OUTOFMEMORY;
  }
}


STDMETHODIMP GearsFactory::isVersionAtLeast(const BSTR version_bstr_in,
                                            VARIANT_BOOL *retval) {
  const BSTR version_bstr = ActiveXUtils::SafeBSTR(version_bstr_in);

  int major;
  int minor;
  if (!ParseMajorMinorVersion(version_bstr, &major, &minor)) {
    RETURN_EXCEPTION(STRING16(L"Invalid version string."));
  }

  if ((PRODUCT_VERSION_MAJOR < major) ||
      (PRODUCT_VERSION_MAJOR == major && PRODUCT_VERSION_MINOR < minor)) {
    *retval = VARIANT_FALSE;
  } else {
    *retval = VARIANT_TRUE;
  }

  RETURN_NORMAL();
}


// InitBaseFromDOM needs the object to be sited.  We override SetSite just to
// know when this happens, so we can do some one-time setup afterward.
STDMETHODIMP GearsFactory::SetSite(IUnknown *site) {
  HRESULT hr = IObjectWithSiteImpl<GearsFactory>::SetSite(site);
  InitBaseFromDOM(m_spUnkSite);
  return hr;
}


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
