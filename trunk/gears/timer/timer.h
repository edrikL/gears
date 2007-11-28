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

#ifndef GEARS_TIMER_TIMER_H__
#define GEARS_TIMER_TIMER_H__

#if BROWSER_FF
#include <nsComponentManagerUtils.h>
#include "gears/third_party/gecko_internal/nsITimer.h"
#endif

#include <map>
#include "gears/third_party/scoped_ptr/scoped_ptr.h"
#include "gears/third_party/linked_ptr/linked_ptr.h"

#if BROWSER_FF
#include "ff/genfiles/timer_ff.h"  // from OUTDIR
#elif BROWSER_IE
#include "ie/genfiles/timer_ie.h"  // from OUTDIR
#endif
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"

#if BROWSER_FF
// Object identifiers
extern const char *kGearsTimerClassName;
extern const nsCID kGearsTimerClassId;
#endif

// Provides a COM interface for creating and destroying timers in Javascript.
// Sample usage:
//   var timer = google.gears.factory.create('beta.timer', '1.0');
//   timer.setTimeout('alert(\'Hello!\');', 1000);
class GearsTimer
    : public ModuleImplBaseClass,
#if BROWSER_FF
      public GearsTimerInterface,
#elif BROWSER_IE
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsTimer>,
      public CWindowImpl<GearsTimer>,
      public IDispatchImpl<GearsTimerInterface>,
#endif
      public JsEventHandlerInterface {
 public:
#if BROWSER_FF
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
#elif BROWSER_IE
  BEGIN_COM_MAP(GearsTimer)
    COM_INTERFACE_ENTRY(GearsTimerInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsTimer)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  void FinalRelease();

  BEGIN_MSG_MAP(GearsTimer)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
  END_MSG_MAP()
#endif
  // End boilerplate code. Begin interface.

  GearsTimer() :
#if BROWSER_IE
    in_handler_(false), release_on_final_message_(false),
#endif
    next_timer_id_(1)  {}
  ~GearsTimer() {}

  // Creates a timeout that executes handler after timeout ms.  handler can
  // be a string to eval(), or a function to call.  The id of this timeout is
  // returned in timer_id.
  // JS function is setTimeout(variant handler, int timeout).
#if BROWSER_FF
  NS_IMETHOD SetTimeout(PRInt32 *retval);
#elif BROWSER_IE
  STDMETHOD(setTimeout)(VARIANT in_value, int timeout, int *timer_id);
#endif

  // Clears the timeout with the specified id.
#if BROWSER_FF
  NS_IMETHOD ClearTimeout(PRInt32 timer_id);
#elif BROWSER_IE
  STDMETHOD(clearTimeout)(int timer_id);
#endif

  // Creates an interval that executes handler every timeout ms.  handler can
  // be a string to eval(), or a function to call.  The id of this interval is
  // returned in timer_id.
  // JS function is setInterval(variant handler, int timeout).
#if BROWSER_FF
  NS_IMETHOD SetInterval(PRInt32 *retval);
#elif BROWSER_IE
  STDMETHOD(setInterval)(VARIANT in_value, int timeout, int *timer_id);
#endif

  // Clears the interval with the specified id.
#if BROWSER_FF
  NS_IMETHOD ClearInterval(PRInt32 timer_id);
#elif BROWSER_IE
  STDMETHOD(clearInterval)(int timer_id);
#endif

  // This is the callback used to handle the 'JSEVENT_UNLOAD' event.
  void HandleEvent(JsEventType event_type);

 private:
  // Contains the information that represents a single timeout or interval.
  // This includes the data representing the platform's timer structure, and
  // the callback information for when the timer fires.
  struct TimerInfo {
    TimerInfo() : timer_id(0), owner(NULL) {}
    ~TimerInfo();

    void SetOwner(GearsTimer *new_owner) {
      owner = new_owner;
      owner->AddRef();
    }

    linked_ptr<JsRootedCallback> callback;
    std::string16 script;
    bool repeat;
    int timer_id;
    GearsTimer *owner;
#if BROWSER_FF
    nsCOMPtr<nsITimer> timer;
#endif
  };

  // Sets up any structures that aren't needed until the interface is used.
  // It's ok to call this multiple times.
  void Initialize();

  // Creates a timer that has a function callback with the specified timeout.
  // repeat determines if this is an interval or timeout.  Returns the id of
  // the new timer, or -1 on failure.
  int CreateFunctionTimer(JsRootedCallback *timer_callback,
                          int timeout,
                          bool repeat);

  // Creates a timer that has a string to eval() with the specified timeout.
  // repeat determines if this is an interval or timeout.  Returns the id of
  // the new timer, or -1 on failure.
  int CreateStringTimer(const char16 *full_script,
                        int timeout,
                        bool repeat);

  // Called by Create*Timer() to actually create the timer.  Returns the id of
  // the new timer, or -1 on failure.
  int CreateTimerCommon(const TimerInfo &timer_info, int timeout);
  void HandleTimer(TimerInfo *timer_info);

  std::map<int, TimerInfo> timers_;
  int next_timer_id_;
  scoped_ptr<JsEventMonitor> unload_monitor_;

#if BROWSER_FF
  static void TimerCallback(nsITimer *timer, void *closure);
#elif BROWSER_IE
  LRESULT OnTimer(UINT msg, WPARAM timer_id,
                  LPARAM unused_param, BOOL& handled);
  void OnFinalMessage();

  bool in_handler_;
  bool release_on_final_message_;
#endif

  DISALLOW_EVIL_CONSTRUCTORS(GearsTimer);
};

#endif  // GEARS_TIMER_TIMER_H__
