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

#include "gears/base/common/string_utils.h"
#include "gears/ui/common/html_dialog.h"
#include "gears/ui/common/settings_dialog.h"

void SettingsDialog::Run() {
  HtmlDialog settings_dialog;

  // Populate the arguments() property with the current allowed and denied
  // sites.
  if (!PopulateOrigins(&settings_dialog.arguments["allowed"],
                       PermissionsDB::PERMISSION_ALLOWED)) {
    return;
  }
  if (!PopulateOrigins(&settings_dialog.arguments["denied"],
                       PermissionsDB::PERMISSION_DENIED)) {
    return;
  }
  if (!PopulateShortcuts(&settings_dialog.arguments["shortcuts"])) {
    return;
  }

  // Show the dialog.
  const int kSettingsDialogWidth = 400;
  const int kSettingsDialogHeight = 350;
  settings_dialog.DoModal(STRING16(L"settings_dialog.html"),
                          kSettingsDialogWidth, kSettingsDialogHeight);

  // Process the result() property and remove any sites or shortcuts the user
  // removed.
  ProcessResult(&settings_dialog.result);
}


bool SettingsDialog::PopulateOrigins(Json::Value *json_array,
    PermissionsDB::PermissionValue value) {
  PermissionsDB *capabilities = PermissionsDB::GetDB();
  if (!capabilities) {
    return false;
  }

  std::vector<SecurityOrigin> list;
  if (!capabilities->GetOriginsByValue(value, &list)) {
    return false;
  }

  (*json_array) = Json::Value(Json::arrayValue);
  for (size_t i = 0; i < list.size(); ++i) {
    // JSON needs UTF-8.
    std::string site_string;
    if (!String16ToUTF8(list[i].url().c_str(), &site_string)) {
      LOG(("SettingsDialog::PopulateSites: Could not convert site string."));
      return false;
    }

    json_array->append(Json::Value(site_string));
  }

  return true;
}


bool SettingsDialog::PopulateShortcuts(Json::Value *json_array) {
  PermissionsDB *capabilities = PermissionsDB::GetDB();
  if (!capabilities) {
    return false;
  }

  (*json_array) = Json::Value(Json::arrayValue);
  std::vector<SecurityOrigin> origins;

  if (!capabilities->GetOriginsWithShortcuts(&origins)) {
    return false;
  }

  for (std::vector<SecurityOrigin>::iterator origin = origins.begin();
       origin != origins.end(); ++origin) {
    std::vector<std::string16> names;
    if (!capabilities->GetOriginShortcuts(*origin, &names)) {
      return false;
    }

    for (std::vector<std::string16>::iterator name = names.begin();
         name != names.end(); ++name) {
      std::string16 app_url;
      std::string16 ico_url;
      std::string16 msg;

      if (!capabilities->GetShortcut(*origin, name->c_str(), &app_url,
                                     &ico_url, &msg)) {
        return false;
      }

      // JSON needs UTF-8.
      std::string app_url_utf8;
      std::string ico_url_utf8;
      std::string name_utf8;
      std::string origin_utf8;
      if (!String16ToUTF8(app_url.c_str(), &app_url_utf8) ||
          !String16ToUTF8(ico_url.c_str(), &ico_url_utf8) ||
          !String16ToUTF8(name->c_str(), &name_utf8) ||
          !String16ToUTF8(origin->url().c_str(), &origin_utf8)) {
        return false;
      }

      Json::Value shortcut;
      shortcut["iconUrl"] = Json::Value(ico_url_utf8);
      shortcut["appUrl"] = Json::Value(app_url_utf8);
      shortcut["appName"] = Json::Value(name_utf8);
      shortcut["origin"] = Json::Value(origin_utf8);

      json_array->append(shortcut);
    }
  }

  return true;
}


void SettingsDialog::ProcessResult(Json::Value *dialog_result) {
  // Note: the logic here is coupled to the way the JS inside of
  // settings_dialog.html.m4 packages it's results. As implemented now, the
  // dialog should only ever return null or an object containing an array of
  // zero or more SecurityOrigin urls, and an array containing zero or more
  // shortcuts to remove.

  if (Json::nullValue == dialog_result->type()) {
    // Nothing to do, the user pressed cancel.
    return;
  }

  if (!dialog_result->isObject()) {
    LOG(("SettingsDialog::ProcessResult: Invalid dialog_result type."));
    assert(false);
    return;
  }

  PermissionsDB *capabilities = PermissionsDB::GetDB();
  if (!capabilities) {
    return;
  }

  const Json::Value remove_sites = (*dialog_result)["removeSites"];
  if (!remove_sites.isArray()) {
    LOG(("SettingsDialog::ProcessResult: Invalid remove_sites type."));
    assert(false);
    return;
  }

  for (size_t i = 0; i < remove_sites.size(); ++i) {
    Json::Value item = remove_sites[i];

    if (!item.isString()) {
      LOG(("SettingsDialog::ProcessResult: Invalid item type."));
      assert(false);
      continue;
    }

    std::string16 origin_string;
    if (!UTF8ToString16(item.asString().c_str(), &origin_string)) {
      LOG(("SettingsDialog::ProcessResult: Could not convert origin."));
      continue;
    }

    SecurityOrigin origin;
    if (!origin.InitFromUrl(origin_string.c_str())) {
      continue;
    }

    capabilities->SetCanAccessGears(origin,
                                    PermissionsDB::PERMISSION_DEFAULT);
  }

  const Json::Value remove_shortcuts = (*dialog_result)["removeShortcuts"];
  if (!remove_shortcuts.isArray()) {
    LOG(("SettingsDialog::ProcessResult: Invalid remove_shortcuts type."));
    assert(false);
    return;
  }

  for (size_t i = 0; i < remove_shortcuts.size(); ++i) {
    Json::Value item = remove_shortcuts[i];

    if (!item.isObject()) {
      LOG(("SettingsDialog::ProcessResult: Invalid item type."));
      assert(false);
      continue;
    }

    if (!item["origin"].isString()) {
      LOG(("SettingsDialog::ProcessResult: Invalid item.origin type."));
      assert(false);
      continue;
    }

    if (!item["appName"].isString()) {
      LOG(("SettingsDialog::ProcessResult: Invalid item.appName type."));
      assert(false);
      continue;
    }

    std::string16 origin_string;
    if (!UTF8ToString16(item["origin"].asString().c_str(), &origin_string)) {
      LOG(("SettingsDialog::ProcessResult: Could not convert origin."));
      continue;
    }

    std::string16 app_name_string;
    if (!UTF8ToString16(item["appName"].asString().c_str(), &app_name_string)) {
      LOG(("SettingsDialog::ProcessResult: Could not convert appName."));
      continue;
    }

    SecurityOrigin origin;
    if (!origin.InitFromUrl(origin_string.c_str())) {
      continue;
    }

    capabilities->DeleteShortcut(origin, app_name_string.c_str());
  }
}
