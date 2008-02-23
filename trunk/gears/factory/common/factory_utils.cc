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

#ifdef WIN32
#include <windows.h>
#endif
#include "common/genfiles/product_constants.h"  // from OUTDIR
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/factory/common/factory_utils.h"
#include "gears/ui/common/permissions_dialog.h"

#if BROWSER_IE
#include "gears/factory/ie/factory.h"
#elif BROWSER_FF
#include "gears/factory/firefox/factory.h"
#elif BROWSER_NPAPI
#include "gears/factory/npapi/factory.h"
#elif BROWSER_SAFARI
#include "gears/factory/safari/factory_utils.h"
#endif


const char16 *kAllowedClassVersion = STRING16(L"1.0");


const char16 *kGoogleUpdateClientsRegKey =
                  STRING16(L"Software\\Google\\Update\\ClientState");
const char16 *kGoogleUpdateGearsClientGuid =
                  STRING16(L"{283EAF47-8817-4c2b-A801-AD1FADFB7BAA}");
const char16 *kGoogleUpdateDidRunValue = STRING16(L"dr");


void AppendBuildInfo(std::string16 *s) {
  s->append(STRING16(PRODUCT_VERSION_STRING));


#ifdef OFFICIAL_BUILD
  s->append(STRING16(L";official"));
#else
  s->append(STRING16(L";developer"));
#endif


#ifdef DEBUG
  s->append(STRING16(L";dbg"));
#else
  s->append(STRING16(L";opt"));
#endif


#if defined(OS_MACOSX)
  s->append(STRING16(L";osx"));
#elif defined(LINUX)
  s->append(STRING16(L";linux"));
#elif defined(WIN32)
#ifdef WINCE
  s->append(STRING16(L";wince"));
#else
  s->append(STRING16(L";win32"));
#endif
#else
  s->append(STRING16(L";unknown_os"));
#endif


#if BROWSER_IE
  s->append(STRING16(L";ie"));
#ifdef WINCE
  s->append(STRING16(L"_mobile"));
#endif
#elif BROWSER_FF
  s->append(STRING16(L";firefox"));
#elif BROWSER_SAFARI
  s->append(STRING16(L";safari"));
#else
  s->append(STRING16(L";unknown_browser"));
#endif
}


bool HasPermissionToUseGears(GearsFactory *factory,
                             bool use_temporary_permissions,
                             const char16 *custom_icon_url,
                             const char16 *custom_name,
                             const char16 *custom_message) {
  // First check is_creation_suspended, because the factory can be suspended
  // even when permission_state_ is an ALLOWED_* value.
  if (factory->is_creation_suspended_) {
    return false;
  }

  // Check for a cached value.
  if (factory->permission_state_ == ALLOWED_PERMANENTLY) {
    return true;
  } else if (factory->permission_state_ == DENIED_PERMANENTLY) {
    return false;
  } else if (factory->permission_state_ == ALLOWED_TEMPORARILY &&
             use_temporary_permissions) {
    return true;
  } else if (factory->permission_state_ == DENIED_TEMPORARILY &&
             use_temporary_permissions) {
    return false;
  }

  // See if the user has permanently allowed or denied this origin.
  const SecurityOrigin &origin = factory->EnvPageSecurityOrigin();

  PermissionsDB *permissions = PermissionsDB::GetDB();
  if (!permissions) { return false; }

  switch (permissions->GetCanAccessGears(origin)) {
    // Origin was found in database. Save choice for page lifetime,
    // so things continue working even if underlying database changes.
    case PermissionsDB::PERMISSION_ALLOWED:
      factory->permission_state_ = ALLOWED_PERMANENTLY;
      return true;
    case PermissionsDB::PERMISSION_DENIED:
      factory->permission_state_ = DENIED_PERMANENTLY;
      return false;
    // Origin was not found in database.
    case PermissionsDB::PERMISSION_NOT_SET:
      break;
    // All other values are unexpected.
    default:
      assert(false); 
  }

  std::string16 full_icon_url;
  if (custom_icon_url && custom_icon_url[0]) {
    if (ResolveAndNormalize(factory->EnvPageLocationUrl().c_str(),
                            custom_icon_url,
                            &full_icon_url)) {
      custom_icon_url = full_icon_url.c_str();
    }
  }

  // Display the modal dialog. Should not happen in a faceless worker.
  assert(!factory->EnvIsWorker());
  factory->permission_state_ = ShowPermissionsPrompt(origin,
                                                     custom_icon_url,
                                                     custom_name,
                                                     custom_message);

  // Return the boolean decision.
  if (factory->permission_state_ == ALLOWED_PERMANENTLY ||
      factory->permission_state_ == ALLOWED_TEMPORARILY) {
    return true;
  }
  return false;
}

#ifdef BROWSER_SAFARI
// See factory_utils.mm for (temporary) Safari version.
#else
PermissionState ShowPermissionsPrompt(const SecurityOrigin &origin,
                                      const char16 *custom_icon_url,
                                      const char16 *custom_name,
                                      const char16 *custom_message) {
  return PermissionsDialog::Prompt(origin,
                                   custom_icon_url,
                                   custom_name,
                                   custom_message);
}
#endif

void SetActiveUserFlag() {
#ifdef WIN32
  // We use the HKCU version of the Google Update "did run" value so that
  // we can write to it from IE on Vista.
  HKEY reg_client_state;
  DWORD result = RegCreateKeyExW(HKEY_CURRENT_USER, kGoogleUpdateClientsRegKey,
                                 0, NULL, 0, KEY_WRITE, NULL,
                                 &reg_client_state, NULL);
  if (result == ERROR_SUCCESS) {
    HKEY reg_client;
    result = RegCreateKeyExW(reg_client_state, kGoogleUpdateGearsClientGuid,
                             0, NULL, 0, KEY_WRITE, NULL, &reg_client, NULL);
    if (result == ERROR_SUCCESS) {
      const char16 *kValue = L"1";
      const size_t num_bytes = sizeof(kValue[0]) * 2;  // includes trailing '\0'
      RegSetValueExW(reg_client, kGoogleUpdateDidRunValue, 0, REG_SZ,
                     reinterpret_cast<const BYTE *>(kValue), num_bytes);
      RegCloseKey(reg_client);
    }
    RegCloseKey(reg_client_state);
  }
#endif  // #ifdef WIN32
}
