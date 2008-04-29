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

#include "gears/desktop/desktop.h"

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/file.h"
#include "gears/base/common/http_utils.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/permissions_db.h"
#include "gears/base/common/png_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/desktop/file_dialog_utils.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/http_request.h"
#include "gears/ui/common/html_dialog.h"

#include "third_party/scoped_ptr/scoped_ptr.h"

#if defined(OFFICIAL_BUILD) || BROWSER_NPAPI
#define USE_FILE_PICKER 0
#else
#define USE_FILE_PICKER 1
#endif
DECLARE_GEARS_WRAPPER(GearsDesktop);

template<>
void Dispatcher<GearsDesktop>::Init() {
  RegisterMethod("createShortcut", &GearsDesktop::CreateShortcut);
#if USE_FILE_PICKER
  RegisterMethod("getLocalFiles", &GearsDesktop::GetLocalFiles);
#else
  // File picker is not ready for this build
#endif  // USE_FILE_PICKER
}


static const int kControlPanelIconDimensions = 16;
#ifdef WIN32
static const PngUtils::ColorFormat kDesktopIconFormat = PngUtils::FORMAT_BGRA;
#else
static const PngUtils::ColorFormat kDesktopIconFormat = PngUtils::FORMAT_RGBA;
#endif


void GearsDesktop::CreateShortcut(JsCallContext *context) {
  if (EnvIsWorker()) {
    context->SetException(
                 STRING16(L"createShortcut is not supported in workers."));
    return;
  }

  GearsDesktop::ShortcutInfo shortcut_info;
  JsObject icons;

  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &shortcut_info.app_name },
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &shortcut_info.app_url },
    { JSPARAM_REQUIRED, JSPARAM_OBJECT, &icons },
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &shortcut_info.app_description },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  if (shortcut_info.app_name.empty()) {
    context->SetException(STRING16(L"Required argument 1 is missing."));
    return;
  }

  if (shortcut_info.app_url.empty()) {
    context->SetException(STRING16(L"Required argument 2 is missing."));
    return;
  }

  // Verify that the name is acceptable.
  std::string16 error;

  // Gears doesn't allow spaces in path names, but desktop shortcuts are the
  // exception, so instead of rewriting our path validation code, patch a
  // temporary string.
  std::string16 name_without_spaces = shortcut_info.app_name;
  ReplaceAll(name_without_spaces, std::string16(STRING16(L" ")),
             std::string16(STRING16(L"_")));
  if (!IsUserInputValidAsPathComponent(name_without_spaces, &error)) {
    context->SetException(error);
    return;
  }

  // Normalize and resolve, in case this is a relative URL.
  if (!ResolveUrl(&shortcut_info.app_url, &error)) {
    context->SetException(error);
    return;
  }

  // Get the icons the user specified
  icons.GetPropertyAsString(STRING16(L"16x16"), &shortcut_info.icon16x16.url);
  icons.GetPropertyAsString(STRING16(L"32x32"), &shortcut_info.icon32x32.url);
  icons.GetPropertyAsString(STRING16(L"48x48"), &shortcut_info.icon48x48.url);
  icons.GetPropertyAsString(STRING16(L"128x128"),
                            &shortcut_info.icon128x128.url);

  // Validate that we got at least one that we can use on all platforms
  if (shortcut_info.icon16x16.url.empty() &&
      shortcut_info.icon32x32.url.empty() &&
      shortcut_info.icon48x48.url.empty() &&
      shortcut_info.icon128x128.url.empty()) {
    context->SetException(STRING16(L"Invalid value for icon parameter. At "
                                   L"least one icon must be specified."));
    return;
  }

  // Resolve the icon urls
  if (!shortcut_info.icon16x16.url.empty() &&
      !ResolveUrl(&shortcut_info.icon16x16.url, &error) ||
      !shortcut_info.icon32x32.url.empty() &&
      !ResolveUrl(&shortcut_info.icon32x32.url, &error) ||
      !shortcut_info.icon48x48.url.empty() &&
      !ResolveUrl(&shortcut_info.icon48x48.url, &error) ||
      !shortcut_info.icon128x128.url.empty() &&
      !ResolveUrl(&shortcut_info.icon128x128.url, &error)) {
    context->SetException(error);
    return;
  }

  // Check if we should display UI.
  bool allow_create_shortcut;
  if (!AllowCreateShortcut(shortcut_info, &allow_create_shortcut)) {
    context->SetException(GET_INTERNAL_ERROR_MESSAGE());
    return;
  }

  if (!allow_create_shortcut) {
    return;
  }

  // Set up the shortcuts dialog
  HtmlDialog shortcuts_dialog;
  shortcuts_dialog.arguments = Json::Value(Json::objectValue);

  // Json needs utf8.
  std::string app_url_utf8;
  std::string app_name_utf8;
  std::string app_description_utf8;
  std::string icon16_url_utf8;
  std::string icon32_url_utf8;
  std::string icon48_url_utf8;
  std::string icon128_url_utf8;
  if (!String16ToUTF8(shortcut_info.app_url.c_str(), &app_url_utf8) ||
      !String16ToUTF8(shortcut_info.app_name.c_str(), &app_name_utf8) ||
      !String16ToUTF8(shortcut_info.app_description.c_str(),
                      &app_description_utf8) ||
      !String16ToUTF8(shortcut_info.icon16x16.url.c_str(), &icon16_url_utf8) ||
      !String16ToUTF8(shortcut_info.icon32x32.url.c_str(), &icon32_url_utf8) ||
      !String16ToUTF8(shortcut_info.icon48x48.url.c_str(), &icon48_url_utf8) ||
      !String16ToUTF8(shortcut_info.icon128x128.url.c_str(),
                      &icon128_url_utf8)) {
    context->SetException(GET_INTERNAL_ERROR_MESSAGE());
    return;
  }

  // Populate the JSON object we're passing to the dialog.
  shortcuts_dialog.arguments["name"] = Json::Value(app_name_utf8);
  shortcuts_dialog.arguments["link"] = Json::Value(app_url_utf8);
  shortcuts_dialog.arguments["description"] = Json::Value(app_description_utf8);
  shortcuts_dialog.arguments["icon16x16"] = Json::Value(icon16_url_utf8);
  shortcuts_dialog.arguments["icon32x32"] = Json::Value(icon32_url_utf8);
  shortcuts_dialog.arguments["icon48x48"] = Json::Value(icon48_url_utf8);
  shortcuts_dialog.arguments["icon128x128"] = Json::Value(icon128_url_utf8);

  // Show the dialog.
  // TODO(cprince): Consider moving this code to /ui/common/shortcut_dialog.cc
  // to keep it alongside the permission and settings dialog code.  And consider
  // sharing these constants to keep similar dialogs the same size.
  const int kShortcutsDialogWidth = 360;
  const int kShortcutsDialogHeight = 220;
  shortcuts_dialog.DoModal(STRING16(L"shortcuts_dialog.html"),
                           kShortcutsDialogWidth, kShortcutsDialogHeight);

  bool allow = false;
  bool permanently = false;

  if (shortcuts_dialog.result == Json::Value::null) {
    // A null value is ok.  We interpret it as denying the shortcut temporarily.
    allow = false;
  } else if(shortcuts_dialog.result["allow"].isBool()) {
    allow = shortcuts_dialog.result["allow"].asBool();
    
    // We interpret an explicit false as permanently denying shortcut creation.
    if (!allow) {
      permanently = true;
    }
  } else {
    assert(false);
    LOG(("CreateShortcut: Unexpected result"));
    return;
  }

  if (allow) {
    // Ensure the directory we'll be storing the icons in exists.
    std::string16 icon_dir;
    if (!GetDataDirectory(EnvPageSecurityOrigin(), &icon_dir)) {
      context->SetException(GET_INTERNAL_ERROR_MESSAGE());
      return;
    }
    AppendDataName(STRING16(L"icons"), kDataSuffixForDesktop, &icon_dir);
    if (!File::RecursivelyCreateDir(icon_dir.c_str())) {
      context->SetException(GET_INTERNAL_ERROR_MESSAGE());
      return;
    }
  }

  // TODO(zork): Get the shortcut location from the dialog.
  uint32 locations =
