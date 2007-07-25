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
// Adds timers that are usable in worker threads.

#ifndef GEARS_TIMERS_IE_TIMERS_H__
#define GEARS_TIMERS_IE_TIMERS_H__

#include <map>
#include "ie/genfiles/interfaces.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/html_event_monitor.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/common.h"

class ATL_NO_VTABLE GearsTimer
    : public GearsBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsTimer>,
      public CWindowImpl<GearsTimer>,
      public IDispatchImpl<GearsTimerInterface> {
 public:
  BEGIN_COM_MAP(GearsTimer)
    COM_INTERFACE_ENTRY(GearsTimerInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsTimer)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  // End boilerplate code. Begin interface.

  // need a default constructor to CreateInstance objects in IE
  GearsTimer() : next_timer_id_(1), in_handler_(false) {}

  void FinalRelease();

  STDMETHOD(setTimeout)(IDispatch *in_value, int timeout, int *timer_id);
  STDMETHOD(clearTimeout)(int timer_id);
  STDMETHOD(setInterval)(IDispatch *in_value, int timeout, int *timer_id);
  STDMETHOD(clearInterval)(int timer_id);

  BEGIN_MSG_MAP(GearsTimer)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
  END_MSG_MAP()

 private:
  struct TimerInfo {
    TimerInfo() : timer_id(0), owner(NULL) {}
    ~TimerInfo();

    void SetOwner(GearsTimer *new_owner) {
      owner = new_owner;
      iunk_owner = new_owner->_GetRawUnknown();
    }

    CComPtr<IDispatch> timer_func;
    std::string16 script;
    bool repeat;
    int timer_id;
    // owner can't be a CComPtr, due to oddities with ATL and the "this"
    // pointer.
    GearsTimer *owner;
    CComPtr<IUnknown> iunk_owner; // This is used to refcount owner.
  };

  void Initialize(); // okay to call this multiple times
  int CreateDispatchTimer(IDispatch *handler,
                          int timeout,
                          bool repeat);
  int CreateStringTimer(const char16 *full_script,
                        int timeout,
                        bool repeat);
  int CreateTimerCommon(TimerInfo &timer_info, int timeout);
  LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  static void HandleEventUnload(void *user_param);  // callback for 'onunload'

  scoped_ptr<HtmlEventMonitor> unload_monitor_;  // for 'onunload' notifications

  std::map<int, TimerInfo> timers_;
  int next_timer_id_;
  bool in_handler_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsTimer);
};

#endif // GEARS_TIMERS_IE_TIMERS_H__
