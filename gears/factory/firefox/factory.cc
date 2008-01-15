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

#include "common/genfiles/product_constants.h"  // from OUTDIR
#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/channel/firefox/channel.h"
#include "gears/console/firefox/console_ff.h"
#include "gears/database/firefox/database.h"
#include "gears/desktop/desktop_ff.h"
#include "gears/factory/common/factory_utils.h"
#include "gears/httprequest/firefox/httprequest_ff.h"
#include "gears/localserver/firefox/localserver_ff.h"
#include "gears/timer/timer.h"
#include "gears/workerpool/firefox/workerpool.h"

#ifdef DEBUG
#include "gears/cctests/test_ff.h"
#endif

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
    : is_creation_suspended_(false),
      is_permission_granted_(false),
      is_permission_value_from_user_(false) {
  SetActiveUserFlag();
}


NS_IMETHODIMP GearsFactory::Create(//const nsAString &object
                                   //const nsAString &version
                                   nsISupports **retval) {
  nsresult nr;

  // Make sure the user gives this site permission to use Gears.

  if (!HasPermissionToUseGears(this, NULL, NULL, NULL)) {
    RETURN_EXCEPTION(STRING16(L"Page does not have permission to use "
                              PRODUCT_FRIENDLY_NAME L"."));
  }

  JsParamFetcher js_params(this);

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

  std::string16 object;
  if (!js_params.GetAsString(0, &object)) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  nsCOMPtr<nsISupports> isupports = NULL;

  nr = NS_ERROR_FAILURE;
  if (object == STRING16(L"beta.channel")) {
    isupports = do_QueryInterface(new GearsChannel(), &nr);
  } else if (object == STRING16(L"beta.console")) {
    isupports = do_QueryInterface(new GearsConsole(), &nr);
  } else if (object == STRING16(L"beta.database")) {
    isupports = do_QueryInterface(new GearsDatabase(), &nr);
  } else if (object == STRING16(L"beta.desktop")) {
    isupports = do_QueryInterface(new GearsDesktop(), &nr);
  } else if (object == STRING16(L"beta.httprequest")) {
    isupports = do_QueryInterface(new GearsHttpRequest(), &nr);
  } else if (object == STRING16(L"beta.localserver")) {
    isupports = do_QueryInterface(new GearsLocalServer(), &nr);
  } else if (object == STRING16(L"beta.test")) {
#ifdef DEBUG
    isupports = do_QueryInterface(new GearsTest(), &nr);
#else
    RETURN_EXCEPTION(STRING16(L"Object is only available in debug build."));
#endif
  } else if (object == STRING16(L"beta.timer")) {
    isupports = do_QueryInterface(new GearsTimer(), &nr);
  } else if (object == STRING16(L"beta.workerpool")) {
    isupports = do_QueryInterface(new GearsWorkerPool(), &nr);
  }  else {
    RETURN_EXCEPTION(STRING16(L"Unknown object."));
  }

  // setup the ModuleImplBaseClass (copy settings from this factory)
  if (NS_SUCCEEDED(nr) && isupports) {
    bool base_init_succeeded = false;

    nsCOMPtr<GearsBaseClassInterface> idl_base =
        do_QueryInterface(isupports, &nr);
    if (NS_SUCCEEDED(nr) && idl_base) {
      ModuleImplBaseClass *native_base = NULL;
      idl_base->GetNativeBaseClass(&native_base);
      if (native_base) {
        if (native_base->InitBaseFromSibling(this)) {
          base_init_succeeded = true;
        }
      }
    }
    if (!base_init_succeeded) {
      RETURN_EXCEPTION(STRING16(L"Error initializing base class."));
    }
  }

  if (NS_FAILED(nr) || !isupports) {
    RETURN_EXCEPTION(STRING16(L"Failed to create requested object."));
  }

  *retval = isupports.get();
  (*retval)->AddRef(); // ~nsCOMPtr will Release, so must AddRef here
  assert((*retval)->AddRef() == 3 &&
         (*retval)->Release() == 2);
  RETURN_NORMAL();
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

  if (js_params.GetCount(false) != 3) {
    RETURN_EXCEPTION(STRING16(L"getPermission() requires 3 parameters."));
  } else if (!js_params.GetAsString(0, &site_name)) {
    RETURN_EXCEPTION(STRING16(L"siteName must be a string."));
  } else if (!js_params.GetAsString(1, &image_url)) {
    RETURN_EXCEPTION(STRING16(L"imageUrl must be a string."));
  } else if (!js_params.GetAsString(2, &extra_message)) {
    RETURN_EXCEPTION(STRING16(L"extraMessage must be a string."));
  }

  if (HasPermissionToUseGears(this, image_url.c_str(),
                              site_name.c_str(), extra_message.c_str())) {
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
  *retval = is_permission_granted_ ? PR_TRUE : PR_FALSE;
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