#ifdef WINCE
      // WinCE only supports the start menu.
      SHORTCUT_LOCATION_STARTMENU;
#else
      SHORTCUT_LOCATION_DESKTOP;
#endif
  if (!SetShortcut(&shortcut_info, allow, permanently,
                   locations, &error)) {
    context->SetException(error);
  }
}

// Check whether or not to create shortcut and display UI.
// Reasons for not displaying UI:
// * The shortcut already exists and is identical to the current one.
// * The "never allow bit" is set for that shortcut.
bool GearsDesktop::AllowCreateShortcut(
                       const GearsDesktop::ShortcutInfo &shortcut_info,
                       bool *allow) {
  PermissionsDB *capabilities = PermissionsDB::GetDB();
  if (!capabilities) {
   return false;
  }

  std::string16 app_url;
  std::string16 icon16x16_url;
  std::string16 icon32x32_url;
  std::string16 icon48x48_url;
  std::string16 icon128x128_url;
  std::string16 msg;
  bool allow_shortcut_creation;
  if (!capabilities->GetShortcut(EnvPageSecurityOrigin(),
                                 shortcut_info.app_name.c_str(),
                                 &app_url,
                                 &icon16x16_url,
                                 &icon32x32_url,
                                 &icon48x48_url,
                                 &icon128x128_url,
                                 &msg,
                                 &allow_shortcut_creation)) {
    // If shortcut doesn't exist in DB then it's OK to create it.
    *allow = true;
    return true;
  }

  // If user has elected not to display shortcut UI, then forbid creation.
  if (!allow_shortcut_creation) {
    *allow = false;
    return true;
  }

  // Check that input parameters exactly match those stored in the table.
  // Note that we do not take the description parameter into consideration when 
  // doing so!
  if (app_url != shortcut_info.app_url) {
    *allow = true;
    return true;
  }

  // Compare icons urls.
  if(icon16x16_url != shortcut_info.icon16x16.url ||
     icon32x32_url != shortcut_info.icon32x32.url ||
     icon48x48_url != shortcut_info.icon48x48.url ||
     icon128x128_url != shortcut_info.icon128x128.url) {
     *allow = true;
     return true;
   }

  *allow = false;
  return true;
}

