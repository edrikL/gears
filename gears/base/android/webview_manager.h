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

#ifndef GEARS_BASE_ANDROID_WEBVIEW_MANAGER_H__
#define GEARS_BASE_ANDROID_WEBVIEW_MANAGER_H__

#include <map>

#include "gears/base/android/java_global_ref.h"
#include "gears/base/common/mutex.h"
#include "gears/base/npapi/module.h"


// This class manages the association between an NPP instance
// and the webview corresponding to the page that created the Gears plugin.
// (See http://code.google.com/android/reference/android/webkit/WebView.html)
// Several Gears classes (e.g. Desktop, AndroidRadioDataProvider,
// AndroidGpsLocationProvider) need the webview object so that they
// can pass it to the Java side. The corresponding Java classes
// extract a Context object from the webview instance and use it
// to get access to various system services, such as the TelephonyManager
// or the LocationManager.
class WebViewManager {
 public:
  // Registers an NPP instance with a webview instance.
  static void RegisterWebView(NPP plugin_instance, jobject web_view);
  // Removes the association between an NPP instance and a webview instance.
  static void UnregisterWebView(NPP plugin_instance);
  // Retrieves the webview associated with the NPP instance that corresponds
  // to the current call context. Note that in order to succeeed, this method
  // must be called from sites that have a call context. This imposes an
  // important restriction: objects that need the webview must attempt to
  // acquire it only from the "main" javascript threads (either the thread
  // where the document JS runner is executed, or a worker thread) and only
  // from a script-invoked method call on a Gears object.
  // Attempting to acquire the webview from anywhere else is an error
  // and will trigger an assertion on debug builds.
  static bool GetWebView(jobject *web_view_out);

 private:
  typedef std::map<NPP, JavaGlobalRef<jobject> > WebViewMap;
  static WebViewMap map_;
  static Mutex map_mutex_;

  DISALLOW_EVIL_CONSTRUCTORS(WebViewManager);
};

#endif  // GEARS_BASE_ANDROID_WEBVIEW_MANAGER_H__
