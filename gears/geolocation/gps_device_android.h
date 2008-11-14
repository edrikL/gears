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

#ifndef GEARS_GEOLOCATION_GPS_DEVICE_ANDROID_H__
#define GEARS_GEOLOCATION_GPS_DEVICE_ANDROID_H__

#include "gears/base/android/java_string.h"
#include "gears/geolocation/geolocation.h"
#include "gears/geolocation/gps_location_provider.h"
#include "gears/geolocation/looper_thread_android.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

// Implements the GPS device on Android.

class AndroidGpsDevice
    : public GpsDeviceBase,
      public AndroidLooperThread {
 public:
  friend GpsDeviceBase *GpsLocationProvider::NewGpsDevice(
      GpsDeviceBase::ListenerInterface *listener);

 private:
  AndroidGpsDevice(GpsDeviceBase::ListenerInterface *listener);
  virtual ~AndroidGpsDevice();

  // AndroidLooperThread
  virtual void BeforeLoop();
  virtual void AfterLoop();

  // Native callback, invoked when there is a new location available.
  static void OnLocationChanged(JNIEnv *env,
                                jclass cls,
                                jobject location,
                                jlong self);

  // Native callback, invoked when there is a provider error.
  static void OnProviderError(JNIEnv *env,
                              jclass cls,
                              jboolean is_disabled,
                              jlong self);

  void PositionChanged(const Position &new_position);

  void ProviderError(bool is_disabled);

  // Container for the Java class counterpart.
  JavaClass provider_java_class_;
  // Global reference to the Java instance.
  JavaGlobalRef<jobject> provider_java_object_;
  // The webview instance.
  JavaGlobalRef<jobject> webview_;
  // The signature of our native callback methods.
  static JNINativeMethod native_methods_[];
  // The current best position estimate.
  Position position_;

  DISALLOW_EVIL_CONSTRUCTORS(AndroidGpsDevice);
};

#endif  // GEARS_GEOLOCATION_GPS_DEVICE_ANDROID_H__
