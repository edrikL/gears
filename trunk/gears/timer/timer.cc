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

#if BROWSER_FF
#ifdef WIN32
#include <windows.h>  // must manually #include before nsIEventQueueService.h
#endif
#include <gecko_sdk/include/nspr.h>  // for PR_*
#include <gecko_sdk/include/nsServiceManagerUtils.h>  // for NS_IMPL_* and NS_INTERFACE_*
#include <gecko_sdk/include/nsCOMPtr.h>
#include <gecko_internal/jsapi.h>
#include <gecko_internal/nsIDOMClassInfo.h>
#include <gecko_internal/nsITimerInternal.h>
#elif BROWSER_IE
#include <dispex.h>
#include "gears/base/ie/activex_utils.h"
#endif

#include "gears/timer/timer.h"

#if BROWSER_FF
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
#endif

// Disables the timer when the TimerInfo is deleted.
GearsTimer::TimerInfo::~TimerInfo() {
#if BROWSER_FF
  if (timer) {
    timer->Cancel();
  }
#endif
  if (owner) {
#if BROWSER_IE
    if (owner->IsWindow() && timer_id != 0) {
      owner->KillTimer(timer_id);
    }
#endif
    owner->Release();
  }
}

// Creates a timeout.  This is done by determining the type of the handler
// passed in, then calling the associated helper function.
// JS function is setTimeout(variant, int).
#if BROWSER_FF
NS_IMETHODIMP GearsTimer::SetTimeout(//variant *timer_code,
                                     //PRInt32 timeout,
                                     PRInt32 *timer_id) {
#elif BROWSER_IE
STDMETHODIMP GearsTimer::setTimeout(VARIANT in_value,
                                    int timeout,
                                    int *timer_id) {
#endif
  // Protect against modifying output param on failure.
  int timer_id_temp;
  std::string16 script;

  // Once we aquire the callback, we're responsible for its lifetime until it's
  // passed into CreateFunctionTimer.
  JsRootedCallback *timer_callback = 0;

  // Determine which type of timer to create based on the in_value type.
#if BROWSER_FF
  JsParamFetcher js_params(this);
  int timeout;

  if (js_params.GetCount(false) != 2) {
    RETURN_EXCEPTION(STRING16(L"Requires two parameters."));
  }

  if (!js_params.GetAsInt(1, &timeout)) {
    RETURN_EXCEPTION(STRING16(L"Second parameter must be an integer."));
  }

  if ((js_params.GetType(0) == JSPARAM_FUNCTION &&
      !js_params.GetAsNewRootedCallback(0, &timer_callback)) ||
      !js_params.GetAsString(0, &script)) {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be a function or string."));
  }
#elif BROWSER_IE
  if (in_value.vt == VT_BSTR) {
    script = in_value.bstrVal;
  } else if (in_value.vt == VT_DISPATCH) {
    timer_callback = new JsRootedToken(NULL, in_value);
  } else {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be a function or string."));
  }
#endif

  // If a string was passed in, create as a string timer.  Otherwise it's
  // a function.
  if (timer_callback) {
    timer_id_temp = CreateFunctionTimer(timer_callback, timeout, false);
  } else {
    timer_id_temp = CreateStringTimer(script.c_str(), timeout, false);
  }

  if (timer_id_temp == 0) {
    RETURN_EXCEPTION(STRING16(L"Timer creation failed."));
  }

  *timer_id = timer_id_temp;
  RETURN_NORMAL();
}

// Removes the timeout with the specified id.
#if BROWSER_FF
NS_IMETHODIMP GearsTimer::ClearTimeout(PRInt32 timer_id) {
#elif BROWSER_IE
STDMETHODIMP GearsTimer::clearTimeout(int timer_id) {
#endif
  timers_.erase(timer_id);
  RETURN_NORMAL();
}

// Creates an interval.  This is done by determining the type of the handler
// passed in, then calling the associated helper function.
// JS function is setInterval(variant handler, int timeout).
#if BROWSER_FF
NS_IMETHODIMP GearsTimer::SetInterval(//variant *timer_code,
                                      //PRInt32 timeout,
                                      PRInt32 *timer_id) {
#elif BROWSER_IE
STDMETHODIMP GearsTimer::setInterval(VARIANT in_value,
                                    int timeout,
                                    int *timer_id) {
#endif
  // Protect against modifying output param on failure.
  int timer_id_temp;
  std::string16 script;

  // Once we aquire the callback, we're responsible for its lifetime until it's
  // passed into CreateFunctionTimer.
  JsRootedCallback *timer_callback = 0;

  // Determine which type of timer to create based on the in_value type.
#if BROWSER_FF
  JsParamFetcher js_params(this);
  int timeout;

  if (js_params.GetCount(false) != 2) {
    RETURN_EXCEPTION(STRING16(L"Requires two parameters."));
  }

  if (!js_params.GetAsInt(1, &timeout)) {
    RETURN_EXCEPTION(STRING16(L"Second parameter must be an integer."));
  }

  if ((js_params.GetType(0) == JSPARAM_FUNCTION &&
      !js_params.GetAsNewRootedCallback(0, &timer_callback)) ||
      !js_params.GetAsString(0, &script)) {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be a function or string."));
  }
#elif BROWSER_IE
  if (in_value.vt == VT_BSTR) {
    script = in_value.bstrVal;
  } else if (in_value.vt == VT_DISPATCH) {
    timer_callback = new JsRootedToken(NULL, in_value);
  } else {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be a function or string."));
  }
#endif

  // If a string was passed in, create as a string timer.  Otherwise it's
  // a function.
  if (timer_callback) {
    timer_id_temp = CreateFunctionTimer(timer_callback, timeout, true);
  } else {
    timer_id_temp = CreateStringTimer(script.c_str(), timeout, true);
  }

  if (timer_id_temp == 0) {
    RETURN_EXCEPTION(STRING16(L"Timer creation failed."));
  }

  *timer_id = timer_id_temp;
  RETURN_NORMAL();
}

// Removes the interval with the specified id.
#if BROWSER_FF
NS_IMETHODIMP GearsTimer::ClearInterval(PRInt32 timer_id) {
#elif BROWSER_IE
STDMETHODIMP GearsTimer::clearInterval(int timer_id) {
#endif
  timers_.erase(timer_id);
  RETURN_NORMAL();
}

// Makes sure the object's structures are initialized.  We need to set up the
// unload monitor for the web page.
void GearsTimer::Initialize() {
#if BROWSER_IE
  // Make sure we have an HWND
  if (!IsWindow()) {
    if (!Create(kMessageOnlyWindowParent,    // parent
                NULL,                        // position
                NULL,                        // name
                kMessageOnlyWindowStyle)) {  // style
      assert(false);
    }
  }
#endif

  // Create an event monitor to remove remaining timers when the page
  // unloads.
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD,
                                             this));
  }
}

// Initializes the TimerInfo structure, and pass it to the common creation
// function.
int GearsTimer::CreateFunctionTimer(JsRootedCallback *timer_callback,
                                    int timeout,
                                    bool repeat) {
  TimerInfo timer_info;
  timer_info.callback.reset(timer_callback);
  timer_info.repeat = repeat;

  return CreateTimerCommon(timer_info, timeout);
}

// Initializes the TimerInfo structure, and pass it to the common creation
// function.
int GearsTimer::CreateStringTimer(const char16 *full_script,
                                  int timeout,
                                  bool repeat) {
  TimerInfo timer_info;
  timer_info.script = full_script;
  timer_info.repeat = repeat;

  return CreateTimerCommon(timer_info, timeout);
}

// Creates the platform's timer object, perform all common initialization of the
// TimerInfo structure, and store the TimerInfo in the map.
int GearsTimer::CreateTimerCommon(const TimerInfo &timer_info, int timeout) {
  Initialize();

  // Store the timer info
  int timer_id = ++next_timer_id_;

  // Add the timer to the map.
  timers_[timer_id] = timer_info;
  timers_[timer_id].timer_id = timer_id;
  timers_[timer_id].SetOwner(this);

  // Create the actual timer.
#if BROWSER_FF
  nsresult result;
  timers_[timer_id].timer = do_CreateInstance("@mozilla.org/timer;1", &result);

  if (NS_FAILED(result)) {
    timers_.erase(timer_id);
    return 0;
  }

  // Turning off idle causes the callback to be invoked in this thread,
  // instead of in the Timer idle thread.
  nsCOMPtr<nsITimerInternal> timer_internal(do_QueryInterface(
                                                timers_[timer_id].timer));
  timer_internal->SetIdle(false);

  // Cast because the two constants are defined in different anonymous
  // enums, so they aren't literally of the same type, which throws a
  // warning on gcc.
  PRUint32 type = timers_[timer_id].repeat
      ? static_cast<PRUint32>(nsITimer::TYPE_REPEATING_SLACK)
      : static_cast<PRUint32>(nsITimer::TYPE_ONE_SHOT);

  // Start the timer
  timers_[timer_id].timer->InitWithFuncCallback(TimerCallback,
                                                &timers_[timer_id],
                                                timeout, type);
#elif BROWSER_IE
  if (0 == SetTimer(timer_id, timeout, NULL)) {
    timers_.erase(timer_id);
    return 0;
  }
#endif

  return timer_id;
}

// Handles the page being unloaded.  Clean up any remaining active timers.
void GearsTimer::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);

  // Use a ComPtr to AddRef the Timer object to keep it from getting deleted
  // while we iterate through the map
