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
  if (!PopulateArguments(&settings_dialog.arguments["allowed"],
                         CapabilitiesDB::CAPABILITY_ALLOWED)) {
    return;
  }
  if (!PopulateArguments(&settings_dialog.arguments["denied"],
                         CapabilitiesDB::CAPABILITY_DENIED)) {
    return;
  }

  // Show the dialog.
  const int kSettingsDialogWidth = 400;
  const int kSettingsDialogHeight = 350;
  settings_dialog.DoModal(STRING16(L"settings_dialog.html"),
                          kSettingsDialogWidth, kSettingsDialogHeight);

  // Process the result() property and remove any sites the user removed.
  ProcessResult(&settings_dialog.result);
}


bool SettingsDialog::PopulateArguments(Json::Value *json_object,
    CapabilitiesDB::CapabilityStatus status) {
  CapabilitiesDB *capabilities = CapabilitiesDB::GetDB();
  if (!capabilities) {
    return false;
  }

  std::vector<SecurityOrigin> list;
  if (!capabilities->GetOriginsByStatus(status, &list)) {
    return false;
  }

  (*json_object) = Json::Value(Json::arrayValue);
  for (size_t i = 0; i < list.size(); ++i) {
    // JSON needs UTF-8.
    std::string site_string;
    if (!String16ToUTF8(list[i].url().c_str(), &site_string)) {
      LOG(("SettingsDialog::PopulateSites: Could not convert site string."));
      return false;
    }

    json_object->append(Json::Value(site_string));
  }

  return true;
}


void SettingsDialog::ProcessResult(Json::Value *dialog_result) {
  // Note: the logic here is coupled to the way the JS inside of
  // settings_dialog.html.m4 packages it's results. As implemented now, the
  // dialog should only ever return null or an array of one or more
  // SecurityOrigin urls.

  if (Json::nullValue == dialog_result->type()) {
    // Nothing to do, the user pressed cancel.
    return;
  }

  if (!dialog_result->isArray()) {
    LOG(("SettingsDialog::ProcessResult: Invalid dialog_result type."));
    assert(false);
    return;
  }

  CapabilitiesDB *capabilities = CapabilitiesDB::GetDB();
  if (!capabilities) {
    return;
  }

  for (size_t i = 0; i < dialog_result->size(); ++i) {
    Json::Value item = (*dialog_result)[i];

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
                                    CapabilitiesDB::CAPABILITY_DEFAULT);
  }
}
