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

#include "gears/geolocation/gps_device_android.h"

#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_exception_scope.h"
#include "gears/base/android/webview_manager.h"

static const char *kGpsProviderClass =
    GEARS_JAVA_PACKAGE "/AndroidGpsLocationProvider";

static const char *kLocationClass = "android/location/Location";

static JavaClass::Method provider_methods[] = {
  { JavaClass::kNonStatic, "<init>", "(Landroid/webkit/WebView;J)V"},
  { JavaClass::kNonStatic, "shutdown", "()V"},
};

enum ProviderMethodID {
  METHOD_ID_INIT = 0,
  METHOD_ID_SHUTDOWN,
};

static const char16 *kGpsDisabledMessage =
    STRING16(L"GPS is disabled.");
static const char16 *kGpsErrorMessage =
    STRING16(L"GPS failed to get a position fix.");

JNINativeMethod AndroidGpsDevice::native_methods_[] = {
  {"nativeLocationChanged",
   "(Landroid/location/Location;J)V",
   reinterpret_cast<void*>(AndroidGpsDevice::OnLocationChanged)
  },
  {"nativeProviderError",
   "(ZJ)V",
   reinterpret_cast<void*>(AndroidGpsDevice::OnProviderError)
  },
};

// Local function
// This assumes that position_2 is a good fix.
static bool PositionsDiffer(const Position &position_1,
                            const Position &position_2);
static bool GetPositionFromJavaLocation(JNIEnv *env,
                                        const jobject &location,
                                        Position *position);

// GpsLocationProvider factory method for platform-dependent GPS devices.
// static
GpsDeviceBase *GpsLocationProvider::NewGpsDevice(
    GpsDeviceBase::ListenerInterface *listener) {
  return new AndroidGpsDevice(listener);
}

