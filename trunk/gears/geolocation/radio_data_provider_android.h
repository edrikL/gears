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

#ifndef GEARS_GEOLOCATION_RADIO_DATA_PROVIDER_ANDROID_H__
#define GEARS_GEOLOCATION_RADIO_DATA_PROVIDER_ANDROID_H__

#include "gears/geolocation/device_data_provider.h"
#include "gears/geolocation/looper_thread_android.h"

// Implements the Radio provider on Android. On this platform,
// the following radio data is available:
// - Cell ID
// - Location area code
// - Mobile country code
// - Mobile network code
// - Signal strength (dBm)
// - Carrier name
class AndroidRadioDataProvider : public RadioDataProviderImplBase,
                                 public AndroidLooperThread {
 public:
  AndroidRadioDataProvider();
  virtual ~AndroidRadioDataProvider();

  // RadioDataProviderImplBase
  virtual bool GetData(RadioData* data);

 private:
  // Native callback, invoked when there is new radio data.
  static void OnUpdateAvailable(JNIEnv* env,
                                jclass cls,
                                jobject radio_data,
                                jlong self);
  // Inspects the new_radio_data and decides when to notify
  // the listeners.
  void NewRadioDataAvailable(RadioData* new_radio_data);

  // AndroidLooperThread
  virtual void BeforeLoop();
  virtual void AfterLoop();

  // Container for the Java class counterpart.
  JavaClass provider_java_class_;
  // Global reference to the AndroidDataProvider instance.
  JavaGlobalRef<jobject> provider_java_object_;
  // The webview instance.
  JavaGlobalRef<jobject> webview_;
  // The signature of our native callback method.
  static JNINativeMethod native_methods_[];

  RadioData radio_data_;
  // Mutex that protects the access to the radio_data_.
  Mutex data_mutex_;

  DISALLOW_EVIL_CONSTRUCTORS(AndroidRadioDataProvider);
};

#endif  // GEARS_GEOLOCATION_RADIO_DATA_PROVIDER_ANDROID_H__
