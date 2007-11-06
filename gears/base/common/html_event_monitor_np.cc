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
//
// This implementation makes use of window.addEventListener() to watch the
// event.  IE does not support this, but it's not a target for the NPAPI port,
// so we're OK.

#include "gears/base/common/html_event_monitor.h"

#include <assert.h>

#include "gears/base/common/string_utils.h"
#include "gears/base/npapi/np_utils.h"
#include "gears/base/npapi/scoped_npapi_handles.h"
#include "gears/base/npapi/plugin.h"

// HtmlEventMonitorHook handles its own callbacks.
DECLARE_GEARS_BRIDGE(HtmlEventMonitorHook, HtmlEventMonitorHook);

// This class serves as the bridge between the GearsFactory implementation and
// the browser binding layer.
class HtmlEventMonitorHook : public PluginBase<HtmlEventMonitorHook> {
 public:
  static void InitClass() {
    // Safari treats "handleEvent" as a method, while Firefox treats it as a
    // property.  Go figure.
    RegisterMethod("handleEvent", &HtmlEventMonitorHook::HandleEvent);
    RegisterProperty("handleEvent", &HtmlEventMonitorHook::HandleEvent);
  }

  HtmlEventMonitorHook(NPP instance) :
      PluginBase<HtmlEventMonitorHook>(instance) {
  }

  void Init(HtmlEventMonitor::HtmlEventCallback function, void *user_param) {
    function_ = function;
    user_param_ = user_param;
  }

  HtmlEventMonitorHook *gears_obj() { return this; }

  void HandleEvent() {
    function_(user_param_);  // invoke user callback
  }

 private:
  HtmlEventMonitor::HtmlEventCallback function_;
  void *user_param_;

  DISALLOW_EVIL_CONSTRUCTORS(HtmlEventMonitorHook);
};

HtmlEventMonitorHook* CreateHtmlEventMonitorHook(JsContextPtr context) {
  return static_cast<HtmlEventMonitorHook *>(
      NPN_CreateObject(context, GetNPClass<HtmlEventMonitorHook>()));
}

//
// HtmlEventMonitor
//

bool HtmlEventMonitor::Start(NPP context, NPObject *event_source) {
  assert(!event_hook_);
  assert(NPVARIANT_IS_VOID(event_source_));

  // create the event hook
  HtmlEventMonitorHook *event_hook = CreateHtmlEventMonitorHook(context);
  ScopedNPObject event_hook_scoped(event_hook);
  event_hook->Init(function_, user_param_);

  // call window.addEventListener(event_name, event_hook, false)
  NPObject* window = event_source;

  static NPIdentifier add_event_listener_id =
      NPN_GetStringIdentifier("addEventListener");

  std::string event_name_utf8;
  if (!String16ToUTF8(&event_name_[2], &event_name_utf8))  // skip leading "on"
    return false;

  NPVariant args[3];
  STDSTRING_TO_NPVARIANT(event_name_utf8, args[0]);
  OBJECT_TO_NPVARIANT(event_hook, args[1]);
  BOOLEAN_TO_NPVARIANT(false, args[2]);

  // call the method
  ScopedNPVariant result;
  if (!NPN_Invoke(context, window, add_event_listener_id, args,
                  ARRAYSIZE(args), &result))
    return false;

  // only modify data members if everything succeeded
  event_hook_scoped.release();
  event_context_ = context;
  event_hook_ = event_hook;
  event_source_.Reset(event_source);
  return true;
}

void HtmlEventMonitor::Stop() {
  assert(NPVARIANT_IS_OBJECT(event_source_));
  assert(event_hook_);

  // call window.removeEventListener(event_name, event_hook, false)
  NPObject* window = NPVARIANT_TO_OBJECT(event_source_);

  static NPIdentifier remove_event_listener_id =
      NPN_GetStringIdentifier("removeEventListener");

  std::string event_name_utf8;
  if (!String16ToUTF8(&event_name_[2], &event_name_utf8))  // skip leading "on"
    return;

  NPVariant args[3];
  STDSTRING_TO_NPVARIANT(event_name_utf8, args[0]);
  OBJECT_TO_NPVARIANT(event_hook_, args[1]);
  BOOLEAN_TO_NPVARIANT(false, args[2]);

  // call the method
  ScopedNPVariant result;
  if (!NPN_Invoke(event_context_, window, remove_event_listener_id,
                  args, 3, &result))
    return;

  // destroy the event hook
  NPN_ReleaseObject(event_hook_);
  event_context_ = NULL;
  event_hook_ = NULL;
  event_source_.Reset();
}
