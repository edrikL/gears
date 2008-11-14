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

#ifndef GEARS_BASE_ANDROID_JAVA_CLASS_H__
#define GEARS_BASE_ANDROID_JAVA_CLASS_H__

// The package name for Gears classes depends on whether they're baked
// into the Android tree or not. If we don't have access to a class
// loader, we have to use the Android tree package.
#if USING_CLASS_LOADER
#  define GEARS_JAVA_PACKAGE "com/google/android/gears"
#else
#  define GEARS_JAVA_PACKAGE "android/webkit/gears"
#endif

#include "gears/base/android/java_global_ref.h"
#include "gears/base/android/java_jni.h"
#include <string>

// JNI "jclass" wrapper, with helper functions for finding classes and
// their methods. Copy constructor and assignment are safe.
// Sample usage:
//   JavaClass clazz;
//   if (clazz.FindClass("java/lang/String")) {
//     jmethodID id = clazz.GetMethodID(JavaClass::kNonStatic,
//                                      "hashCode",
//                                      "()I");
//     if (id)
//       ...
//   }
class JavaClass {
 public:
  // The scope of the instance used in the call to a method. Use
  // kStatic for methods declared with the "static" keyword, and
  // kNonStatic for others, including constructors.
  typedef int MethodScope;
  static const MethodScope kNonStatic = 0;
  static const MethodScope kStatic = 1;

  // Structure used as an in-out parameter to
  // GetMultipleMethodIDs(). The scope, name and signature fields are
  // initialized by the caller. The id field is initialized by the
  // callee.
  struct Method {
    MethodScope scope;
    const char *name;
    const char *signature;
    jmethodID id;
  };

  // Construct with name "uninitialized" and no JNI class.
  JavaClass();

  // Construct with given name and JNI class.
  JavaClass(const char *name, jclass class_ref);

  // Copy constructor. The class members are copy-safe.
  JavaClass(const JavaClass &other)
      : class_name_(other.class_name_),
        class_ref_(other.class_ref_) { }

  // Assignment operator. The class members are assignment-safe.
  JavaClass &operator=(const JavaClass &rhs) {
    class_name_ = rhs.class_name_;
    class_ref_ = rhs.class_ref_;
    return *this;
  }

  // Return the contained jclass global reference if initialized, or
  // NULL otherwise.
  jclass Get() const { return class_ref_.Get(); }

  // Initializes name and JNI class by finding "name" in the global
  // namespace. Returns true on success.
  bool FindClass(const char *name);

  // Returns the method ID for a method in this class, or 0 if not found.
  jmethodID GetMethodID(MethodScope scope,
                        const char *name,
                        const char *signature) const;

  // Initializes multiple method IDs. The ID fields of the array are
  // assigned in-place. On failure, all IDs are guaranteed to be reset
  // to 0. Returns true on success.
  bool GetMultipleMethodIDs(Method *methods, int num_methods) const;

  // Register native methods. The class must be initialized prior to
  // this call. Returns true on success, false on failure.
  bool RegisterNativeMethods(const JNINativeMethod *methods,
                             int num_methods);

 private:
  // The name of the class if initialized, or "uninitialized" if not.
  std::string class_name_;
  // Global reference to the class if initialized, or NULL if not.
  JavaGlobalRef<jclass> class_ref_;
};

#endif // GEARS_BASE_ANDROID_JAVA_CLASS_H__
