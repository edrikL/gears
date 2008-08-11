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

#include "gears/base/common/leak_counter.h"

#if ENABLE_LEAK_COUNTING

#include <assert.h>
#include <stdio.h>

#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/basictypes.h"

static AtomicWord leak_counter_counters[MAX_LEAK_COUNTER_TYPE];

static const char *leak_counter_names[] = {
  "DocumentJsRunner",
  "FFHttpRequest",
  "JavaScriptWorkerInfo",
  "JsArray",
  "JsCallContext",
  "JsContextWrapper",
  "JsEventMonitor",
  "JsObject",
  "JsRunner",
  "JsWrapperDataForFunction",
  "JsWrapperDataForInstance",
  "JsWrapperDataForProto",
  "ModuleImplBaseClass",
  "ModuleWrapper",
  "PoolThreadsManager",
  "ProgressInputStream",
  "SafeHttpRequest",
  NULL
};

void LeakCounterDumpCounts() {
  int total = 0;
  for (int i = 0; i < MAX_LEAK_COUNTER_TYPE; i++) {
    total += leak_counter_counters[i];
  }
  if (total == 0) {
    return;
  }
  printf("Gears is leaking memory. Known leaks include %d objects:\n", total);
  for (int i = 0; i < MAX_LEAK_COUNTER_TYPE; i++) {
    int count = leak_counter_counters[i];
    if (count == 0) {
      continue;
    }
    printf("  %6d  %s\n", count, leak_counter_names[i]);
  }
}

void LeakCounterIncrement(LeakCounterType type, int delta) {
  AtomicIncrement(&leak_counter_counters[type], delta);
}

void LeakCounterInitialize() {
  // The +1 is for the NULL value at the end of leak_counter_names.
  assert(MAX_LEAK_COUNTER_TYPE + 1 == ARRAYSIZE(leak_counter_names));
  for (int i = 0; i < MAX_LEAK_COUNTER_TYPE; i++) {
    leak_counter_counters[i] = 0;
  }
}

#endif  // ENABLE_LEAK_COUNTING
