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

// Work-around for unimplemented __aeabi_atexit and __cxa_atexit on Android.

#include "gears/base/common/common.h"
#include <pthread.h>

// Define to 1 for LOTS of debug output. Beware that LOG() may end up
// calling us recursively.
#define TRACE_ATEXIT    0

// Can't use std::list<> as we can't guarantee it won't go recursive
// back aeabi_atexit(). This is singly-linked list.
class AtExitNode {
 public:
  AtExitNode(AtExitNode *next, void *object, void (*destructor)(void *))
      : next_(next),
        object_(object),
        destructor_(destructor) {
#if TRACE_ATEXIT
    LOG(("Push %p %p\n", object, destructor));
#endif
  }
  ~AtExitNode() {
    // Call the destructor callback.
#if TRACE_ATEXIT
    LOG(("Pop %p %p\n", destructor_, object_));
#endif
    destructor_(object_);
  }
  AtExitNode *GetNext() { return next_; }

 private:
  // Next in list, or NULL if this is the tail.
  AtExitNode *next_;
  // The user data for the destructor.
  void *object_;
  // Destructor callback.
  void (*destructor_)(void *);
};

// Head of the linked list, or NULL if no entries.
static AtExitNode *head;

// Protects the linked list. Can't use class Mutex as it may have
// aeabi_atexit() side-effects.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// The linker flags "--wrap __aeabi_atexit" rewrites all calls to
// __aeabi_atexit to __wrap___aeabi_atexit instead, which we handle
// instead of the system library.
extern "C" int __wrap___aeabi_atexit(void *object,
                                     void (*destructor)(void *),
                                     void *dso_handle);
// This style of destructor *does* work and will be called by
// dlclose().
extern "C" void GearsAtExit() __attribute__((destructor));

int __wrap___aeabi_atexit(void *object,
                          void (*destructor)(void *),
                          void */*dso_handle*/) {
  // Atomically push this handler to the head of the list.
  pthread_mutex_lock(&mutex);
  head = new AtExitNode(head, object, destructor);
  pthread_mutex_unlock(&mutex);
  return 0;
}

void GearsAtExit() {
  // dlclose() has called us due the library being unloaded. Pop all
  // handlers in LIFO order.
  while (head != NULL) {
    AtExitNode *node = head;
    head = node->GetNext();
    // Delete this node. The destructor calls the callback.
    delete node;
  }
}
