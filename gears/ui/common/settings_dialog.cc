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

#include "gears/ui/common/settings_dialog.h"

#include "gears/base/common/string_utils.h"
#include "gears/ui/common/html_dialog.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

static const char *kPermissionsString = "permissions";
static const char *kLocalStorageString = "localStorage";
static const char *kLocationDataString = "locationData";
static const char *kPermissionStateString = "permissionState";
static const char16 *kSettingsDialogString = STRING16(L"settings_dialog.html");

// Gets sites where the permission for local data has the given value and
// appends them to the array.
static bool GetLocalDataPermissions(PermissionsDB::PermissionValue value,
                                    Json::Value *json_object);

#ifdef BROWSER_WEBKIT
bool SettingsDialog::visible_ = false;
#endif

#ifdef BROWSER_WEBKIT
void ProcessResultsCallback(Json::Value *result, void *closure) {
  scoped_ptr<HtmlDialog> dialog(static_cast<HtmlDialog *>(closure));
  if (result) {
    SettingsDialog::ProcessResult(result);
  }
  SettingsDialog::SetVisible(false);
}
#endif

void SettingsDialog::Run() {
  scoped_ptr<HtmlDialog> settings_dialog(new HtmlDialog());

  // Populate the arguments property.
  settings_dialog->arguments[kPermissionsString] =
      Json::Value(Json::objectValue);
  if (!PopulatePermissions(&(settings_dialog->arguments[kPermissionsString]))) {
    return;
  }

  // Show the dialog.
  const int kSettingsDialogWidth = 400;
  const int kSettingsDialogHeight = 350;
#ifdef BROWSER_WEBKIT
  visible_ = true;
  if (!settings_dialog->DoModeless(STRING16(kSettingsDialogString),
                                   kSettingsDialogWidth, kSettingsDialogHeight,
                                   ProcessResultsCallback, 
                                   settings_dialog.get()) ) {
    return;
  }
  settings_dialog.release();
#else
  settings_dialog->DoModal(STRING16(kSettingsDialogString),
                           kSettingsDialogWidth, kSettingsDialogHeight);

  // Process the result() property and update any permissions or shortcuts.
  ProcessResult(&settings_dialog->result);
#endif
}


// static
bool SettingsDialog::PopulatePermissions(Json::Value *json_object) {
  // Populate the supplied object with permissions for all origins. The object
  // has a property named for each domain. Each of these properties is an object
  // with properties named for each class of permissions. Each of these is also
  // an object, with a 'permissionState' property and other properties specific
  // to the class. For example, the localStorage permission class may have
  // members describing current disk space use and whether the storage should be
  // cleared.
  // eg.
  // permissions: {
  //   "http://www.google.com": {
  //     localStorage: { permissionState: 1 },
  //     locationData: { permissionState: 0 }
  //   }
  // }
  // TODO(aa): Pass list of created shortcuts here.
  //
  // TODO(steveblock): Domains which are denied for all classes of permissions
  // should come last. Sites should be sorted alphabetically within each group.

  // For now, we only get permissions for local storage.
  // TODO(andreip): Add
  // PermissionsDB::GetAllPermissions(*map<origin_name,
  //                                  map<permission_class, permission_state> >)
  // or equivalent.
  return
      GetLocalDataPermissions(PermissionsDB::PERMISSION_ALLOWED, json_object) &&
      GetLocalDataPermissions(PermissionsDB::PERMISSION_DENIED, json_object);
}

void SettingsDialog::ProcessResult(Json::Value *dialog_result) {
  // Note: the logic here is coupled to the way the JS inside of
  // settings_dialog.html.m4 packages it's results. As implemented now, the
  // dialog should only ever return null or an object containing ...
  // - modifiedOrigins : an object where each property gives the permission
  //                     classes modified for a given origin.
  // - TODO            : an array containing zero or more shortcuts to remove.

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

  const Json::Value modified_origins = (*dialog_result)["modifiedOrigins"];
  if (!modified_origins.isObject()) {
    LOG(("SettingsDialog::ProcessResult: Invalid modified_origins type."));
    assert(false);
    return;
  }

  std::vector<std::string> origin_names = modified_origins.getMemberNames();
  for (int i = 0; i < static_cast<int>(origin_names.size()); ++i) {
    std::string16 origin;
    if (!UTF8ToString16(origin_names[i].c_str(), &origin)) {
      LOG(("SettingsDialog::ProcessResult: Could not convert name."));
      continue;
    }

    SecurityOrigin security_origin;
    if (!security_origin.InitFromUrl(origin.c_str())) {
      continue;
    }

    Json::Value entry = modified_origins[origin_names[i]];
    if (!entry.isObject()) {
      LOG(("SettingsDialog::ProcessResult: Invalid entry type."));
      assert(false);
      continue;
    }

    Json::Value local_storage_permission =
        entry[kLocalStorageString][kPermissionStateString];
    if (local_storage_permission.isInt()) {
      PermissionsDB::PermissionValue local_storage_permission_value =
          static_cast<PermissionsDB::PermissionValue>(
          local_storage_permission.asInt());
      capabilities->SetPermission(security_origin,
                                  PermissionsDB::PERMISSION_LOCAL_DATA,
                                  local_storage_permission_value);
    }

    Json::Value location_data_permission =
        entry[kLocationDataString][kPermissionStateString];
    if (location_data_permission.isInt()) {
      PermissionsDB::PermissionValue location_data_permission_value =
          static_cast<PermissionsDB::PermissionValue>(
          location_data_permission.asInt());
      capabilities->SetPermission(security_origin,
                                  PermissionsDB::PERMISSION_LOCATION_DATA,
                                  location_data_permission_value);
    }
  }
}

// Local function
bool GetLocalDataPermissions(PermissionsDB::PermissionValue value,
                             Json::Value *json_object) {
  PermissionsDB *capabilities = PermissionsDB::GetDB();
  if (!capabilities) {
    return false;
  }

  std::vector<SecurityOrigin> list;
  if (!capabilities->GetOriginsByValue(value,
                                       PermissionsDB::PERMISSION_LOCAL_DATA,
                                       &list)) {
    return false;
  }

  assert(json_object->isObject());
  for (size_t i = 0; i < list.size(); ++i) {
    // JSON needs UTF-8.
    std::string origin_string;
    if (!String16ToUTF8(list[i].url().c_str(), &origin_string)) {
      LOG(("SettingsDialog::PopulateSites: Could not convert origin string."));
      return false;
    }
    Json::Value localStorage;
    localStorage[kPermissionStateString] = Json::Value(value);
    Json::Value entry;
    entry[kLocalStorageString] = localStorage;
    (*json_object)[origin_string] = entry;
  }

  return true;
}
