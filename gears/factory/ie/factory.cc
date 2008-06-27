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

#include "gears/base/common/module_wrapper.h"
#include "gears/base/common/process_utils_win32.h"
#include "gears/base/common/string16.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#include "gears/base/ie/detect_version_collision.h"
#include "gears/console/console.h"
#include "gears/database/database.h"
#include "gears/database2/manager.h"
#include "gears/desktop/desktop.h"

#ifdef OFFICIAL_BUILD
// The Dummy module is not included in official builds.
#else
#include "gears/dummy/dummy_module.h"
#endif

#include "gears/factory/common/factory_utils.h"
#include "gears/factory/ie/factory.h"
#include "gears/geolocation/geolocation.h"
#include "gears/httprequest/httprequest.h"
#include "gears/localserver/ie/localserver_ie.h"
#include "gears/timer/timer.h"
#include "gears/ui/ie/string_table.h"
#include "gears/workerpool/workerpool.h"
#ifdef WINCE
// The Image, Canvas and Media API are not yet available for WinCE.
#else
#include "gears/canvas/canvas.h"
#include "gears/image/image_loader.h"
#include "gears/media/audio.h"
#include "gears/media/audio_recorder.h"
#endif  // WINCE

#ifdef USING_CCTESTS
#include "gears/cctests/test.h"
#endif


GearsFactory::GearsFactory() : is_creation_suspended_(false) {
  SetActiveUserFlag();
}


