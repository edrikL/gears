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

#ifdef USING_CCTESTS

#include "gears/base/common/timed_call_test.h"

#include <assert.h>


#if defined(OS_MACOSX)
#include <CoreFoundation/CoreFoundation.h>
// extern from timed_call.cc
extern CFStringRef kGearsCustomMode;
#endif

#include "gears/base/common/common.h"
#include "gears/base/common/stopwatch.h"
#include "gears/base/common/timed_call.h"

static bool TestCallbackSanity();
static bool TestCallbackSorting();
static bool TestCallbackRepeat();

static Stopwatch *callback_watch;

static bool callback_test_toggle = false;
const char kCallbackArgTest[] = "CallbackStringSucceeded";

static int callback_sorting_counter = 0;
const char kCallbackArg1[] = "CallbackArg1";
const char kCallbackArg2[] = "CallbackArg2";
const char kCallbackArg3[] = "CallbackArg3";

static int callback_repeat_counter = 0;
const char kCallbackArgRepeat[] = "CallbackArgRepeat";

//
// Timed Callback Test Helper Functions
//

// Convenience function for getting the stopwatch time
int StopwatchGetElapsed(Stopwatch *watch) {
  watch->Stop();
  int time = watch->GetElapsed();
  watch->Start();
  return time;
}

static void CallbackTestCallback(void *arg) {
  LOG(("CallbackTestCallback - %s, elapsed: %d\n", arg,
      StopwatchGetElapsed(callback_watch) ));
  if (arg != kCallbackArgTest) {
    LOG(("CallbackTestCallback - failed: arg is wrong"));
    return;
  }
  callback_test_toggle = true;
}

static void CallbackTestCallback1(void *arg) {
  LOG(("CallbackTestCallback1 - %s, elapsed: %d\n", arg,
      StopwatchGetElapsed(callback_watch) ));
  if (arg != kCallbackArg1) {
    LOG(("CallbackTestCallback1 - failed: arg is wrong"));
    return;
  }
  if (callback_sorting_counter != 0) {
    LOG(("CallbackTestCallback1 - failed: counter is %d, should be 0",
      callback_sorting_counter));
  }
  callback_sorting_counter = 1;
}

static void CallbackTestCallback2(void *arg) {
  LOG(("CallbackTestCallback2 - %s, elapsed: %d\n", arg,
      StopwatchGetElapsed(callback_watch) ));
  if (arg != kCallbackArg2) {
    LOG(("CallbackTestCallback2 - failed: arg is wrong"));
    return;
  }
  if (callback_sorting_counter != 1) {
    LOG(("CallbackTestCallback2 - failed: counter is %d, should be 1",
      callback_sorting_counter));
  }
  callback_sorting_counter = 2;
}

static void CallbackTestCallback3(void *arg) {
  LOG(("CallbackTestCallback3 - %s, elapsed: %d\n", arg,
      StopwatchGetElapsed(callback_watch) ));
  if (arg != kCallbackArg3) {
    LOG(("CallbackTestCallback3 - failed: arg is wrong"));
    return;
  }
  if (callback_sorting_counter != 2) {
    LOG(("CallbackTestCallback3 - failed: counter is %d, should be 2",
      callback_sorting_counter));
  }
  callback_sorting_counter = 3;
}

static void CallbackTestCallbackRepeat(void *arg) {
  LOG(("CallbackTestCallbackRepeat - %s, elapsed: %d, repeat %d\n", arg,
      StopwatchGetElapsed(callback_watch), callback_repeat_counter));
  if (arg != kCallbackArgRepeat) {
    LOG(("CallbackTestCallbackRepeat - failed: arg is wrong"));
    return;
  }
  callback_repeat_counter++;
}

//
// TestTimedCallbackAll
//
bool TestTimedCallbackAll(std::string16 *error) {
  bool ok = true;
  ok &= TestCallbackSanity();
  ok &= TestCallbackSorting();
  ok &= TestCallbackRepeat();
  // TODO(chimene): Add tests for things like:
  // delete timer inside a callback
  // destroy thread after setting a timer inside it
  // calling Fire()
  // etc..
  if (!ok) {
    assert(error); \
    *error += STRING16(L"TestTimedCallbackAll - failed. "); \
  }
  return ok;
}

//
// TestCallbackSanity
//
static bool TestCallbackSanity() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestCallbackSanity - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}
  LOG(("TestCallbackSanity - started\n"));
  callback_test_toggle = false;
  callback_watch = new Stopwatch();
  callback_watch->Start();

  LOG(("TestCallbackSanity - timer\n"));
  TimedCall timer(2000, false, &CallbackTestCallback,
    reinterpret_cast<void*>(const_cast<char*>(kCallbackArgTest)));

  // Allow the message loop to deal with incoming messages,
  // so the timer isn't stifled by the thread being busy
  // sleep/yield does not work - you have to go back to the message loop
  // Give it 3 seconds to callback, should fire in 2
  LOG(("TestCallbackSanity - wait\n"));