#if USE_FILE_PICKER

// Display an open file dialog returning the selected files.
// Parameters:
//  filters - in - a vector of filters
//  module - in - used to create javascript objects and arrays
//  files - out - a vector of filenames
//  error - out - the error message is placed in here
static bool DisplayFileDialog(const std::vector<FileDialog::Filter> &filters,
                              const ModuleImplBaseClass& module,
                              std::vector<std::string16> *files,
                              std::string16* error) {
  assert(files);
  assert(error);
  // create and display dialog
  scoped_ptr<FileDialog> dialog(NewFileDialog(FileDialog::MULTIPLE_FILES,
                                              module));
  if (!dialog.get()) {
    *error = STRING16(L"Failed to create dialog.");
    return false;
  }
  return dialog->OpenDialog(filters, files, error);
}

void GearsDesktop::GetLocalFiles(JsCallContext *context) {
  JsArray filters;

  JsArgument argv[] = {
    { JSPARAM_OPTIONAL, JSPARAM_ARRAY, &filters },
  };
  int argc = context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  // TODO(cdevries): set focus to tab where this function was called

  // Form the vector of filters.
  std::vector<FileDialog::Filter> vec_filters;
  std::string16 error;
  // If the optional JavaScript array was provided, convert it.
  if (argc == 1) {
    if (!FileDialogUtils::FiltersToVector(filters, &vec_filters, &error)) {
      context->SetException(error);
      return;
    }
  }
  // If the optional JavaScript array was not provided, or has zero length,
  // set a default value.
  if (vec_filters.empty()) {
    FileDialog::Filter filter = { STRING16(L"All Files"), STRING16(L"*.*") };
    vec_filters.push_back(filter);
  }

  std::vector<std::string16> files;
  if (!DisplayFileDialog(vec_filters, *this, &files, &error)) {
    context->SetException(error);
    return;
  }

  // Convert returned files.
  scoped_ptr<JsArray> files_array(GetJsRunner()->NewArray());
  if (!files_array.get()) {
    context->SetException(STRING16("Failed to create file array."));
    return;
  }
  if (!FileDialogUtils::FilesToJsObjectArray(files, *this, files_array.get(),
                                             &error)) {
    context->SetException(error);
    return;
  }

  context->SetReturnValue(JSPARAM_ARRAY, files_array.get());
}

