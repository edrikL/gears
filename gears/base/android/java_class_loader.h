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

#ifndef GEARS_BASE_ANDROID_JAVA_CLASS_LOADER_H__
#define GEARS_BASE_ANDROID_JAVA_CLASS_LOADER_H__

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_global_ref.h"
#include "gears/base/android/java_jni.h"

// Java class loader. This loads classes from a .jar or .dex file. If
// using a .jar archive, it must have a "classes.dex" file containing
// all of the classes.
// Sample usage:
//   JavaClassLoader loader;
//   if (loader.Open("/foo/bar/classes.jar")) {
//     JavaClass clazz;
//     if (loader.LoadClass("google/gears/MyClass", &clazz)) {
//       jmethodID id = clazz.GetMethodID(...);
//       ...
//     }
//   }
class JavaClassLoader {
 public:
  JavaClassLoader();
  ~JavaClassLoader();

  // Open a .jar or .dex class file. Returns true on success.
  bool Open(const char *path);
  // Close the class file. Safe to call multiple times. Also called
  // automatically when destructed.
  void Close();
  // Load a class from the currently open class file. Returns true if
  // the class is found, and initializes *ret_class. Returns false if
  // not found, and does not change *ret_class. ret_class must not be
  // NULL.
  bool LoadClass(const char *name, JavaClass *ret_class);

 private:
  // Copy constructor and assignment are actually safe, but aren't
  // cheap or useful.
  DISALLOW_EVIL_CONSTRUCTORS(JavaClassLoader);

  // Lazy construction, performed by Open(path). Initializes
  // the members, below. Returns true on success.
  bool Initialize();

  // True if Initialize() has been called and succeeded.
  bool initialized_;
  // Container for class DexFile
  JavaClass dex_file_class_;
  // ID of public DexFile::DexFile(String)
  jmethodID dex_file_constructor_;
  // ID of public Class DexFile::loadClass(String, ClassLoader)
  jmethodID dex_file_load_class_;
  // Reference to an instance of DexFile if open, or NULL.
  JavaGlobalRef<jobject> dex_file_object_;
};

#endif // GEARS_BASE_ANDROID_JAVA_CLASS_LOADER_H__
