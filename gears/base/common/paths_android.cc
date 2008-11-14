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

#include "gears/base/common/common_np.h"
#include "gears/base/common/file.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"

// The browser lives in either /data/data/com.android.browser or
// /data/data/com.google.android.browser depending on build
// version. Try them in order.
static const char16 *const kAndroidBrowserDirChoices[] = {
  STRING16(L"/data/data/com.android.browser"),
  STRING16(L"/data/data/com.google.android.browser"),
};
static const std::string16 kResourcesDirSuffix(STRING16(L"/app_plugins"));
static const std::string16 kBaseDirSuffix(STRING16(L"/gears"));
static const std::string16 kInstallDirSuffix(STRING16(L"/app_plugins"));

// Protect initialization of directory detection.
static Mutex android_browser_dirs_mutex;
static bool android_browser_dirs_initialized = false;
// Path to Gears resources.
static std::string16 android_gears_resources_dir;
// Path to Gears base directory for application data.
static std::string16 android_gears_base_dir;
// Path to Gears install directory.
static std::string16 android_gears_install_dir;

static void EnsureBrowserDir() {
  // Initialize directories if first call.
  MutexLock lock(&android_browser_dirs_mutex);
  if (android_browser_dirs_initialized)
    return;
  // Find where the browser lives.
  const char16 *android_browser_dir = NULL;
  for (size_t i = 0; i < ARRAYSIZE(kAndroidBrowserDirChoices); ++i) {
    if (File::DirectoryExists(kAndroidBrowserDirChoices[i])) {
      android_browser_dir = kAndroidBrowserDirChoices[i];
      break;
    }
  }
  assert(android_browser_dir != NULL);
  // Assign to resources, base and install dirs.
  android_gears_resources_dir = android_browser_dir + kResourcesDirSuffix
      + STRING16(L"/" PRODUCT_SHORT_NAME L"-" PRODUCT_VERSION_STRING);
  android_gears_base_dir = android_browser_dir + kBaseDirSuffix;
  android_gears_install_dir = android_browser_dir + kInstallDirSuffix
      + STRING16(L"/" PRODUCT_SHORT_NAME L"-" PRODUCT_VERSION_STRING);
  android_browser_dirs_initialized = true;
}

bool GetBaseResourcesDirectory(std::string16 *path) {
  EnsureBrowserDir();
  *path = android_gears_resources_dir;
  return true;
}

bool GetBaseDataDirectory(std::string16 *path) {
  EnsureBrowserDir();
  // Create the data directory.
  if (!File::RecursivelyCreateDir(android_gears_base_dir.c_str())) {
    return false;
  } else {
    *path = android_gears_base_dir;
    return true;
  }
}

bool GetInstallDirectory(std::string16 *path) {
  EnsureBrowserDir();
  *path = android_gears_install_dir;
  return true;
}