#if defined(OS_MACOSX)
  int rval;
  while (!callback_test_toggle && StopwatchGetElapsed(callback_watch) < 3000) {
    rval = CFRunLoopRunInMode(kGearsCustomMode, 3.0, false);
    LOG(("TestCallbackSanity - RunInMode rval: %d\n", rval));
    if (rval == kCFRunLoopRunFinished)  // No timers to wait on
      break;
  }

#elif defined(WIN32)
  MSG msg;
  while (!callback_test_toggle && StopwatchGetElapsed(callback_watch) < 3000) {
    Sleep(1);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
#endif
  callback_watch->Stop();
  LOG(("TestCallbackSanity - elapsed:%d", callback_watch->GetElapsed()));

  TEST_ASSERT(callback_test_toggle == true);

  delete callback_watch;
  callback_watch = NULL;

  LOG(("TestCallbackSanity - passed\n"));
  return true;
}

//
// TestCallbackSorting
//
static bool TestCallbackSorting() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestCallbackSorting - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}
  LOG(("TestCallbackSorting - started\n"));
  callback_sorting_counter = 0;
  callback_watch = new Stopwatch();
  callback_watch->Start();

  // Set timers out of order, they should be fired in order by timeout
  TimedCall timer_a(2000, false, &CallbackTestCallback3,
    reinterpret_cast<void*>(const_cast<char*>(kCallbackArg3)));
  TimedCall timer_b(1000, false, &CallbackTestCallback1,
    reinterpret_cast<void*>(const_cast<char*>(kCallbackArg1)));
  TimedCall timer_c(1500, false, &CallbackTestCallback2,
    reinterpret_cast<void*>(const_cast<char*>(kCallbackArg2)));

  LOG(("TestCallbackSorting - wait\n"));
#if defined(OS_MACOSX)
  int rval;
  while (callback_sorting_counter != 3 &&
         StopwatchGetElapsed(callback_watch) < 3000) {
    rval = CFRunLoopRunInMode(kGearsCustomMode, 3.0, false);
    LOG(("TestCallbackSorting - RunInMode rval: %d\n", rval));
    if (rval == kCFRunLoopRunFinished)  // No timers to wait on
      break;
  }
#elif defined(WIN32)
  MSG msg;
  while (callback_sorting_counter != 3 &&
         StopwatchGetElapsed(callback_watch) < 3000) {
    Sleep(1);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
#endif
  callback_watch->Stop();
  LOG(("TestCallbackSorting - elapsed:%d", callback_watch->GetElapsed()));

  TEST_ASSERT(callback_sorting_counter == 3);

  delete callback_watch;
  callback_watch = NULL;

  LOG(("TestCallbackSorting - passed\n"));
  return true;
}

//
// TestCallbackRepeat
//
static bool TestCallbackRepeat() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestCallbackRepeat - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}
  LOG(("TestCallbackRepeat - started\n"));
  callback_repeat_counter = 0;
  callback_watch = new Stopwatch();
  callback_watch->Start();

  // Repeat every 500 ms from now
  // Repeats are scheduled right before the timer fires.
  // Should fire at 500, 1000, 1500, 2000, 2500, 3000 (6 times)
  TimedCall timer(500, true, &CallbackTestCallbackRepeat,
    reinterpret_cast<void*>(const_cast<char*>(kCallbackArgRepeat)));

  LOG(("TestCallbackRepeat - wait\n"));
#if defined(OS_MACOSX)
  int rval;
  while (callback_repeat_counter <= 5 &&
         StopwatchGetElapsed(callback_watch) < 3000) {
    rval = CFRunLoopRunInMode(kGearsCustomMode, 3.0, false);
    LOG(("TestCallbackRepeat - RunInMode rval: %d\n", rval));
    if (rval == kCFRunLoopRunFinished)  // No timers to wait on
      break;
  }
#elif defined(WIN32)
  MSG msg;
  while (callback_repeat_counter <= 5 &&
         StopwatchGetElapsed(callback_watch) < 3000) {
    Sleep(1);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
#endif
  callback_watch->Stop();
  LOG(("TestCallbackRepeat - elapsed:%d", callback_watch->GetElapsed()));

  // Must repeat at least 5 times, the exact number is dependent on anyone else
  // who triggers the event loop.
  TEST_ASSERT(callback_repeat_counter >= 5);

  delete callback_watch;
  callback_watch = NULL;

  LOG(("TestCallbackRepeat - passed\n"));
  return true;
}

#endif  // USING_CCTESTS
