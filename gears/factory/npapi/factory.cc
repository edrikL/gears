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
#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"
#include "gears/factory/common/factory_utils.h"

GearsFactory::GearsFactory()
    : is_creation_suspended_(false),
      is_permission_granted_(false),
      is_permission_value_from_user_(false) {
  SetActiveUserFlag();
}

void GearsFactory::Create() {
  // TODO(mpcomplete): implement me
}

void GearsFactory::GetBuildInfo() {
  // TODO(mpcomplete): implement me
}

void GearsFactory::GetVersion() {
  // TODO(mpcomplete): implement me
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
