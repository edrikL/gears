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
#include <windows.h> // must manually #include before nsIEventQueueService.h
#endif
#include <nsCOMPtr.h>
#include <nspr.h> // for PR_*
#include <nsServiceManagerUtils.h> // for NS_IMPL_* and NS_INTERFACE_*
#include "gears/third_party/gecko_internal/jsapi.h"
#include "gears/third_party/gecko_internal/nsITimerInternal.h"
#include "gears/third_party/gecko_internal/nsIDOMClassInfo.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "gears/base/common/js_runner.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/base/firefox/factory.h"
#include "gears/timer/firefox/timer.h"

// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_ADDREF(GearsTimer)
NS_IMPL_RELEASE(GearsTimer)
NS_INTERFACE_MAP_BEGIN(GearsTimer)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsTimerInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsTimerInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsTimer)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsTimerClassName = "GearsTimer";
const nsCID kGearsTimerClassId = {0x8ba11b96, 0x1431, 0x4796, {0xa4, 0xd3,
                                  0x19, 0xbd, 0xb2, 0xb0, 0xeb, 0x8}};
                                  // {8BA11B96-1431-4796-A4D3-19BDB2B0EB08}

NS_IMETHODIMP GearsTimer::SetTimeout(//variant *timer_code,
                                     //PRInt32 timeout,
                                     PRInt32 *retval) {

  // Create a param fetcher so we can also retrieve the context.
  std::string16 script;
  PRInt32 timeout;
  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) != 2) {
    RETURN_EXCEPTION(STRING16(L"Requires two parameters."));
  }

  if (!js_params.GetAsInt(1, &timeout)) {
    RETURN_EXCEPTION(STRING16(L"Second parameter must be an integer."));
  }

  // Temporary variable to hold the timer id, so we only set it on success.
  int timer_id;

  // Once we aquire the callback, we're responsible for its lifetime until it's
  // passed into CreateFunctionTimer.
  JsRootedCallback *timer_callback;

  // If a string was passed in, create as a string timer.  Otherwise it's
  // a function.
  if (js_params.GetAsString(0, &script)) {
    timer_id = CreateStringTimer(script.c_str(), timeout, false);
  } else if (js_params.GetAsNewRootedCallback(0, &timer_callback)) {
    timer_id = CreateFunctionTimer(timer_callback, timeout, false);
  } else {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be a function or string."));
  }

  if (timer_id == 0) {
    RETURN_EXCEPTION(STRING16(L"setTimeout failed."));
  }

  *retval = timer_id;
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsTimer::ClearTimeout(PRInt32 timer_id) {
  DeleteTimer(timer_id);
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsTimer::SetInterval(//variant *timer_code,
                                      //PRInt32 timeout,
                                      PRInt32 *retval) {

  // Create a param fetcher so we can also retrieve the context.
  std::string16 script;
  PRInt32 timeout;
  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) != 2) {
    RETURN_EXCEPTION(STRING16(L"Requires two parameters."));
  }

  if (!js_params.GetAsInt(1, &timeout)) {
    RETURN_EXCEPTION(STRING16(L"Second parameter must be an integer."));
  }

  // Temporary variable to hold the timer id, so we only set it on success.
  int timer_id;

  // Once we aquire the callback, we're responsible for its lifetime until it's
  // passed into CreateFunctionTimer.
  JsRootedCallback *timer_callback;

  // If a string was passed in, create as a string timer.  Otherwise it's
  // a function.
  if (js_params.GetAsString(0, &script)) {
    timer_id = CreateStringTimer(script.c_str(), timeout, true);
  } else if (js_params.GetAsNewRootedCallback(0, &timer_callback)) {
    timer_id = CreateFunctionTimer(timer_callback, timeout, true);
  } else {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be a function or string."));
  }

  if (timer_id == 0) {
    RETURN_EXCEPTION(STRING16(L"setInterval failed."));
  }

  *retval = timer_id;
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsTimer::ClearInterval(PRInt32 timer_id) {
  DeleteTimer(timer_id);
  RETURN_NORMAL();
}

