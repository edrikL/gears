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
#include <windows.h>  // must manually #include before nsIEventQueueService.h
#endif

#if BROWSER_FF
#include <nsCOMPtr.h>
#include <nspr.h>  // for PR_*
#include <nsServiceManagerUtils.h>  // for NS_IMPL_* and NS_INTERFACE_*
#include "gears/third_party/gecko_internal/jsapi.h"
#include "gears/third_party/gecko_internal/nsIDOMClassInfo.h"
#include "gears/desktop/desktop_ff.h"
#elif BROWSER_IE
#include <dispex.h>
#include "gears/base/ie/activex_utils.h"
#include "gears/desktop/desktop_ie.h"
#endif

#include "gears/base/common/paths.h"
#include "gears/base/common/permissions_db.h"
#include "gears/base/common/http_utils.h"
#include "gears/base/common/png_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/http_request.h"
#include "gears/ui/common/html_dialog.h"

#if BROWSER_FF
// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_ADDREF(GearsDesktop)
NS_IMPL_RELEASE(GearsDesktop)
NS_INTERFACE_MAP_BEGIN(GearsDesktop)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsDesktopInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsDesktopInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsDesktop)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsDesktopClassName = "GearsDesktop";
const nsCID kGearsDesktopClassId = { 0x273640f, 0xfe6d, 0x4a26, { 0x95, 0xc7,
                                     0x45, 0x5d, 0xe8, 0x3c, 0x60, 0x49 } };
                                  // {0273640F-FE6D-4a26-95C7-455DE83C6049}
#endif

static const int kControlPanelIconDimensions = 16;
#ifdef WIN32
static const PngUtils::ColorFormat kDesktopIconFormat = PngUtils::FORMAT_BGRA;
#else
static const PngUtils::ColorFormat kDesktopIconFormat = PngUtils::FORMAT_RGBA;
#endif


#if BROWSER_FF
NS_IMETHODIMP GearsDesktop::CreateShortcut() {
#elif BROWSER_IE
STDMETHODIMP GearsDesktop::createShortcut(BSTR name, BSTR description, BSTR url,
                                          VARIANT var_icons) {
#endif
  if (EnvIsWorker()) {
    RETURN_EXCEPTION(STRING16(L"createShortcut is not supported in workers."));
  }

  File::ShortcutInfo shortcut_info;
  JsObject icons;

#if BROWSER_FF
  // Create a param fetcher so we can also retrieve the context.
  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) != 4) {
    RETURN_EXCEPTION(STRING16(L"Incorrect number of arguments."));
  }
  
  if (!js_params.GetAsString(0, &shortcut_info.app_name) ||
      shortcut_info.app_name.empty()) {
    RETURN_EXCEPTION(STRING16(L"First parameter is required and must be a "
                              L"string."));
  }
  
  if (!js_params.GetAsString(1, &shortcut_info.app_description) ||
      shortcut_info.app_description.empty()) {
    RETURN_EXCEPTION(STRING16(L"Second parameter is required and must be a "
                              L"string."));
  }
  
  if (!js_params.GetAsString(2, &shortcut_info.app_url) ||
      shortcut_info.app_url.empty()) {
    RETURN_EXCEPTION(STRING16(L"Third parameter is required and must be a "
                              L"string."));
  }
  
  if (!js_params.GetAsObject(3, &icons)) {
    RETURN_EXCEPTION(STRING16(L"Fourth parameter is required and must be an "
                              L"object."));
  }
#elif BROWSER_IE
  if (name && name[0]) {
    shortcut_info.app_name = name;
  } else {
    RETURN_EXCEPTION(STRING16(L"First parameter is required."));
  }
  
  if (description && description[0]) {
    shortcut_info.app_description = description;
  } else {
    RETURN_EXCEPTION(STRING16(L"Second parameter is required."));
  }
  
  if (url && url[0]) {
    shortcut_info.app_url = url;
  } else {
    RETURN_EXCEPTION(STRING16(L"Third parameter is required."));
  }
  
  // Verify that we were passed an object for the icons parameter.
  if (!(var_icons.vt & VT_DISPATCH) || !icons.SetObject(var_icons, NULL)) {
    RETURN_EXCEPTION(STRING16(L"Fourth parameter is requied and must be an "
                              L"object."));
  }
