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
#include "gears/workerpool/npapi/workerpool.h"
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
  std::string16 version = STRING16(L"1.0");  // default for this optional param
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &class_name },
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &version },
  };
  int argc = js_runner->GetArguments(ARRAYSIZE(argv), argv);
  if (argc < 1)
    return;  // JsRunner sets an error message.

  // Check the version string.
  if (version != kAllowedClassVersion) {
    RETURN_EXCEPTION(STRING16(L"Invalid version string. Must be 1.0."));
  }

  // TODO(mpcomplete): implement me.

  // Create an instance of the object.
  //
  // Do case-sensitive comparisons, which are always better in APIs. They make
  // code consistent across callers, and they are easier to support over time.
  ScopedModuleWrapper object(NULL);
  if (class_name == STRING16(L"beta.database")) {
    object.reset(CreateGearsDatabase(this));
  } else if (class_name == STRING16(L"beta.workerpool")) {
    object.reset(CreateGearsWorkerPool(this));
  } else {
    RETURN_EXCEPTION(STRING16(L"Unknown object."));
  }

  if (!object.get())
    return;  // Create function sets an error message.

  // Give up ownership of the object and return it.
  JsToken token = object.get()->GetWrapperToken();
  js_runner->SetReturnValue(JSPARAM_OBJECT_TOKEN, &token);

  RETURN_NORMAL();
}

void GearsFactory::GetBuildInfo() {
  std::string16 build_info;
  AppendBuildInfo(&build_info);
  GetJsRunner()->SetReturnValue(JSPARAM_STRING16, &build_info);

  RETURN_NORMAL();
}

void GearsFactory::GetVersion() {
  std::string16 version(PRODUCT_VERSION_STRING);
  GetJsRunner()->SetReturnValue(JSPARAM_STRING16, &version);
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
