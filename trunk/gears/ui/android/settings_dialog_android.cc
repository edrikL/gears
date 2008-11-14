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

#include "gears/ui/common/settings_dialog.h"
#include "gears/ui/android/settings_dialog_android.h"

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_class_loader.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_string.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/basictypes.h"
#include "gears/base/common/common.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/npapi/module.h"

JavaGlobalRef<jobject> global_android_context;
void runSettingsDialog(JNIEnv* env, jclass clazz, jobject context);

static const char* kHtmlPluginSettingsClass =
    GEARS_JAVA_PACKAGE "/GearsPluginSettings";
static const char* kNativePluginSettingsClass =
    GEARS_JAVA_PACKAGE "/PluginSettings";

static JavaClass::Method methods[1] = {
  { JavaClass::kNonStatic, "<init>", "(Landroid/webkit/Plugin;)V" },
};

JNINativeMethod native_methods[1] = {
  { "runSettingsDialog", "(Landroid/content/Context;)V",
    (void*) runSettingsDialog },
};

void runSettingsDialog(JNIEnv* env, jclass clazz, jobject context) {
  JniSetCurrentThread(env);
  global_android_context = context;
  SettingsDialog::Run(NULL);
  global_android_context.Release();
}

bool InitializeSettingsDialogClickHandler(jobject plugin) {
  JavaClass settingsClass;

  bool useNativeDialog = true;

  if (!settingsClass.FindClass(kNativePluginSettingsClass)) {
    LOG(("Couldn't load PluginSettings class\n"));
    useNativeDialog = false;
    LOG(("We try to load the GearsPluginSettings class instead\n"));
    if (!settingsClass.FindClass(kHtmlPluginSettingsClass)) {
      LOG(("Couldn't load GearsPluginSettings class\n"));
      return false;
    }
  }

  if (!settingsClass.GetMultipleMethodIDs(methods, NELEM(methods))) {
    LOG(("Couldn't get GearsPluginSettings methods\n"));
    return false;
  }

  if (useNativeDialog) {
    jniRegisterNativeMethods(JniGetEnv(), kNativePluginSettingsClass,
        native_methods, 1);
  } else {
    jniRegisterNativeMethods(JniGetEnv(), kHtmlPluginSettingsClass,
        native_methods, 1);
  }

  JavaGlobalRef<jobject> object;

  object.MoveLocal(JniGetEnv()->NewObject(settingsClass.Get(),
      methods[0].id, plugin));
  return true;
}
