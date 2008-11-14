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
// JNI "jclass" wrapper with helper functions.

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_exception_scope.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/android/java_local_frame.h"
#include "gears/base/common/common.h"

JavaClass::JavaClass()
    : class_name_("uninitialized"),
      class_ref_() {
}

JavaClass::JavaClass(const char *name, jclass class_ref)
    : class_name_(name),
      class_ref_(class_ref) {
}

bool JavaClass::FindClass(const char *name) {
  JNIEnv *env = JniGetEnv();
  // Clean up exceptions, local references.
  JavaExceptionScope exception_scope;
  JavaLocalFrame frame;
  // Get the named class, e.g "java/lang/String".
  jclass new_class = env->FindClass(name);
  if (!new_class) {
    LOG(("Couldn't find class \"%s\"\n", name));
    return false;
  }
  // Retain a global reference to the class we just found.
  class_ref_.Reset(new_class);
  class_name_ = name;
  // Replace '/' with '.' in the class name.
  for (std::string::size_type i = 0; i < class_name_.size(); ++i) {
    if (class_name_[i] == '/') {
      class_name_[i] = '.';
    }
  }
  return true;
}

jmethodID JavaClass::GetMethodID(MethodScope scope,
                                 const char *name,
                                 const char *signature) const {
  // Class must be initialized.
  assert(class_ref_.Get() != NULL);
  JNIEnv *env = JniGetEnv();
  // Clean up exceptions.
  JavaExceptionScope exception_scope;
  // Get the ID of the member belonging to this class.
  jmethodID id;
  if (scope == kStatic) {
    id = env->GetStaticMethodID(class_ref_.Get(), name, signature);
  } else {
    id = env->GetMethodID(class_ref_.Get(), name, signature);
  }
  if (!id) {
    LOG(("Couldn't find %s method %s.%s\n",
         scope == kStatic ? "static" : "non-static",
         class_name_.c_str(),
         name));
  }
  return id;
}

bool JavaClass::GetMultipleMethodIDs(Method *methods, int num_methods) const {
  for (int i = 0; i < num_methods; ++i) {
    // Get the method ID of the current method in the initializer array
    methods[i].id = GetMethodID(methods[i].scope,
                                methods[i].name,
                                methods[i].signature);
    if (!methods[i].id) {
      // Not found. Reset all method IDs so far and return a failure.
      for (int clear = 0; clear < i; ++clear) {
        methods[clear].id = 0;
      }
      return false;
    }
  }
  return true;
}

bool JavaClass::RegisterNativeMethods(const JNINativeMethod *methods,
                                      int num_methods) {
  // Class must be initialized.
  assert(class_ref_.Get() != NULL);
  // Clean up exceptions.
  JavaExceptionScope exception_scope;
  if (JniGetEnv()->RegisterNatives(class_ref_.Get(),
                                   methods,
                                   num_methods) == 0) {
    return true;
  } else {
    LOG(("Couldn't register native methods on class %s\n",
         class_name_.c_str()));
    return false;
  }
}