#endif

  // Verify that the name is acceptable.
  if (shortcut_info.app_name.length() >= 50) {
    RETURN_EXCEPTION(STRING16(L"Application name must be less than 50 "
                              L"characters."));
  }

  if (shortcut_info.app_name.find('\\', 0) != std::string16::npos ||
      shortcut_info.app_name.find('/', 0) != std::string16::npos ||
      shortcut_info.app_name.find(':', 0) != std::string16::npos ||
      shortcut_info.app_name.find('*', 0) != std::string16::npos ||
      shortcut_info.app_name.find('?', 0) != std::string16::npos ||
      shortcut_info.app_name.find('\"', 0) != std::string16::npos ||
      shortcut_info.app_name.find('<', 0) != std::string16::npos ||
      shortcut_info.app_name.find('>', 0) != std::string16::npos ||
      shortcut_info.app_name.find('|', 0) != std::string16::npos) {
    RETURN_EXCEPTION(STRING16(L"Application name cannot contain: \"\\/:*?<>|"));
  }

  // Normalize and resolve, in case this is a relative URL.
  std::string16 error;
  if (!ResolveUrl(&shortcut_info.app_url, &error)) {
    RETURN_EXCEPTION(error.c_str());
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
      shortcut_info.icon48x48.url.empty()) {
    RETURN_EXCEPTION(STRING16(L"Invalid value for icon parameter. At least one "
                              L"of the 16x16, 32x32, or 48x48 sizes must be "
                              L"specified."));
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
    RETURN_EXCEPTION(error.c_str());
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
    RETURN_EXCEPTION(GET_INTERNAL_ERROR_MESSAGE().c_str());
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
  const int kShortcutsDialogWidth = 360;
  const int kShortcutsDialogHeight = 200;
  shortcuts_dialog.DoModal(STRING16(L"shortcuts_dialog.html"),
                           kShortcutsDialogWidth, kShortcutsDialogHeight);

  // A null value is ok.  We interpret this as denying the shortcut.
  if (shortcuts_dialog.result == Json::Value::null) {
    RETURN_NORMAL();
  }

  assert(shortcuts_dialog.result.isBool());
  if (!shortcuts_dialog.result.asBool()) {
    RETURN_NORMAL();
  }

  // Ensure the directory we'll be storing the icons in exists.
  std::string16 icon_dir;
  if (!GetDataDirectory(EnvPageSecurityOrigin(), &icon_dir) ||
      !AppendDataName(STRING16(L"icons"), kDataSuffixForDesktop, &icon_dir) ||
      !File::RecursivelyCreateDir(icon_dir.c_str())) {
    return false;
  }

  if (!SetShortcut(&shortcut_info, &error)) {
    RETURN_EXCEPTION(error.c_str());
  } else {
    RETURN_NORMAL();
  }
}

// Handle all the icon creation and creation call required to actually install
// a shortcut.
bool GearsDesktop::SetShortcut(File::ShortcutInfo *shortcut,
                               std::string16 *error) {
  PermissionsDB *capabilities = PermissionsDB::GetDB();
  if (!capabilities) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
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

  // Create the desktop shortcut using platform-specific code
  if (!File::CreateDesktopShortcut(EnvPageSecurityOrigin(), *shortcut, error)) {
    return false;
  }

  // Create the database entry.
  // TODO(aa): Perhaps we want to change how we store these in permissions db
  // now that we have them in a more structured way?
  std::vector<std::string16> icon_urls;
  if (!shortcut->icon16x16.url.empty())
    icon_urls.push_back(shortcut->icon16x16.url);
  if (!shortcut->icon32x32.url.empty())
    icon_urls.push_back(shortcut->icon32x32.url);
  if (!shortcut->icon48x48.url.empty())
    icon_urls.push_back(shortcut->icon48x48.url);
  if (!shortcut->icon128x128.url.empty())
    icon_urls.push_back(shortcut->icon128x128.url);

  capabilities->SetShortcut(EnvPageSecurityOrigin(),
                            shortcut->app_name.c_str(),
                            shortcut->app_url.c_str(),
                            icon_urls,
                            shortcut->app_description.c_str());
  return true;
}

bool GearsDesktop::WriteControlPanelIcon(const File::ShortcutInfo &shortcut) {
  const File::IconData *chosen_icon = NULL;
  
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

bool GearsDesktop::FetchIcon(File::IconData *icon, int expected_size,
                             std::string16 *error) {
  // Icons are optional. Only try to fetch if one was provided.
  if (icon->url.empty()) {
    return true;
  }

  // Fetch the png data
  ScopedHttpRequestPtr request(HttpRequest::Create());

  request.get()->SetCachingBehavior(HttpRequest::USE_ALL_CACHES);
  request.get()->SetRedirectBehavior(HttpRequest::FOLLOW_ALL);

  // Get the current icon.
  int status = 0;
  if (!request.get()->Open(HttpConstants::kHttpGET,
                           icon->url.c_str(), false) ||
      !request.get()->Send() ||
      !request.get()->GetStatus(&status) ||
      status != HTTPResponse::RC_REQUEST_OK) {
    *error = STRING16(L"Could not load icon ");
    *error += icon->url.c_str();
    *error += STRING16(L".");
    return false;
  }

  // Extract the data.
  if (!request.get()->GetResponseBody(&icon->png_data)) {
    *error = STRING16(L"Invalid data for icon ");
    *error += icon->url;
    *error += STRING16(L".");
    return false;
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

  if (!AppendDataName(STRING16(L"icons"), kDataSuffixForDesktop, icon_loc)) {
    return false;
  }

  *icon_loc += kPathSeparator;
  *icon_loc += app_name;
  *icon_loc += STRING16(L"_cp");
  *icon_loc += STRING16(L".png");

  return true;
}

bool GearsDesktop::ResolveUrl(std::string16 *url, std::string16 *error) {
  std::string16 full_url;
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
