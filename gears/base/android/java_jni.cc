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

#include "gears/base/android/java_jni.h"
#include "gears/base/common/thread_locals.h"
#include "gears/base/npapi/module.h"
#include "third_party/npapi/nphostapi.h"
#include <assert.h>
#include <pthread.h>

// TLS key for each JNIEnv* entry. Initialized on the main thread by
// JniRegisterMainThread(), and by JniAttachCurrentThread() on others.
static const ThreadLocalValue<JNIEnv *> s_jni_env_tls;
// The ID of the main thread, initialized by JniRegisterMainThread().
static pthread_t s_main_thread_id = 0;
// Pointer to the browser's main thread queuing function. This is an
// NPAPI call which we'll store here separately to the NPN structure
// as this is a special case that is available on all
// threads. Initialized by JniRegisterMainThread().
static NPN_PluginThreadAsyncCallProcPtr s_plugin_thread_async_call;
// Pointer to the browser's JVM. Initialized by
// JniRegisterMainThread(). Needed to create new environments for
// other threads.
static JavaVM *s_jvm;

static void JniEnvTlsCallback(void *env) {
  // Called on thread destruction if JniDetachCurrentThread has not
  // already been explicitly called. This should clean it up.
  LOG(("Detach thread %ld from JNIEnv %p\n",
       static_cast<long>(pthread_self()),
       env));
  if (s_jvm->DetachCurrentThread()) {
    LOG(("Couldn't detach JNIEnv %p\n", env));
    assert(false);
  }
}

JNIEnv *JniGetEnv() {
  // The main thread will have a TLS entry from startup calling
  // JniRegisterMainThread. Other threads should have called
  // JniAttachCurrentThread first.
  JNIEnv *env = s_jni_env_tls.GetValue();
  assert(env != NULL);
  return env;
}

void JniRegisterMainThread(JNIEnv *env) {
  // Register the current thread as the main thread.
  assert(s_main_thread_id == 0);
  s_main_thread_id = pthread_self();
  // Set in TLS. Future JniGetEnv() calls will pick up this
  // JNIEnv. Does not register a destruction clean up as that will
  // kill the browser's environment.
  assert(env != NULL);
  s_jni_env_tls.SetValue(env, NULL);
  // Store a pointer to NPN_PluginThreadAsyncCall so other threads can
  // access it.
  NPNetscapeFuncs *npn_funcs = reinterpret_cast<NPNetscapeFuncs *>(
      ThreadLocals::GetValue(kNPNFuncsKey));
  assert(npn_funcs != NULL);
  assert(npn_funcs->pluginthreadasynccall != NULL);
  assert(s_plugin_thread_async_call == NULL);
  s_plugin_thread_async_call = npn_funcs->pluginthreadasynccall;
  // Get the JVM that this environment is attached to. We'll need this
  // to attach other threads to.
  assert(s_jvm == NULL);
  if (env->GetJavaVM(&s_jvm)) {
    LOG(("Couldn't get JVM!\n"));
  }
  LOG(("JVM is %p\n", s_jvm));
  assert(s_jvm != NULL);
}

void JniUnregisterMainThread() {
  // Remove the TLS entry for the main thread.
  assert(pthread_self() == s_main_thread_id);
  s_jni_env_tls.DestroyValue();
  s_main_thread_id = 0;
  s_plugin_thread_async_call = NULL;
  s_jvm = NULL;
}

JNIEnv *JniAttachCurrentThread() {
  JNIEnv *env;
#ifdef DEBUG
  // Check we haven't already registered this thread, and that it's
  // not the main thread.
  assert(pthread_self() != s_main_thread_id);
  env = s_jni_env_tls.GetValue();
  assert(env == NULL);
#endif
  // Each new thread needs registering with the JVM so it can have a
  // separate environment to access it with.
  assert(s_jvm != NULL);
  if (s_jvm->AttachCurrentThread(&env, NULL)) {
    LOG(("Couldn't attach a new thread to the JVM!\n"));
    return NULL;
  }
  assert(env != NULL);
  // Set the TLS entry. Register a clean up handler for thread
  // destruction if not explicity performed.
  s_jni_env_tls.SetValue(env, JniEnvTlsCallback);  
  LOG(("Attached thread %ld to JNIEnv %p\n",
       static_cast<long>(pthread_self()),
       env));
  return env;
}

void JniDetachCurrentThread() {
  // Detach this thread from the JVM.
  assert(pthread_self() != s_main_thread_id);
  assert(s_jni_env_tls.HasValue());
  // Removing the TLS entry for this thread will cause the clean up
  // callback to be invoked with the old value.
  s_jni_env_tls.DestroyValue();
}

void JniSetCurrentThread(JNIEnv *env) {
  if (env != NULL) {
    s_jni_env_tls.SetValue(env, NULL);
  } else {
    s_jni_env_tls.DestroyValue();
  }
}

// Place an asynchronous callback on the browser's main thread
// queue. This is used to perform operations that are only safe on the
// main thread. The function is called with user_data as its only
// argument. The function is normally a stub which casts user_data to
// a class and calls a method.
// This call succeeds even if the current thread is the main
// thread. The main thread must already have been initialized with
// JniRegisterMainThread(), and the current thread (if not the main
// thread) must have been attached with JniAttachCurrentThread().
// TODO(jripley): This would be better in npn_bindings.cc, but we'd need
// to expose a few symbols.
void NPN_PluginThreadAsyncCall(NPP id, void (*func)(void *), void *user_data) {
  // This NPN call is safe regardless of which thread we call it
  // on. Note that in the case of the Android browser, the calling
  // thread must have registered itself with the JVM first.
#ifdef DEBUG
  assert(s_plugin_thread_async_call != NULL);
  assert(s_main_thread_id != 0);
  assert(s_jni_env_tls.HasValue());
#endif
  // The Android browser implementation does not require the NPP
  // argument to be non-NULL.
  s_plugin_thread_async_call(NULL, func, user_data);
}
