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

#include "gears/geolocation/radio_data_provider_android.h"

#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_exception_scope.h"
#include "gears/base/android/webview_manager.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

static const char* kRadioProviderClass =
    GEARS_JAVA_PACKAGE "/AndroidRadioDataProvider";

static JavaClass::Method provider_methods[] = {
  { JavaClass::kNonStatic, "<init>", "(Landroid/webkit/WebView;J)V"},
  { JavaClass::kNonStatic, "shutdown", "()V"},
};

enum ProviderMethodID {
  METHOD_ID_INIT = 0,
  METHOD_ID_SHUTDOWN,
};

enum SupportedAndroidRadioTypes {
  SUPPORTED_ANDROID_RADIO_TYPES_UNKNOWN = 0,
  SUPPORTED_ANDROID_RADIO_TYPES_GSM,
  SUPPORTED_ANDROID_RADIO_TYPES_WCDMA,
};

JNINativeMethod AndroidRadioDataProvider::native_methods_[] = {
  {"onUpdateAvailable",
   "(L" GEARS_JAVA_PACKAGE "/AndroidRadioDataProvider$RadioData;J)V",
   reinterpret_cast<void*>(AndroidRadioDataProvider::OnUpdateAvailable)
  },
};


// Utility function that creates a C++ RadioData object given a Java
// RadioData object.
static bool InitFromJavaRadioData(JNIEnv* env,
                                  jobject radio_data,
                                  RadioData* new_radio_data_out);

// Utility function that inspects a RadioData object and checks if all
// possible data is set.
static bool IsAllDataAvailable(const RadioData &radio_data);

// static
template<>
RadioDataProviderImplBase *RadioDataProvider::DefaultFactoryFunction() {
  return new AndroidRadioDataProvider();
}

AndroidRadioDataProvider::AndroidRadioDataProvider() {
  JavaLocalFrame frame;
  // This provider must run on a Looper thread in order to receive events
  // from the Telephony service. We cannot use the AndroidMessageLoop
  // as that cannot receive events from the service.

  // Set up the jni objects for the Java radio data provider instance
  // that will be created later, once the thread is started.
  if (!provider_java_class_.FindClass(kRadioProviderClass)) {
    LOG(("Could not find the AndroidRadioDataProvider class.\n"));
    assert(false);
    return;
  }

  // Get the provider method IDs.
  if (!provider_java_class_.GetMultipleMethodIDs(provider_methods,
                                                 NELEM(provider_methods))) {
    LOG(("Could not find the AndroidRadioDataProvider methods.\n"));
    assert(false);
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
  // We're now ready to go. Start the thread.
  Start();
}

AndroidRadioDataProvider::~AndroidRadioDataProvider() {
  Stop();
}

bool AndroidRadioDataProvider::GetData(RadioData* data) {
  assert(data);
  MutexLock lock(&data_mutex_);
  *data = radio_data_;
  return IsAllDataAvailable(*data);
}

void AndroidRadioDataProvider::NewRadioDataAvailable(
    RadioData* new_radio_data) {
  bool is_update_available = false;
  data_mutex_.Lock();
  if (new_radio_data && !radio_data_.Matches(*new_radio_data)) {
    radio_data_ = *new_radio_data;
    is_update_available = true;
  }
  // Avoid holding the mutex locked while notifying observers.
  data_mutex_.Unlock();

  if (is_update_available) {
    NotifyListeners();
  }
}

void AndroidRadioDataProvider::BeforeLoop() {
  JavaExceptionScope scope;
  JavaLocalFrame frame;

  // Construct the Java Radio provider object.
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
    // Step 4: Register the native callback.
    jniRegisterNativeMethods(JniGetEnv(),
                             kRadioProviderClass,
                             native_methods_,
                             NELEM(native_methods_));

  }
}

void AndroidRadioDataProvider::AfterLoop() {
  // Call the shutdown method if the Java object was created
  // successfully.
  if (provider_java_object_.Get()) {
    JniGetEnv()->CallVoidMethod(provider_java_object_.Get(),
                                provider_methods[METHOD_ID_SHUTDOWN].id);
  }
}

//static
void AndroidRadioDataProvider::OnUpdateAvailable(JNIEnv* env,
                                                 jclass cls,
                                                 jobject radio_data,
                                                 jlong self) {
  assert(radio_data);
  assert(self);
  AndroidRadioDataProvider *self_ptr =
      reinterpret_cast<AndroidRadioDataProvider*>(self);
  RadioData new_radio_data;
  if (InitFromJavaRadioData(env, radio_data, &new_radio_data)) {
    self_ptr->NewRadioDataAvailable(&new_radio_data);
  }
}

