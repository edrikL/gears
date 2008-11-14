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

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented.
#ifdef OS_ANDROID

#include "gears/geolocation/looper_thread_android.h"

#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_exception_scope.h"

static const char* kLooperClass = "android/os/Looper";

static JavaClass::Method looper_methods[] = {
  { JavaClass::kStatic, "prepare", "()V"},
  { JavaClass::kStatic, "loop", "()V"},
  { JavaClass::kStatic, "myLooper", "()Landroid/os/Looper;"},
  { JavaClass::kNonStatic, "quit", "()V"},
};

enum LooperMethodID {
  METHOD_ID_PREPARE = 0,
  METHOD_ID_LOOP,
  METHOD_ID_MYLOOPER,
  METHOD_ID_QUIT,
};

AndroidLooperThread::AndroidLooperThread() {
  JavaLocalFrame frame;
  // Load the Looper class.
  if (!looper_class_.FindClass(kLooperClass)) {
    LOG(("Could not find the Looper class.\n"));
    assert(false);
    return;
  }

  // Get the looper method IDs.
  if (!looper_class_.GetMultipleMethodIDs(looper_methods,
                                          NELEM(looper_methods))) {
    LOG(("Could not find the Looper methods.\n"));
    assert(false);
    return;
  }
}

AndroidLooperThread::~AndroidLooperThread() {
  assert(!IsRunning());
}

void AndroidLooperThread::Stop() {
  // The thread will signal this event once the looper
  // instance was obtained.
  looper_created_event_.Wait();
  assert(looper_java_object_.Get());
  JniGetEnv()->CallVoidMethod(looper_java_object_.Get(),
                              looper_methods[METHOD_ID_QUIT].id);
  Join();
}

void AndroidLooperThread::Run() {
  JavaExceptionScope scope;
  JavaLocalFrame frame;
  // The provider thread is now running. We need to do things in the
  // following order:
  // 1. Prepare the looper
  // 2. Get the looper object for this thread.
  // 3. Invoke BeforeLoop().
  // 4. Start the message loop.
  // 5. Invoke AfterLoop().

  // Step 1: Prepare the looper.
  JniGetEnv()->CallStaticVoidMethod(looper_class_.Get(),
                                    looper_methods[METHOD_ID_PREPARE].id);

  // Step 2: Get the looper object.
  jobject looper = JniGetEnv()->CallStaticObjectMethod(
      looper_class_.Get(),
      looper_methods[METHOD_ID_MYLOOPER].id);
  assert(looper);
  looper_java_object_.MoveLocal(looper);
  looper_created_event_.Signal();

  // Step 3: Invoke BeforeLoop();
  BeforeLoop();

  // Step 4: Start the message loop. This will block until
  // Looper::Quit() is called in the AndroidLooperThread destructor.
  JniGetEnv()->CallStaticVoidMethod(looper_class_.Get(),
                                    looper_methods[METHOD_ID_LOOP].id);

  // Step 5: Invoke AfterLoop().
  AfterLoop();
}

#endif  // OS_ANDROID
