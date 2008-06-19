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
#include "gears/base/common/ipc_message_queue.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/permissions_db.h"
#include "gears/base/common/png_utils.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/serialization.h"
#include "gears/base/common/url_utils.h"
#ifdef BROWSER_WEBKIT
#include "gears/desktop/curl_icon_downloader.h"
#endif
#include "gears/desktop/file_dialog_utils.h"
#include "gears/notifier/notification.h"
#include "gears/notifier/notifier_process.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/http_request.h"
#include "gears/ui/common/html_dialog.h"

#include "third_party/scoped_ptr/scoped_ptr.h"

DECLARE_GEARS_WRAPPER(GearsDesktop);

template<>
void Dispatcher<GearsDesktop>::Init() {
  RegisterMethod("createShortcut", &GearsDesktop::CreateShortcut);
  RegisterMethod("getLocalFiles", &GearsDesktop::GetLocalFiles);

#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
#else
  RegisterMethod("createNotification", &GearsDesktop::CreateNotification);
  RegisterMethod("removeNotification", &GearsDesktop::RemoveNotification);
  RegisterMethod("showNotification", &GearsDesktop::ShowNotification);
#endif  // OFFICIAL_BUILD
}


static const int kControlPanelIconDimensions = 16;
#ifdef WIN32
static const PngUtils::ColorFormat kDesktopIconFormat = PngUtils::FORMAT_BGRA;
#else
static const PngUtils::ColorFormat kDesktopIconFormat = PngUtils::FORMAT_RGBA;
#endif

GearsDesktop::GearsDesktop()
    : ModuleImplBaseClassVirtual("GearsDesktop") {
#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
#else
  GearsNotification::RegisterAsSerializable();
#endif  // OFFICIAL_BUILD
}

#ifdef OS_ANDROID
Desktop::Desktop(const SecurityOrigin &security_origin,
                 BrowsingContext *context, NPP js_context)
    : security_origin_(security_origin),
      js_call_context_(js_context),
      browsing_context_(context) {
}
#else
Desktop::Desktop(const SecurityOrigin &security_origin,
                 BrowsingContext *context)
    : security_origin_(security_origin),
      browsing_context_(context) {
}
#endif

bool Desktop::ValidateShortcutInfo(ShortcutInfo *shortcut_info) {
  // Gears doesn't allow spaces in path names, but desktop shortcuts are the
  // exception, so instead of rewriting our path validation code, patch a
  // temporary string.
  std::string16 name_without_spaces = shortcut_info->app_name;
  ReplaceAll(name_without_spaces, std::string16(STRING16(L" ")),
             std::string16(STRING16(L"_")));
  if (!IsUserInputValidAsPathComponent(name_without_spaces, &error_)) {
    return false;
  }

  // Normalize and resolve, in case this is a relative URL.
  if (!ResolveUrl(&shortcut_info->app_url, &error_)) {
    return false;
  }

  // We only allow shortcuts to be created within origin for now.
  if (!security_origin_.IsSameOriginAsUrl(shortcut_info->app_url.c_str())) {
    error_ = STRING16(L"Cannot create cross-origin shortcuts.");
    return false;
  }

  // Validate that we got at least one icon.
  if (shortcut_info->icon16x16.url.empty() &&
      shortcut_info->icon32x32.url.empty() &&
      shortcut_info->icon48x48.url.empty() &&
      shortcut_info->icon128x128.url.empty()) {
    error_ = STRING16(L"Invalid value for icon parameter. At "
                      L"least one icon must be specified.");
    return false;
  }

  // Resolve the icon urls
  if (!shortcut_info->icon16x16.url.empty() &&
      !ResolveUrl(&shortcut_info->icon16x16.url, &error_) ||
      !shortcut_info->icon32x32.url.empty() &&
      !ResolveUrl(&shortcut_info->icon32x32.url, &error_) ||
      !shortcut_info->icon48x48.url.empty() &&
      !ResolveUrl(&shortcut_info->icon48x48.url, &error_) ||
      !shortcut_info->icon128x128.url.empty() &&
      !ResolveUrl(&shortcut_info->icon128x128.url, &error_)) {
    return false;
  }

  // Check if we should display UI.
  return AllowCreateShortcut(*shortcut_info);
}