void GearsTimer::Initialize() {
  // Monitor 'onunload' to shutdown timers when the page goes away.
  //
  // TODO(zork): This code needs to be updated to fire the unload event
  // for workers
  if (!EnvIsWorker() && unload_monitor_ == NULL) {
    unload_monitor_.reset(new HtmlEventMonitor(kEventUnload,
                                               HandleEventUnload, this));
    nsCOMPtr<nsIDOMEventTarget> event_source;
    if (NS_SUCCEEDED(DOMUtils::GetWindowEventTarget(
                                   getter_AddRefs(event_source)))) {
      unload_monitor_->Start(event_source);
    }
  }
}

PRInt32 GearsTimer::CreateFunctionTimer(JsRootedCallback *timer_callback,
                                        int timeout,
                                        bool repeat) {
  TimerInfo timer_info;
  timer_info.callback.reset(timer_callback);
  timer_info.repeat = repeat;

  return CreateTimerCommon(&timer_info, timeout);
}

PRInt32 GearsTimer::CreateStringTimer(const char16 *script,
                                      int timeout,
                                      bool repeat) {
  TimerInfo timer_info;
  timer_info.script = script;
  timer_info.repeat = repeat;

  return CreateTimerCommon(&timer_info, timeout);
}

PRInt32 GearsTimer::CreateTimerCommon(TimerInfo *timer_info, int timeout) {
  Initialize();

  // Store the timer info
  int timer_id = ++next_timer_id_;

  // Add the timer to the map.
  worker_timers_[timer_id] = *timer_info;

  TimerInfo *ti = &worker_timers_[timer_id];
  ti->owner = this;
  ti->timer_id = timer_id;

  // Create the actual timer.
  nsresult result;
  ti->timer = do_CreateInstance("@mozilla.org/timer;1", &result);

  if (NS_FAILED(result)) {
    worker_timers_.erase(timer_id);
    return 0;
  }

  // Turning off idle causes the callback to be invoked in this thread,
  // instead of in the Timer idle thread.
  nsCOMPtr<nsITimerInternal> timer_internal(do_QueryInterface(ti->timer));
  timer_internal->SetIdle(false);

  // Cast because the two constants are defined in different anonymous
  // enums, so they aren't literally of the same type, which throws a
  // warning on gcc.
  PRUint32 type = ti->repeat
      ? static_cast<PRUint32>(nsITimer::TYPE_REPEATING_SLACK)
      : static_cast<PRUint32>(nsITimer::TYPE_ONE_SHOT);

  // Start the timer
  ti->timer->InitWithFuncCallback(TimerCallback, ti, timeout, type);
  return timer_id;
}

void GearsTimer::DeleteTimer(PRInt32 timer_id) {
  std::map<int, TimerInfo>::iterator timer = worker_timers_.find(timer_id);

  if (timer != worker_timers_.end()) {
    timer->second.timer->Cancel();
    worker_timers_.erase(timer);
  }
}

void GearsTimer::HandleEventUnload(void *user_param) {
  // Use a nsCOMPtr to prevent gears_timer from getting deleted while we
  // iterate throught the map.
  nsCOMPtr<GearsTimer> gears_timer = static_cast<GearsTimer*>(user_param);

  // Iterate through the timers, removing them.
  while (!gears_timer->worker_timers_.empty()) {
    std::map<int, TimerInfo>::iterator timer =
        gears_timer->worker_timers_.begin();
    timer->second.timer->Cancel();
    gears_timer->worker_timers_.erase(timer);
  }
}

void GearsTimer::TimerCallback(nsITimer *timer, void *closure) {
  TimerInfo *ti = reinterpret_cast<TimerInfo *>(closure);

  // Store the information required to clean up the timer, in case it gets
  // deleted in the handler.
  bool repeat = ti->repeat;
  nsCOMPtr<GearsTimer> owner = ti->owner;
  int timer_id = ti->timer_id;

  // Invoke JavaScript timer handler.  *ti can become invalid here, if the timer
  // gets deleted.
  if (ti->callback.get()) {
    ti->owner->GetJsRunner()->InvokeCallback(ti->callback.get(), 0, NULL);
  } else {
    ti->owner->GetJsRunner()->Eval(ti->script.c_str());
  }

  // If this is a one shot timer, we're done with the timer object.
  if (!repeat) {
    owner->DeleteTimer(timer_id);
  }
}