// Internal

static bool InitFromJavaRadioData(JNIEnv* env,
                                  jobject radio_data,
                                  RadioData* new_radio_data_out) {
  assert(new_radio_data_out);
  JavaLocalFrame frame;
  JavaGlobalRef<jclass> clazz = env->GetObjectClass(radio_data);

  // Get the cell id.
  jfieldID fid = env->GetFieldID(clazz.Get(),
                                 "cellId",
                                 "I");
  if (fid == NULL) {
    LOG(("Could not extract cellId field ID.\n"));
    assert(false);
    return false;
  }

  CellData cell_data;
  cell_data.cell_id = env->GetIntField(radio_data, fid);

  // Get the LAC
  fid = env->GetFieldID(clazz.Get(),
                        "locationAreaCode",
                        "I");
  if (fid == NULL) {
    LOG(("Could not extract lac field ID.\n"));
    assert(false);
    return false;
  }

  cell_data.location_area_code = env->GetIntField(radio_data, fid);

  // Get the mcc.
  fid = env->GetFieldID(clazz.Get(),
                        "mobileCountryCode",
                        "I");
  if (fid == NULL) {
    LOG(("Could not extract mcc field ID.\n"));
    assert(false);
    return false;
  }

  cell_data.mobile_country_code = env->GetIntField(radio_data, fid);

  // Get the mnc
  fid = env->GetFieldID(clazz.Get(),
                        "mobileNetworkCode",
                        "I");
  if (fid == NULL) {
    LOG(("Could not extract mnc field ID.\n"));
    assert(false);
    return false;
  }

  cell_data.mobile_network_code = env->GetIntField(radio_data, fid);

  // Get the signal strength.
  fid = env->GetFieldID(clazz.Get(),
                        "signalStrength",
                        "I");
  if (fid == NULL) {
    LOG(("Could not extract signal strength field ID.\n"));
    assert(false);
    return false;
  }

  cell_data.radio_signal_strength = env->GetIntField(radio_data, fid);

  // Get the network type.
  fid = env->GetFieldID(clazz.Get(),
                        "radioType",
                        "I");
  if (fid == NULL) {
    LOG(("Could not extract radio type field ID.\n"));
    assert(false);
    return false;
  }

  int radio_type = env->GetIntField(radio_data, fid);
  switch (radio_type) {
    case SUPPORTED_ANDROID_RADIO_TYPES_GSM:
      new_radio_data_out->radio_type = RADIO_TYPE_GSM;
      break;
    case SUPPORTED_ANDROID_RADIO_TYPES_WCDMA:
      new_radio_data_out->radio_type = RADIO_TYPE_WCDMA;
      break;
    case SUPPORTED_ANDROID_RADIO_TYPES_UNKNOWN:
      new_radio_data_out->radio_type = RADIO_TYPE_UNKNOWN;
      break;
    default:
      LOG(("Invalid radio type.\n"));
      assert(false);
      return false;
  };

  // Get the carrier name.
  fid = env->GetFieldID(clazz.Get(),
                        "carrierName",
                        "Ljava/lang/String;");
  if (fid == NULL) {
    LOG(("Could not extract carrierName field ID.\n"));
    assert(false);
    return false;
  }

  JavaString carrier_name(
      static_cast<jstring>(env->GetObjectField(radio_data, fid)));
  // carrier_name can be NULL. Currently there is a bug in JNI that
  // manifests itself by creating jstrings that point to a string
  // containing the word "null" instead of the jstrings being NULL
  // themselves. TODO(andreip): remove the comparison against "null" once
  // the bug is fixed.
  if (carrier_name.Get() != NULL &&
      carrier_name.ToString8().compare("null") != 0) {
    carrier_name.ToString16(&new_radio_data_out->carrier);
    new_radio_data_out->cell_data.clear();
    new_radio_data_out->cell_data.push_back(cell_data);
  }

  return true;
}

static bool IsAllDataAvailable(const RadioData& radio_data) {
  return radio_data.cell_data.size() == 1 &&
      -1 != radio_data.cell_data[0].cell_id &&
      -1 != radio_data.cell_data[0].location_area_code &&
      -1 != radio_data.cell_data[0].mobile_network_code &&
      -1 != radio_data.cell_data[0].mobile_country_code &&
      -1 != radio_data.cell_data[0].radio_signal_strength &&
      !radio_data.carrier.empty();
}

#endif  // OS_ANDROID
