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

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. WIN32_CPPSRCS) are implemented.
#ifdef OS_ANDROID

#include "gears/geolocation/wifi_data_provider_android.h"

#include "gears/base/android/java_exception_scope.h"
#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/webview_manager.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

static const char* kWifiProviderClass =
    GEARS_JAVA_PACKAGE "/AndroidWifiDataProvider";

static JavaClass::Method provider_methods[] = {
  { JavaClass::kNonStatic, "<init>", "(Landroid/webkit/WebView;J)V"},
  { JavaClass::kNonStatic, "shutdown", "()V"},
};

enum ProviderMethodID {
  METHOD_ID_INIT = 0,
  METHOD_ID_SHUTDOWN,
};


JNINativeMethod AndroidWifiDataProvider::native_methods_[] = {
  {"onUpdateAvailable",
   "(Ljava/util/List;J)V",
   reinterpret_cast<void*>(AndroidWifiDataProvider::OnUpdateAvailable)
  },
};


// Utility functions that convert from Java to C++ objects.
static bool InitFromJava(jobject wifi_data, WifiData* new_wifi_data_out);
static bool InitFromJavaScanResult(jobject scan_result,
                                   AccessPointData* data_out);

// static
template<>
WifiDataProviderImplBase *WifiDataProvider::DefaultFactoryFunction() {
  return new AndroidWifiDataProvider();
}

AndroidWifiDataProvider::AndroidWifiDataProvider()
    : supported_(false),
#if USING_CCTESTS
      first_callback_made_(false),
#endif
      is_first_scan_complete_(false) {
  JavaLocalFrame frame;
  // This provider must run on a Looper thread in order to receive events
  // from the WifiManager. We cannot use the AndroidMessageLoop
  // as that cannot receive events from this service.

  // Set up the jni objects for the Java wifi data provider instance
  // that will be created later, once the thread is started.
  if (!provider_java_class_.FindClass(kWifiProviderClass)) {
    LOG(("Could not find the AndroidWifiDataProvider class.\n"));
    // Fail gracefully. API not available.
    return;
  }

  // Get the provider method IDs.
  if (!provider_java_class_.GetMultipleMethodIDs(provider_methods,
                                                 NELEM(provider_methods))) {
    LOG(("Could not find the AndroidWifiDataProvider methods.\n"));
    // Fail gracefully. API not compatible.
    return;
  }

  // Get the WebView pointer. Note that this MUST execute on either
  // the browser main thread or a worker thread. See WebView::RegisterWebView()
  // method comments for more details aboud this constraint.
  jobject webview;
  if (!WebViewManager::GetWebView(&webview)) {
    LOG(("Could not find the window object.\n"));
    assert(false);
    return;
  }
  webview_.Reset(webview);
  // Construction succeeded.
  supported_ = true;
  // We're now ready to go. Start the thread.
  Start();
}

AndroidWifiDataProvider::~AndroidWifiDataProvider() {
  if (supported_) {
    Stop();
  }
}

bool AndroidWifiDataProvider::GetData(WifiData* data) {
  if (!supported_) {
    return false;
  }
  assert(data);
  MutexLock lock(&data_mutex_);
  *data = wifi_data_;
  return is_first_scan_complete_;
}

void AndroidWifiDataProvider::NewWifiDataAvailable(WifiData* new_wifi_data) {
  assert(supported_);
  assert(new_wifi_data);
  bool is_update_available = false;
  data_mutex_.Lock();
  if (!wifi_data_.Matches(*new_wifi_data)) {
    wifi_data_ = *new_wifi_data;
    is_update_available = true;
    is_first_scan_complete_ = true;
  }
  // Avoid holding the mutex locked while notifying observers.
  data_mutex_.Unlock();

  if (is_update_available) {
    NotifyListeners();
  }

#if USING_CCTESTS
  // This is needed for running the WiFi test on the emulator.
  // See wifi_data_provider_android.h for details.
  if (!first_callback_made_ && wifi_data_.access_point_data.empty()) {
    first_callback_made_ = true;
    NotifyListeners();
  }
#endif
}

void AndroidWifiDataProvider::BeforeLoop() {
  assert(supported_);
  JavaExceptionScope scope;
  JavaLocalFrame frame;

  // Construct the Java Wifi provider object.
  jlong native_object_ptr = reinterpret_cast<jlong>(this);
  jobject provider = JniGetEnv()->NewObject(provider_java_class_.Get(),
                                            provider_methods[METHOD_ID_INIT].id,
                                            webview_.Get(),
                                            native_object_ptr);

  if (scope.Occurred()) {
    // Something went wrong when instantiating the Java object.
    LOG(("Could not instantiate the Java object.\n"));
    // We're not asserting as this can happen due to other reasons
    // than Gears programmer error. We also don't exit now but let
    // the normal operation of the Geolocation stack run its course.
    // We'll get stopped once the location fix is terminated.
    // No need to call 'scope.Clear();', as the exception
    // will be cleared by the destructor of scope.
  } else {
    provider_java_object_.MoveLocal(provider);
    // Register the native callback.
    provider_java_class_.RegisterNativeMethods(native_methods_,
                                               NELEM(native_methods_));
  }
}

void AndroidWifiDataProvider::AfterLoop() {
  assert(supported_);
  // Call the shutdown method if the Java object was created
  // successfully.
  if (provider_java_object_.Get()) {
    JniGetEnv()->CallVoidMethod(provider_java_object_.Get(),
                                provider_methods[METHOD_ID_SHUTDOWN].id);
  }
}

