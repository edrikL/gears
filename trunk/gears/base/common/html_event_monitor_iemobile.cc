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

#ifdef BROWSER_IEMOBILE

#include "gears/base/common/html_event_monitor.h"

#include "gears/base/common/wince_compatibility.h"

// Since there is no way to clear an onunload handler set via
// IPIEHTMLWindow2::put_onunload, we use a special static HtmlEventMonitorHook
// that sets the callback to the noop function below.
static void Noop(void*) {}

// HtmlEventMonitorHook is used to detect:
// 1. real unload events from the browser via the IPIEHTMLWindow2::put_onunload
//    hook,
// 2. fake unload events from the client application via the
//    GearsFactory::PrivateSendUnloadEvent() hook.
class HtmlEventMonitorHook
    : public IDispEventSimpleImpl<0, HtmlEventMonitorHook, &IID_IDispatch>,
      public UnloadEventHandlerInterface {
 public:
  HtmlEventMonitorHook(HtmlEventMonitor::HtmlEventCallback function,
                       void *user_param);

  // Ugly smelly goo
  IDispatch* GetIDispatch();

  // This handles a real event fired by IPIEHTMLWindow2::put_onunload.
  VARIANT_BOOL STDMETHODCALLTYPE HandleRealEvent();

  BEGIN_SINK_MAP(HtmlEventMonitorHook)
    SINK_ENTRY_INFO(0, IID_IDispatch, DISPID_VALUE, HandleRealEvent, &EventInfo)
  END_SINK_MAP()

  // UnloadEventHandlerInterface
  // This handles a 'fake' unload event, sent by the application code via
  // GearsFactory::PrivateSendUnloadEvent.
  virtual void HandleFakeEvent();

 private:
  HtmlEventMonitor::HtmlEventCallback function_;
  void *user_param_;
  static _ATL_FUNC_INFO EventInfo;

  DISALLOW_EVIL_CONSTRUCTORS(HtmlEventMonitorHook);
};

_ATL_FUNC_INFO HtmlEventMonitorHook::EventInfo = {
    CC_STDCALL, VT_BOOL, 0, {}
};

HtmlEventMonitorHook::HtmlEventMonitorHook(
    HtmlEventMonitor::HtmlEventCallback function, void *user_param)
    : function_(function), user_param_(user_param) {
}

IDispatch* HtmlEventMonitorHook::GetIDispatch() {
  // suitable for passing to attachEvent()
  return reinterpret_cast<IDispatch*>(this);
}

VARIANT_BOOL HtmlEventMonitorHook::HandleRealEvent() {
  function_(user_param_);  // invoke user callback
  return VARIANT_TRUE;
}

void HtmlEventMonitorHook::HandleFakeEvent() {
  function_(user_param_);
}

//
// HtmlEventMonitor
//

bool HtmlEventMonitor::Start(IPIEHTMLWindow2 *event_source) {
  // We currently only support unload events.
  if (event_name_ != kEventUnload) { return false; }

  // attach the event hook to the 'fake' source.
  event_hook_ = new HtmlEventMonitorHook(function_, user_param_);
  UnloadEventSource::RegisterHandler(event_hook_);
  // Attach the hook to the IPIEHTMLWindow2 but only if there is no
  // other handler already installed.
  CComVariant existing_unload_handler;
  HRESULT hr = event_source->get_onunload(&existing_unload_handler);
  // If the above failed, we return true since we have set the
  // 'fake' hook anyway.
  if FAILED(hr) { return true; }
  if (VT_DISPATCH == V_VT(&existing_unload_handler)) {
    // We found a handler. We cannot overwrite it, so return.
    return true;
  }

  // There is no existing handler set, so we can install our own.
  VARIANT var = {0};
  var.vt = VT_DISPATCH;
  var.pdispVal = event_hook_->GetIDispatch();
  hr = event_source->put_onunload(var);
  if SUCCEEDED(hr) {
    event_source_ = event_source;
  }
  return true;
}

void HtmlEventMonitor::Stop() {
  assert(event_hook_);
  UnloadEventSource::UnregisterHandler();
  if (event_source_) {
    // We were also attached to IPIEHTMLWindow2
    static HtmlEventMonitorHook dummy_handler(Noop, NULL);
    VARIANT var = {0};
    var.vt = VT_DISPATCH;
    var.pdispVal = dummy_handler.GetIDispatch();
    HRESULT hr = event_source_->put_onunload(var);
    event_source_ = NULL;
  }
  event_hook_->Release();
  event_hook_ = NULL;
}

#endif  // BROWSER_IEMOBILE
