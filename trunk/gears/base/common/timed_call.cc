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

#include "gears/base/common/timed_call.h"

#include <assert.h>

#if defined(OS_MACOSX)
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <set>

#include "gears/base/common/stopwatch.h"  // for GetCurrentTimeMillis()
#include "gears/base/common/thread_locals.h"

// Some of this code was borrowed from google3/thread/timedcall(.h|.cc)
// Some other parts were borrowed from timer/timer.cc
// This timer is designed as a shared timer, so there is only one OS timer
// per thread handling all the timer callbacks we are asked to handle
// in that thread.

// TODO(chimene) What if the user changes the system time or if an NTP
// sync happens?  gettimeofday() and CFAbsoluteTimeGetCurrent() are not
// monotonically increasing in those cases.

// Named according to style from base/common/thread_locals.h
static const ThreadLocals::Slot kTimerSingletonKey = ThreadLocals::Alloc();

class PlatformTimer;

// less-than operation on pointers to Timer
struct TimedCallPtrLessThan {
  bool operator()(const TimedCall *a, const TimedCall *b) const {
    if (a->deadline() != b->deadline()) {
      return a->deadline() < b->deadline();
    }
    return a < b;
  }
};

// TimerSingleton manages the queue of Timers that will fire,
// and is thread-local.
class TimerSingleton {
 public:
  static TimerSingleton *GetLocalSingleton();
  static void StaticCallback();

  void Insert(TimedCall *timer);
  void Erase(TimedCall *timer);

 private:
  void Callback();
  void RearmTimer();

  // deletes self, used when thread is destroyed
  static void DestructCallback(void *victim);

  TimerSingleton();
  ~TimerSingleton();

  std::set<TimedCall*, TimedCallPtrLessThan> *timer_queue_;
  PlatformTimer *platform_timer_;

  DISALLOW_EVIL_CONSTRUCTORS(TimerSingleton);
};


// PlatformTimer contains the platform-specific timer implementations,
// and has the method SetNextFire, which takes a timeout in milliseconds.

#if defined(OS_MACOSX)

// kGearsCustomMode puts this timer in its own category in the run loop
// so we can filter for it in the unit test. This is externed from there,
// so make sure to change the name in both places.
CFStringRef kGearsCustomMode = CFSTR("GearsCustomMode");

class PlatformTimer {
 public:
  PlatformTimer() {
    CFRunLoopAddCommonMode(CFRunLoopGetCurrent(), kGearsCustomMode);

    // We construct a timer that fires a long time from now
    // (3ish years in the future) and later change the fire date in SetNextFire.
    // See: developer.apple.com/documentation/CoreFoundation
    // /Reference/CFRunLoopTimerRef/Reference/reference.html
    // specifically, the discussion for CFRunLoopTimerSetNextFireDate.
    CFAbsoluteTime fire_date = CFAbsoluteTimeGetCurrent() + 100000000.0;
    CFTimeInterval interval = 100000000.0;

    timer_ = CFRunLoopTimerCreate(NULL, fire_date,
                                  interval, 0, 0,
                                  TimerCallback, NULL);
    assert(CFRunLoopTimerIsValid(timer_));
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer_, kGearsCustomMode);
  }

  ~PlatformTimer() {
    CFRunLoopTimerInvalidate(timer_);
    CFRelease(timer_);
  }

  void Cancel() {
    // Mac doesn't need cancel.
  }

  void SetNextFire(int64 timeout) {
    // CFAbsoluteTime is a double, specified in seconds.

    // TODO(chimene) is gettimeofday's version of time close enough to
    // CFAbsoluteTimeGetCurrent that we can use it?

    CFAbsoluteTime fireDate = CFAbsoluteTimeGetCurrent() + (1.0e-3 * timeout);
    CFRunLoopTimerSetNextFireDate(timer_, fireDate);
  }

  static void TimerCallback(CFRunLoopTimerRef ref, void *context) {
    TimerSingleton::StaticCallback();
  }

  CFRunLoopTimerRef timer_;
  DISALLOW_EVIL_CONSTRUCTORS(PlatformTimer);
};
#elif defined(WIN32)
class PlatformTimer {
 public:
  PlatformTimer()
      : timer_id_(0) {
  }

  ~PlatformTimer() {
    Cancel();
  }

  // Windows timers automatically fire again unless you KillTimer them,
  // or reset them with the same timer_id.
  void Cancel() {
    if (timer_id_ != 0) {
      KillTimer(NULL, timer_id_);
    }
    timer_id_ = 0;
  }

