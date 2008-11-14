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

#include "url_intercept_android.h"

#include <assert.h>

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_exception_scope.h"
#include "gears/base/android/java_global_ref.h"
#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/android/java_string.h"
#include "gears/base/common/common.h"
#include "gears/base/common/stopwatch.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/localserver/common/localserver_db.h"
#include "gears/localserver/android/http_request_android.h"

#define URL_INTERCEPT_HANDLER_GEARS_CLASS \
    GEARS_JAVA_PACKAGE "/UrlInterceptHandlerGears"
#define SERVICE_REQUEST_CLASS \
    GEARS_JAVA_PACKAGE "/UrlInterceptHandlerGears$ServiceRequest"
#define SERVICE_RESPONSE_CLASS \
    GEARS_JAVA_PACKAGE "/UrlInterceptHandlerGears$ServiceResponse"

// The Android time.h header doesn't declare asctime_r, even though
// it's implemented.
extern "C" char *asctime_r(struct tm const *, char *);

// Safe default MIME type.
static const std::string16 kDefaultMimeType(STRING16(L"text/plain"));
// Arbitrary redirection limit to prevent network spam if a cycle is
// accidentally (or deliberately) setup.
static const int kRedirectLimit = 10;
// Fake cache entries created by LocalServer will expire after this
// many milliseconds to prevent any long lasting ill-effects.
static const int kCacheExpiryMilliseconds = 60000;

// Singleton instance.
UrlInterceptAndroid *UrlInterceptAndroid::instance_;

// Methods called on the Java class UrlInterceptHandlerGears.
JavaClass::Method UrlInterceptAndroid::handler_methods_[
    HANDLER_METHOD_COUNT] = {
  { JavaClass::kNonStatic, "<init>", "()V" },
  { JavaClass::kStatic, "enableLogging", "(Z)V" },
  { JavaClass::kNonStatic, "register", "()V" },
  { JavaClass::kNonStatic, "unregister", "()V" },
  { JavaClass::kStatic,
    "getInstance",
    "()L" URL_INTERCEPT_HANDLER_GEARS_CLASS ";" },
};
// Methods called on the inner class ServiceRequst.
JavaClass::Method UrlInterceptAndroid::service_request_methods_[
    SERVICE_REQUEST_METHOD_COUNT] = {
  { JavaClass::kNonStatic,
    "getUrl",
    "()Ljava/lang/String;" },
  { JavaClass::kNonStatic,
    "getRequestHeader",
    "(Ljava/lang/String;)Ljava/lang/String;" },
};
// Methods called on the inner class ServiceResponse.
JavaClass::Method UrlInterceptAndroid::service_response_methods_[
    SERVICE_RESPONSE_METHOD_COUNT] = {
  { JavaClass::kNonStatic,
    "<init>",
    "(L" GEARS_JAVA_PACKAGE "/UrlInterceptHandlerGears;)V" },
  { JavaClass::kNonStatic,
    "setResultArray",
    "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;[B)V" },
  { JavaClass::kNonStatic,
    "setResultFile",
    "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;"
    "Ljava/lang/String;)Z" },
  { JavaClass::kNonStatic,
    "setResponseHeader",
    "(Ljava/lang/String;Ljava/lang/String;)V" },
  { JavaClass::kNonStatic,
    "getContentType",
    "()Ljava/lang/String;" },
};
// Native methods called by the Java class.
JNINativeMethod UrlInterceptAndroid::native_methods_[] = {
  { "nativeService",
    "(L" SERVICE_REQUEST_CLASS ";)L" SERVICE_RESPONSE_CLASS ";",
    reinterpret_cast<void *>(UrlInterceptAndroid::NativeService) },
};

// static
bool UrlInterceptAndroid::Create() {
  assert(instance_ == NULL);
  instance_ = new UrlInterceptAndroid();
  if (instance_->Init()) {
    LOG(("UrlInterceptAndroid initialized\n"));
    return true;
  } else {
    LOG(("Initializing UrlInterceptAndroid failed\n"));
    delete instance_;
    instance_ = NULL;
    return false;
  }
}

// static
void UrlInterceptAndroid::Delete() {
  if (instance_ != NULL) {
    delete instance_;
    instance_ = NULL;
  }
}

