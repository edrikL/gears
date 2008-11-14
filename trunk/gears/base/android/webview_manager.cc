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

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented.
#ifdef OS_ANDROID
#include "gears/base/android/webview_manager.h"

#include "gears/base/npapi/browser_utils.h"

WebViewManager::WebViewMap WebViewManager::map_;
Mutex WebViewManager::map_mutex_;

//static
void WebViewManager::RegisterWebView(NPP plugin_instance, jobject web_view) {
  MutexLock lock(&map_mutex_);
  assert(map_.find(plugin_instance) == map_.end());
  map_[plugin_instance] = web_view;
}

//static
void WebViewManager::UnregisterWebView(NPP plugin_instance) {
  MutexLock lock(&map_mutex_);
  assert(map_.find(plugin_instance) != map_.end());
  map_.erase(plugin_instance);
}

//static
bool WebViewManager::GetWebView(jobject *web_view_out) {
  MutexLock lock(&map_mutex_);

  // Try to acquire the current call context.
  JsCallContext *call_context = BrowserUtils::GetCurrentJsCallContext();
  if (!call_context) {
    LOG(("Illegal call site! No call context here."));
    assert(false);
    return false;
  }

  // Get the NPP instance associated with the current call context.
  NPP instance = reinterpret_cast<NPP> (call_context->js_context());
  if (!instance) {
    LOG(("Could not get the NPP instance.\n"));
    assert(false);
    return false;
  }

  WebViewMap::const_iterator iter = map_.find(instance);
  if (iter == map_.end()) {
    LOG(("No webview instance associated with this plugin instance"));
    assert(false);
    return false;
  }

  assert(web_view_out);
  *web_view_out = iter->second.Get();
  return true;
}

#endif  // OS_ANDROID