#else
// File picker is not ready for this build
#endif  // USE_FILE_PICKER

// Handle all the icon creation and creation call required to actually install
// a shortcut.
bool GearsDesktop::SetShortcut(GearsDesktop::ShortcutInfo *shortcut,
                               const bool allow,
                               const bool permanently,
                               uint32 locations,
                               std::string16 *error) {
  PermissionsDB *capabilities = PermissionsDB::GetDB();
  if (!capabilities) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  // Create the database entry.

  // If the user wants to deny shortcut creation permanently then write to the
  // db & return.
  if (!allow && permanently) {
    capabilities->SetShortcut(EnvPageSecurityOrigin(),
                              shortcut->app_name.c_str(),
                              shortcut->app_url.c_str(),
                              shortcut->icon16x16.url.c_str(),
                              shortcut->icon32x32.url.c_str(),
                              shortcut->icon48x48.url.c_str(),
                              shortcut->icon128x128.url.c_str(),
                              shortcut->app_description.c_str(),
                              allow);

    return true;
  }

  // If the user denies shortcut creation temporarily, this is where we bail
  // from the creation logic.
  if (!allow) {
    return true;
  }

  if (!FetchIcon(&shortcut->icon16x16, 16, error) ||
      !FetchIcon(&shortcut->icon32x32, 32, error) ||
      !FetchIcon(&shortcut->icon48x48, 48, error) ||
      !FetchIcon(&shortcut->icon128x128, 128, error)) {
    return false;
  }

  // Save the icon file that will be used in the control panel
  if (!WriteControlPanelIcon(*shortcut)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

#if defined(WIN32) || defined(OS_MACOSX)
  const GearsDesktop::IconData *next_largest_provided = NULL;

  // For each icon size, we use the provided one if available.  If not, and we
  // have a larger version, we scale the closest image to fit because our
  // box-filter minify looks better than the automatic minify done by the OS.

  if (!shortcut->icon128x128.raw_data.empty()) {
    next_largest_provided = &shortcut->icon128x128;
  }

  if (shortcut->icon48x48.raw_data.empty()) {
    if (next_largest_provided) {
      shortcut->icon48x48.width = 48;
      shortcut->icon48x48.height = 48;
      PngUtils::ShrinkImage(&next_largest_provided->raw_data.at(0),
                            next_largest_provided->width,
                            next_largest_provided->height, 48, 48,
                            &shortcut->icon48x48.raw_data);
    }
  } else {
    next_largest_provided = &shortcut->icon48x48;
  }

  if (shortcut->icon32x32.raw_data.empty()) {
    if (next_largest_provided) {
      shortcut->icon32x32.width = 32;
      shortcut->icon32x32.height = 32;
      PngUtils::ShrinkImage(&next_largest_provided->raw_data.at(0),
                            next_largest_provided->width,
                            next_largest_provided->height, 32, 32,
                            &shortcut->icon32x32.raw_data);
    }
  } else {
    next_largest_provided = &shortcut->icon32x32;
  }

  if (shortcut->icon16x16.raw_data.empty()) {
    if (next_largest_provided) {
      shortcut->icon16x16.width = 16;
      shortcut->icon16x16.height = 16;
      PngUtils::ShrinkImage(&next_largest_provided->raw_data.at(0),
                            next_largest_provided->width,
                            next_largest_provided->height, 16, 16,
                            &shortcut->icon16x16.raw_data);
    }
  }
#endif

  // Create the desktop shortcut using platform-specific code
  assert(allow);
  if (!CreateShortcutPlatformImpl(EnvPageSecurityOrigin(), *shortcut,
                                  locations, error)) {
    return false;
  }

  capabilities->SetShortcut(EnvPageSecurityOrigin(),
                            shortcut->app_name.c_str(),
                            shortcut->app_url.c_str(),
                            shortcut->icon16x16.url.c_str(),
                            shortcut->icon32x32.url.c_str(),
                            shortcut->icon48x48.url.c_str(),
                            shortcut->icon128x128.url.c_str(),
                            shortcut->app_description.c_str(),
                            allow);
  return true;
}

bool GearsDesktop::WriteControlPanelIcon(
                       const GearsDesktop::ShortcutInfo &shortcut) {
  const GearsDesktop::IconData *chosen_icon = NULL;

  // Pick the best icon we can for the control panel
  if (!shortcut.icon16x16.png_data.empty()) {
    chosen_icon = &shortcut.icon16x16;
  } else if (!shortcut.icon32x32.png_data.empty()) {
    chosen_icon = &shortcut.icon32x32;
  } else if (!shortcut.icon48x48.png_data.empty()) {
    chosen_icon = &shortcut.icon48x48;
  } else if (!shortcut.icon128x128.png_data.empty()) {
    chosen_icon = &shortcut.icon128x128;
  } else {
    // Caller should have ensured that there was at least one icon
    assert(false);
  }

  std::string16 icon_loc;
  if (!GetControlPanelIconLocation(EnvPageSecurityOrigin(), shortcut.app_name,
                                   &icon_loc)) {
    return false;
  }

  // Write the control panel icon.
  File::CreateNewFile(icon_loc.c_str());
  return File::WriteVectorToFile(icon_loc.c_str(), &chosen_icon->png_data);
}

bool GearsDesktop::FetchIcon(GearsDesktop::IconData *icon, int expected_size,
                             std::string16 *error) {
  // Icons are optional. Only try to fetch if one was provided.
  if (icon->url.empty()) {
    return true;
  }

  // Get the current icon.
  if (IsDataUrl(icon->url.c_str())) {
    // data URL
    std::string16 mime_type, charset;
    if (!ParseDataUrl(icon->url, &mime_type, &charset, &icon->png_data)) {
      *error = STRING16(L"Could not load icon ");
      *error += icon->url.c_str();
      *error += STRING16(L".");
      return false;
    }
  } else {
    // Fetch the png data
    scoped_refptr<HttpRequest> request;
    HttpRequest::Create(&request);

    request->SetCachingBehavior(HttpRequest::USE_ALL_CACHES);
    request->SetRedirectBehavior(HttpRequest::FOLLOW_ALL);

    int status = 0;
    if (!request->Open(HttpConstants::kHttpGET, icon->url.c_str(), false) ||
        !request->Send() ||
        !request->GetStatus(&status) ||
        status != HTTPResponse::RC_REQUEST_OK) {
      *error = STRING16(L"Could not load icon ");
      *error += icon->url.c_str();
      *error += STRING16(L".");
      return false;
    }

    // Extract the data.
    if (!request->GetResponseBody(&icon->png_data)) {
      *error = STRING16(L"Invalid data for icon ");
      *error += icon->url;
      *error += STRING16(L".");
      return false;
    }
  }

  // Decode the png
  if (!PngUtils::Decode(&icon->png_data.at(0), icon->png_data.size(),
                        kDesktopIconFormat,
                        &icon->raw_data, &icon->width, &icon->height)) {
    *error = STRING16(L"Could not decode PNG data for icon ");
    *error += icon->url;
    *error += STRING16(L".");
    return false;
  }

  if (icon->width != expected_size || icon->height != expected_size) {
    *error = STRING16(L"Icon ");
    *error += icon->url;
    *error += STRING16(L" has incorrect size. Expected ");
    *error += IntegerToString16(expected_size);
    *error += STRING16(L"x");
    *error += IntegerToString16(expected_size);
    *error += STRING16(L".");
    return false;
  }

  return true;
}

// Get the location of one of the icon files stored in the data directory.
bool GearsDesktop::GetControlPanelIconLocation(const SecurityOrigin &origin,
                                               const std::string16 &app_name,
                                               std::string16 *icon_loc) {
  if (!GetDataDirectory(origin, icon_loc)) {
    return false;
  }

  AppendDataName(STRING16(L"icons"), kDataSuffixForDesktop, icon_loc);
  *icon_loc += kPathSeparator;
  *icon_loc += app_name;
  *icon_loc += STRING16(L"_cp");
  *icon_loc += STRING16(L".png");

  return true;
}

bool GearsDesktop::ResolveUrl(std::string16 *url, std::string16 *error) {
  std::string16 full_url;
  if (IsDataUrl(url->c_str()))
    return true;  // don't muck with data URLs

  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url->c_str(),
                           &full_url)) {
    *error = STRING16(L"Could not resolve url ");
    *error += *url;
    *error += STRING16(L".");
    return false;
  }
  *url = full_url;
  return true;
}
