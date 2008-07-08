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
#include <stdlib.h>
#include <gecko_sdk/include/nsXPCOM.h>
#include <gecko_internal/nsIDOMClassInfo.h>

#include "gears/factory/firefox/factory.h"

#include "genfiles/product_constants.h"
#include "gears/base/common/common.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/base/common/permissions_manager.h"
#include "gears/base/common/string16.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/cctests/test.h"
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
#include "gears/geolocation/geolocation.h"
#include "gears/image/image_loader.h"
#include "gears/canvas/canvas.h"
#include "gears/media/audio.h"
#include "gears/media/audio_recorder.h"

#include "gears/httprequest/httprequest.h"
#include "gears/localserver/firefox/localserver_ff.h"
#include "gears/timer/timer.h"
#include "gears/workerpool/workerpool.h"

// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_ADDREF(GearsFactory)
NS_IMPL_RELEASE(GearsFactory)
NS_INTERFACE_MAP_BEGIN(GearsFactory)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsFactoryInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsFactoryInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsFactory)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsFactoryContractId = "@google.com/gears/factory;1"; // [naming]
const char *kGearsFactoryClassName = "GearsFactory";
const nsCID kGearsFactoryClassId = {0x93b2e433, 0x35ab, 0x46e7, {0xa9, 0x50,
                                    0x41, 0x8f, 0x92, 0x2c, 0xc6, 0xef}};
                                   // {93B2E433-35AB-46e7-A950-418F922CC6EF}


GearsFactory::GearsFactory()
    : is_creation_suspended_(false) {
  SetActiveUserFlag();
}


NS_IMETHODIMP GearsFactory::Create(//const nsAString &object
                                   //const nsAString &version
                                   nsISupports **retval) {
  JsParamFetcher js_params(this);

  // Get the name of the object they're trying to create.

  std::string16 module_name;
  if (!js_params.GetAsString(0, &module_name)) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

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
  if (!js_params.IsOptionalParamPresent(1, false)) {
    version = STRING16(L"1.0");  // default value for this optional param
  } else {
    if (!js_params.GetAsString(1, &version)) {
      RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
    }
  }

  if (version != kAllowedClassVersion) {
    RETURN_EXCEPTION(STRING16(L"Invalid version string. Must be 1.0."));
  }

  // Create an instance of the object.
  //
  // Do case-sensitive comparisons, which are always better in APIs. They make
  // code consistent across callers, and they are easier to support over time.

  // First try to create a dispatcher-based module.
  std::string16 error;
  bool success = CreateDispatcherModule(module_name, &js_params, &error);

  if (success) {
    RETURN_NORMAL();
  } else if (error.length() > 0) {
    RETURN_EXCEPTION(error.c_str());
  }

  // There was no dispatcher-based implementation of this object. Try to create
  // an isupports module.
  success = CreateISupportsModule(module_name, retval, &error);
  if (success) {
    RETURN_NORMAL();
  } else if (error.length() > 0) {
    RETURN_EXCEPTION(error.c_str());
  } else {
    RETURN_EXCEPTION(STRING16(L"Unknown object."));
  }
}

bool GearsFactory::CreateDispatcherModule(const std::string16 &object_name,
                                          JsParamFetcher *js_params,
                                          std::string16 *error) {
  scoped_refptr<ModuleImplBaseClass> object;

  if (object_name == STRING16(L"beta.test")) {
#ifdef USING_CCTESTS
    CreateModule<GearsTest>(module_environment_.get(), NULL, &object);
#else
    *error = STRING16(L"Object is only available in test build.");
    return false;
#endif
  } else if (object_name == STRING16(L"beta.database")) {
    CreateModule<GearsDatabase>(module_environment_.get(), NULL, &object);
  } else if (object_name == STRING16(L"beta.desktop")) {
    CreateModule<GearsDesktop>(module_environment_.get(), NULL, &object);
  } else if (object_name == STRING16(L"beta.httprequest")) {
    CreateModule<GearsHttpRequest>(module_environment_.get(), NULL, &object);
  } else if (object_name == STRING16(L"beta.timer")) {
    CreateModule<GearsTimer>(module_environment_.get(), NULL, &object);
  } else if (object_name == STRING16(L"beta.workerpool")) {
    CreateModule<GearsWorkerPool>(module_environment_.get(), NULL, &object);
#ifdef OFFICIAL_BUILD
  // The Canvas, Console, Database2, Geolocation, Media, and Image APIs have not been
  // finalized for official builds.
#else
#ifdef WIN32
  } else if (object_name == STRING16(L"beta.canvas")) {
    CreateModule<GearsCanvas>(module_environment_.get(), NULL, &object);
#endif
  } else if (object_name == STRING16(L"beta.console")) {
    CreateModule<GearsConsole>(module_environment_.get(), NULL, &object);
  } else if (object_name == STRING16(L"beta.databasemanager")) {
    CreateModule<Database2Manager>(module_environment_.get(), NULL, &object);
  } else if (object_name == STRING16(L"beta.geolocation")) {
    CreateModule<GearsGeolocation>(module_environment_.get(), NULL, &object);
  } else if (object_name == STRING16(L"beta.imageloader")) {
    CreateModule<GearsImageLoader>(module_environment_.get(), NULL, &object);
  } else if (object_name == STRING16(L"beta.audio")) {
    CreateModule<GearsAudio>(module_environment_.get(), NULL, &object);
  } else if (object_name == STRING16(L"beta.audiorecorder")) {
    CreateModule<GearsAudioRecorder>(module_environment_.get(), NULL, &object);
#endif  // OFFICIAL_BUILD
#ifdef OFFICIAL_BUILD
  // The Dummy module is not included in official builds.
#else
  } else if (object_name == STRING16(L"beta.dummymodule")) {
    CreateModule<DummyModule>(module_environment_.get(), NULL, &object);
#endif // OFFICIAL_BUILD
  } else {
    // Don't return an error here. Caller handles reporting unknown modules.
    error->clear();
    return false;
  }

  if (!object) {
    *error = STRING16(L"Failed to create requested object.");
    return false;
  }

  js_params->SetReturnValue(object->GetWrapperToken());
  return true;
}

