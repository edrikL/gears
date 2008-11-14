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

#ifndef GEARS_GEOLOCATION_WIFI_DATA_PROVIDER_ANDROID_H__
#define GEARS_GEOLOCATION_WIFI_DATA_PROVIDER_ANDROID_H__

#include "gears/geolocation/device_data_provider.h"
#include "gears/geolocation/looper_thread_android.h"

// Implements the Wifi provider on Android.
// TODO(andreip): refactor the data providers as there is quite a bit
// of code replication currently.
class AndroidWifiDataProvider : public WifiDataProviderImplBase,
                                public AndroidLooperThread {
 public:
  AndroidWifiDataProvider();
  virtual ~AndroidWifiDataProvider();

  // WifiDataProviderImplBase
  virtual bool GetData(WifiData* data);

 private:
  // Native callback, invoked when there is new wifi data.
  static void OnUpdateAvailable(JNIEnv* env,
                                jclass cls,
                                jobject wifi_data,
                                jlong self);
  // Inspects the new_wifi_data and decides when to notify
  // the listeners.
  void NewWifiDataAvailable(WifiData* new_wifi_data);

  // AndroidLooperThread
  virtual void BeforeLoop();
  virtual void AfterLoop();

  // True if construction was successful and the API is
  // supported. False if the helper Java classes cannot be
  // initialized.
  bool supported_;

  // Container for the Java class counterpart.
  JavaClass provider_java_class_;
  // Global reference to the AndroidWifiDataProvider instance.
  JavaGlobalRef<jobject> provider_java_object_;
  // The webview instance.
  JavaGlobalRef<jobject> webview_;
  // The signature of our native callback method.
  static JNINativeMethod native_methods_[];

  WifiData wifi_data_;
  // Mutex that protects the access to the wifi_data_.
  Mutex data_mutex_;

#if USING_CCTESTS
  // This is needed for running the WiFi test on the emulator.  Due to
  // the lack of a wpa_supplicant component for the emulator, we
  // cannot get any fake results from the WiFi stack. This means that
  // the scan results are always an empty set of access points, which
  // means that this data provider never notifies any listeners. For
  // the purposes of the unit tests, where we are interested in whether
  // all the Gears component initialize properly and do not crash,
  // we add this flag here that allows us to notify our listeners exactly
  // once, thereby completeing the test.
  bool first_callback_made_;
#endif

  // Whether we've successfully completed a scan for WiFi data.
  // Note: the Java Wifi data provider always calls back
  // with the results of the last scan. This list can be empty
  // either because there simply hasn't been any scan since the phone
  // was booted or because there have been many scans but no actual APs
  // were found (e.g. the user is in the wilderness). The flag below
  // does not distinguish between these situations.
  bool is_first_scan_complete_;

  DISALLOW_EVIL_CONSTRUCTORS(AndroidWifiDataProvider);
};

#endif  // GEARS_GEOLOCATION_WIFI_DATA_PROVIDER_ANDROID_H__
