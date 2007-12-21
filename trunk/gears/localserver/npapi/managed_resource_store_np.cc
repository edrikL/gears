// Copyright 2005, Google Inc.
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

#include "gears/localserver/npapi/managed_resource_store_np.h"

#include "gears/base/common/url_utils.h"
#include "gears/base/npapi/module_wrapper.h"

DECLARE_GEARS_WRAPPER(GearsManagedResourceStore);

// static
void Dispatcher<GearsManagedResourceStore>::Init() {
  RegisterProperty("name", &GearsManagedResourceStore::GetName, NULL);
  RegisterProperty("requiredCookie",
                   &GearsManagedResourceStore::GetRequiredCookie, NULL);
  RegisterProperty("enabled", &GearsManagedResourceStore::GetEnabled,
                   &GearsManagedResourceStore::SetEnabled);
  RegisterProperty("manifestUrl", &GearsManagedResourceStore::GetManifestUrl,
                   &GearsManagedResourceStore::SetManifestUrl);
  RegisterProperty("lastUpdateCheckTime",
                   &GearsManagedResourceStore::GetLastUpdateCheckTime, NULL);
  RegisterProperty("updateStatus",
                   &GearsManagedResourceStore::GetUpdateStatus, NULL);
  RegisterProperty("lastErrorMessage",
                   &GearsManagedResourceStore::GetLastErrorMessage, NULL);
  RegisterProperty("currentVersion",
                   &GearsManagedResourceStore::GetCurrentVersion, NULL);
  RegisterMethod("checkForUpdate",
                 &GearsManagedResourceStore::CheckForUpdate);
}

//------------------------------------------------------------------------------
// GetName
//------------------------------------------------------------------------------
void GearsManagedResourceStore::GetName(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetRequiredCookie
//------------------------------------------------------------------------------
void GearsManagedResourceStore::GetRequiredCookie(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetEnabled
//------------------------------------------------------------------------------
void GearsManagedResourceStore::GetEnabled(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// SetEnabled
//------------------------------------------------------------------------------
void GearsManagedResourceStore::SetEnabled(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetManifestUrl
//------------------------------------------------------------------------------
void GearsManagedResourceStore::GetManifestUrl(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// SetManifestUrl
//------------------------------------------------------------------------------
void  GearsManagedResourceStore::SetManifestUrl(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetLastUpdateCheckTime
//------------------------------------------------------------------------------
void GearsManagedResourceStore::GetLastUpdateCheckTime(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetUpdateStatus
//------------------------------------------------------------------------------
void GearsManagedResourceStore::GetUpdateStatus(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetLastErrorMessage
//------------------------------------------------------------------------------
void GearsManagedResourceStore::GetLastErrorMessage(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// CheckForUpdate
//------------------------------------------------------------------------------
void GearsManagedResourceStore::CheckForUpdate(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetCurrentVersion
//------------------------------------------------------------------------------
void GearsManagedResourceStore::GetCurrentVersion(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}
