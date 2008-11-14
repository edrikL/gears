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

#ifndef GEARS_LOCALSERVER_ANDROID_URL_INTERCEPT_ANDROID_H__
#define GEARS_LOCALSERVER_ANDROID_URL_INTERCEPT_ANDROID_H__

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_global_ref.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/common/string16.h"

// Singleton instance which intercepts all URLs passing through the
// browser and services them using LocalServer. Also called by
// HttpRequestAndroid when caching is allowed.
class UrlInterceptAndroid {
 public:
  // Create an instance and initialize it. Returns true if
  // initialization succeeded. Returns false if initialization failed,
  // and deletes the instance. Called by NP_Initialize().
  static bool Create();
  // Delete the instance. Called by NP_Shutdown().
  static void Delete();

 private:
  UrlInterceptAndroid();
  ~UrlInterceptAndroid();

  // Initialize members and install URL intercept handler.
  bool Init();
  // Unregister the handler and shut down.
  void Shutdown();
  // JNI native method called for all URLs passing through the
  // browser. Simply wraps an instance call to Service().
  static jobject NativeService(JNIEnv *env,
                               jclass clazz,
                               jobject service_request);
  // Parse a string of CR-LF separated headers, extracting the MIME
  // type and encoding.
  static void ParseContentType(const std::string16 &headers,
                               std::string16 *mime_type,
                               std::string16 *encoding);
  // Handles an URL, returning a ServiceResponse object if handled, or
  // NULL otherwise.
  jobject Service(jobject service_request);

  // Methods called on the Java class UrlInterceptHandlerGears.
  enum HandlerMethod {
    HANDLER_METHOD_CONSTRUCTOR = 0,
    HANDLER_METHOD_ENABLE_LOGGING,
    HANDLER_METHOD_REGISTER,
    HANDLER_METHOD_UNREGISTER,
    HANDLER_METHOD_GET_INSTANCE,
    HANDLER_METHOD_COUNT,
  };
  // Methods called on the inner class ServiceRequst.
  enum ServiceRequestMethod {
    SERVICE_REQUEST_METHOD_GET_URL = 0,
    SERVICE_REQUEST_METHOD_GET_REQUEST_HEADER,
    SERVICE_REQUEST_METHOD_COUNT,
  };
  // Methods called on the inner class ServiceResponse.
  enum ServiceResponseMethod {
    SERVICE_RESPONSE_METHOD_CONSTRUCTOR = 0,
    SERVICE_RESPONSE_METHOD_SET_RESULT_ARRAY,
    SERVICE_RESPONSE_METHOD_SET_RESULT_FILE,
    SERVICE_RESPONSE_METHOD_SET_RESPONSE_HEADER,
    SERVICE_RESPONSE_METHOD_GET_CONTENT_TYPE,
    SERVICE_RESPONSE_METHOD_COUNT,
  };
  // Simple method ID array accessors.
  static jmethodID GetHandlerMethod(HandlerMethod i) {
    assert(handler_methods_[i].id != 0);
    return handler_methods_[i].id;
  }
  static jmethodID GetServiceRequestMethod(ServiceRequestMethod i) {
    assert(service_request_methods_[i].id != 0);
    return service_request_methods_[i].id;
  }
  static jmethodID GetServiceResponseMethod(ServiceResponseMethod i) {
    assert(service_response_methods_[i].id != 0);
    return service_response_methods_[i].id;
  }

  // Static singleton instance.
  static UrlInterceptAndroid *instance_;
  // Arrays containing the Java method signatures and IDs for each
  // class.
  static JavaClass::Method
      handler_methods_[HANDLER_METHOD_COUNT];
  static JavaClass::Method
      service_request_methods_[SERVICE_REQUEST_METHOD_COUNT];
  static JavaClass::Method
      service_response_methods_[SERVICE_RESPONSE_METHOD_COUNT];
  // Native methods called by the Java class.
  static JNINativeMethod native_methods_[];

  // True if Init() succeeded and Shutdown() needs to be called on
  // destruction.
  bool initialized_;
  // Container for Java class UrlInterceptHandlerGears.
  JavaClass handler_class_;
  // Container for Java class ServiceRequest.
  JavaClass service_request_class_;
  // Container for Java class ServiceResponse.
  JavaClass service_response_class_;
  // Singleton instance of Java class UrlInterceptHandlerGears.
  JavaGlobalRef<jobject> handler_object_;  
};

#endif // GEARS_LOCALSERVER_ANDROID_URL_INTERCEPT_ANDROID_H__
