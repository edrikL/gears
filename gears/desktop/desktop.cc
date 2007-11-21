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

struct GearsDesktop::ShortcutInfo {
  std::vector<std::string16> icon_urls;
  std::string16 app_name;
  std::string16 app_url;
  std::string16 app_description;
};

// JS function is createShortcuts(variant shortcuts).
// shortcuts should be an object with the structure:
// var shortcuts = [ {
//     appName: 'My App',
//     appDescription: 'It does things.',
//     appUrl: 'http://www.myapp.com/',
//     iconUrls: ['http://www.myapp.com/icon48x48.png',
//                'http://www.myapp.com/icon16x16.png',
//                'http://www.myapp.com/icon32x32.png']
//   }, {...} ]
#if BROWSER_FF
NS_IMETHODIMP GearsDesktop::CreateShortcuts() {
#elif BROWSER_IE
STDMETHODIMP GearsDesktop::createShortcuts(VARIANT shortcuts) {
#endif
  if (EnvIsWorker()) {
    RETURN_EXCEPTION(STRING16(L"createShortcuts is not supported in workers."));
  }

  JsArray shortcut_array;

#if BROWSER_FF
  // Create a param fetcher so we can also retrieve the context.
  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) != 1) {
    RETURN_EXCEPTION(STRING16(L"Requires one parameter."));
  }

  if (!js_params.GetAsArray(0, &shortcut_array)) {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be an array of objects."));
  }
#elif BROWSER_IE
  // Verify that we were passed an array.
  if (!(shortcuts.vt & VT_DISPATCH) ||
      !shortcut_array.SetArray(shortcuts, NULL)) {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be an array of objects."));
  }
