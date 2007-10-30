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
#include "gears/base/common/permissions_db.h"
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

const char16 *kGoogleUpdateClientsRegKey =
                  STRING16(L"Software\\Google\\Update\\ClientState");
const char16 *kGoogleUpdateGearsClientGuid =
                  STRING16(L"{283EAF47-8817-4c2b-A801-AD1FADFB7BAA}");
const char16 *kGoogleUpdateDidRunValue = STRING16(L"dr");

bool ParseMajorMinorVersion(const char16 *version, int *major, int *minor) {
  const char16 *start = version;
  const char16 *end;

  bool parsed_ok = false;
  int major_version = -1;
  int minor_version = -1;

  // scan for: DIGITS '.' DIGITS '\0'
  major_version = ParseLeadingInteger(start, &end);
  if (start != end) { // equal on str16tol() error
    if (*end == L'.') {
      start = end + 1;
      minor_version = ParseLeadingInteger(start, &end);
      if (start != end) {
        if (*end == L'\0') {
          parsed_ok = true;
        }
      }
    }
  }

  if (!parsed_ok || major_version < 0 || minor_version < 0) {
    return false;
  }

  if (major) *major = major_version;
  if (minor) *minor = minor_version;
  return true; // succeeded
}


void AppendBuildInfo(std::string16 *s) {
  s->append(STRING16(PRODUCT_VERSION_STRING));

  // TODO(miket): when we have a need to come up with a schema for this
  // string, do so. This is sufficient for current (nonexistent) purposes.
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

#if BROWSER_IE
  s->append(STRING16(L";ie"));
#elif BROWSER_FF
  s->append(STRING16(L";firefox"));
#elif BROWSER_SAFARI
  s->append(STRING16(L";safari"));
#else
  s->append(STRING16(L";unknown"));
#endif
}


bool HasPermissionToUseGears(GearsFactory *factory,
                             const char16 *custom_icon_url,
                             const char16 *custom_name,
                             const char16 *custom_message) {
  // First check is_creation_suspended, because the factory can be suspended
  // even when is_permission_granted.
  if (factory->is_creation_suspended_) {
    return false;
  }

  // Check for a cached value.
  if (factory->is_permission_granted_) {
    return true;
  } else if (factory->is_permission_value_from_user_) {
    // and !factory->is_permission_granted_
    return false;
  }

  // See if the user has permanently allowed or denied this origin.
  const SecurityOrigin &origin = factory->EnvPageSecurityOrigin();

  PermissionsDB *permissions = PermissionsDB::GetDB();
  if (!permissions) { return false; }

  switch (permissions->GetCanAccessGears(origin)) {
    // Origin was found in database. Save choice for page lifetime,
    // so things continue working even if underlying database changes.
    case PermissionsDB::PERMISSION_DENIED:
      factory->is_permission_granted_ = false;
      factory->is_permission_value_from_user_ = true;
      return factory->is_permission_granted_;
    case PermissionsDB::PERMISSION_ALLOWED:
      factory->is_permission_granted_ = true;
      factory->is_permission_value_from_user_ = true;
      return factory->is_permission_granted_;
    // Origin was not found in database.
    case PermissionsDB::PERMISSION_DEFAULT:
      break;
    // All other values are unexpected.
    default:
      assert(false); 
  }

  std::string16 full_icon_url;
  if (custom_icon_url) {
    if (ResolveAndNormalize(factory->EnvPageLocationUrl().c_str(),
                            custom_icon_url,
                            &full_icon_url)) {
      custom_icon_url = full_icon_url.c_str();
    }
  }

  // Display the modal dialog. Should not happen in a faceless worker.
  assert(!factory->EnvIsWorker());
  bool allow_origin = ShowPermissionsPrompt(origin,
                                            custom_icon_url,
                                            custom_name,
                                            custom_message);

  factory->is_permission_granted_ = allow_origin;
  factory->is_permission_value_from_user_ = true;

  // Return the decision.
  return allow_origin;
}

#ifndef BROWSER_SAFARI
bool ShowPermissionsPrompt(const SecurityOrigin &origin,
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
  // We won't create any registry keys here; instead, we'll just open
  // preexisting ones. This means that this code should be a no-op on
  // installations that didn't include the Google Update service.
  //
  // We use the HKCU version of the Google Update "did run" value so that
  // we can write to it from IE on Vista.
  HKEY reg_client_state;
  DWORD result = RegOpenKeyExW(HKEY_CURRENT_USER, kGoogleUpdateClientsRegKey,
                               0, KEY_READ, &reg_client_state);
  if (result == ERROR_SUCCESS) {
    HKEY reg_client;
    result = RegOpenKeyExW(reg_client_state, kGoogleUpdateGearsClientGuid,
                           0, KEY_WRITE, &reg_client);
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
