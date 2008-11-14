// Copyright 2008, Google Inc.
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
//
// Android-specific implementation of desktop shortcut creation.

#include <errno.h>

#include "gears/desktop/desktop.h"

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_class_loader.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_string.h"
#include "gears/base/android/webview_manager.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/file.h"
#include "gears/base/common/basictypes.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"

#include "gears/localserver/common/http_constants.h"

static const char* kShortcutSignature =
    "(Landroid/webkit/WebView;Ljava/lang/String;Ljava/lang/String;"
    "Ljava/lang/String;)V";
static const char* kDesktopClass = GEARS_JAVA_PACKAGE "/DesktopAndroid";

static JavaClass::Method methods[] = {
  { JavaClass::kStatic, "setShortcut", kShortcutSignature },
};

static bool GetIconPath(const SecurityOrigin &origin,
                        const std::string16 link_name,
                        std::string16 *icon_path,
                        std::string16 *error) {
  if (!GetDataDirectory(origin, icon_path)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  AppendDataName(STRING16(L"icons"), kDataSuffixForDesktop, icon_path);
  *icon_path += kPathSeparator;
  *icon_path += link_name;
  *icon_path += STRING16(L"_android");
  *icon_path += STRING16(L".png");
  return true;
}

static bool WriteIconFile(const Desktop::ShortcutInfo &shortcut,
                          const std::string16 &icon_path,
                          std::string16 *error) {
  const Desktop::IconData *chosen_icon = NULL;

  // Try to pick the best icon size of the available choices.
  if (!shortcut.icon48x48.png_data.empty()) { // 48 is default size for android
    chosen_icon = &shortcut.icon48x48;
  } else if (!shortcut.icon128x128.png_data.empty()) { // better to be too big
    chosen_icon = &shortcut.icon128x128;
  } else if (!shortcut.icon32x32.png_data.empty()) {
    chosen_icon = &shortcut.icon32x32;
  } else if (!shortcut.icon16x16.png_data.empty()) {
    chosen_icon = &shortcut.icon16x16;
  } else {
    // Caller should have verified that we had at least one icon
    assert(false);
  }

  // (over)write the icon file
  File::CreateNewFile(icon_path.c_str());
  if (!File::WriteVectorToFile(icon_path.c_str(), &chosen_icon->png_data)) {
    *error = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }
  return true;
}

static bool AddShortcutAndroid(const Desktop::ShortcutInfo &shortcut,
                               const std::string16 &icon_path) {

  JavaClass desktopClass;
  if (!desktopClass.FindClass(kDesktopClass)) {
    LOG(("Couldn't load DesktopAndroid class\n"));
    return false;
  }

  if (!desktopClass.GetMultipleMethodIDs(methods, NELEM(methods))) {
    LOG(("Couldn't get DesktopAndroid methods\n"));
    return false;
  }

  jobject webview = NULL;
  if (WebViewManager::GetWebView(&webview)) {
    JNIEnv *env = JniGetEnv();
    env->CallStaticVoidMethod(desktopClass.Get(), methods[0].id, webview,
                              JavaString(shortcut.app_name).Get(),
                              JavaString(shortcut.app_url).Get(),
                              JavaString(icon_path).Get());
  } else {
    LOG(("Desktop: we could not call the shortcut method"));
    return false;
  }

  return true;
}

bool Desktop::CreateShortcutPlatformImpl(
    const SecurityOrigin &origin, const Desktop::ShortcutInfo &shortcut,
    uint32 locations, std::string16 *error) {
  // Note: We assume that link_name has already been validated by the caller to
  // have only legal filename characters and that launch_url has already been
  // resolved to an absolute URL.

  std::string16 icon_path;
  if (!GetIconPath(origin, shortcut.app_name, &icon_path, error)) {
    return false;
  }

  // We save the icon files...
  if (!WriteIconFile(shortcut, icon_path, error)) {
    return false;
  }

  // ...and ask the Java side to send the icon to the content provider
  // Shortcut checks are done in the Java side
  AddShortcutAndroid(shortcut, icon_path);
  return true;
}
