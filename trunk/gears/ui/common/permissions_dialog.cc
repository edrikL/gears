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

#include "gears/base/common/common.h"
#include "gears/base/common/permissions_db.h"
#include "gears/base/common/string_utils.h"
#include "gears/third_party/jsoncpp/json.h"
#include "gears/ui/common/html_dialog.h"
#include "gears/ui/common/permissions_dialog.h"


static bool ToJsonStringValue(const char16 *str,
                              Json::Value *json_value) {
  assert(str);
  assert(json_value);

  std::string str8;
  if (!String16ToUTF8(str, &str8)) {
    LOG(("PermissionsDialog::ToJsonStringValue: Could not convert string."));
    return false;
  }
  *json_value = Json::Value(str8);
  return true;
}


static bool PopulateArguments(const SecurityOrigin &origin,
                              const char16 *custom_icon,
                              const char16 *custom_name,
                              const char16 *custom_message,
                              Json::Value *args) {
  assert(args);
  assert(args->isObject());
  assert(args->size() == 0);

  // Show something more user-friendly for kUnknownDomain.
  // TODO(aa): This is needed by settings dialog too. Factor this out into a
  // common utility.
  std::string16 display_origin(origin.url());
  if (origin.host() == kUnknownDomain) {
    ReplaceAll(display_origin,
               std::string16(kUnknownDomain),
               std::string16(STRING16(L"<no domain>")));
  }

  if (!ToJsonStringValue(display_origin.c_str(), &((*args)["origin"]))) {
    return false;
  }

  if (custom_icon && custom_icon[0] &&
      !ToJsonStringValue(custom_icon, &((*args)["customIcon"]))) {
    return false;
  }

  if (custom_name && custom_name[0] &&
      !ToJsonStringValue(custom_name, &((*args)["customName"]))) {
    return false;
  }

  if (custom_message && custom_message[0] &&
      !ToJsonStringValue(custom_message, &((*args)["customMessage"]))) {
    return false;
  }

  return true;
}


PermissionState PermissionsDialog::Prompt(const SecurityOrigin &origin,
                                          const char16 *custom_icon,
                                          const char16 *custom_name,
                                          const char16 *custom_message) {
  // Note: Arguments and results are coupled to the values that
  // permissions_dialog.html.m4 is expecting.
  HtmlDialog dialog;
  if (!PopulateArguments(origin, custom_icon, custom_name, custom_message,
                         &dialog.arguments)) {
    return NOT_SET;
  }

#ifdef WINCE
  const int kDialogWidth = 230;
  const int kDialogHeight = 240;
#else
  const int kDialogWidth = 360;
  const int kDialogHeight = 215;
#endif
  const char16 *kDialogFile = STRING16(L"permissions_dialog.html");

  dialog.DoModal(kDialogFile, kDialogWidth, kDialogHeight);

  // Extract the dialog results.
  bool allow;
  bool permanently;

  if (dialog.result == Json::Value::null) {
    // A null dialog.result is okay; it means the user closed the dialog
    // instead of pressing any button.
    allow = false;
    permanently = false;
  } else {
    if (!dialog.result["allow"].isBool() ||
        !dialog.result["permanently"].isBool()) {
      assert(false);
      LOG(("PermissionsDialog::Prompt() - Unexpected result."));
      return NOT_SET;
    }

    allow = dialog.result["allow"].asBool();
    permanently = dialog.result["permanently"].asBool();
  }

  // Store permanent results in the DB.
  if (permanently) {
    PermissionsDB::PermissionValue value = 
                       allow ? PermissionsDB::PERMISSION_ALLOWED
                             : PermissionsDB::PERMISSION_DENIED;
    PermissionsDB *permissions = PermissionsDB::GetDB();
    if (permissions) {
      permissions->SetCanAccessGears(origin, value);
    }
  }

  // Convert the results to a PermissionState value.
  if (allow) {
    if (permanently) return ALLOWED_PERMANENTLY;
    else return ALLOWED_TEMPORARILY;
  } else {
    if (permanently) return DENIED_PERMANENTLY;
    else return DENIED_TEMPORARILY;
  }
}
