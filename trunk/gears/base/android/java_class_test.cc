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
// Tests for JNI "jclass" wrapper, JavaClass.

#ifdef USING_CCTESTS

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"

static bool TestJavaClass_HasValidReference(JavaClass *clazz,
                                            std::string16 *error) {
  if (clazz->Get() != NULL) {
    return true;
  } else {
    *error += UTF8ToString16(
        "FindClass on java/lang/String has NULL reference\n");
    return false;
  }
}

static bool TestJavaClass_GetsNonStaticMethod(JavaClass *clazz,
                                              std::string16 *error) {
  // int length();
  jmethodID id = clazz->GetMethodID(JavaClass::kNonStatic,
                                    "length",
                                    "()I");
  if (id != 0) {
    return true;
  } else {
    *error += UTF8ToString16(
        "Couldn't get method ID for int length()\n");
    return false;
  }
}

static bool TestJavaClass_FailsUnknownMethod(JavaClass *clazz,
                                             std::string16 *error) {
  // long longness();
  jmethodID id = clazz->GetMethodID(JavaClass::kNonStatic,
                                    "longness",
                                    "()J");
  if (id == 0) {
    return true;
  } else {
    *error += UTF8ToString16(
        "Incorrectly found method long longness()\n");
    return false;
  }
}

static bool TestJavaClass_GetsStaticMethod(JavaClass *clazz,
                                           std::string16 *error) {
  // static String valueOf(int);
  jmethodID id = clazz->GetMethodID(JavaClass::kStatic,
                                    "valueOf",
                                    "(I)Ljava/lang/String;");
  if (id != 0) {
    return true;
  } else {
    *error += UTF8ToString16(
        "Couldn't get method ID for static String valueOf(int)\n");
    return false;
  }
}

static bool TestJavaClass_GetsMultipleMethods(JavaClass *clazz,
                                              std::string16 *error) {
  JavaClass::Method methods[] = {
    { JavaClass::kNonStatic,
      "length",
      "()I" },
    { JavaClass::kNonStatic,
      "hashCode",
      "()I" },
    { JavaClass::kStatic,
      "valueOf",
      "(I)Ljava/lang/String;" },
  };
  if (clazz->GetMultipleMethodIDs(methods, NELEM(methods))) {
    // Check that all elements are non-zero id.
    for (int i = 0; i < NELEM(methods); ++i) {
      if (methods[i].id == 0) {
        *error += UTF8ToString16(
            "Some elements of successful multiple method get were 0\n");
        return false;
      }
    }
    return true;
  } else {
    *error += UTF8ToString16(
        "Couldn't get multiple method IDs in java.lang.String\n");
    return false;
  }
}

static bool TestJavaClass_ConstructsFromJclass(JavaClass *existing_clazz,
                                               std::string16 *error) {
  // Test construction from an existing jclass.
  JavaClass clazz("java/lang/String", existing_clazz->Get());
  if (clazz.Get() != NULL) {
    if (clazz.GetMethodID(JavaClass::kNonStatic,
                          "length",
                          "()I")) {
      return true;
    } else {
      *error += UTF8ToString16("Existing class couldn't get method length()\n");
      return false;
    }
  } else {
    *error += UTF8ToString16("Constructing from existing class failed\n");
    return false;
  }
}

static bool TestJavaClass_CopyConstructs(JavaClass *clazz,
                                         std::string16 *error) {
  JavaClass copied_clazz(*clazz);
  if (copied_clazz.Get() != NULL) {
    if (copied_clazz.GetMethodID(JavaClass::kNonStatic,
                                 "length",
                                 "()I")) {
      return true;
    } else {
      *error += UTF8ToString16("Copied class couldn't get method length()\n");
      return false;
    }
  } else {
    *error += UTF8ToString16("Copied class has NULL reference\n");
    return false;
  }
}

static bool TestJavaClass_Assigns(JavaClass *clazz,
                                  std::string16 *error) {
  JavaClass assigned_clazz;
  assigned_clazz = *clazz;
  if (assigned_clazz.Get() != NULL) {
    if (assigned_clazz.GetMethodID(JavaClass::kNonStatic,
                                   "length",
                                   "()I")) {
      return true;
    } else {
      *error += UTF8ToString16("Assigned class couldn't get method length()\n");
      return false;
    }
  } else {
    *error += UTF8ToString16("Assigned class has NULL reference\n");
    return false;
  }
}

bool TestJavaClass(std::string16 *error) {
  // All tests rely on at least FindClass() working.
  JavaClass clazz;
  if (!clazz.FindClass("java/lang/String")) {
    const char *msg = "FindClass on java/lang/String failed\n";
    LOG((msg));
    *error += UTF8ToString16(msg);
    return false;
  }
  bool ok = true;
  std::string16 local_error;
  ok &= TestJavaClass_HasValidReference(&clazz, &local_error);
  ok &= TestJavaClass_GetsNonStaticMethod(&clazz, &local_error);
  ok &= TestJavaClass_FailsUnknownMethod(&clazz, &local_error);
  ok &= TestJavaClass_GetsStaticMethod(&clazz, &local_error);
  ok &= TestJavaClass_GetsMultipleMethods(&clazz, &local_error);
  ok &= TestJavaClass_ConstructsFromJclass(&clazz, &local_error);
  ok &= TestJavaClass_CopyConstructs(&clazz, &local_error);
  ok &= TestJavaClass_Assigns(&clazz, &local_error);
  if (ok) {
    LOG(("TestJavaClass succeeded\n"));
    return true;
  } else {
    LOG((String16ToUTF8(local_error).c_str()));
    *error += local_error;
    return false;
  }
}

#endif  // USING_CCTESTS