#endif

  int length;
  if (!shortcut_array.GetLength(&length)) {
    RETURN_EXCEPTION(STRING16(L"First parameter is not a valid array."));
  }

  std::vector<ShortcutInfo> shortcut_infos;
  HtmlDialog shortcuts_dialog;
  shortcuts_dialog.arguments = Json::Value(Json::arrayValue);

  // Iterate across the array of shortcuts
  for (int i = 0; i < length; ++i) {
    JsObject shortcut_object;
    if( !shortcut_array.GetElementAsObject(i, &shortcut_object)) {
      RETURN_EXCEPTION(STRING16(L"First parameter is not a valid array."));
    }

    ShortcutInfo shortcut_info;
    JsArray icon_array;

    // Iterate across the array of Icon URLs
    if (shortcut_object.GetPropertyAsArray(std::string16(STRING16(L"iconUrls")),
                                           &icon_array)) {
      int icon_count;
      if (!icon_array.GetLength(&icon_count)) {
        RETURN_EXCEPTION(
            STRING16(L"Invalid icon data. iconUrls is not a valid array."));
      }
      if (icon_count == 0) {
        RETURN_EXCEPTION(STRING16(
            L"Invalid icon data. iconUrls doesn't have any entries."));
      }
      for (int j = 0; j < icon_count; ++j) {
        std::string16 icon_string;
        if( !icon_array.GetElementAsString(j, &icon_string)) {
          RETURN_EXCEPTION(
              STRING16(L"Invalid icon data. iconUrls is not a valid array."));
        }

        // Normalize and resolve, in case this is a relative URL.
        std::string16 full_url;
        if (ResolveAndNormalize(EnvPageLocationUrl().c_str(),
                                icon_string.c_str(),
                                &full_url)) {
          icon_string = full_url;
        }

        shortcut_info.icon_urls.push_back(icon_string);
      }
    } else {
      RETURN_EXCEPTION(STRING16(L"Invalid icon data. iconUrls was not set."));
    }

    if (!shortcut_object.GetPropertyAsString(
                             std::string16(STRING16(L"appName")),
                             &shortcut_info.app_name)) {
      RETURN_EXCEPTION(STRING16(L"Invalid icon data. appName was not set."));
    }

    if (!shortcut_object.GetPropertyAsString(std::string16(STRING16(L"appUrl")),
                                             &shortcut_info.app_url)) {
      RETURN_EXCEPTION(STRING16(L"Invalid icon data. appUrl was not set."));
    }

    if (!shortcut_object.GetPropertyAsString(
                             std::string16(STRING16(L"appDescription")),
                             &shortcut_info.app_description)) {
      RETURN_EXCEPTION(
          STRING16(L"Invalid icon data. appDescription was not set."));
    }

    // Verify that the name is acceptable.
    if (shortcut_info.app_name.length() >= 50) {
      RETURN_EXCEPTION(
          STRING16(L"Application name must be less than 50 characters."));
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
      RETURN_EXCEPTION(
          STRING16(L"Application name cannot contain: \"\\/:*?<>|"));
    }

    // Normalize and resolve, in case this is a relative URL.
    std::string16 full_url;
    if (ResolveAndNormalize(EnvPageLocationUrl().c_str(),
                            shortcut_info.app_url.c_str(),
                            &full_url)) {
      shortcut_info.app_url = full_url;
    }

    // Json needs utf8.
    std::string app_url_utf8;
    std::string icon_url_utf8;
    std::string app_name_utf8;
    std::string app_description_utf8;
    if (!String16ToUTF8(shortcut_info.app_url.c_str(), &app_url_utf8) ||
        !String16ToUTF8(shortcut_info.icon_urls[0].c_str(), &icon_url_utf8) ||
        !String16ToUTF8(shortcut_info.app_name.c_str(), &app_name_utf8) ||
        !String16ToUTF8(shortcut_info.app_description.c_str(),
                        &app_description_utf8)) {
      RETURN_EXCEPTION(STRING16(L"Error converting to utf8."));
    }

    // Populate the JSON object we're passing to the dialog.
    Json::Value shortcut;
    shortcut["name"] = Json::Value(app_name_utf8);
    shortcut["icon"] = Json::Value(icon_url_utf8);
    shortcut["link"] = Json::Value(app_url_utf8);
    shortcut["description"] = Json::Value(app_description_utf8);

    shortcuts_dialog.arguments.append(shortcut);
    shortcut_infos.push_back(shortcut_info);
  }

  // Show the dialog.
  const int kShortcutsDialogWidth = 360;
  const int kShortcutsDialogHeight = 265;
  shortcuts_dialog.DoModal(STRING16(L"shortcuts_dialog.html"),
                           kShortcutsDialogWidth, kShortcutsDialogHeight);

  // A null value is ok.  We interpret this as denying all shortcuts.
  if (shortcuts_dialog.result == Json::Value::null) {
    RETURN_NORMAL();
  }

  if (!shortcuts_dialog.result.isArray()) {
    assert(false);
    LOG(("ShortcutsDialog: Unexpected result type."));
    RETURN_NORMAL();
  }

  if (shortcuts_dialog.result.size() != shortcut_infos.size()) {
    assert(false);
    LOG(("ShortcutsDialog: Unexpected result size."));
    RETURN_NORMAL();
  }

  // Ensure the directory we'll be storing the icons in exists.
  std::string16 icon_dir;
  if (!GetDataDirectory(EnvPageSecurityOrigin(), &icon_dir) ||
      !AppendDataName(STRING16(L"icons"), kDataSuffixForDesktop, &icon_dir) ||
      !File::RecursivelyCreateDir(icon_dir.c_str())) {
    return false;
  }

  // Find the shortcuts the user approved, and create them.
  for (size_t i = 0; i < shortcuts_dialog.result.size(); ++i) {
    if (shortcuts_dialog.result[i].isBool() &&
        shortcuts_dialog.result[i].asBool()) {
      std::string16 error;
      if (!SetShortcut(&(shortcut_infos[i]), &error)) {
        RETURN_EXCEPTION(error.c_str());
      }
    }
  }

  RETURN_NORMAL();
}

