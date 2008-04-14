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

// This file is used by both Win32 and WinCE.
// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented
#ifdef WIN32

#include "gears/base/common/stopwatch.h"
#include <assert.h>
#include <windows.h>
#ifdef WINCE
#include "gears/base/common/wince_compatibility.h"
#endif

// Returns the current time in milliseconds since the epoch (midnight January 1,
// 1970 GMT).
int64 GetCurrentTimeMillis() {
  // The FILETIME structure is a 64-bit value representing the number of
  // 100-nanosecond intervals since January 1, 1601 (UTC). We offset to our
  // epoch (January 1, 1970 GMT) and convert to milliseconds.
  const int64 kOffset = 116444736000000000LL;
  FILETIME file_time;
  GetSystemTimeAsFileTime(&file_time);
  int64 file_time_int64 = (static_cast<int64>(file_time.dwHighDateTime) << 32) |
                          (static_cast<int64>(file_time.dwLowDateTime));
  return (file_time_int64 - kOffset) / 10000;
}

// Returns a monotonically incrementing counter.
int64 GetTicks() {
  LARGE_INTEGER ticks;
  BOOL result = QueryPerformanceCounter(&ticks);
  return result == 0 ? 0 : ticks.QuadPart;
}

// Returns the number of microseconds elapsed between the start and end tick
// counts.
int64 GetTickDeltaMicros(int64 start, int64 end) {
  static int64 ticks_per_second = 0;
  if (ticks_per_second == 0) {
    LARGE_INTEGER tick_freq;
    if (QueryPerformanceFrequency(&tick_freq) != 0) {
      ticks_per_second = tick_freq.QuadPart;
    }
  }
  return ticks_per_second == 0 ? 0 : (end - start) * 1000000 / ticks_per_second;
}

#endif
