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

#include <dispex.h>
#include "gears/timer/ie/timer.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/ie/activex_utils.h"


GearsTimer::TimerInfo::~TimerInfo() {
  if (owner != NULL && owner->IsWindow() && timer_id != 0) {
    owner->KillTimer(timer_id);
  }
}

void GearsTimer::FinalRelease() {
  if (IsWindow()) {
    DestroyWindow();
  }
}

void GearsTimer::OnFinalMessage() {
  // Release the last reference to this, and cause a delete.
  if (release_on_final_message_) {
    this->Release();
  }
}

STDMETHODIMP GearsTimer::setTimeout(VARIANT in_value,
                                    int timeout,
                                    int *timer_id) {
  // Protect against modifying output param on failure.
  int timer_id_temp;

  // Determine which type of timer to create based on the in_value type.
  if (in_value.vt == VT_BSTR) {
    timer_id_temp = CreateStringTimer(in_value.bstrVal, timeout, false);
  } else if (in_value.vt == VT_DISPATCH) {
    timer_id_temp = CreateDispatchTimer(in_value.pdispVal, timeout, false);
  } else {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be a function or string."));
  }

  if (timer_id_temp == 0) {
    RETURN_EXCEPTION(STRING16(L"Timer creation failed."));
  }

  *timer_id = timer_id_temp;
  RETURN_NORMAL();
}

STDMETHODIMP GearsTimer::clearTimeout(int timer_id) {
  timers_.erase(timer_id);
  RETURN_NORMAL();
}

STDMETHODIMP GearsTimer::setInterval(VARIANT in_value,
                                     int timeout,
                                     int *timer_id) {
  // Protect against modifying output param on failure.
  int timer_id_temp;

  // Determine which type of timer to create based on the in_value type.
  if (in_value.vt == VT_BSTR) {
    timer_id_temp = CreateStringTimer(in_value.bstrVal, timeout, true);
  } else if (in_value.vt == VT_DISPATCH) {
    timer_id_temp = CreateDispatchTimer(in_value.pdispVal, timeout, true);
  } else {
    RETURN_EXCEPTION(
        STRING16(L"First parameter must be a function or string."));
  }

  if (timer_id_temp == 0) {
    RETURN_EXCEPTION(STRING16(L"Timer creation failed."));
  }

  *timer_id = timer_id_temp;
  RETURN_NORMAL();
}

STDMETHODIMP GearsTimer::clearInterval(int timer_id) {
  timers_.erase(timer_id);
  RETURN_NORMAL();
}

void GearsTimer::Initialize() {
  // Make sure we have an HWND
  if (!IsWindow()) {
    if (!Create(HWND_MESSAGE)) {
      assert(false);
    }
  }

  // Create an HTML event monitor to remove remaining timers when the page
  // unloads
  //
  // TODO(zork): This needs to be updated to send an unload event to workers
  // as well.
  if (!EnvIsWorker() && unload_monitor_ == NULL) {
    unload_monitor_.reset(new HtmlEventMonitor(kEventUnload,
                                               HandleEventUnload, this));
    CComPtr<IHTMLWindow3> event_source;
    IUnknown *site = this->EnvPageIUnknownSite();
    assert(site);
    HRESULT hr = ActiveXUtils::GetHtmlWindow3(site, &event_source);
    assert(SUCCEEDED(hr));
    unload_monitor_->Start(event_source);
  }
}

int GearsTimer::CreateDispatchTimer(IDispatch *handler,
                                    int timeout,
                                    bool repeat) {
  TimerInfo timer_info;
  timer_info.timer_func = handler;
  timer_info.repeat = repeat;

  return CreateTimerCommon(timer_info, timeout);
}

int GearsTimer::CreateStringTimer(const char16 *full_script,
                                  int timeout,
                                  bool repeat) {
  TimerInfo timer_info;
  timer_info.script = full_script;
  timer_info.repeat = repeat;

  return CreateTimerCommon(timer_info, timeout);
}

int GearsTimer::CreateTimerCommon(TimerInfo &timer_info, int timeout) {
  Initialize();

  int timer_id = ++next_timer_id_;
  if (0 == SetTimer(timer_id, timeout, NULL)) {
    return 0;
  }

  timers_[timer_id] = timer_info;
  timers_[timer_id].timer_id = timer_id;
  timers_[timer_id].SetOwner(this);

  return timer_id;
}

void GearsTimer::HandleEventUnload(void *user_param) {
  // Use a CComPtr to AddRef the Timer object to keep it from getting deleted
  // while we iterate through the map
  CComPtr<GearsTimer> gears_timer = static_cast<GearsTimer*>(user_param);

  gears_timer->timers_.clear();
}

LRESULT GearsTimer::OnTimer(UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled) {
  // Protect against a delete while running the timer handler.
  CComPtr<GearsTimer> gears_timer = this;
  bHandled = TRUE;

  // Prevent re-entry.
  if (in_handler_) {
    return 0;
  }
  in_handler_ = true;

  std::map<int, TimerInfo>::iterator timer = timers_.find(wParam);

  if (timer == timers_.end()) {
    // This can happen if the event has already been posted, but the timer was
    // deleted.
    KillTimer(wParam);
  } else {
    // Store the repeat flag, in case the timer object is deleted in the
    // callback.
    bool repeat = timer->second.repeat;

    // Disable the timer now if it's a one shot.
    if (!repeat) {
      KillTimer(wParam);
    }

    // Invoke the handler.  timer can potentially be invalid after this, if the
    // handler deletes the timer.
    if (timer->second.timer_func) {

      CComQIPtr<IDispatchEx> dispatchex = timer->second.timer_func;

      DISPPARAMS invoke_params = {0};

      // Invoke JavaScript timer handler
      if (dispatchex) {
        DISPID disp_this = DISPID_THIS;
        VARIANT var[1];
        var[0].vt = VT_DISPATCH;
        var[0].pdispVal = dispatchex;

        invoke_params.rgdispidNamedArgs = &disp_this;
        invoke_params.cNamedArgs = 1;
        invoke_params.cArgs = 1;
        invoke_params.rgvarg = var;

        HRESULT hr = dispatchex->InvokeEx(
            DISPID_VALUE,           // DISPID_VALUE = default action
            LOCALE_USER_DEFAULT,
            DISPATCH_METHOD,        // dispatch/invoke as...
            &invoke_params,         // parameters
            NULL,
            NULL,
            NULL);
      } else {
        invoke_params.cArgs = 0;
        invoke_params.rgvarg = 0;

        HRESULT hr = timer->second.timer_func->Invoke(
            DISPID_VALUE, IID_NULL, // DISPID_VALUE = default action
            LOCALE_USER_DEFAULT,
            DISPATCH_METHOD,        // dispatch/invoke as...
            &invoke_params,         // parameters
            NULL,                   // receives result (NULL okay)
            NULL,                   // receives exception (NULL okay)
            NULL);                  // receives badarg index (NULL okay)
      }
    } else {
      GetJsRunner()->Eval(timer->second.script.c_str());
    }

    if (!repeat) {
      // If this is just a timeout, we're done with the timer.
      timers_.erase(wParam);
    }

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
