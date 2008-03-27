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

#include "gears/factory/npapi/factory.h"

#include <assert.h>
#include <stdlib.h>

#include "common/genfiles/product_constants.h"  // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/string16.h"
#include "gears/base/npapi/module_wrapper.h"
#include "gears/database/npapi/database.h"
#include "gears/factory/common/factory_utils.h"
#include "gears/httprequest/npapi/httprequest_np.h"
#include "gears/localserver/npapi/localserver_np.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"
#include "gears/timer/timer.h"
#ifdef BROWSER_WEBKIT
// TODO(playmobil): Add support for worker pools in Safari build.
#else
#include "gears/workerpool/npapi/workerpool.h"
#endif

#ifdef DEBUG
#include "gears/cctests/test.h"
#endif

// static
template <>
void Dispatcher<GearsFactory>::Init() {
  RegisterProperty("version", &GearsFactory::GetVersion, NULL);
  RegisterMethod("create", &GearsFactory::Create);
  RegisterMethod("getBuildInfo", &GearsFactory::GetBuildInfo);
  RegisterMethod("getPermission", &GearsFactory::GetPermission);
}

GearsFactory::GearsFactory()
    : is_creation_suspended_(false),
      permission_state_(NOT_SET) {
  SetActiveUserFlag();
}

void GearsFactory::Create(JsCallContext *context) {
  bool use_temporary_permissions = true;
  if (!HasPermissionToUseGears(this, use_temporary_permissions,
                               NULL, NULL, NULL)) {
    context->SetException(STRING16(L"Page does not have permission to use "
                                   PRODUCT_FRIENDLY_NAME L"."));
    return;
  }

  std::string16 class_name;
  std::string16 version = STRING16(L"1.0");  // default for this optional param
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &class_name },
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &version },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set())
    return;

  // Check the version string.
  if (version != kAllowedClassVersion) {
    context->SetException(STRING16(L"Invalid version string. Must be 1.0."));
    return;
  }

  // Create an instance of the object.
  //
  // Do case-sensitive comparisons, which are always better in APIs. They make
  // code consistent across callers, and they are easier to support over time.
  GComPtr<ModuleImplBaseClass> object(NULL);
  if (class_name == STRING16(L"beta.database")) {
    object.reset(CreateModule<GearsDatabase>(GetJsRunner()));
  } else if (class_name == STRING16(L"beta.localserver")) {
    object.reset(CreateModule<GearsLocalServer>(GetJsRunner()));
#ifdef BROWSER_WEBKIT
// TODO(playmobil): Add support for worker pools in Safari build.
#else
  } else if (class_name == STRING16(L"beta.workerpool")) {
    object.reset(CreateModule<GearsWorkerPool>(GetJsRunner()));
#endif
  } else if (class_name == STRING16(L"beta.httprequest")) {
    object.reset(CreateModule<GearsHttpRequest>(GetJsRunner()));
  } else if (class_name == STRING16(L"beta.timer")) {
    object.reset(CreateModule<GearsTimer>(GetJsRunner()));
  } else if (class_name == STRING16(L"beta.test")) {
#ifdef DEBUG
    object.reset(CreateModule<GearsTest>(GetJsRunner()));
#else
    context->SetException(L"Object is only available in debug build.");
    return;
#endif
  } else {
    context->SetException(STRING16(L"Unknown object."));
    return;
  }

  if (!object.get())
    return;  // Create function sets an error message.

  if (!object->InitBaseFromSibling(this)) {
    context->SetException(STRING16(L"Error initializing base class."));
    return;
  }

  context->SetReturnValue(JSPARAM_MODULE, object.get());
}

void GearsFactory::GetBuildInfo(JsCallContext *context) {
  std::string16 build_info;
  AppendBuildInfo(&build_info);
  context->SetReturnValue(JSPARAM_STRING16, &build_info);
}

void GearsFactory::GetPermission(JsCallContext *context) {
  std::string16 site_name;
  std::string16 image_url;
  std::string16 extra_message;
  
  JsArgument argv[] = {
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &site_name },
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &image_url },
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &extra_message },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  
  if (context->is_exception_set())
    return;
  
  bool use_temporary_permissions = false;
  bool has_permission = false;
  if (HasPermissionToUseGears(this, use_temporary_permissions,
                              image_url.c_str(), site_name.c_str(),
                              extra_message.c_str())) {
    has_permission = true;
  }
  
  context->SetReturnValue(JSPARAM_BOOL, &has_permission);
}

void GearsFactory::GetVersion(JsCallContext *context) {
  std::string16 version(STRING16(PRODUCT_VERSION_STRING));
  context->SetReturnValue(JSPARAM_STRING16, &version);
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
