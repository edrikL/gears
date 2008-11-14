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
//
// Wrapper for JNI headers.

#ifndef GEARS_BASE_ANDROID_JAVA_JNI_H__
#define GEARS_BASE_ANDROID_JAVA_JNI_H__

// This unfortunately also pollutes the namespace with #define LOG
#ifdef LOG
#  define LOG_DEFINITION_CLASH
#  undef LOG
#endif
// Include files from the Android tree.
#include <nativehelper/jni.h>
#include <nativehelper/JNIHelp.h>
// Don't use the JNIHelp.h version in any case.
#undef LOG
// Reinstate our version if we undefined it.
#ifdef LOG_DEFINITION_CLASH
#  undef LOG_DEFINITION_CLASH
#  ifdef DEBUG
#    define LOG(args) do { LOG_INNER args ; } while (0)
#  else
#    define LOG(args) do { } while (0)
#  endif
#endif

// Note that these functions require the TLS entry for kNPNFuncsKey on
// the main thread to have been initialized to point to a
// NPNetscapeFuncs structure.

// Get the Java JNI environment for the current thread. The current
// thread should be registered before this call.
JNIEnv *JniGetEnv();

// Register the current thread and associated JNIEnv as the main
// thread environment. Cannot be called again before
// JniUnregisterMainThread().
void JniRegisterMainThread(JNIEnv *env);

// Unregister the current thread as the main thread. This must be the
// same thread that was registered previously with
// JniRegisterMainThread(). Calls to JniGetEnv() on the main thread
// while unregistered will fail.
void JniUnregisterMainThread();

// Attach the current thread to the JVM. This cannot be the main
// thread. The main thread must have registered itself previously with
// JniRegisterMainThrad(). The environment will be automatically
// cleaned up on thread destruction if JniDetachCurrentThread() is not
// explicitly called. Returns the new environment.
JNIEnv *JniAttachCurrentThread();

// Explicitly detach the current thread from the JVM, deleting the
// environment and removing its entry from thread local storage.
void JniDetachCurrentThread();

// Explicity set the current thread's JNI environment. This is to be
// used when the thread has been created and is managed by the
// VM. Calling multiple times overwrites the old value. Use NULL to
// clear. This will not call JniDetachCurrentThread() on thread
// termination as that is the responsibility of the VM in this case.
void JniSetCurrentThread(JNIEnv *env);

// Simple attach/detach of the current thread for the lifetime of the
// instance.
class ScopedJniAttachCurrentThread {
 public:
  ScopedJniAttachCurrentThread() { JniAttachCurrentThread(); }
  ~ScopedJniAttachCurrentThread() { JniDetachCurrentThread(); }
};

#endif // GEARS_BASE_ANDROID_JAVA_CLASS_LOADER_H__
