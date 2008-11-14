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

#ifndef GEARS_BASE_ANDROID_JAVA_LOCAL_FRAME_H__
#define GEARS_BASE_ANDROID_JAVA_LOCAL_FRAME_H__

#include <assert.h>
#include "gears/base/android/java_jni.h"

// A scoped local reference cleaner. Objects returned by JNI are
// "local" references, which are automatically deleted when we return
// from a native method. However, if we're not a native method call,
// this doesn't work, and in many cases it is preferable to clean up
// more opportunistically. This class achieves that: on construction
// we "push" a stack frame with capacity for 16 objects, and on
// destruction we "pop" the frame, destroying any local references
// accumulated during the lifetime at this frame depth. There is also
// a convenience method "pop()" which allows a local reference to
// survive the frame being popped - useful for return values. Sample
// usage:
//   jobject MyFoo() {
//     JavaLocalFrame frame;
//     jobject bar = FancyJavaTricks();
//     return frame.pop(bar);
//   }
class JavaLocalFrame {
 public:
  // Push a stack frame at construction.
  JavaLocalFrame()
      : popped_(false) {
    int ret = JniGetEnv()->PushLocalFrame(kDefaultCapacity);
    (void) ret; // Inhibit unused variable warning.
    // The VM could in theory run out of memory.
    assert(ret >= 0);
  }
  // Pop a stack frame at destruction, deleting all local references
  // at this stack frame depth. Does not pop the frame is pop() was
  // already called.
  ~JavaLocalFrame() {
    if (!popped_)
      pop(NULL);
  }
  // Pop the frame and also carry across a result object to the parent frame.
  jobject pop(jobject result) {
    popped_=  true;
    return JniGetEnv()->PopLocalFrame(result);
  }

 private:
  // Set to true by pop() to prevent the destructor popping the frame
  // again.
  bool popped_;
  // Assume nobody uses more than 16 local references inside a frame.
  // Note: most VMs do not actually perform any checking.
  static const int kDefaultCapacity = 16;
};

#endif // GEARS_BASE_ANDROID_JAVA_LOCAL_FRAME_H__