UrlInterceptAndroid::UrlInterceptAndroid()
    : initialized_(false),
      handler_class_(),
      service_request_class_(),
      service_response_class_(),
      handler_object_() {
  // Two-stage construction. Init() is called after creating this
  // instance.
}

UrlInterceptAndroid::~UrlInterceptAndroid() {
  if (initialized_) {
    // Call shutdown if Init() succeeded.
    Shutdown();
  }
}

bool UrlInterceptAndroid::Init() {
  // Clear exceptions and local frame on leaving scope.
  JavaExceptionScope exceptions;
  JavaLocalFrame frame;
  JNIEnv *env = JniGetEnv();
  // Find the UrlInterceptHandlerGears class and inner classes.
  if (!handler_class_.FindClass(URL_INTERCEPT_HANDLER_GEARS_CLASS) ||
      !service_request_class_.FindClass(SERVICE_REQUEST_CLASS) ||
      !service_response_class_.FindClass(SERVICE_RESPONSE_CLASS)) {
    LOG(("Couldn't get UrlInterceptHandlerGears classes\n"));
    return false;
  }
  // Double check the enums match the arrays.
  assert(NELEM(handler_methods_) == HANDLER_METHOD_COUNT);
  assert(NELEM(service_request_methods_) == SERVICE_REQUEST_METHOD_COUNT);
  assert(NELEM(service_response_methods_) == SERVICE_RESPONSE_METHOD_COUNT);
  // Fill in the method IDs for these classes.
  if (!handler_class_.GetMultipleMethodIDs(
          handler_methods_, NELEM(handler_methods_)) ||
      !service_request_class_.GetMultipleMethodIDs(
          service_request_methods_, NELEM(service_request_methods_)) ||
      !service_response_class_.GetMultipleMethodIDs(
          service_response_methods_, NELEM(service_response_methods_))) {
    LOG(("Couldn't get UrlInterceptHandlerGears methods\n"));
    return false;
  }
  // Register native methods.
  jniRegisterNativeMethods(env,
                           URL_INTERCEPT_HANDLER_GEARS_CLASS,
                           native_methods_,
                           NELEM(native_methods_));
  // Check if there's already an instance (possibly created by a
  // previously loaded plugin).
  jobject instance = env->CallStaticObjectMethod(
      handler_class_.Get(),
      GetHandlerMethod(HANDLER_METHOD_GET_INSTANCE));
  if (instance == NULL) {
    // No singleton instance. Create an instance of
    // UrlInterceptHandlerGears and create a global reference to
    // retain it.
    instance = env->NewObject(handler_class_.Get(),
                              GetHandlerMethod(HANDLER_METHOD_CONSTRUCTOR));
    if (JavaExceptionScope::Clear() || instance == NULL) {
      LOG(("Couldn't construct UrlInterceptHandlerGears instance\n"));
      return false;
    }
  }
  handler_object_.Reset(instance);
#ifdef DEBUG
  // Enable logging in the Java class.
  env->CallStaticVoidMethod(handler_class_.Get(),
                            GetHandlerMethod(HANDLER_METHOD_ENABLE_LOGGING),
                      
                      static_cast<jboolean>(true));
#endif
  // Register the instance as a handler.
  env->CallVoidMethod(handler_object_.Get(),
                      GetHandlerMethod(HANDLER_METHOD_REGISTER));
  initialized_ = true;
  return true;
}

void UrlInterceptAndroid::Shutdown() {
  // Unregister the instance and release its global reference.
  JniGetEnv()->CallVoidMethod(handler_object_.Get(),
                              GetHandlerMethod(HANDLER_METHOD_UNREGISTER));
  handler_object_.Release();
}

// static
jobject UrlInterceptAndroid::NativeService(
    JNIEnv *env, jclass clazz, jobject service_request) {
  // Called by Java UrlInterceptHandlerGears.service(). This is a
  // wrapper to call the C++ instance.
  assert(instance_ != NULL);
  return instance_->Service(service_request);
}