AndroidGpsDevice::AndroidGpsDevice(GpsDeviceBase::ListenerInterface *listener)
    : GpsDeviceBase(listener) {
  assert(listener);
  JavaLocalFrame frame;
  if (!provider_java_class_.FindClass(kGpsProviderClass)) {
    LOG(("Could not find the AndroidGpsDevice class.\n"));
    assert(false);
    return;
  }

  // Get the provider method IDs.
  if (!provider_java_class_.GetMultipleMethodIDs(provider_methods,
                                                 NELEM(provider_methods))) {
    LOG(("Could not find the AndroidGpsDevice methods.\n"));
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

void AndroidGpsDevice::BeforeLoop() {
  JavaExceptionScope scope;
  JavaLocalFrame frame;
  // Construct the Java Gps provider.
  jlong native_object_ptr = reinterpret_cast<jlong>(this);
  jobject object = JniGetEnv()->NewObject(provider_java_class_.Get(),
                                          provider_methods[METHOD_ID_INIT].id,
                                          webview_.Get(),
                                          native_object_ptr);
  if (scope.Occurred()) {
    // Something went wrong when instantiating the Java object.
    LOG(("Could not instantiate the Java object.\n"));
    position_.error_code = kGeolocationLocationAcquisitionErrorCode;
    position_.error_message = kGpsErrorMessage;
    // No need to call 'scope.Clear();', as the exception
    // will be cleared by the destructor of scope.
  } else {
    provider_java_object_.MoveLocal(object);
    // Register the native callback.
    jniRegisterNativeMethods(JniGetEnv(),
                             kGpsProviderClass,
                             native_methods_,
                             NELEM(native_methods_));
  }
}

AndroidGpsDevice::~AndroidGpsDevice() {
  Stop();
}

void AndroidGpsDevice::AfterLoop() {
  // Call the shutdown method if the Java object was created
  // successfully.
  if (provider_java_object_.Get()) {
    JniGetEnv()->CallVoidMethod(provider_java_object_.Get(),
                                provider_methods[METHOD_ID_SHUTDOWN].id);
  }
}

// static
void AndroidGpsDevice::OnLocationChanged(JNIEnv *env,
                                         jclass cls,
                                         jobject location,
                                         jlong self) {
  assert(location);
  assert(self);
  AndroidGpsDevice *self_ptr =
      reinterpret_cast<AndroidGpsDevice*>(self);
  Position position;
  if (GetPositionFromJavaLocation(env, location, &position) &&
      position.IsGoodFix()) {
    self_ptr->PositionChanged(position);
  }
}

// static
void AndroidGpsDevice::OnProviderError(JNIEnv *env,
                                       jclass cls,
                                       jboolean is_disabled,
                                       jlong self) {
  assert(self);
  AndroidGpsDevice *self_ptr =
      reinterpret_cast<AndroidGpsDevice*>(self);
  self_ptr->ProviderError(is_disabled);
}

void AndroidGpsDevice::PositionChanged(const Position &new_position) {
  if (PositionsDiffer(position_, new_position)) {
    position_ = new_position;
    LOG(("Position changed, updating listener.\n"));
    listener_->GpsPositionUpdateAvailable(position_);
  }
}

void AndroidGpsDevice::ProviderError(bool is_disabled) {
  if (is_disabled) {
    LOG(("GPS disabled, updating listener.\n"));
    listener_->GpsFatalError(kGeolocationLocationAcquisitionErrorCode,
                             kGpsDisabledMessage);
  } else {
    LOG(("GPS error, updating listener.\n"));
    listener_->GpsFatalError(kGeolocationLocationAcquisitionErrorCode,
                             kGpsErrorMessage);
  }
}

static bool GetPositionFromJavaLocation(JNIEnv *env,
                                        const jobject &location,
                                        Position *position) {
  assert(position);
  JavaLocalFrame frame;
  JavaExceptionScope scope;
  JavaClass clazz(kLocationClass, env->GetObjectClass(location));

  JavaClass::Method methods[] = {
    { JavaClass::kNonStatic, "getLatitude", "()D"},
    { JavaClass::kNonStatic, "getLongitude", "()D"},
    { JavaClass::kNonStatic, "hasAltitude", "()Z"},
    { JavaClass::kNonStatic, "getAltitude", "()D"},
    { JavaClass::kNonStatic, "hasAccuracy", "()Z"},
    { JavaClass::kNonStatic, "getAccuracy", "()F"},
    { JavaClass::kNonStatic, "getTime", "()J"},
  };

  enum LocationMethods {
    LOCATION_METHODS_GET_LATITUDE,
    LOCATION_METHODS_GET_LONGITUDE,
    LOCATION_METHODS_HAS_ALTITUDE,
    LOCATION_METHODS_GET_ALTITUDE,
    LOCATION_METHODS_HAS_ACCURACY,
    LOCATION_METHODS_GET_ACCURACY,
    LOCATION_METHODS_GET_TIME,
  };

  if (!clazz.GetMultipleMethodIDs(methods, NELEM(methods))) {
    LOG(("Could not find the Location methods.\n"));
    assert(false);
    return false;
  }

  position->latitude = env->CallDoubleMethod(
      location,
      methods[LOCATION_METHODS_GET_LATITUDE].id);

  position->longitude = env->CallDoubleMethod(
      location,
      methods[LOCATION_METHODS_GET_LONGITUDE].id);

  if (env->CallBooleanMethod(location,
                             methods[LOCATION_METHODS_HAS_ALTITUDE].id)) {
    position->altitude = env->CallDoubleMethod(
        location,
        methods[LOCATION_METHODS_GET_ALTITUDE].id);
  }

  if (env->CallBooleanMethod(location,
                             methods[LOCATION_METHODS_HAS_ACCURACY].id)) {
    position->accuracy = env->CallFloatMethod(
       location,
       methods[LOCATION_METHODS_GET_ACCURACY].id);
  } else {
    // On the Android emulator valid position fixes do not have
    // the accuracy set. For debugging purposes, we set it to 0 here.
    position->accuracy = 0;
  }

  position->timestamp = env->CallLongMethod(
      location,
      methods[LOCATION_METHODS_GET_TIME].id);

  return true;
}

// Local function
// TODO(andreip/steveblock): move this to GpsDeviceBase.
// static
bool PositionsDiffer(const Position &position_1, const Position &position_2) {
  assert(position_2.IsGoodFix());

  if (!position_1.IsGoodFix()) {
    return true;
  }
  return position_1.latitude != position_2.latitude ||
         position_1.longitude != position_2.longitude ||
         position_1.accuracy != position_2.accuracy ||
         position_1.altitude != position_2.altitude;
}



#endif  // OS_ANDROID
