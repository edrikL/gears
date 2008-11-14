/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//////////////////////////////////////////////////////////////
//
// Main plugin entry point implementation
//

#include <unistd.h>
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/thread_locals.h"
#include "gears/base/common/message_queue.h"
#include "gears/base/npapi/module.h"
#include "gears/base/android/java_class.h"
#include "gears/base/android/java_class_loader.h"
#include "gears/base/android/java_jni.h"
#include "gears/installer/android/lib_updater.h"
#include "gears/localserver/android/http_request_android.h"
#include "gears/localserver/android/url_intercept_android.h"
#include "gears/ui/android/settings_dialog_android.h"
#include "third_party/npapi/nphostapi.h"

#if USING_CLASS_LOADER
// Define static const char *s_classes_to_load[]. Generated file.
#include "genfiles/classes_to_load.inc"
#endif

// In Android builds, only the NPAPI interface is exported. This
// allows the linker to throw away a large amount of symbol table and
// garbage collect a lot of unused functions.
#define EXPORT __attribute__((visibility("default")))

// Global key to access the TLS NPNetscapeFuncs structure. This is
// only non-NULL for the main thread.
const ThreadLocals::Slot kNPNFuncsKey = ThreadLocals::Alloc();
// This is the NPNetscapeFuncs structure used by the main thread.
static NPNetscapeFuncs s_browser_funcs;
// The string returned by NP_GetMIMEDescription().
static const char kMimeDescription[] =
    "application/x-googlegears::Google Gears";

#if USING_CLASS_LOADER
// Container array for all Java classes we need to load at
// startup. They are automatically destructed when the library is
// unloaded.
static JavaClass s_java_classes[NELEM(s_classes_to_load)];
// The name of the JAR file containing the Java classes.
static const std::string16 kGearsAndroidJar(STRING16(L"gears-android.jar"));
#endif  // USING_CLASS_LOADER

// Export these symbols without C++ name mangling.
extern "C" {
  NPError NP_Initialize(NPNetscapeFuncs *npn_funcs,
                        NPPluginFuncs *npp_funcs,
                        JNIEnv *env,
                        jobject plugin_object) EXPORT;
  NPError NP_Shutdown() EXPORT;
  char *NP_GetMIMEDescription() EXPORT;
  NPError NP_GetValue(void *future,
                      NPPVariable variable,
                      void *value) EXPORT;
};