#if BROWSER_FF
  nsCOMPtr<GearsTimer> gears_timer(this);
#elif BROWSER_IE
  CComPtr<GearsTimer> gears_timer(this);
#endif

  timers_.clear();
}

// Perform the non-platform specific work that occurs when a timer fires.
void GearsTimer::HandleTimer(TimerInfo *timer_info) {
  // Store the information required to clean up the timer, in case it gets
  // deleted in the handler.
  bool repeat = timer_info->repeat;
  int timer_id = timer_info->timer_id;

  // Invoke JavaScript timer handler.  *timer_info can become invalid here, if
  // the timer gets deleted in the handler.
  if (timer_info->callback.get()) {
    GetJsRunner()->InvokeCallback(timer_info->callback.get(), 0, NULL, NULL);
  } else {
    GetJsRunner()->Eval(timer_info->script);
  }

  // If this is a one shot timer, we're done with the timer object.
  if (!repeat) {
    timers_.erase(timer_id);
  }
}

// Perform the platform specific work when a timer fires.
#if BROWSER_FF
void GearsTimer::TimerCallback(nsITimer *timer, void *closure) {
  TimerInfo *timer_info = reinterpret_cast<TimerInfo *>(closure);

  // Protect against a delete while running the timer handler.
  nsCOMPtr<GearsTimer> owner = timer_info->owner;

  owner->HandleTimer(timer_info);
}
#elif BROWSER_IE
LRESULT GearsTimer::OnTimer(UINT msg,
                            WPARAM timer_id,
                            LPARAM unused_param,
                            BOOL& handled) {
  // Protect against a delete while running the timer handler.
  CComPtr<GearsTimer> gears_timer = this;
  handled = TRUE;

  // Prevent re-entry.
  if (in_handler_) {
    return 0;
  }
  in_handler_ = true;

  std::map<int, TimerInfo>::iterator timer = timers_.find(timer_id);

  if (timer == timers_.end()) {
    // This can happen if the event has already been posted, but the timer was
    // deleted.
    KillTimer(timer_id);
  } else {
    HandleTimer(&(timer->second));
  }

  // If we would delete ourself here, we need to defer the release until we're
  // out of the message handler.
  if (gears_timer->m_dwRef == 1) {
    release_on_final_message_ = true;
    AddRef();
    PostMessage(WM_DESTROY, 0, 0);
  }

  // Turn off re-entry prevention.
  in_handler_ = false;

  return 0;
}

// After the last instance is released, we destroy the window we created to
// capture timer events.
void GearsTimer::FinalRelease() {
  if (IsWindow()) {
    DestroyWindow();
  }
}

// If we're deleted in the message loop, we defer the final release to here.
void GearsTimer::OnFinalMessage() {
  // Release the last reference to this, and cause a delete.
  if (release_on_final_message_) {
    this->Release();
  }
}
#endif