//static
void AndroidWifiDataProvider::OnUpdateAvailable(JNIEnv*  /* env */,
                                                jclass  /* cls */,
                                                jobject wifi_data,
                                                jlong self) {
  assert(self);
  AndroidWifiDataProvider *self_ptr =
      reinterpret_cast<AndroidWifiDataProvider*>(self);
  WifiData new_wifi_data;
  if (wifi_data) {
    InitFromJava(wifi_data, &new_wifi_data);
  }
  // We notify regardless of whether new_wifi_data is empty
  // or not. The arbitrator will decide what to do with an empty
  // WifiData object.
  self_ptr->NewWifiDataAvailable(&new_wifi_data);
}

// Internal

// Despicable JNI goo.
static bool InitFromJava(jobject wifi_data, WifiData* new_wifi_data_out) {
  assert(new_wifi_data_out);
  assert(wifi_data);

  JavaClass::Method list_methods[] = {
    { JavaClass::kNonStatic, "iterator", "()Ljava/util/Iterator;"},
  };

  enum ListMethodID {
    METHOD_ID_ITERATOR = 0,
  };

  JavaClass::Method iterator_methods[] = {
    { JavaClass::kNonStatic, "hasNext", "()Z"},
    { JavaClass::kNonStatic, "next", "()Ljava/lang/Object;"},
  };

  enum IteratorMethodID {
    METHOD_ID_HASNEXT = 0,
    METHOD_ID_NEXT,
  };

  JavaLocalFrame frame;
  JavaClass list_class("List", JniGetEnv()->GetObjectClass(wifi_data));

  if (!list_class.GetMultipleMethodIDs(list_methods, NELEM(list_methods))) {
    LOG(("Could not find the List<ScanResults> methods.\n"));
    assert(false);
    return false;
  }

  jobject iterator = JniGetEnv()->CallObjectMethod(
      wifi_data,
      list_methods[METHOD_ID_ITERATOR].id);

  if (!iterator) {
    LOG(("Could not get the List<ScanResults> iterator.\n"));
    assert(false);
    return false;
  }

  JavaClass iter_class("Iterator", JniGetEnv()->GetObjectClass(iterator));
  if (!iter_class.GetMultipleMethodIDs(iterator_methods,
                                       NELEM(iterator_methods))) {
    LOG(("Could not find the Iterator methods.\n"));
    assert(false);
    return false;
  }
  // Iterate the list of ScanResults.
  while (JniGetEnv()->CallBooleanMethod(
             iterator,
             iterator_methods[METHOD_ID_HASNEXT].id)) {
    // Get the next ScanResult.
    jobject scan_result = JniGetEnv()->CallObjectMethod(
        iterator,
        iterator_methods[METHOD_ID_NEXT].id);
    // Convert it to an AccessPointData.
    AccessPointData data;
    if (InitFromJavaScanResult(scan_result, &data)) {
      new_wifi_data_out->access_point_data.push_back(data);
    }
    JniGetEnv()->DeleteLocalRef(scan_result);
  }
  return true;
}

static bool InitFromJavaScanResult(jobject scan_result,
                                   AccessPointData* data_out) {
  assert(scan_result);
  assert(data_out);

  JavaLocalFrame frame;
  JavaClass clazz("ScanResult", JniGetEnv()->GetObjectClass(scan_result));

  // Get the mac_address from the BSSID field.
  jfieldID fid = JniGetEnv()->GetFieldID(clazz.Get(),
                                         "BSSID",
                                         "Ljava/lang/String;");
  if (fid == NULL) {
    LOG(("Could not extract BSSID field ID.\n"));
    assert(false);
    return false;
  }

  JavaString mac_address(
      static_cast<jstring>(JniGetEnv()->GetObjectField(scan_result, fid)));
  if (mac_address.Get() != NULL) {
    mac_address.ToString16(&(data_out->mac_address));
    LOG(("Got mac_address: %s \n", mac_address.ToString8().c_str()));
  }

  // Get the SSID from the SSID field.
  fid = JniGetEnv()->GetFieldID(clazz.Get(), "SSID", "Ljava/lang/String;");
  if (fid == NULL) {
    LOG(("Could not extract SSID field ID.\n"));
    assert(false);
    return false;
  }

  JavaString ssid(
      static_cast<jstring>(JniGetEnv()->GetObjectField(scan_result, fid)));
  if (ssid.Get() != NULL) {
    ssid.ToString16(&(data_out->ssid));
    LOG(("Got ssid: %s \n", ssid.ToString8().c_str()));
  }

  // Get channel from the frequency field.
  fid = JniGetEnv()->GetFieldID(clazz.Get(), "frequency", "I");
  if (fid == NULL) {
    LOG(("Could not extract frequency field ID.\n"));
    assert(false);
    return false;
  }

  data_out->channel = JniGetEnv()->GetIntField(scan_result, fid);

  // Get the radio_signal_strength from the level field.
  fid = JniGetEnv()->GetFieldID(clazz.Get(), "level", "I");
  if (fid == NULL) {
    LOG(("Could not extract level field ID.\n"));
    assert(false);
    return false;
  }

  data_out->radio_signal_strength = JniGetEnv()->GetIntField(scan_result, fid);

  return true;
}

#endif  // OS_ANDROID
