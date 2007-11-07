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
#include "gears/base/common/js_runner.h"
#include "gears/base/common/string16.h"
#include "gears/database/npapi/database.h"
#include "gears/factory/common/factory_utils.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

GearsFactory::GearsFactory()
    : is_creation_suspended_(false),
      is_permission_granted_(false),
      is_permission_value_from_user_(false) {
  SetActiveUserFlag();
}

void GearsFactory::Create() {
  JsRunnerInterface* js_runner = GetJsRunner();

  // TODO(mpcomplete): implement HTMLDialog.
#if 0
  if (!HasPermissionToUseGears(this, NULL, NULL, NULL)) {
    RETURN_EXCEPTION(STRING16(L"Page does not have permission to use "
                              PRODUCT_FRIENDLY_NAME L"."));
  }
#endif

  std::string16 class_name;
  std::string16 version;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &class_name },
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &version },
  };
  int argc = js_runner->GetArguments(ARRAYSIZE(argv), argv);
  if (argc < 2)
    return;  // JsRunner sets an error message.

  // TODO(mpcomplete): implement me.

  // Parse the version string.

  int major_version_desired;
  int minor_version_desired;
  if (!ParseMajorMinorVersion(version.c_str(),
                              &major_version_desired,
                              &minor_version_desired)) {
    RETURN_EXCEPTION(STRING16(L"Invalid version string."));
  }

  // Create an instance of the object.
  //
  // Do case-sensitive comparisons, which are always better in APIs. They make
  // code consistent across callers, and they are easier to support over time.
  ScopedModuleWrapper object(NULL);
  if (class_name == STRING16(L"beta.database")) {
    if (major_version_desired == kGearsDatabaseVersionMajor &&
        minor_version_desired <= kGearsDatabaseVersionMinor) {
      object.reset(CreateGearsDatabase(this));
    }
  } else {
    RETURN_EXCEPTION(STRING16(L"Unknown object."));
  }

  if (!object.get())
    RETURN_EXCEPTION(STRING16(L"Failed to create requested object."));

  if (!object.get()->GetGearsObject()->InitBaseFromSibling(this))
    RETURN_EXCEPTION(STRING16(L"Error initializing base class."));

  // Give up ownership of the object and return it.
  JsToken token = object.get()->GetWrapperToken();
  JsReturnValue js_retval = { JSPARAM_OBJECT_TOKEN, &token };
  js_runner->SetReturnValue(js_retval);

  RETURN_NORMAL();
}

void GearsFactory::GetBuildInfo() {
  std::string16 build_info;
  AppendBuildInfo(&build_info);
  JsReturnValue js_retval = { JSPARAM_STRING16, &build_info };
  GetJsRunner()->SetReturnValue(js_retval);

  RETURN_NORMAL();
}

void GearsFactory::GetVersion() {
  std::string16 version(PRODUCT_VERSION_STRING);
  JsReturnValue js_retval = { JSPARAM_STRING16, &version };
  GetJsRunner()->SetReturnValue(js_retval);
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
