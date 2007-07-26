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

#ifndef GEARS_TIMERS_FIREFOX_TIMERS_H__
#define GEARS_TIMERS_FIREFOX_TIMERS_H__

#include <map>
#include <nsComponentManagerUtils.h>
#include "gears/third_party/gecko_internal/nsITimer.h"

#include "ff/genfiles/timer.h" // from OUTDIR
#include "gears/base/common/base_class.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/common.h"

struct TimerEvent;

// Object identifiers
extern const char *kGearsTimerClassName;
extern const nsCID kGearsTimerClassId;

class GearsTimer
    : public GearsBaseClass,
      public GearsTimerInterface {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  // need a default constructor to instance objects from the Factory
  GearsTimer() : next_timer_id_(1) {}
  ~GearsTimer() {}

  NS_IMETHOD SetTimeout(TimerHandler *timer_callback,
                        PRInt32 timeout,
                        PRInt32 *retval);
  NS_IMETHOD ClearTimeout(PRInt32 timer_id);
  NS_IMETHOD SetInterval(TimerHandler *timer_callback,
                         PRInt32 timeout,
                         PRInt32 *retval);
  NS_IMETHOD ClearInterval(PRInt32 timer_id);

 private:
  struct TimerInfo {
    TimerInfo() : function(0), context(NULL), timer_id(0) {}

    JsToken function;
    JsContextPtr context;
    std::string16 script;
    bool repeat;
    nsCOMPtr<GearsTimer> owner;
    nsCOMPtr<nsITimer> timer;
    PRInt32 timer_id;
  };

  void Initialize(); // okay to call this multiple times
  PRInt32 CreateFunctionTimer(JsToken timer_function,
                              JsContextPtr script_context,
                              int timeout,
                              bool repeat);
  PRInt32 CreateStringTimer(const char16 *script,
                            int timeout,
                            bool repeat);
  PRInt32 CreateTimerCommon(TimerInfo *timer_info, int timeout);
  void DeleteTimer(PRInt32 timer_id);

  static void HandleEventUnload(void *user_param);  // callback for 'onunload'
  static void TimerCallback(nsITimer *timer, void *closure);

  std::map<int, TimerInfo> worker_timers_;
  int next_timer_id_;
  scoped_ptr<HtmlEventMonitor> unload_monitor_;  // for 'onunload' notifications

  DISALLOW_EVIL_CONSTRUCTORS(GearsTimer);
};

#endif // GEARS_TIMERS_FIREFOX_TIMERS_H__
