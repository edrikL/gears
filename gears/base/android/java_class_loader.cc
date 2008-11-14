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
// Java class loader.

#include "gears/base/android/java_class_loader.h"
#include "gears/base/android/java_class.h"
#include "gears/base/android/java_exception_scope.h"
#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/android/java_string.h"
#include "gears/base/common/common.h"

JavaClassLoader::JavaClassLoader()
    : initialized_(false),
      dex_file_class_(),
      dex_file_constructor_(0),
      dex_file_load_class_(0),
      dex_file_object_() {
}

JavaClassLoader::~JavaClassLoader() {
  Close();
}

bool JavaClassLoader::Initialize() {
  if (initialized_)
    return true;
  // GetMethodID may throw exceptions. This clears them.
  JavaExceptionScope exception_scope;
  // Get class "DexFile".
  if (!dex_file_class_.FindClass("dalvik/system/DexFile")) {
    LOG(("Couldn't get DexFile class\n"));
    return false;
  }
  // Get method "DexFile::DexFile(String)".
  jmethodID dex_file_constructor = dex_file_class_.GetMethodID(
      JavaClass::kNonStatic,
      "<init>",
      "(Ljava/lang/String;)V");
  if (!dex_file_constructor) {
    LOG(("Couldn't get DexFile::DexFile(String)\n"));
    return false;
  }
  // Get method "Class DexFile::loadClass(String, ClassLoader)".
  jmethodID dex_file_load_class = dex_file_class_.GetMethodID(
      JavaClass::kNonStatic,
      "loadClass",
      "(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/Class;");
  if (!dex_file_load_class) {
    LOG(("Couldn't get DexFile::loadClass(String, ClassLoader)\n"));
    return false;
  }
  // Assign the method IDs we'll need for LoadClass().
  dex_file_constructor_ = dex_file_constructor;
  dex_file_load_class_ = dex_file_load_class;
  initialized_ = true;
  return true;
}

bool JavaClassLoader::Open(const char *path) {
  // Ensure the Java JNI IDs we need are initialized.
  if (!Initialize())
    return false;
  JNIEnv *env = JniGetEnv();
  // Construct DexFile object
  assert(dex_file_class_.Get());
  jobject new_object = NULL;
  {
    // NewObject may throw exceptions.
    JavaString path_jstring(path);
    JavaExceptionScope exception_scope;
    new_object = env->NewObject(dex_file_class_.Get(),
                                dex_file_constructor_,
                                path_jstring.Get());
    // Bug in Dalvik? NewObject is returning an initialized object as
    // well as an exception on error.
    if (JavaExceptionScope::Occurred() || new_object == NULL) {
      LOG(("new_object = %p, Couldn't open \"%s\"\n", new_object, path));
      return false;
    }
  }
  dex_file_object_.Reset(new_object);
  return dex_file_object_.Get();
}

void JavaClassLoader::Close() {
  // Remove the global reference and let the VM garbage collect it.
  dex_file_object_.Reset(0);
}

bool JavaClassLoader::LoadClass(const char *name, JavaClass *ret_class) {
  assert(ret_class != NULL);
  assert(initialized_);
  assert(dex_file_object_.Get());
  JNIEnv *env = JniGetEnv();
  // Clear any exceptions loadClass may generate.
  JavaExceptionScope exception_scope;
  // Clean up local references.
  JavaLocalFrame frame;
  // Call "DexFile::loadClass(name, null)".
  jobject loaded_class = env->CallObjectMethod(
      dex_file_object_.Get(),
      dex_file_load_class_,
      JavaString(name).Get(),
      0);
  if (!loaded_class) {
    // Class not found.
    return false;
  }
  // That returned a java/lang/Class object, but this can't easily be
  // converted to a jclass needed for JavaClass. Find it again.
  JavaClass found_class;
  if (!found_class.FindClass(name)) {
    LOG(("DexFile::loadClass(\"%s\", null) succeeded but FindClass did not!\n",
         name));
    return false;
  }
  // Success, we can assign to ret_class.
  *ret_class = found_class;
  return true;
}