bool GearsFactory::CreateISupportsModule(const std::string16 &object_name,
                                         nsISupports **retval,
                                         std::string16 *error) {
  nsCOMPtr<nsISupports> isupports = NULL;

  nsresult nr = NS_ERROR_FAILURE;
  if (object_name == STRING16(L"beta.localserver")) {
    isupports = do_QueryInterface(new GearsLocalServer(), &nr);
  }  else {
    // Don't return an error here. Caller handles reporting unknown modules.
    error->clear();
    return false;
  }

  if (NS_FAILED(nr) || !isupports) {
    *error = STRING16(L"Failed to create requested object.");
    return false;
  }

  // setup the ModuleImplBaseClass (copy settings from this factory)
  bool base_init_succeeded = false;
  nsCOMPtr<GearsBaseClassInterface> idl_base =
      do_QueryInterface(isupports, &nr);
  if (NS_SUCCEEDED(nr) && idl_base) {
    ModuleImplBaseClass *native_base = NULL;
    idl_base->GetNativeBaseClass(&native_base);
    if (native_base) {
      native_base->InitModuleEnvironment(module_environment_.get());
      base_init_succeeded = true;
    }
  }

  if (!base_init_succeeded) {
    *error = STRING16(L"Error initializing base class.");
    return false;
  }

  *retval = isupports.get();
  (*retval)->AddRef(); // ~nsCOMPtr will Release, so must AddRef here
  assert((*retval)->AddRef() == 4 &&
         (*retval)->Release() == 3);
  return true;
}


NS_IMETHODIMP GearsFactory::GetBuildInfo(nsAString &retval) {
  std::string16 build_info;
  AppendBuildInfo(&build_info);
  retval.Assign(build_info.c_str());
  RETURN_NORMAL();
}


NS_IMETHODIMP GearsFactory::GetVersion(nsAString &retval) {
  retval.Assign(STRING16(PRODUCT_VERSION_STRING));
  RETURN_NORMAL();
}


NS_IMETHODIMP GearsFactory::GetPermission(PRBool *retval) {
  JsParamFetcher js_params(this);
  std::string16 site_name;
  std::string16 image_url;
  std::string16 extra_message;

  if (js_params.IsOptionalParamPresent(0, false)) {
    if (!js_params.GetAsString(0, &site_name))
      RETURN_EXCEPTION(STRING16(L"siteName must be a string."));
  }

  if (js_params.IsOptionalParamPresent(1, false)) {
    if (!js_params.GetAsString(1, &image_url))
      RETURN_EXCEPTION(STRING16(L"imageUrl must be a string."));
  }

  if (js_params.IsOptionalParamPresent(2, false)) {
    if (!js_params.GetAsString(2, &extra_message))
      RETURN_EXCEPTION(STRING16(L"extraMessage must be a string."));
  }

  scoped_ptr<PermissionsDialog::CustomContent> custom_content(
      new PermissionsDialog::CustomContent(image_url.c_str(),
                                           site_name.c_str(),
                                           extra_message.c_str()));
  assert(custom_content.get());
  if (GetPermissionsManager()->AcquirePermission(
      PermissionsDB::PERMISSION_LOCAL_DATA,
      custom_content.get())) {
    *retval = PR_TRUE;
  } else {
    *retval = PR_FALSE;
  }
  RETURN_NORMAL();
}


// Purposely ignores 'is_creation_suspended_'.  The 'hasPermission' property
// indicates whether USER opt-in is still required, not whether DEVELOPER
// methods have been called correctly (e.g. allowCrossOrigin).
NS_IMETHODIMP GearsFactory::GetHasPermission(PRBool *retval) {
  if (GetPermissionsManager()->HasPermission(
      PermissionsDB::PERMISSION_LOCAL_DATA)) {
    *retval = PR_TRUE;
  } else {
    *retval = PR_FALSE;
  }

  RETURN_NORMAL();
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