// This symbol is not external in the Android browser's NPAPI
// implementation.
static NPError NP_GetEntryPoints(NPPluginFuncs *npp_funcs) {
  if (npp_funcs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;
  if (npp_funcs->size < sizeof(NPPluginFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;
  npp_funcs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
  npp_funcs->newp          = NPP_New;
  npp_funcs->destroy       = NPP_Destroy;
  npp_funcs->setwindow     = NPP_SetWindow;
  npp_funcs->newstream     = NPP_NewStream;
  npp_funcs->destroystream = NPP_DestroyStream;
  npp_funcs->asfile        = NPP_StreamAsFile;
  npp_funcs->writeready    = NPP_WriteReady;
  npp_funcs->write         = NPP_Write;
  npp_funcs->print         = NPP_Print;
  npp_funcs->event         = NPP_HandleEvent;
  npp_funcs->urlnotify     = NPP_URLNotify;
  npp_funcs->getvalue      = NPP_GetValue;
  npp_funcs->setvalue      = NPP_SetValue;
  npp_funcs->javaClass     = NULL;
  return NPERR_NO_ERROR;
}

NPError NP_Initialize(NPNetscapeFuncs *npn_funcs,
                      NPPluginFuncs *npp_funcs,
                      JNIEnv *env,
                      jobject plugin_object) {
#ifdef DEBUG
  LOG(("Main thread pthread_id %ld\n", static_cast<long>(pthread_self())));
  LOG(("Main thread pid %d\n", getpid()));
  LOG(("Main thread tid %d\n", gettid()));
#endif
  if (npn_funcs == NULL) {
    LOG(("NULL function table\n"));
    return NPERR_INVALID_FUNCTABLE_ERROR;
  }
  int major = (npn_funcs->version & 0xff00) >> 8;
  if (major > NP_VERSION_MAJOR) {
    LOG(("Got NPN major version %d, but we support up to %d\n",
         major, NP_VERSION_MAJOR));
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  }
  // Check the structure size supports at least up to and including
  // NPN_PluginThreadAsyncCall.
  NPNetscapeFuncs *null_npfuncs = static_cast<NPNetscapeFuncs *>(0);
  int need_size =
      reinterpret_cast<int>(&null_npfuncs->pluginthreadasynccall) +
      sizeof(null_npfuncs->pluginthreadasynccall);
  if (npn_funcs->size < need_size) {
    LOG(("Got NPN structure size %u, but expected at least %d\n",
         npn_funcs->size, need_size));
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  }
  if (npn_funcs->pluginthreadasynccall == NULL) {
    LOG(("NPN_PluginThreadAsyncCall required but not implemented\n"));
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  }
  // Store a local copy of the browser callback function table.
  s_browser_funcs = *npn_funcs;
  // Set a TLS version of this. Only the main thread can call these
  // functions safely, with the exception of
  // NPN_PluginThreadAsyncCall.
  ThreadLocals::SetValue(kNPNFuncsKey, &s_browser_funcs, NULL);
  // Register the main thread with JNI.
  JniRegisterMainThread(env);

#if USING_CLASS_LOADER
  // Load our Java class file.
  JavaClassLoader loader;
  std::string16 install_dir16;
  if (!GetInstallDirectory(&install_dir16)) {
    LOG(("Couldn't get install directory\n"));
    return NPERR_INVALID_PLUGIN_DIR_ERROR;
  }
  std::string16 jar_path16 = install_dir16 + kPathSeparator + kGearsAndroidJar;
  std::string jar_path8;
  if (!String16ToUTF8(jar_path16.data(), jar_path16.size(), &jar_path8)) {
    LOG(("Bad characters in install directory\n"));
    return NPERR_INVALID_PLUGIN_DIR_ERROR;
  }
  if (!loader.Open(jar_path8.c_str())) {
    LOG(("Couldn't load JAR\n"));
    return NPERR_INVALID_PLUGIN_ERROR;
  }
  // Load the classes out of the file.
  for (int i = 0; i < NELEM(s_classes_to_load); ++i) {
    if (!loader.LoadClass(s_classes_to_load[i], &s_java_classes[i])) {
      LOG(("Couldn't load %s\n", s_classes_to_load[i]));
      return NPERR_INVALID_PLUGIN_ERROR;
    }
  }
#endif  // USING_CLASS_LOADER

  // Delete any stale temporary files.
  std::string16 temp_dir;
  if (!File::GetBaseTemporaryDirectory(&temp_dir)) {
    LOG(("Couldn't get base temporary file directory\n"));
    return NPERR_INVALID_PLUGIN_DIR_ERROR;
  }
  if (temp_dir.empty() || temp_dir == STRING16(L"/")) {
    LOG(("Not deleting suspicious temporary file directory \"%s\"\n",
         String16ToUTF8(temp_dir).c_str()));
    return NPERR_INVALID_PLUGIN_DIR_ERROR;
  }
  File::DeleteRecursively(temp_dir.c_str());
  // Initialize the message queue for the browser thread.
  ThreadMessageQueue::GetInstance()->InitThreadMessageQueue();

  // Initialize HttpRequestAndroid
  if (!HttpRequestAndroid::InitJni()) {
    LOG(("Couldn't initialize HttpRequestAndroid\n"));
    return NPERR_INVALID_PLUGIN_ERROR;
  }
  if (!InitializeSettingsDialogClickHandler(plugin_object)) {
    LOG(("Couldn't set the settings dialog!\n"));
    return NPERR_INVALID_PLUGIN_ERROR;
  }
  // Initialize URL interception to enable LocalServer.
  if (!UrlInterceptAndroid::Create()) {
    LOG(("Couldn't start URL interception\n"));
    return NPERR_INVALID_PLUGIN_ERROR;
  }

  // Done, initialize the function table for calls going from browser
  // to plugin.
  NPError np_error = NP_GetEntryPoints(npp_funcs);
  if (NPERR_NO_ERROR == np_error) {
    // Start the update checks.
    // This starts a thread, so it should only happen if the
    // Gears library was loaded successfully.
    LibUpdater::StartUpdateChecks();
  }

  return np_error;
}

NPError NP_Shutdown() {
  // Stop the update checks.
  LibUpdater::StopUpdateChecks();
  // Shutdown URL interception.
  UrlInterceptAndroid::Delete();
#if 0  // Do not disable access - global destructors require JNI.
  // Disable JNI access.
  JniUnregisterMainThread();
  // Disable access to the browser functions.
  ThreadLocals::DestroyValue(kNPNFuncsKey);
#endif
  return NPERR_NO_ERROR;
}

char *NP_GetMIMEDescription() {
  return const_cast<char *>(kMimeDescription);
}

NPError NP_GetValue(void *future, NPPVariable variable, void *value) {
  switch (variable) {
    case NPPVpluginNameString:
      *(char **) value =
          const_cast<char *>(PRODUCT_SHORT_NAME_ASCII);
      break;
    case NPPVpluginDescriptionString:
      *(char **) value =
          const_cast<char *>(PRODUCT_FRIENDLY_NAME_ASCII " Plugin");
      break;
    default:
      return NPERR_INVALID_PARAM;
  }
  return NPERR_NO_ERROR;
}