jobject UrlInterceptAndroid::Service(jobject service_request) {
  // The Java class got a request to service an URL. Check if it's in
  // our database, and if so return a ServiceResponse object.
  // Clean up local references when out of scope.
  JavaLocalFrame frame;
  JNIEnv *env = JniGetEnv();
  // Get the URL from the ServiceRequest object.
  jobject url_object = env->CallObjectMethod(
      service_request,
      GetServiceRequestMethod(SERVICE_REQUEST_METHOD_GET_URL));
  std::string16 url(JavaString(static_cast<jstring>(url_object)).ToString16());
  WebCacheDB* db = WebCacheDB::GetDB();
  if (!db) {
    LOG(("No WebCacheDB\n"));
    return false;
  }
  WebCacheDB::PayloadInfo payload;
  // Try servicing this URL. Handle redirection here to prevent it
  // being queued on the browser's network request queue. This would
  // block if the network is unavailable. If redirection fails, let
  // the browser handle, which in worst case tries the network.
  for (int i = 0;; ++i) {
    if (!db->Service(url.c_str(), NULL, false, &payload)) {
      LOG(("No service for \"%s\"\n", String16ToUTF8(url).c_str()));
      return NULL;
    }
    if (!payload.IsHttpRedirect()) {
      LOG(("Not a redirect\n"));
      break;
    }
    std::string16 location;
    if (!payload.GetHeader(HttpConstants::kLocationHeader, &location) ||
        location.empty()) {
      LOG(("Redirect but empty Location header\n"));
      break;
    }
    if (location == url) {
      LOG(("Redirect to self. Bad redirect, bad!\n"));
      break;
    }
    if (++i >= kRedirectLimit) {
      LOG(("Redirect limit (%d) exceeded\n", i));
      break;
    }
    LOG(("Redirecting to %s\n", String16ToUTF8(location).c_str()));
    std::string16 resolved;
    if (!ResolveAndNormalize(url.c_str(), location.c_str(), &resolved)) {
      LOG(("Couldn't resolve %s -> %s\n",
           String16ToUTF8(url).c_str(),
           String16ToUTF8(location).c_str()));
      break;
    }
    if (!HttpRequestAndroid::IsSameOrigin(url, resolved)) {
      LOG(("Not cross-origin redirecting LocalServer from %s -> %s\n",
           String16ToUTF8(url).c_str(),
           String16ToUTF8(resolved).c_str()));
      break;
    }
    LOG(("Resolved to same-origin %s\n", String16ToUTF8(resolved).c_str()));
    url = resolved;
  }
  // Create a ServiceResponse instance. This will be garbage collected
  // if not returned to the parent frame using pop(). This is a local
  // class so it needs a "parent" instance when constructed.
  jobject response = env->NewObject(
      service_response_class_.Get(),
      GetServiceResponseMethod(SERVICE_RESPONSE_METHOD_CONSTRUCTOR),
      handler_object_.Get());
  // Split the header string into individual lines.
  std::vector<std::string16> header_entries;
  Tokenize(payload.headers, std::string16(STRING16(L"\r\n")), &header_entries);
  // Parse each line.
  for (std::vector<std::string16>::const_iterator it = header_entries.begin();
       it != header_entries.end();
       ++it) {
    std::string16::size_type colon = it->find(static_cast<char16>(':'));
    if (colon == std::string16::npos) {
      LOG(("Bad request header line: \"%s\"\n", String16ToUTF8(*it).c_str()));
      continue;
    }
    std::string16 key(it->data(), colon);
    std::string16 value(it->c_str() + colon + 1);
    key = StripWhiteSpace(key);
    value = StripWhiteSpace(value);
    // Remove "Pragma: no-cache" and any "Cache-Control:" lines. We
    // want to set caching behavior explicitly. In any case, the
    // browser will refuse to save a no-cache response into its cache.
    if (MakeLowerString(key) ==
        MakeLowerString(std::string16(HttpConstants::kPragmaHeader)) &&
        MakeLowerString(value) ==
        MakeLowerString(std::string16(HttpConstants::kNoCache))) {
      LOG(("Skipping Pragma: no-cache header\n"));
      continue;
    } else if (
        MakeLowerString(key) ==
        MakeLowerString(std::string16(HttpConstants::kCacheControlHeader))) {
      LOG(("Removing Cache-Control: %s header\n",
           String16ToUTF8(value).c_str()));
      continue;
    }
    LOG(("Header %s: %s\n",
         String16ToUTF8(key).c_str(),
         String16ToUTF8(value).c_str()));
    env->CallVoidMethod(
        response,
        GetServiceResponseMethod(SERVICE_RESPONSE_METHOD_SET_RESPONSE_HEADER),
        JavaString(key).Get(),
        JavaString(value).Get());
  }
  // Explicitly cache for an arbitrary short time. This prevents any
  // long lasting ill-effects from caching entries which shouldn't
  // really be cached.
  time_t expires_sec = static_cast<time_t>(
      (GetCurrentTimeMillis() + kCacheExpiryMilliseconds) / 1000);
  // Convert to GMT relative time.
  struct tm expires_tm;
  gmtime_r(&expires_sec, &expires_tm);
  // Convert to a ASCII string. The output of ANSI C asctime() is
  // allowed by HTTP/1.1. The output is documented as at most 26
  // characters according to the manual page.
  char expires_str[32];
  asctime_r(&expires_tm, expires_str);
  // Remove trailing line endings.
  int len = strlen(expires_str);
  if (expires_str[len - 1] == '\n') {
    expires_str[len - 1] = '\0';
  }
  LOG(("Set expiry to %s\n", expires_str));
  env->CallVoidMethod(
      response,
      GetServiceResponseMethod(SERVICE_RESPONSE_METHOD_SET_RESPONSE_HEADER),
      JavaString(HttpConstants::kExpiresHeader).Get(),
      JavaString(expires_str).Get());
  // Extract the MIME type and encoding from the "Content-Type" header.
  jstring content_type = static_cast<jstring>(
      env->CallObjectMethod(
          response,
          GetServiceResponseMethod(SERVICE_RESPONSE_METHOD_GET_CONTENT_TYPE)));
  // Set sensible defaults.
  std::string16 mime_type(kDefaultMimeType);
  std::string16 encoding;
  if (content_type != NULL) {
    // It exists, take the information.
    HttpRequestAndroid::ParseContentType(JavaString(content_type).ToString16(),
                                         &mime_type,
                                         &encoding);
  }
  if (!payload.cached_filepath.empty()) {
    LOG(("Response for %s comes from file %s\n",
         String16ToUTF8(url).c_str(),
         String16ToUTF8(payload.cached_filepath).c_str()));
    // Setup for a file stream.
    if (env->CallBooleanMethod(
            response,
            GetServiceResponseMethod(SERVICE_RESPONSE_METHOD_SET_RESULT_FILE),
            payload.status_code,
            JavaString(payload.status_line).Get(),
            JavaString(mime_type).Get(),
            encoding.empty() ? NULL : JavaString(encoding).Get(),
            JavaString(payload.cached_filepath).Get())) {
      LOG(("Successfully setup a stream directly from the file\n"));
      // Return the response to the parent frame.
      return frame.pop(response);
    } else {
      LOG(("Couldn't setup a stream directly from the file\n"));
      return NULL;
    }
  } else {
    // Setup for a stream coming from an array in memory.
    jsize data_size;
    if (payload.data.get() != NULL) {
      // Setup for a stream coming from an array in memory.
      data_size = static_cast<jsize>(payload.data->size());
      LOG(("Response for %s comes from data of length %d\n",
         String16ToUTF8(url).c_str(), static_cast<int>(data_size)));
    } else {
      // Empty payload.
      LOG(("Response for %s is an empty payload\n",
           String16ToUTF8(url).c_str()));
      data_size = 0;
    }
    // Create a byte array from the payload data.
    JavaGlobalRef<jbyteArray> byte_array(env->NewByteArray(data_size));
    if (byte_array.Get() == NULL) {
      LOG(("Couldn't allocate byte[]\n"));
      return NULL;
    }
    if (payload.data.get() != NULL) {
      // Pin it in memory so we can copy into it.
      jbyte *pinned_array = env->GetByteArrayElements(byte_array.Get(), NULL);
      if (!pinned_array) {
        LOG(("Couldn't pin byte[]\n"));
        return NULL;
      }
      // Copy the data.
      std::copy(payload.data->begin(), payload.data->end(), pinned_array);
      // Release the array back to the VM.
      env->ReleaseByteArrayElements(byte_array.Get(), pinned_array, 0);
    }
    // Setup for an array in memory.
    env->CallVoidMethod(
        response,
        GetServiceResponseMethod(SERVICE_RESPONSE_METHOD_SET_RESULT_ARRAY),
        payload.status_code,
        JavaString(payload.status_line).Get(),
        JavaString(mime_type).Get(),
        encoding.empty() ? NULL : JavaString(encoding).Get(),
        byte_array.Get());
    LOG(("Successfully setup a stream from memory\n"));
    return frame.pop(response);
  }
}