  void SetNextFire(int64 timeout) {
    // clamp timeout to USER_TIMER_MAXIMUM, because on Windows XP pre-SP2,
    // if it's larger than USER_TIMER_MAXIMUM it is changed to a time out of 1.
    // This would be bad.
    // USER_TIMER_MAXIMUM is approximately 24.8 days.  If we have a timer
    // that should go off after longer than this period, we will wake up after
    // the 24 days and just rearm the timer again.

    if (timeout > USER_TIMER_MAXIMUM)
      timeout = USER_TIMER_MAXIMUM;

    // Avoid negative timer delays doing weird things
    if (timeout < USER_TIMER_MINIMUM)
      timeout = USER_TIMER_MINIMUM;

    timer_id_ = SetTimer(NULL, timer_id_, (UINT)timeout, &OnTimer);

    if (timer_id_ == 0) {
      // There was an error
      // TODO(chimene) Log it instead of asserting
      assert(timer_id_ != 0);
    }
  }

 private:
  static void CALLBACK OnTimer(HWND wnd, UINT msg,
                               UINT_PTR event_id, DWORD time) {
    TimerSingleton::StaticCallback();
  }

  UINT_PTR timer_id_;
  DISALLOW_EVIL_CONSTRUCTORS(PlatformTimer);
};
#else  // TODO(chimene): Linux, wince, etc.
class PlatformTimer {
 public:
  PlatformTimer() {
    // Reminder that this doesn't work yet.
    assert(0);
  }
  ~PlatformTimer() {}
  void Cancel() {}
  void SetNextFire(int64 timeout) {}

 private:
  DISALLOW_EVIL_CONSTRUCTORS(PlatformTimer);
};
#endif

//
// TimerSingleton
//

void TimerSingleton::Insert(TimedCall *call) {
  timer_queue_->insert(call);
  RearmTimer();
}

void TimerSingleton::Erase(TimedCall *call) {
  timer_queue_->erase(call);
  RearmTimer();
}

TimerSingleton *TimerSingleton::GetLocalSingleton() {
  TimerSingleton *singleton = reinterpret_cast<TimerSingleton*>
    (ThreadLocals::GetValue(kTimerSingletonKey));

  if (singleton != NULL)
    return singleton;

  singleton = new TimerSingleton();
  ThreadLocals::SetValue(kTimerSingletonKey, singleton, &DestructCallback);

  return singleton;
}

void TimerSingleton::StaticCallback() {
  GetLocalSingleton()->Callback();
}

void TimerSingleton::Callback() {
  int64 now = GetCurrentTimeMillis();

  // Copy the timer queue so that callbacks can't add more TimedCalls
  // that fire immediately and get us stuck in this while loop.
  // If a callback adds an immediate callback, we'll schedule the
  // platform timer for 0 ms in the future, but we'll return to give the
  // message loop time to process other messages.
  std::set<TimedCall*, TimedCallPtrLessThan>
    current_queue(timer_queue_->begin(), timer_queue_->end());

  std::set<TimedCall*, TimedCallPtrLessThan>::iterator i =
    current_queue.begin();

  // Activate all timers that are past the deadline
  while (i != current_queue.end() && (*i)->deadline() <= now) {
    TimedCall *current_timer = (*i);
    timer_queue_->erase(current_timer);

    current_timer->Fire();

    ++i;
  }

  RearmTimer();
}

void TimerSingleton::RearmTimer() {
  if (timer_queue_->empty()) {
    platform_timer_->Cancel();
    return;
  }

  int64 next_deadline = (*timer_queue_->begin())->deadline();

  int64 now = GetCurrentTimeMillis();
  int64 next_fire = next_deadline - now;

  if (next_fire < 0) {
    next_fire = 0;
  }

  platform_timer_->SetNextFire(next_fire);
}

TimerSingleton::TimerSingleton() {
  timer_queue_ = new std::set<TimedCall*, TimedCallPtrLessThan>;
  platform_timer_ = new PlatformTimer();
}

void TimerSingleton::DestructCallback(void *victim) {
  TimerSingleton *timer_singleton_victim =
    reinterpret_cast<TimerSingleton*>(victim);
  delete timer_singleton_victim;
}

TimerSingleton::~TimerSingleton() {
  delete timer_queue_;
  timer_queue_ = NULL;
  delete platform_timer_;
  platform_timer_ = NULL;
}

//
// TimedCall
//
TimedCall::TimedCall(int64 delay_millis, bool repeat,
                     TimedCallback callback, void *callback_parameter)
    : delay_(delay_millis),
      repeat_(repeat),
      deadline_(GetCurrentTimeMillis() + delay_millis),
      callback_(callback),
      callback_parameter_(callback_parameter) {
  assert(callback_);
  TimerSingleton::GetLocalSingleton()->Insert(this);
}

void TimedCall::Fire() {
    // Re-schedule a repeating timer
    // Re-scheduling after the callback would be bad
    // because ~TimedCall() can be called inside the callback.
    if (repeat_) {
      int64 now = GetCurrentTimeMillis();
      deadline_ = now + delay_;
      TimerSingleton::GetLocalSingleton()->Insert(this);
    }

    // Callback is called here
    if (callback_ != NULL) {
      (*callback_)(callback_parameter_);
    }
}

void TimedCall::Cancel() {
  // Works even if we've already fired
  TimerSingleton::GetLocalSingleton()->Erase(this);
}

TimedCall::~TimedCall() {
  Cancel();
}