// Handle all the icon creation and creation call required to actually install
// a shortcut.
bool GearsDesktop::SetShortcut(ShortcutInfo *shortcut, std::string16 *error) {
  PermissionsDB *capabilities = PermissionsDB::GetDB();
  if (!capabilities) {
    *error = STRING16(L"Could not open permissions Database.");
    return false;
  }

  File::DesktopIcons desktop_icons;

  std::vector<std::string16>::iterator icon_url;
  for (icon_url = shortcut->icon_urls.begin();
       icon_url != shortcut->icon_urls.end();
       ++icon_url) {
    ScopedHttpRequestPtr request(HttpRequest::Create());

    request.get()->SetCachingBehavior(HttpRequest::USE_ALL_CACHES);
    request.get()->SetRedirectBehavior(HttpRequest::FOLLOW_ALL);

    // Get the current icon.
    if (!request.get()->Open(HttpConstants::kHttpGET,
                             icon_url->c_str(), false) ||
        !request.get()->Send()) {
      *error = STRING16(L"Could not load icon ");
      *error += icon_url->c_str();
      *error += STRING16(L".");
      return false;
    }

    // Extract the data.
    std::vector<uint8> png;
    if (!request.get()->GetResponseBody(&png)) {
      *error = STRING16(L"Could not load icon ");
      *error += icon_url->c_str();
      *error += STRING16(L".");
      return false;
    }

    // Decode the png
    File::IconData new_icon;
    if (!PngUtils::Decode(&png.at(0), png.size(), kDesktopIconFormat,
                          &new_icon.raw_data, &new_icon.width,
                          &new_icon.height)) {
      *error = STRING16(L"Could not decode PNG data for icon ");
      *error += icon_url->c_str();
      *error += STRING16(L".");
      return false;
    }

    if (new_icon.width != new_icon.height) {
      *error = STRING16(L"Icon width and height must be equal.");
      return false;
    }

    // TODO(aa): Blech, this has to copy the data out of new_icon and png
    // because we don't know which slot in desktop_icons to use ahead of time.
    // This can be fixed later when the caller passes us the dimensions of
    // each icon.
    if (!UpdateDesktopIcons(png, new_icon, &desktop_icons, error)) {
      return false;
    }
  }

  // Save the icon file that will be used in the control panel
  if (!WriteControlPanelIcon(shortcut->app_name, desktop_icons)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  // Create the desktop shortcut using platform-specific code
  if (!File::CreateDesktopShortcut(EnvPageSecurityOrigin(),
                                   shortcut->app_name,
                                   shortcut->app_url,
                                   desktop_icons,
                                   error)) {
    return false;
  }

  // Create the database entry.
  // TODO(aa): Check for failure?
  capabilities->SetShortcut(EnvPageSecurityOrigin(),
                            shortcut->app_name.c_str(),
                            shortcut->app_url.c_str(),
                            shortcut->icon_urls,
                            shortcut->app_description.c_str());
  return true;
}

bool GearsDesktop::WriteControlPanelIcon(const std::string16 &name,
                                         const File::DesktopIcons &icons) {
  const File::IconData *chosen_icon;
  
  // Pick the best icon we can for the control panel
  if (!icons.icon16x16.png_data.empty()) {
    chosen_icon = &icons.icon16x16;
  } else if (!icons.icon32x32.png_data.empty()) {
    chosen_icon = &icons.icon32x32;
  } else if (!icons.icon48x48.png_data.empty()) {
    chosen_icon = &icons.icon48x48;
  } else if (!icons.icon128x128.png_data.empty()) {
    chosen_icon = &icons.icon128x128;
  } else {
    // Caller should have ensured that there was at least one icon
    assert(false);
  }

  std::string16 icon_loc;
  if (!GetControlPanelIconLocation(EnvPageSecurityOrigin(), name, &icon_loc)) {
    return false;
  }

  // Write the control panel icon.
  File::CreateNewFile(icon_loc.c_str());
  return File::WriteVectorToFile(icon_loc.c_str(), &chosen_icon->png_data);
}

bool GearsDesktop::UpdateDesktopIcons(const std::vector<uint8> &png,
                                      const File::IconData &new_icon,
                                      File::DesktopIcons *icons,
                                      std::string16 *error) {

  File::IconData *icon = NULL;
  switch (new_icon.width) {
    case 16:
      icon = &icons->icon16x16;
      break;
    case 32:
      icon = &icons->icon32x32;
      break;
    case 48:
      icon = &icons->icon48x48;
      break;
    case 128:
      icon = &icons->icon128x128;
      break;
    default:
      *error = STRING16(L"Icon width ");
      *error += IntegerToString16(new_icon.width);
      *error += STRING16(L" isn't supported.");
      return false;
  }

  if (!icon->png_data.empty()) {
    *error = STRING16(L"Duplicate icon with ");
    *error += IntegerToString16(new_icon.width);
    *error += STRING16(L" found.");
    return false;
  }

  *icon = new_icon;
  icon->png_data = png;
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