bool Desktop::InitializeDialog(ShortcutInfo *shortcut_info,
                               HtmlDialog *shortcuts_dialog,
                               DialogStyle style) {
  shortcuts_dialog->arguments = Json::Value(Json::objectValue);

  // Json needs utf8.
  std::string app_url_utf8;
  std::string app_name_utf8;
  std::string app_description_utf8;
  std::string icon16_url_utf8;
  std::string icon32_url_utf8;
  std::string icon48_url_utf8;
  std::string icon128_url_utf8;
  if (!String16ToUTF8(shortcut_info->app_url.c_str(), &app_url_utf8) ||
      !String16ToUTF8(shortcut_info->app_name.c_str(), &app_name_utf8) ||
      !String16ToUTF8(shortcut_info->app_description.c_str(),
                      &app_description_utf8) ||
      !String16ToUTF8(shortcut_info->icon16x16.url.c_str(), &icon16_url_utf8) ||
      !String16ToUTF8(shortcut_info->icon32x32.url.c_str(), &icon32_url_utf8) ||
      !String16ToUTF8(shortcut_info->icon48x48.url.c_str(), &icon48_url_utf8) ||
      !String16ToUTF8(shortcut_info->icon128x128.url.c_str(),
                      &icon128_url_utf8)) {
    error_ = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  // Populate the JSON object we're passing to the dialog.
  shortcuts_dialog->arguments["name"] = Json::Value(app_name_utf8);
  shortcuts_dialog->arguments["link"] = Json::Value(app_url_utf8);
  shortcuts_dialog->arguments["description"] = Json::Value(app_description_utf8);
  shortcuts_dialog->arguments["icon16x16"] = Json::Value(icon16_url_utf8);
  shortcuts_dialog->arguments["icon32x32"] = Json::Value(icon32_url_utf8);
  shortcuts_dialog->arguments["icon48x48"] = Json::Value(icon48_url_utf8);
  shortcuts_dialog->arguments["icon128x128"] = Json::Value(icon128_url_utf8);

  switch (style) {
  case DIALOG_STYLE_STANDARD:
    // We default to the standard style.
    break;
  case DIALOG_STYLE_SIMPLE:
    shortcuts_dialog->arguments["style"] = Json::Value("simple");
    break;
  default:
    assert(false);
    return false;
  }

  return true;
}

bool Desktop::HandleDialogResults(ShortcutInfo *shortcut_info,
                                  HtmlDialog *shortcuts_dialog) {
  bool allow = false;
  bool permanently = false;

  if (shortcuts_dialog->result == Json::Value::null) {
    // A null value is ok.  We interpret it as denying the shortcut temporarily.
    allow = false;
  } else if(shortcuts_dialog->result["allow"].isBool()) {
    allow = shortcuts_dialog->result["allow"].asBool();

    // We interpret an explicit false as permanently denying shortcut creation.
    if (!allow) {
      permanently = true;
    }
  } else {
    assert(false);
    LOG(("CreateShortcut: Unexpected result"));
    return true;
  }

  // Default the locations.
  if (allow) {
    // Ensure the directory we'll be storing the icons in exists.
    std::string16 icon_dir;
    if (!GetDataDirectory(security_origin_, &icon_dir)) {
      error_ = GET_INTERNAL_ERROR_MESSAGE();
      return false;
    }
    AppendDataName(STRING16(L"icons"), kDataSuffixForDesktop, &icon_dir);
    if (!File::RecursivelyCreateDir(icon_dir.c_str())) {
      error_ = GET_INTERNAL_ERROR_MESSAGE();
      return false;
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

  if (shortcuts_dialog->result["locations"].asBool()) {
    locations = shortcuts_dialog->result["locations"].asInt();
  }
  if (!SetShortcut(shortcut_info, allow, permanently, locations, &error_)) {
    return false;
  }

  return true;
}

void GearsDesktop::CreateShortcut(JsCallContext *context) {
  if (EnvIsWorker()) {
    context->SetException(
                 STRING16(L"createShortcut is not supported in workers."));
    return;
  }

  Desktop::ShortcutInfo shortcut_info;
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

  // Get the icons the user specified
  icons.GetPropertyAsString(STRING16(L"16x16"), &shortcut_info.icon16x16.url);
  icons.GetPropertyAsString(STRING16(L"32x32"), &shortcut_info.icon32x32.url);
  icons.GetPropertyAsString(STRING16(L"48x48"), &shortcut_info.icon48x48.url);
  icons.GetPropertyAsString(STRING16(L"128x128"),
                            &shortcut_info.icon128x128.url);

  // Prepare the shortcut.
#ifdef OS_ANDROID
  Desktop desktop(EnvPageSecurityOrigin(), EnvPageBrowsingContext(),
                  EnvPageJsContext());
#else
  Desktop desktop(EnvPageSecurityOrigin(), EnvPageBrowsingContext());
#endif
  if (!desktop.ValidateShortcutInfo(&shortcut_info)) {
    if (desktop.has_error())
      context->SetException(desktop.error());
    return;
  }

  // Show the dialog.
  // TODO(cprince): Consider moving this code to /ui/common/shortcut_dialog.cc
  // to keep it alongside the permission and settings dialog code.  And consider
  // sharing these constants to keep similar dialogs the same size.

  const int kShortcutsDialogWidth = 360;
  const int kShortcutsDialogHeight = 320;

  HtmlDialog shortcuts_dialog;
  if (!desktop.InitializeDialog(&shortcut_info, &shortcuts_dialog,
                                Desktop::DIALOG_STYLE_STANDARD) ||
      !shortcuts_dialog.DoModal(STRING16(L"shortcuts_dialog.html"),
                               kShortcutsDialogWidth, kShortcutsDialogHeight) ||
      !desktop.HandleDialogResults(&shortcut_info, &shortcuts_dialog)) {
    if (desktop.has_error())
      context->SetException(desktop.error());
    return;
  }
}

// Check whether the user has forbidden this shortcut from being created
// permanently.
bool Desktop::AllowCreateShortcut(const Desktop::ShortcutInfo &shortcut_info) {
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
  bool allow_shortcut_creation = false;
  if (!capabilities->GetShortcut(security_origin_,
                                 shortcut_info.app_name.c_str(),
                                 &app_url,
                                 &icon16x16_url,
                                 &icon32x32_url,
                                 &icon48x48_url,
                                 &icon128x128_url,
                                 &msg,
                                 &allow_shortcut_creation)) {
    // If shortcut doesn't exist in DB then it's OK to create it.
    return true;
  }

  return allow_shortcut_creation;
}

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

// Handle all the icon creation and creation call required to actually install
// a shortcut.
bool Desktop::SetShortcut(Desktop::ShortcutInfo *shortcut,
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
    capabilities->SetShortcut(security_origin_,
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

  if (!FetchIcon(&shortcut->icon16x16, error, false, NULL) ||
      !FetchIcon(&shortcut->icon32x32, error, false, NULL) ||
      !FetchIcon(&shortcut->icon48x48, error, false, NULL) ||
      !FetchIcon(&shortcut->icon128x128, error, false, NULL)) {
    return false;
  }

  if (!DecodeIcon(&shortcut->icon16x16, 16, error) ||
      !DecodeIcon(&shortcut->icon32x32, 32, error) ||
      !DecodeIcon(&shortcut->icon48x48, 48, error) ||
      !DecodeIcon(&shortcut->icon128x128, 128, error)) {
    return false;
  }

  // Save the icon file that will be used in the control panel
  if (!WriteControlPanelIcon(*shortcut)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

#if defined(WIN32) || defined(OS_MACOSX)
  const Desktop::IconData *next_largest_provided = NULL;

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
  if (!CreateShortcutPlatformImpl(security_origin_, *shortcut,
                                  locations, error)) {
    return false;
  }

  capabilities->SetShortcut(security_origin_,
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

bool Desktop::WriteControlPanelIcon(
                       const Desktop::ShortcutInfo &shortcut) {
  const Desktop::IconData *chosen_icon = NULL;

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
  if (!GetControlPanelIconLocation(security_origin_, shortcut.app_name,
                                   &icon_loc)) {
    return false;
  }

  // Write the control panel icon.
  File::CreateNewFile(icon_loc.c_str());
  return File::WriteVectorToFile(icon_loc.c_str(), &chosen_icon->png_data);
}

bool Desktop::FetchIcon(Desktop::IconData *icon, std::string16 *error,
                        bool async, scoped_refptr<HttpRequest> *async_request) {
  // Icons are optional. Only try to fetch if one was provided.
  if (icon->url.empty()) {
    return true;
  }

  if (!icon->png_data.empty()) {
    // Icons can come pre-fetched.  In that case, no need to fetch again.
  } else if (IsDataUrl(icon->url.c_str())) {
    // data URL
    std::string16 mime_type, charset;
    if (!ParseDataUrl(icon->url, &mime_type, &charset, &icon->png_data)) {
      *error = STRING16(L"Could not load icon ");
      *error += icon->url.c_str();
      *error += STRING16(L".");
      return false;
    }
  } else {
#ifdef BROWSER_WEBKIT
  // Workaround for bug in OS X 10.4, see definition of GetURLData() for
  // details.
  if (!GetURLData(icon->url, &(icon->png_data))) {
    *error = STRING16(L"Could not load icon ");
    *error += icon->url.c_str();
    *error += STRING16(L".");
    return false;
  }
#else
    // Fetch the png data
    scoped_refptr<HttpRequest> request;
    HttpRequest::Create(&request);

    request->SetCachingBehavior(HttpRequest::USE_ALL_CACHES);
    request->SetRedirectBehavior(HttpRequest::FOLLOW_ALL);

    int status = 0;
    if (!request->Open(HttpConstants::kHttpGET, icon->url.c_str(), 
                       async, browsing_context_.get()) ||
        !request->Send() ||
        (!async && (!request->GetStatus(&status) ||
                    status != HTTPResponse::RC_REQUEST_OK))) {
      *error = STRING16(L"Could not load icon ");
      *error += icon->url.c_str();
      *error += STRING16(L".");
      return false;
    }

    if (async) {
      assert(async_request);
      *async_request = request;
      return true;
    }

    // Extract the data.
    if (!request->GetResponseBody(&icon->png_data)) {
      *error = STRING16(L"Invalid data for icon ");
      *error += icon->url;
      *error += STRING16(L".");
      return false;
    }
#endif
  }

  return true;
}

bool Desktop::DecodeIcon(Desktop::IconData *icon, int expected_size,
                         std::string16 *error) {
  // Icons are optional.  Only try to decode if one was provided.
  if (icon->url.empty()) {
    return true;
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
bool Desktop::GetControlPanelIconLocation(const SecurityOrigin &origin,
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

bool Desktop::ResolveUrl(std::string16 *url, std::string16 *error) {
  std::string16 full_url;
  if (IsDataUrl(url->c_str()))
    return true;  // don't muck with data URLs

  if (!ResolveAndNormalize(security_origin_.full_url().c_str(),
                           url->c_str(), &full_url)) {
    *error = STRING16(L"Could not resolve url ");
    *error += *url;
    *error += STRING16(L".");
    return false;
  }

  *url = full_url;
  return true;
}

#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
#else

uint32 FindNotifierProcess(JsCallContext *context) {
  uint32 process_id = NotifierProcess::FindProcess();
  if (!process_id) {
    // TODO (jianli): Add the async start support.
    if (!NotifierProcess::StartProcess()) {
      context->SetException(STRING16(L"Failed to start notifier process."));
      return 0;
    }
    process_id = NotifierProcess::FindProcess();
    if (!process_id) {
      context->SetException(STRING16(L"Failed to find notifier process."));
      return 0;
    }
  }
  return process_id;
}

void GearsDesktop::CreateNotification(JsCallContext *context) {
  scoped_refptr<GearsNotification> notification;
  if (!CreateModule<GearsNotification>(GetJsRunner(), &notification))
    return;  // Create function sets an error message.
  if (!notification->InitBaseFromSibling(this)) {
    context->SetException(STRING16(L"Initializing base class failed."));
    return;
  }
  context->SetReturnValue(JSPARAM_DISPATCHER_MODULE, notification.get());
}

void GearsDesktop::ShowNotification(JsCallContext *context) {
  ModuleImplBaseClass *module = NULL;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_DISPATCHER_MODULE, &module },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;
  if (GearsNotification::kModuleName != module->get_module_name()) {
    context->SetException(STRING16(L"First argument must be a notification."));
    return;
  }

  // Try to find the process of Desktop Notifier.
  uint32 process_id = FindNotifierProcess(context);
  if (!process_id) {
    return;
  }

  // Send the IPC message to the process of Desktop Notifier.
  IpcMessageQueue *ipc_message_queue = IpcMessageQueue::GetSystemQueue();
  assert(ipc_message_queue);
  if (ipc_message_queue) {
    GearsNotification *notification = new GearsNotification();
    notification->CopyFrom(*(static_cast<GearsNotification*>(module)));
    // IpcMessageQueue is responsible for deleting the message data.
    ipc_message_queue->Send(static_cast<IpcProcessId>(process_id),
                            kDesktop_AddNotification,
                            notification);
  }
}

void GearsDesktop::RemoveNotification(JsCallContext *context) {
  std::string16 id;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &id },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  if (id.empty()) {
    context->SetException(
        STRING16(L"Cannot remove the notification with empty id."));
    return;
  }

  // Try to find the process of Desktop Notifier.
  uint32 process_id = FindNotifierProcess(context);
  if (!process_id) {
    return;
  }

  // Send the IPC message to the process of Desktop Notifier.
  IpcMessageQueue *ipc_message_queue = IpcMessageQueue::GetSystemQueue();
  assert(ipc_message_queue);
  if (ipc_message_queue) {
    GearsNotification *notification = new GearsNotification();
    notification->set_id(id);
    // IpcMessageQueue is responsible for deleting the message data.
    ipc_message_queue->Send(static_cast<IpcProcessId>(process_id),
                            kDesktop_RemoveNotification,
                            notification);
  }
}

#endif  // OFFICIAL_BUILD
