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

#include "gears/timer/timer.h"

#if BROWSER_FF
#include <gecko_internal/nsITimerInternal.h>
#endif

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/module_wrapper.h"



#if BROWSER_IE
WindowsPlatformTimer::WindowsPlatformTimer(GearsTimer *gears_timer)
    : gears_timer_(gears_timer),
      in_handler_(false) {
}

WindowsPlatformTimer::~WindowsPlatformTimer() {
}

void WindowsPlatformTimer::OnFinalMessage(HWND hwnd) {
  delete this;
}

LRESULT WindowsPlatformTimer::OnTimer(UINT msg,
                                      WPARAM timer_id,
                                      LPARAM unused_param,
                                      BOOL& handled) {
  handled = TRUE;

  // Prevent re-entry.
  if (in_handler_) {
    return 0;
  }
  in_handler_ = true;
  // We Add/RemoveReference to protect against a delete while running the timer
  // handler.
  gears_timer_->AddReference();

  std::map<int, GearsTimer::TimerInfo>::iterator timer =
      gears_timer_->timers_.find(timer_id);
  if (timer == gears_timer_->timers_.end()) {
    // This can happen if the event has already been posted, but the timer was
    // deleted.
    KillTimer(timer_id);
  } else {
    gears_timer_->HandleTimer(&(timer->second));
  }

  gears_timer_->RemoveReference();
  in_handler_ = false;
  return 0;
}

void WindowsPlatformTimer::Initialize() {
  // Make sure we have an HWND
  if (!IsWindow()) {
    if (!Create(kMessageOnlyWindowParent,    // parent
                NULL,                        // position
                NULL,                        // name
                kMessageOnlyWindowStyle)) {  // style
      assert(false);
    }
  }
}

void WindowsPlatformTimer::CancelGearsTimer(int timer_id) {
  if (IsWindow() && timer_id != 0) {
    KillTimer(timer_id);
  }
}
#endif



// Disables the timer when the TimerInfo is deleted.
GearsTimer::TimerInfo::~TimerInfo() {
#if BROWSER_FF
  if (platform_timer) {
    platform_timer->Cancel();
  }
#endif
  if (owner) {
#if BROWSER_IE
    owner->platform_timer_->CancelGearsTimer(timer_id);
#endif
    owner->RemoveReference();
  }
}



DECLARE_GEARS_WRAPPER(GearsTimer);

template<>
void Dispatcher<GearsTimer>::Init() {
  RegisterMethod("clearInterval", &GearsTimer::ClearInterval);
  RegisterMethod("clearTimeout", &GearsTimer::ClearTimeout);
  RegisterMethod("setInterval", &GearsTimer::SetInterval);
  RegisterMethod("setTimeout", &GearsTimer::SetTimeout);
}

void GearsTimer::SetTimeout(JsCallContext *context) {
  SetTimeoutOrInterval(context, false);
}

void GearsTimer::ClearTimeout(JsCallContext *context) {
  ClearTimeoutOrInterval(context);
}

void GearsTimer::SetInterval(JsCallContext *context) {
  SetTimeoutOrInterval(context, true);
}

void GearsTimer::ClearInterval(JsCallContext *context) {
  ClearTimeoutOrInterval(context);
}

void GearsTimer::SetTimeoutOrInterval(JsCallContext *context, bool repeat) {
  int timeout;

  std::string16 script;
  JsRootedCallback *timer_callback = NULL;

  const int argc = 2;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_UNKNOWN, NULL },
    { JSPARAM_REQUIRED, JSPARAM_INT, &timeout },
  };

  int timer_code_type = context->GetArgumentType(0);
  if (timer_code_type == JSPARAM_FUNCTION) {
    argv[0].type = JSPARAM_FUNCTION;
    argv[0].value_ptr = &timer_callback;
  } else if (timer_code_type == JSPARAM_STRING16) {
    argv[0].type = JSPARAM_STRING16;
    argv[0].value_ptr = &script;
  } else {
    context->SetException(
        STRING16(L"First parameter must be a function or string."));
    return;
  }

  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  TimerInfo timer_info;
  timer_info.repeat = repeat;
  if (timer_callback) {
    timer_info.callback.reset(timer_callback);
  } else {
    timer_info.script = script;
  }

  int result = CreateTimer(timer_info, timeout);
  if (result == 0) {
    context->SetException(STRING16(L"Timer creation failed."));
    return;
  }

  context->SetReturnValue(JSPARAM_INT, &result);
}

void GearsTimer::ClearTimeoutOrInterval(JsCallContext *context) {
  int timer_id;

  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &timer_id },
  };

  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  timers_.erase(timer_id);
}

// Makes sure the object's structures are initialized.  We need to set up the
// unload monitor for the web page.
void GearsTimer::Initialize() {
#if BROWSER_IE
  platform_timer_->Initialize();
#endif

  // Create an event monitor to remove remaining timers when the page
  // unloads.
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD,
                                             this));
  }
}

// Creates the platform's timer object, perform all common initialization of the
// TimerInfo structure, and store the TimerInfo in the map.
int GearsTimer::CreateTimer(const TimerInfo &timer_info, int timeout) {
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
  timers_[timer_id].platform_timer =
      do_CreateInstance("@mozilla.org/timer;1", &result);

  if (NS_FAILED(result)) {
    timers_.erase(timer_id);
    return 0;
  }

  // Turning off idle causes the callback to be invoked in this thread,
  // instead of in the Timer idle thread.
  nsCOMPtr<nsITimerInternal> timer_internal(
      do_QueryInterface(timers_[timer_id].platform_timer));
  timer_internal->SetIdle(false);

  // Cast because the two constants are defined in different anonymous
  // enums, so they aren't literally of the same type, which throws a
  // warning on gcc.
  PRUint32 type = timers_[timer_id].repeat
      ? static_cast<PRUint32>(nsITimer::TYPE_REPEATING_SLACK)
      : static_cast<PRUint32>(nsITimer::TYPE_ONE_SHOT);

  // Start the timer
  timers_[timer_id].platform_timer->InitWithFuncCallback(
      TimerCallback, &timers_[timer_id], timeout, type);
#elif BROWSER_IE
  if (0 == platform_timer_->SetTimer(timer_id, timeout, NULL)) {
    timers_.erase(timer_id);
    return 0;
  }
#endif

  return timer_id;
}

// Handles the page being unloaded.  Clean up any remaining active timers.
void GearsTimer::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);

  // AddReference this Timer object to keep it from getting deleted
  // while we iterate through the map
  AddReference();
  timers_.clear();
  RemoveReference();
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

  // AddReference this Timer object to keep it from getting deleted
  // while running the timer handler.
  GearsTimer *owner = timer_info->owner;
  owner->AddReference();
  owner->HandleTimer(timer_info);
  owner->RemoveReference();
}
#endif
