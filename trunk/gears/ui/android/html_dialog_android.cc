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

#ifdef OS_ANDROID

#include "gears/ui/common/html_dialog.h"
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
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/npapi/module.h"

static const char* kShowDialogSignature =
  "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)"
  "Ljava/lang/String;";

static const char* kHtmlDialogClass = GEARS_JAVA_PACKAGE "/HtmlDialogAndroid";
static const char* kNativeDialogClass = GEARS_JAVA_PACKAGE "/NativeDialog";

static JavaClass::Method methods[] = {
    { JavaClass::kNonStatic, "<init>", "()V" },
    { JavaClass::kNonStatic, "showDialog", kShowDialogSignature },
};

static const char* kWebviewClass = "android/webkit/WebView";

static JavaClass::Method webview_methods[] = {
    { JavaClass::kNonStatic, "getContext",
      "()Landroid/content/Context;" },
};

bool HtmlDialog::DoModalImpl(const char16 *html_filename, int width,
    int height, const char16 *arguments_string) {
  JavaLocalFrame frame;
  jobject context = NULL;

  // To call the dialog, we need an android.content.Context instance.
  // We first try to get it by getting the WebView stored in our NPP
  // instance, and calling the getContext() method on it
  jobject java_webview = NULL;

  JsCallContext* js_context = BrowserUtils::GetCurrentJsCallContext();
  if (js_context != NULL) {
    NPP npp_instance = reinterpret_cast<NPP> (js_context->js_context());
    // Get the WebView instance
    NPError err = NPN_GetValue(npp_instance, NPNVnetscapeWindow, &java_webview);
    if (err == NPERR_NO_ERROR) {
      // Now we can call the getContext() method on the WebView instance
      JavaClass java_webview_class;

      // get the JavaClass for WebView
      if (!java_webview_class.FindClass(kWebviewClass)) {
        LOG(("Couldn't load the WebView class\n"));
        return false;
      }

      if (!java_webview_class.GetMultipleMethodIDs(webview_methods,
          NELEM(webview_methods))) {
        LOG(("Couldn't get WebView methods\n"));
        return false;
      }

      // call getContext()
      context = JniGetEnv()->CallObjectMethod(java_webview,
          webview_methods[0].id);
    }
  } else {
    // If we could not get the NPP instance by using GetCurrentJsCallContext,
    // we are probably being called by the settings dialog, which set the
    // android.content.Context in global_android_context
    context = global_android_context.Get();
  }

  if (context == NULL) {
    LOG(("HtmlDialog: we could not get an android.content.Context instance\n"));
    return false;
  }

  // We need the JavaClass for the Dialog class
  JavaClass java_dialog_class;
  if (!java_dialog_class.FindClass(kNativeDialogClass)) {
    LOG(("Couldn't load the NativeDialog class!\n"));
    LOG(("We try loading the HtmlDialogAndroid class instead.\n"));
    if (!java_dialog_class.FindClass(kHtmlDialogClass)) {
      LOG(("Couldn't load HtmlDialogAndroid class\n"));
      return false;
    }
  }

  if (!java_dialog_class.GetMultipleMethodIDs(methods, NELEM(methods))) {
    LOG(("Couldn't get Dialog class methods\n"));
    return false;
  }

  // We create an instance of the Dialog class
  JavaGlobalRef<jobject> java_dialog_instance(
      JniGetEnv()->NewObject(java_dialog_class.Get(), methods[0].id));

  // Then we get the path to the html resources
  std::string16 android_gears_resources_dir;
  if (!GetBaseResourcesDirectory(&android_gears_resources_dir)) {
    LOG(("HtmlDialog: we could not get the gears resources directory !\n"));
    return false;
  }

  std::string16 html_content_path = android_gears_resources_dir
      + STRING16(L"/") + html_filename;

  // And finally we call the showDialog() method
  jstring result = static_cast<jstring>(JniGetEnv()->CallObjectMethod(
      java_dialog_instance.Get(), methods[1].id, context,
      JavaString(html_content_path).Get(),
      JavaString(arguments_string).Get()));

  JavaString result_string = result;
  if ((result_string.Get() == NULL) ||
      (result_string.ToString8().compare("null") == 0)) {
    // a null jstring is, indeed, returned as a string containing
    // the 'null'...
    SetResult(NULL);
  } else {
    SetResult(result_string.ToString16().c_str());
  }

  return true;
}

bool HtmlDialog::DoModelessImpl(
    const char16 *html_filename, int width, int height,
    const char16 *arguments_string,
    ModelessCompletionCallback callback,
    void *closure) {
  // Unused in Android.
  assert(false);
  return false;
}

bool HtmlDialog::GetLocale(std::string16* p) {
  // TODO: returns the correct locale
  return false;
}

#endif  // OS_ANDROID
