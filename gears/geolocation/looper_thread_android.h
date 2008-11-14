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

#ifndef GEARS_GEOLOCATION_LOOPER_THREAD_ANDROID_H__
#define GEARS_GEOLOCATION_LOOPER_THREAD_ANDROID_H__

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_global_ref.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/android/java_string.h"
#include "gears/base/common/thread.h"

// A type of Thread that uses the Android Java Looper
// to implement its message queue. For more details, please see
// http://code.google.com/android/reference/android/os/Looper.html.
class AndroidLooperThread : public Thread {
 public:
  virtual ~AndroidLooperThread();

 protected:
  // This class is not intended for direct instantiation.
  AndroidLooperThread();
  // Stops the thread by calling Looper::Quit();
  void Stop();
  // Classes deriving from AndroidLooperThread can override this
  // method if they need to perform any special steps before the
  // message looping is started.
  virtual void BeforeLoop() {};
  // Classes deriving from AndroidLooperThread can override this
  // method if they need to perform any special steps after the
  // message looping is terminated.
  virtual void AfterLoop() {};
 private:
  // Thread
  virtual void Run();

  // The looper class.
  JavaClass looper_class_;
  // Global reference to the looper object.
  JavaGlobalRef<jobject> looper_java_object_;
  Event looper_created_event_;

  DISALLOW_EVIL_CONSTRUCTORS(AndroidLooperThread);
};

#endif  // GEARS_GEOLOCATION_LOOPER_THREAD_ANDROID_H__