STDMETHODIMP GearsFactory::create(const BSTR object_name_bstr_in,
                                  const VARIANT *version_variant,
                                  IDispatch **retval) {
  const BSTR object_name_bstr = ActiveXUtils::SafeBSTR(object_name_bstr_in);

  LOG16((L"GearsFactory::create(%s)\n", object_name_bstr));

#ifdef WINCE
  // If privateSetGlobalObject has not been called from JavaScript (the call is
  // made from gears_init.js), then the factory won't be sited. In this case,
  // we should not create any Gears objects.
  if (!IsFactoryInitialized(this)) {
    LOG16((L"GearsFactory::create: Failed - factory has not been sited.\n"));
    RETURN_EXCEPTION(STRING16(PRODUCT_FRIENDLY_NAME
                              L" has not been initialized correctly. "
                              L"Please ensure that you are using the most "
                              L"recent version of gears_init.js."));
  }
#endif

  if (DetectedVersionCollision()) {
    if (!EnvIsWorker()) {
      MaybeNotifyUserOfVersionCollision();  // only notifies once per process
    }
    const int kMaxStringLength = 256;
    char16 error_text[kMaxStringLength];
    if (LoadString(GetGearsModuleHandle(),
                   IDS_VERSION_COLLISION_TEXT, error_text, kMaxStringLength)) {
      RETURN_EXCEPTION(error_text);
    } else {
      RETURN_EXCEPTION(L"Internal Error");
    }
  }

  std::string16 module_name(object_name_bstr);
  
  if (RequiresLocalDataPermissionType(module_name)) {
    // Make sure the user gives this site permission to use Gears unless the
    // module can be created without requiring any permissions.
    // Also check is_creation_suspended, because the factory can be suspended
    // even when permission_states_ is an ALLOWED_* value.
    if (is_creation_suspended_ ||
        !GetPermissionsManager()->AcquirePermission(
        PermissionsDB::PERMISSION_LOCAL_DATA)) {
      RETURN_EXCEPTION(STRING16(L"Page does not have permission to use "
                                PRODUCT_FRIENDLY_NAME L"."));
    }
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

  std::string16 error;
  bool success = false;

  // Try creating a dispatcher-based module first.
  success = CreateDispatcherModule(module_name, retval, &error);
  if (success) {
    RETURN_NORMAL();
  } else if (error.length() > 0) {
    RETURN_EXCEPTION(error.c_str());
  }

  success = CreateComModule(module_name, retval, &error);
  if (success) {
    RETURN_NORMAL();
  } else if (error.length() > 0) {
    RETURN_EXCEPTION(error.c_str());
  } else {
    RETURN_EXCEPTION(STRING16(L"Unknown object."));
  }
}

bool GearsFactory::CreateDispatcherModule(const std::string16 &object_name,
                                          IDispatch **retval,
                                          std::string16 *error) {
  scoped_refptr<ModuleImplBaseClass> object;

  if (object_name == STRING16(L"beta.test")) {
#ifdef USING_CCTESTS
    CreateModule<GearsTest>(GetJsRunner(), &object);
#else
    *error = STRING16(L"Object is only available in test build.");
    return false;
#endif
  } else if (object_name == STRING16(L"beta.database")) {
    CreateModule<GearsDatabase>(GetJsRunner(), &object);
  } else if (object_name == STRING16(L"beta.desktop")) {
    CreateModule<GearsDesktop>(GetJsRunner(), &object);
  } else if (object_name == STRING16(L"beta.httprequest")) {
    CreateModule<GearsHttpRequest>(GetJsRunner(), &object);
  } else if (object_name == STRING16(L"beta.timer")) {
    CreateModule<GearsTimer>(GetJsRunner(), &object);
  } else if (object_name == STRING16(L"beta.workerpool")) {
    CreateModule<GearsWorkerPool>(GetJsRunner(), &object);
#ifdef OFFICIAL_BUILD
  // The Canvas, Console, Database2, Geolocation, Media and Image APIs have not
  // been finalized for official builds.
#else
  } else if (object_name == STRING16(L"beta.databasemanager")) {
    CreateModule<Database2Manager>(GetJsRunner(), &object);
  } else if (object_name == STRING16(L"beta.geolocation")) {
    CreateModule<GearsGeolocation>(GetJsRunner(), &object);
#ifdef WINCE
  // Furthermore, Canvas, Console, Media and Image are unimplemented on WinCE.
#else
#ifdef WIN32
  } else if (object_name == STRING16(L"beta.canvas")) {
    CreateModule<GearsCanvas>(GetJsRunner(), &object);
#endif
  } else if (object_name == STRING16(L"beta.console")) {
    CreateModule<GearsConsole>(GetJsRunner(), &object);
  } else if (object_name == STRING16(L"beta.imageloader")) {
    CreateModule<GearsImageLoader>(GetJsRunner(), &object);
  } else if (object_name == STRING16(L"beta.audio")) {
    CreateModule<GearsAudio>(GetJsRunner(), &object);
  } else if (object_name == STRING16(L"beta.audiorecorder")) {
    CreateModule<GearsAudioRecorder>(GetJsRunner(), &object);
#endif  // WINCE
#endif  // OFFICIAL_BUILD
#ifdef OFFICIAL_BUILD
  // The Dummy module is not included in official builds.
#else
  } else if (object_name == STRING16(L"beta.dummymodule")) {
    CreateModule<DummyModule>(GetJsRunner(), &object);
#endif // OFFICIAL_BUILD
  } else {
    // Don't return an error here. Caller handles reporting unknown modules.
    error->clear();
    return false;
  }

  // setup the ModuleImplBaseClass (copy settings from this factory)
  if (!object->InitBaseFromSibling(this)) {
    *error = STRING16(L"Error initializing base class.");
    return false;
  }

  *retval = object->GetWrapperToken().pdispVal;
  // IE expects that when you return an object up to the script engine, from
  // an IDL-defined API, then the return value will already have been Ref()'d.
  // The NPAPI and Firefox ports do not do this - they have a different
  // calling convention.
  // Note that, when this factory implementation migrates to being Dispatcher-
  // based, then we will presumably just call JsCallContext::SetReturnValue,
  // and we will not need to explicitly Ref() here.
  object->Ref();
  return true;
}

bool GearsFactory::CreateComModule(const std::string16 &object_name,
                                   IDispatch **retval, std::string16 *error) {
  ModuleImplBaseClass *base_class = NULL;
  CComQIPtr<IDispatch> idispatch;

  HRESULT hr = E_FAIL;
  if (object_name == STRING16(L"beta.localserver")) {
    CComObject<GearsLocalServer> *obj;
    hr = CComObject<GearsLocalServer>::CreateInstance(&obj);
    base_class = obj;
    idispatch = obj;
  } else {
    // Don't return an error here. Caller handles reporting unknown modules.
    error->clear();
    return false;
  }

  if (FAILED(hr) || !base_class || !idispatch) {
    *error = STRING16(L"Failed to create requested object.");
    return false;
  }

  // setup the ModuleImplBaseClass (copy settings from this factory)
  if (!base_class->InitBaseFromSibling(this)) {
    *error = STRING16(L"Error initializing base class.");
    return false;
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
  return true;
}


STDMETHODIMP GearsFactory::getBuildInfo(BSTR *retval) {
  LOG16((L"GearsFactory::getBuildInfo()\n"));
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


STDMETHODIMP GearsFactory::getPermission(const VARIANT *site_name_in,
                                         const VARIANT *image_url_in,
                                         const VARIANT *extra_message_in,
                                         VARIANT_BOOL *retval) {
  // Guard against NULL BSTRs.
  // TODO(cprince): Do this automatically in JsParamFetcher for IE.
  std::string16 site_name;
  std::string16 image_url;
  std::string16 extra_message;

  if (ActiveXUtils::OptionalVariantIsPresent(site_name_in)) {
    if (site_name_in->vt == VT_BSTR) {
      site_name.assign(site_name_in->bstrVal);
    } else {
      RETURN_EXCEPTION(STRING16(L"siteName must be a string."));
    }
  }

  if (ActiveXUtils::OptionalVariantIsPresent(image_url_in)) {
    if (image_url_in->vt == VT_BSTR) {
      image_url.assign(image_url_in->bstrVal);
    } else {
      RETURN_EXCEPTION(STRING16(L"imageUrl must be a string."));
    }
  }

  if (ActiveXUtils::OptionalVariantIsPresent(extra_message_in)) {
    if (extra_message_in->vt == VT_BSTR) {
      extra_message.assign(extra_message_in->bstrVal);
    } else {
      RETURN_EXCEPTION(STRING16(L"extraMessage must be a string."));
    }
  }
  scoped_ptr<PermissionsDialog::CustomContent> custom_content(
      new PermissionsDialog::CustomContent(image_url.c_str(),
                                           site_name.c_str(),
                                           extra_message.c_str()));
  assert(custom_content.get());
  if (GetPermissionsManager()->AcquirePermission(
      PermissionsDB::PERMISSION_LOCAL_DATA,
      custom_content.get())) {
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
  if (GetPermissionsManager()->HasPermission(
      PermissionsDB::PERMISSION_LOCAL_DATA)) {
    *retval = VARIANT_TRUE;
  } else {
    *retval = VARIANT_FALSE;
  }

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
  if (!IsFactoryInitialized(this)) {
    if (!InitBaseFromDOM(js_dispatch)) {
      RETURN_EXCEPTION(STRING16(L"Failed to initialize "
                                PRODUCT_FRIENDLY_NAME
                                L"."));
    }
  }
  RETURN_NORMAL();
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

#ifdef WINCE
// On WinCE we initialize the Factory with a call from JavaScript. Since we
// can't guarantee that this call will be made, we need a method to determine
// whether or not an object has been successfully initialized. This method is
// a friend of ModuleImplBaseClass for this purpose.
static bool IsFactoryInitialized(GearsFactory *factory) {
  return factory->module_environment_.get() != NULL;
}
#endif
