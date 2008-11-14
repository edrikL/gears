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
//------------------------------------------------------------------------------
// A cross-platform interface for sending HTTP requests, Android implementation.
// This uses a small Java class to perform the IO operations.
//------------------------------------------------------------------------------

#include <assert.h>

#include "gears/localserver/android/http_request_android.h"

#include "gears/base/android/java_class.h"
#include "gears/base/android/java_class_loader.h"
#include "gears/base/android/java_exception_scope.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_string.h"
#include "gears/base/common/async_router.h"
#include "gears/base/common/common.h"
#include "gears/base/common/http_utils.h"
#include "gears/base/common/message_queue.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/npapi/browser_utils.h"
#include "gears/localserver/common/safe_http_request.h"

// Fully qualified name of our counterpart Java class.
static const char *const kHttpRequestAndroidClassName =
    GEARS_JAVA_PACKAGE "/HttpRequestAndroid";

// Default MIME type associated with a document which doesn't specify it.
static const char16 *const kDefaultMimeType = STRING16(L"text/plain");
// Default encoding for a document which doesn't specify it. RFC-2616
// states there is no default.
static const char16 *const kDefaultEncoding = STRING16(L"");

// Number of bytes to send or receive in one go.
static const int kTransferSize = 4096;

// Limit the number of redirects to this amount. This is to prevent
// endless loops. This number of chosen arbitrarily, but very likely
// to be above any normal redirection count. This is required on
// Android because we don't have an intercept at the HttpURLConnection
// layer. Instead, we turn off instanceFollowRedirects so we can
// handle each hop consistently, potentially using LocalServer.
static const int kRedirectLimit = 10;

// Container for a reference to the Java Class HttpRequestAndroid.
JavaClass HttpRequestAndroid::java_class_;
// List of Java methods. Note that this must match the order of the
// enum JavaMethod.
JavaClass::Method HttpRequestAndroid::java_methods_[JAVA_METHOD_COUNT] = {
  { JavaClass::kNonStatic,
    "<init>",
    "()V" },
  { JavaClass::kStatic,
    "enableLogging",
    "(Z)V" },
  { JavaClass::kNonStatic,
    "initChildThread",
    "()V" },
  { JavaClass::kNonStatic,
    "setRequestHeader",
    "(Ljava/lang/String;Ljava/lang/String;)V" },
  { JavaClass::kNonStatic,
    "getRequestHeader",
    "(Ljava/lang/String;)Ljava/lang/String;" },
  { JavaClass::kNonStatic,
    "open",
    "(Ljava/lang/String;Ljava/lang/String;)Z" },
  { JavaClass::kNonStatic,
    "useLocalServerResult",
    "(Ljava/lang/String;)Z" },
  { JavaClass::kNonStatic,
    "useCacheResult",
    "(Ljava/lang/String;)Z" },
  { JavaClass::kNonStatic,
    "createCacheResult",
    "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;)Z" },
  { JavaClass::kNonStatic,
    "appendCacheResult",
    "([BI)Z" },
  { JavaClass::kNonStatic,
    "saveCacheResult",
    "()Z" },
  { JavaClass::kNonStatic,
    "connectToRemote",
    "()Z" },
  { JavaClass::kNonStatic,
    "parseHeaders",
    "()Z" },
  { JavaClass::kNonStatic,
    "receive",
    "([B)I" },
  { JavaClass::kNonStatic,
    "sendPostData",
    "([BI)Z" },
  { JavaClass::kNonStatic,
    "getResponseLine",
    "()Ljava/lang/String;" },
  { JavaClass::kStatic,
    "getCookieForUrl",
    "(Ljava/lang/String;)Ljava/lang/String;" },
  { JavaClass::kStatic,
    "setCookieForUrl",
    "(Ljava/lang/String;Ljava/lang/String;)V" },
  { JavaClass::kNonStatic,
    "getResponseHeader",
    "(Ljava/lang/String;)Ljava/lang/String;" },
  { JavaClass::kNonStatic,
    "getAllResponseHeaders",
    "()Ljava/lang/String;" },
  { JavaClass::kNonStatic,
    "interrupt",
    "()V" },
};

// The thread ID of the main thread. Initialized in InitJni().
ThreadId HttpRequestAndroid::main_thread_id_;


// Public instance creation, may be called by background/worker
// threads, in which case a SafeHttpRequest is substituted.
bool HttpRequest::Create(scoped_refptr<HttpRequest> *request) {
  if (HttpRequestAndroid::IsMainThread()) {
    request->reset(new HttpRequestAndroid());
    return true;
  } else {
    return HttpRequest::CreateSafeRequest(request);
  }
}

// Public instance creation. This creates a SafeHttpRequest, which
// marshals all calls through to the main thread to allow safe usage
// from a backgroudn thread. SafeHttpRequest calls Create() to
// construct a raw HttpRequestAndroid as required.
bool HttpRequest::CreateSafeRequest(scoped_refptr<HttpRequest> *request) {
  request->reset(new SafeHttpRequest(HttpRequestAndroid::main_thread_id_));
  return true;
}


// Private constructor. Use Create() or CreateSafeRequest() to
// instance an object.
HttpRequestAndroid::HttpRequestAndroid()
    : ref_count_(),
      mutex_(),
      child_thread_(this),
      caching_behavior_(USE_ALL_CACHES),
      redirect_behavior_(FOLLOW_ALL),
      cookie_behavior_(SEND_BROWSER_COOKIES),
      state_(STATE_MAIN_IDLE),
      ready_state_(UNINITIALIZED),
      java_object_(),
      http_listener_(NULL),
      listener_enable_data_available_(false),
      asynchronous_(true),
      was_aborted_(false),
      response_body_(NULL),
      post_blob_(NULL),
      was_redirected_(false),
      served_locally_(false),
      redirect_count_(0),
      method_(),
      initial_url_(),
      current_url_(),
      mime_type_(kDefaultMimeType),
      encoding_(kDefaultEncoding) {
  assert(IsMainThread());
  JNIEnv *env = JniGetEnv();
  // Construct the object, moving it straight into a global reference.
  java_object_.MoveLocal(env->NewObject(
                             java_class_.Get(),
                             GetMethod(JAVA_METHOD_CONSTRUCTOR)));
  assert(java_object_.Get() != 0);
  LOG(("Constructed HttpRequestAndroid at %p\n", this));
  LOG(("main:%p\n", reinterpret_cast<void *>(main_thread_id_)));
}

HttpRequestAndroid::~HttpRequestAndroid() {
  assert(IsMainThread());
  assert(!child_thread_.IsRunning());
  // The JavaGlobalRef will dereference the Java object, and will be
  // garbage collected by the VM at some point in the future.
  assert(ref_count_.Value() == 0);
  LOG(("Destructing an object at %p\n", this));
  LOG(("Destructed an object at %p\n", this));
}

void HttpRequestAndroid::Ref() {
  // Atomically increment reference count.
  ref_count_.Ref();
  LOG(("Inc ref_count_ to %d, called by %p\n",
       ref_count_.Value(),
       __builtin_return_address(0)));
}

void HttpRequestAndroid::Unref() {
  // It is dangerous to decrement the reference count on a background
  // thread, as this may invoke the destructor. Destruction is only
  // safe if performed on the main thread.
  assert(IsMainThread());
  // Atomically decrement reference count, self-deleting if zero.
  if (ref_count_.Unref()) {
    LOG(("Dec ref_count_ to 0, deleting, called by %p\n",
         __builtin_return_address(0)));
    delete this;
  } else {
    LOG(("Dec ref_count_ to %d, called by %p\n",
         ref_count_.Value(),
         __builtin_return_address(0)));
  }
}

void HttpRequestAndroid::StartChild() {
  assert(IsMainThread());
  assert(!child_thread_.IsRunning());
  child_thread_.Start();
  assert(child_thread_.IsRunning());
  LOG(("child:%p\n", reinterpret_cast<void *>(child_thread_.GetThreadId())));
}

void HttpRequestAndroid::StopChild() {
  assert(IsMainThread());
  assert(child_thread_.IsRunning());
  // Shut down the background thread before deletion.
  AndroidMessageLoop::Stop(child_thread_.GetThreadId());
  child_thread_.Join();
}

void HttpRequestAndroid::ThreadRun() {
  assert(IsChildThread());
  // Initialize the Thread instance in the Java object.
  JniGetEnv()->CallVoidMethod(java_object_.Get(),
                              GetMethod(JAVA_METHOD_INIT_CHILD_THREAD));
  // This runs in the background thread, started by the
  // constructor. Just run the message loop until we're told to
  // Stop().
  AndroidMessageLoop::Start();
}

HttpRequest::CachingBehavior HttpRequestAndroid::GetCachingBehavior() {
  return caching_behavior_;
}

bool HttpRequestAndroid::SetCachingBehavior(CachingBehavior behavior) {
  if (!(IsUninitialized() || IsOpen())) return false;
  caching_behavior_ = behavior;
  return true;
}

HttpRequest::RedirectBehavior HttpRequestAndroid::GetRedirectBehavior() {
  return redirect_behavior_;
}

bool HttpRequestAndroid::SetRedirectBehavior(RedirectBehavior behavior) {
  if (!(IsUninitialized() || IsOpen())) return false;
  redirect_behavior_ = behavior;
  return true;
}

bool HttpRequestAndroid::SetCookieBehavior(CookieBehavior behavior) {
if (!(IsUninitialized() || IsOpen())) return false;
  cookie_behavior_ = behavior;
  return true;
}

bool HttpRequestAndroid::GetReadyState(ReadyState *state) {
  *state = ready_state_;
  return true;
}

void HttpRequestAndroid::SetReadyState(ReadyState new_state) {
  // Only called by the main thread.
  assert(IsMainThread());
  LOG(("SetReadyState %d vs %d\n",
       static_cast<int>(new_state),
       static_cast<int>(ready_state_)));
  if (new_state > ready_state_) {
    ready_state_ = new_state;
    if (http_listener_) {
      // Call the listener to inform them our state increased.
      http_listener_->ReadyStateChanged(this);
    }
  }
}

bool HttpRequestAndroid::GetResponseBody(scoped_refptr<BlobInterface> *blob) {
  // Get the response body as a blob. Only valid if the transaction is
  // complete.
  assert(IsMainThread());
  assert(blob != NULL);
  MutexLock lock(&mutex_);
  if (!IsInteractiveOrComplete() || was_aborted_) {
    return false;
  }
  if (response_body_.get()) {
    if (response_body_->IsFinalized()) {
      LOG(("GetResponseBody returning %d bytes\n",
           static_cast<int>(response_body_->Length())));
      response_body_->CreateBlob(blob);
    } else {
      LOG(("Response not finalized\n"));
      return false;
    }
    return true;
  } else {
    LOG(("Response not received yet\n"));
    return false;
  }
}

bool HttpRequestAndroid::GetStatus(int *status) {
  // Get the status line and parse the response code from it.
  assert(status != NULL);
  std::string16 status_line;
  if (!GetStatusLine(&status_line)) {
    return false;
  }
  return ParseHttpStatusLine(status_line, NULL, status, NULL);
}

bool HttpRequestAndroid::GetStatusNoCheck(int *status) {
  // Get the status line and parse the response code from it.
  assert(status != NULL);
  std::string16 status_line;
  if (!GetStatusLineNoCheck(&status_line)) {
    return false;
  }
  return ParseHttpStatusLine(status_line, NULL, status, NULL);
}

bool HttpRequestAndroid::GetStatusText(std::string16 *status_text) {
  // Get the status line and parse the response message from it.
  assert(status_text != NULL);
  std::string16 status_line;
  if (!GetStatusLine(&status_line)) {
    return false;
  }
  return ParseHttpStatusLine(status_line, NULL, NULL, status_text);
}

bool HttpRequestAndroid::GetStatusLine(std::string16 *status_line) {
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  return GetStatusLineNoCheck(status_line);
}

bool HttpRequestAndroid::GetStatusLineNoCheck(std::string16 *status_line) {
  // Get the entire HTTP status line.
  assert(status_line != NULL);
  JavaLocalFrame frame;
  jobject ret = JniGetEnv()->CallObjectMethod(
      java_object_.Get(),
      GetMethod(JAVA_METHOD_GET_RESPONSE_LINE));
  if (status_line != NULL) {
    *status_line = JavaString(static_cast<jstring>(ret)).ToString16();
    return true;
  } else {
    return false;
  }
}

// Check if the Java object may have passed through some redirects in
// the background thread.
bool HttpRequestAndroid::WasRedirected() {
  return IsInteractiveOrComplete() && !was_aborted_ && was_redirected_;
}

// Get the final URL fetched after all redirects.
bool HttpRequestAndroid::GetFinalUrl(std::string16 *final_url) {
  assert(final_url != NULL);
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;

  if (was_redirected_) {
    *final_url = current_url_;
  } else {
    *final_url = initial_url_;
  }

  return true;
}

// Get the initial URL used before redirects.
bool HttpRequestAndroid::GetInitialUrl(std::string16 *initial_url) {
  MutexLock lock(&mutex_);
  assert(initial_url != NULL);
  *initial_url = initial_url_;
  return true;
}

bool HttpRequestAndroid::Open(const char16 *method,
                              const char16 *url,
                              bool async,
                              BrowsingContext *context) {
  // Context is unused.
  assert(IsMainThread());
  assert(!IsRelativeUrl(url));
  if (!IsUninitialized())
    return false;
  LOG(("Open called, %s\n", async ? "asynchronous" : "synchronous"));
  LOG(("URL: %s\n", String16ToUTF8(url).c_str()));
  asynchronous_ = async;
  redirect_count_ = 0;
  // Call the Java open() method.
  if (JniGetEnv()->CallBooleanMethod(java_object_.Get(),
                                     GetMethod(JAVA_METHOD_OPEN),
                                     JavaString(method).Get(),
                                     JavaString(url).Get())) {
    method_ = method;
    initial_url_ = url;
    current_url_ = url;
    SetReadyState(OPEN);
    // Set the user agent, otherwise this defaults to Java0. Non-fatal.
    std::string16 user_agent;
    if (BrowserUtils::GetUserAgentString(&user_agent)) {
      if (!SetRequestHeader(HttpConstants::kUserAgentHeader,
                            user_agent.c_str())) {
        LOG(("Couldn't set User-Agent\n"));
      }
    } else {
      LOG(("Couldn't get user agent string\n"));
    }
    return true;
  } else {
    LOG(("Open failed\n"));
    return false;
  }
}

bool HttpRequestAndroid::SetRequestHeader(const char16 *name,
                                          const char16 *value) {
  assert(IsMainThread());
  if (!IsOpen())
    return false;
  // The Java side is responsible for maintaining the request headers.
  if (ready_state_ < SENT) {
    JniGetEnv()->CallVoidMethod(java_object_.Get(),
                                GetMethod(JAVA_METHOD_SET_REQUEST_HEADER),
                                JavaString(name).Get(),
                                JavaString(value).Get());
    if (std::string16(name) == HttpConstants::kCacheControlHeader &&
        std::string16(value) == HttpConstants::kNoCache) {
      // "Cache-Control: no-cache". Disable caching.
      SetCachingBehavior(BYPASS_ALL_CACHES);
    }
    return true;
  } else {
    LOG(("SetRequestHeader failed. Connection already open.\n"));
    return false;
  }
}

std::string16 HttpRequestAndroid::GetRequestHeader(const char16 *name) {
  assert(IsMainThread());
  JavaLocalFrame frame;
  jobject string_object =
      JniGetEnv()->CallObjectMethod(java_object_.Get(),
                                    GetMethod(JAVA_METHOD_GET_REQUEST_HEADER),
                                    JavaString(name).Get());
  if (string_object != NULL) {
    return JavaString(static_cast<jstring>(string_object)).ToString16();
  } else {
    return std::string16();
  }
}

bool HttpRequestAndroid::Send(BlobInterface* blob) {
  // Initiate the actual connection attempt. The child thread does the
  // heavy lifting and blocking operations.  Set post data, which may
  // be empty.
  assert(IsMainThread());
  if (!IsOpen()) return false;
  if (IsPostOrPut()) {
    post_blob_.reset(blob ? blob : new EmptyBlob());
  } else if (blob) {
    return false;
  }

  if (was_aborted_) {
    return false;  // already aborted, no need to send
  }

  if (cookie_behavior_ == SEND_BROWSER_COOKIES) {
    // Get the cookie for the current URL.
    std::string16 cookie(GetCookieForUrl(current_url_));
    if (!cookie.empty()) {
      // A cookie exists. Set it as a request header.
      SetRequestHeader(HttpConstants::kCookieHeader, cookie.c_str());
    }
  }

  // Fire up the child thread.
  StartChild();

  // Start the state machine.
  SetState(STATE_MAIN_REQUEST);
  // Run the first iteration while we're still on the main thread.
  HandleStateMachine();

  if (!asynchronous_) {
    // This blocks until completion if open() was called with
    // synchronous mode. Manually iterate the message queue until we
    // receive a message with the resulting ready_state_ == COMPLETE.
    while (state_ != STATE_MAIN_IDLE) {
      LOG(("Running sync loop\n"));
      AndroidMessageLoop::RunOnce();
    }
  }
  return true;
}

void HttpRequestAndroid::SwitchToMainThreadState(State state) {
  assert(IsChildThread());
  LOG(("Transferring to main thread with state %s\n", GetStateName(state)));
  // The state_ member will change when this message is received on
  // the main thread. This ensures state_ only changes under the
  // control of the main thread.
  AsyncRouter::GetInstance()->CallAsync(
      main_thread_id_,
      new HttpThreadFunctor(this,
                            HttpThreadFunctor::TARGET_THREAD_MAIN,
                            state));
}

void HttpRequestAndroid::SwitchToChildThreadState(State state) {
  assert(IsMainThread());
  LOG(("Transferring to child thread with state %s\n", GetStateName(state)));
  // Set the new state_ before farming out work to the child.
  SetState(state);
  // Also send the new state_ to the child as a double-check. We can't
  // change state_ until the child explicitly hands control back to
  // the main thread.
  AsyncRouter::GetInstance()->CallAsync(
      child_thread_.GetThreadId(),
      new HttpThreadFunctor(this,
                            HttpThreadFunctor::TARGET_THREAD_CHILD,
                            state));
}

void HttpRequestAndroid::HandleStateMachine() {
  LOG(("Running %s thread in state %s\n",
       IsMainThread() ? "main" : "child",
       GetStateName(state_)));
  // Loop the state machine until the current thread is no longer in
  // control.
  for (;;) {
    switch (state_) {
      case STATE_MAIN_IDLE:
        assert(IsMainThread());
        // Exit the state machine loop.
        return;

      case STATE_MAIN_REQUEST:
        assert(IsMainThread());
        // Try using local sources. Remember whether this was local so
        // that we don't needlessly save the result back into a local
        // cache.
        served_locally_ = ServeLocally();
        if (served_locally_) {
          // Completely served locally. We can skip a few stages in
          // the state machine and continue on the main thread.
          SetState(STATE_MAIN_PARSE_HEADERS);
        } else {
          // Cache miss. We need to do a blocking operation on the
          // network, performed on the child thread.
          SwitchToChildThreadState(STATE_CHILD_CONNECT_TO_REMOTE);
          return;
        }
        break;

      case STATE_CHILD_CONNECT_TO_REMOTE:
        assert(IsChildThread());
        // Connect to the remote site. This will block.
        if (ConnectToRemote()) {
          // Success, continue the state machine normally.
          SwitchToMainThreadState(STATE_MAIN_CONNECTED);
        } else {
          // Failed. End the state machine.
          SwitchToMainThreadState(STATE_MAIN_COMPLETE);
        }
        return;

      case STATE_MAIN_CONNECTED:
        assert(IsMainThread());
        if (was_aborted_) {
          SetState(STATE_MAIN_COMPLETE);
          break;
        }
        // Increment ready state visible by observers to SENT.
        SetReadyState(SENT);
        // Child thread successfully connected to the remote site. If
        // there is POST data, send it now, otherwise skip straight.
        if (method_ == HttpConstants::kHttpPOST) {
          SwitchToChildThreadState(STATE_CHILD_POST);
          return;
        } else {
          SetState(STATE_MAIN_PARSE_HEADERS);
        }
        break;

      case STATE_CHILD_POST:
        assert(IsChildThread());
        if (SendPostData()) {
          // POST completed successfully, parse headers.
          SwitchToMainThreadState(STATE_MAIN_PARSE_HEADERS);
        } else {
          // POST failed, stop.
          SwitchToMainThreadState(STATE_MAIN_COMPLETE);
        }
        return;

      case STATE_MAIN_PARSE_HEADERS:
        assert(IsMainThread());
        if (was_aborted_) {
          SetState(STATE_MAIN_COMPLETE);
          break;
        }
        if (ParseHeaders()) {
          // Test for redirect.
          std::string16 redirect_url = GetRedirect();
          if (redirect_url.empty()) {
            // No redirect.
            // Increment ready state visible by observers to INTERACTIVE.
            SetReadyState(INTERACTIVE);
            // Perform the blocking data read operation on the child
            // thread.
            SwitchToChildThreadState(STATE_CHILD_RECEIVE);
            return;
          } else if (redirect_count_ < kRedirectLimit) {
            // Redirected.
            LOG(("Following redirect\n"));
            was_redirected_ = true;
            current_url_ = redirect_url;
            if (method_ == HttpConstants::kHttpPOST) {
              // Redirected POST requests should turn into GET.
              LOG(("Turning redirected POST into a GET\n"));
              method_ = HttpConstants::kHttpGET;
            }
            // Issue another open() and post a message to the child
            // thread to carry on again.
            if (JniGetEnv()->CallBooleanMethod(
                    java_object_.Get(),
                    GetMethod(JAVA_METHOD_OPEN),
                    JavaString(method_).Get(),
                    JavaString(current_url_).Get())) {
              // Start the state machine from the beginning again.
              SetState(STATE_MAIN_REQUEST);
            } else {
              LOG(("Couldn't open after redirected URL\n"));
              SetState(STATE_MAIN_COMPLETE);
            }
          } else {
            // It's a trap! Don't redirect any more, just stop.
            LOG(("Exceeded redirect limit (%d)\n", kRedirectLimit));
            // Just go to completion state.
            SetState(STATE_MAIN_COMPLETE);
          }
        } else {
          // Failed to parse headers.
          SetState(STATE_MAIN_COMPLETE);
        }
        break;

      case STATE_CHILD_RECEIVE:
        assert(IsChildThread());
        if (Receive()) {
          // Received all data correctly.
          SwitchToMainThreadState(STATE_MAIN_RECEIVED);
        } else {
          // Error in data receive. Stop.
          SwitchToMainThreadState(STATE_MAIN_COMPLETE);
        }
        return;

      case STATE_MAIN_RECEIVED: {
        assert(IsMainThread());
        if (was_aborted_) {
          SetState(STATE_MAIN_COMPLETE);
          break;
        }
        // Done, insert into cache if policy allows.
        if (served_locally_) {
          // Prevent re-saving something that either came out of cache
          // anyway, or came from LocalServer and we don't want to cache.
          LOG(("Was served locally, not inserting into cache\n"));
        } else {
          if (caching_behavior_ == USE_ALL_CACHES) {
            LOG(("Inserting payload into cache\n"));
            if (!InsertIntoCache()) {
              // Not fatal if this doesn't work.
              LOG(("Couldn't insert result into cache\n"));
            }
          }
        }
        // Successful completion!
        SetState(STATE_MAIN_COMPLETE);
        break;
      }

      case STATE_MAIN_COMPLETE:
        assert(IsMainThread());
        // Shut down the child thread now, otherwise we have to wait
        // until the last unreference, which may be a long time if it
        // is held by a JavaScript HttpRequest instance.
        StopChild();
        // Exit the state machine.
        SetState(STATE_MAIN_IDLE);
        // Increment ready state visible by observers to COMPLETE.
        SetReadyState(COMPLETE);
        break;

      default:
        LOG(("Unknown state %d\n", state_));
        assert(false);
        return;
    }
  }
}

bool HttpRequestAndroid::ServeLocally() {
  // Try to serve using local data first. This doesn't block
  // (significantly), and the cache can only be accessed from the main
  // thread, so run this right now on the main thread.
  if (caching_behavior_ == USE_ALL_CACHES) {
    if (UseLocalServerResult()) {
      LOG(("LocalServer hit for %s\n",
           String16ToUTF8(current_url_).c_str()));
      return true;
    } else if (UseCacheResult()) {
      LOG(("Cache hit for %s\n",
           String16ToUTF8(current_url_).c_str()));
      return true;
    } else {
      LOG(("No local hit for %s\n",
           String16ToUTF8(current_url_).c_str()));
      return false;
    }
  } else {
    // Not using LocalServer or cache.
    return false;
  }
}

bool HttpRequestAndroid::UseLocalServerResult() {
  LOG(("UseLocalServerResult\n"));
  // On success, the instance is setup to stream the result back to us
  // directly from LocalServer via receive(), and fills in the
  // headers.
  if (JniGetEnv()->CallBooleanMethod(
          java_object_.Get(),
          GetMethod(JAVA_METHOD_USE_LOCAL_SERVER_RESULT),
          JavaString(current_url_).Get())) {
    LOG(("Got result from local server\n"));
    return true;
  } else {
    LOG(("No result from local server\n"));
    return false;
  }
}

bool HttpRequestAndroid::UseCacheResult() {
  LOG(("UseCacheResult\n"));
  if (JniGetEnv()->CallBooleanMethod(
          java_object_.Get(),
          GetMethod(JAVA_METHOD_USE_CACHE_RESULT),
          JavaString(current_url_).Get())) {
    LOG(("Got result from cache\n"));
    return true;
  } else {
    LOG(("No result from cache\n"));
    return false;
  }
}

bool HttpRequestAndroid::SaveCacheResult() {
  LOG(("SaveCacheResult\n"));
  if (JniGetEnv()->CallBooleanMethod(
          java_object_.Get(),
          GetMethod(JAVA_METHOD_SAVE_CACHE_RESULT))) {
    LOG(("Saved payload into cache\n"));
    return true;
  } else {
    LOG(("Couldn't save payload into cache!\n"));
    return false;
  }
}

// static
bool HttpRequestAndroid::IsMainThread() {
  ThreadId current_thread =
      ThreadMessageQueue::GetInstance()->GetCurrentThreadId();
  return current_thread == main_thread_id_;
}

bool HttpRequestAndroid::IsChildThread() const {
  ThreadId current_thread =
      ThreadMessageQueue::GetInstance()->GetCurrentThreadId();
  return current_thread == child_thread_.GetThreadId();
}

bool HttpRequestAndroid::InsertIntoCache() {
  LOG(("InsertIntoCache\n"));
  JNIEnv *env = JniGetEnv();
  int status_code;
  if (!GetStatusNoCheck(&status_code)) {
    LOG(("No status code! Can't cache that!\n"));
    return false;
  }
  if (!env->CallBooleanMethod(
          java_object_.Get(),
          GetMethod(JAVA_METHOD_CREATE_CACHE_RESULT),
          JavaString(current_url_).Get(),
          status_code,
          JavaString(mime_type_).Get(),
          JavaString(encoding_).Get())) {
    LOG(("createCacheResult() failed\n"));
    return false;
  }
  // Allocate a byte array to send from. Deletes at end of scope.
  JavaGlobalRef<jbyteArray> byte_array(env->NewByteArray(kTransferSize));
  // Pin the byte array so we can write to it.
  jboolean is_copy;
  jbyte *pinned_array = env->GetByteArrayElements(byte_array.Get(), &is_copy);
  assert(pinned_array != NULL);
  int64 pos = 0;
  bool success;
  for (;;) {
    int64 to_send = response_body_->Read(
        reinterpret_cast<uint8 *>(pinned_array),
        pos,
        kTransferSize);
    if (is_copy) {
      // Sync the copy to the VM, but don't free.
      env->ReleaseByteArrayElements(byte_array.Get(), pinned_array, JNI_COMMIT);
    }
    if (to_send == 0) {
      LOG(("Sent all of the body to cache\n"));
      success = true;
      break;
    } else if (to_send < 0) {
      LOG(("Error sending body to cache\n"));
      success = false;
      break;
    } else {
      if (env->CallBooleanMethod(
              java_object_.Get(),
              GetMethod(JAVA_METHOD_APPEND_CACHE_RESULT),
              byte_array.Get(),
              static_cast<int>(to_send))) {
        pos += to_send;
        // Loop.
      } else {
        LOG(("Sending body to cache failed\n"));
        success = false;
        break;
      }
    }
  }
  // Unpin the array.
  env->ReleaseByteArrayElements(byte_array.Get(), pinned_array, 0);
  if (!success) {
    LOG(("Failed to send entire body to cache\n"));
    return false;
  }
  if (!SaveCacheResult()) {
    LOG(("saveCacheResult() failed\n"));
    return false;
  }
  LOG(("InsertIntoCache succeeded\n"));
  return true;
}

bool HttpRequestAndroid::ConnectToRemote() {
  if (JniGetEnv()->CallBooleanMethod(
          java_object_.Get(),
          GetMethod(JAVA_METHOD_CONNECT_TO_REMOTE))) {
    LOG(("ConnectToRemote succeeded\n"));
    return true;
  } else {
    LOG(("ConnectToRemote failed\n"));
    return false;
  }
}

bool HttpRequestAndroid::SendPostData() {
  // Send the entire post blob.
  JNIEnv *env = JniGetEnv();
  // Allocate a byte array to send from. Deletes at end of scope.
  JavaGlobalRef<jbyteArray> byte_array(env->NewByteArray(kTransferSize));
  // Pin the byte array so we can write to it in-place.
  jboolean is_copy;
  jbyte *pinned_array = env->GetByteArrayElements(byte_array.Get(), &is_copy);
  assert(pinned_array != NULL);
  bool success;
  int64 pos = 0;
  for (;;) {
    // Checkpoint abort status.
    if (was_aborted_) {
      success = false;
      break;
    }
    int64 to_send = post_blob_->Read(reinterpret_cast<uint8 *>(pinned_array),
                                     pos,
                                     kTransferSize);
    if (is_copy) {
      // Sync the copy to the VM, but don't free.
      env->ReleaseByteArrayElements(byte_array.Get(), pinned_array, JNI_COMMIT);
    }
    if (to_send == 0) {
      LOG(("Sent all post data\n"));
      success = true;
      break;
    } else if (to_send < 0) {
      LOG(("Error reading post data\n"));
      success = false;
      break;
    } else {
      if (env->CallBooleanMethod(java_object_.Get(),
                                 GetMethod(JAVA_METHOD_SEND_POST_DATA),
                                 byte_array.Get(),
                                 static_cast<int>(to_send))) {
        pos += to_send;
        // Loop.
      } else {
        LOG(("Sending post data failed\n"));
        success = false;
        break;
      }
    }
  }
  // Close the output stream. Special values NULL, 0.
  env->CallBooleanMethod(java_object_.Get(),
                         GetMethod(JAVA_METHOD_SEND_POST_DATA),
                         NULL,
                         0);
  // Unpin the array.
  env->ReleaseByteArrayElements(byte_array.Get(), pinned_array, 0);
  return success;
}

bool HttpRequestAndroid::ParseHeaders() {
  if (!served_locally_) {
    // Parse the headers that came from the remote connection. If
    // locally served, the headers are already parsed on the Java
    // side.
    if (!JniGetEnv()->CallBooleanMethod(
            java_object_.Get(),
            GetMethod(JAVA_METHOD_PARSE_HEADERS))) {
      LOG(("ParseHeaders failed\n"));
      return false;
    }
    LOG(("ParseHeaders succeeded, extracting MIME and encoding\n"));
  }

  // The content-type header contains this information.
  std::string16 content_type;
  if (GetResponseHeaderNoCheck(HttpConstants::kContentTypeHeader,
                               &content_type) &&
      !content_type.empty()) {
    std::string16 mime_type, encoding;
    if (ParseContentType(content_type, &mime_type, &encoding)) {
      mime_type_ = mime_type;
      encoding_ = encoding;
    } else {
      LOG(("Bad content-type header\n"));
      // Not fatal.
    }
  } else {
    LOG(("No content-type header\n"));
    // Still successful - just no header.
  }

  // Apply any set-cookie header.
  std::string16 set_cookie;
  if (GetResponseHeaderNoCheck(HttpConstants::kSetCookieHeader,
                               &set_cookie) &&
      !set_cookie.empty()) {
    SetCookieForUrl(current_url_, set_cookie);
    LOG(("Set cookie for %s \"%s\"\n",
         String16ToUTF8(current_url_).c_str(),
         String16ToUTF8(set_cookie).c_str()));
  } else {
    LOG(("No cookie for %s\n", String16ToUTF8(current_url_).c_str()));
  }
  return true;
}

// static
bool HttpRequestAndroid::ParseContentType(
    const std::string16 &content_type,
    std::string16 *mime_type,
    std::string16 *encoding) {
  /* The entry is of the form: "type/subtype[;parameter]...", for
   * example "text/html; charset=utf-8; base64". We'll ignore
   * non-default transfer encodings and leave it to the requester to
   * figure out, as they will only appear if explicitly asked for.
   */
#ifdef DEBUG
  std::string content_type8;
  String16ToUTF8(content_type.c_str(), &content_type8);
#endif
  std::vector<std::string16> parts;
  if (Tokenize(content_type, std::string16(STRING16(L";")), &parts) == 0) {
#ifdef DEBUG
    LOG(("Bad content-type header: %s\n", content_type8.c_str()));
#endif
    return false;
  }
  if (mime_type != NULL) {
    // Trim whitespace in MIME type.
    const char16 *strip_start = parts[0].c_str();
    int strip_len = static_cast<int>(parts[0].size());
    StripWhiteSpace(&strip_start, &strip_len);
    // Return the MIME type.
    *mime_type = std::string16(strip_start, strip_len);
#ifdef DEBUG
    std::string mime_type8;
    String16ToUTF8(mime_type->c_str(), &mime_type8);
    LOG(("MIME type: %s\n", mime_type8.c_str()));
#endif
  }
  // Extract each ";" separated part after the first, if they exist.
  std::string16 got_encoding;
  for (std::vector<std::string16>::size_type i = 1; i < parts.size(); ++i) {
    const std::string16 &part = parts[i];
    // Strip whitespace from the string.
    const char16 *strip_start = &part[0];
    int strip_len = static_cast<int>(part.size());
    StripWhiteSpace(&strip_start, &strip_len);
    std::string16 stripped(strip_start, strip_len);
    // Split into "key=value" parts.
    std::vector<std::string16> key_value;
    if (Tokenize(stripped, std::string16(STRING16(L"=")), &key_value) != 2) {
#ifdef DEBUG
      LOG(("Bad key=value part of content-type header: %s\n",
           content_type8.c_str()));
#endif
      // MIME type is still important. Don't totally fail.
      if (encoding != NULL) {
        *encoding = std::string16();
      }
      return true;
    }
    if (encoding != NULL) {
      *encoding = key_value[1];
#ifdef DEBUG
      std::string encoding8;
      String16ToUTF8(encoding->c_str(), &encoding8);
      LOG(("Encoding: %s\n", encoding8.c_str()));
#endif
    }
  }
  return true;
}

bool HttpRequestAndroid::Receive() {
  // Receive the entire stream into the ByteStore.
  JNIEnv *env = JniGetEnv();
  // Allocate a byte array to receive into. Deletes at end of scope.
  JavaGlobalRef<jbyteArray> byte_array(env->NewByteArray(kTransferSize));
  {
    MutexLock lock(&mutex_);
    response_body_.reset(new ByteStore());
  }
  int64_t total_bytes = 0;
  bool success;
  bool first_receive = true;
  for (;;) {
    if (was_aborted_) {
      success = false;
      break;
    }
    // Receive a block of data. This may be interrupted by an Abort().
    int got = env->CallIntMethod(java_object_.Get(),
                                 GetMethod(JAVA_METHOD_RECEIVE),
                                 byte_array.Get());
    if (JavaExceptionScope::Clear()) {
      // Workaround for a race in the Java side which causes a
      // java.lang.NullPointerException to be raised. This occurs if
      // our first call to receive() happens concurrently with network
      // connectivity being lost, resulting in a null InputStream. We
      // can catch the exception here, safely discard it, and assume
      // the connection went down mid-receive.
      LOG(("Discarding exception from HttpRequestAndroid.receive(byte[])\n"));
      success = false;
      break;
    } else if (got == 0) {
      LOG(("Receive complete, total bytes %lld\n", total_bytes));
      success = true;
      break;
    } else if (got < 0) {
      LOG(("Got error from receive() after %lld bytes\n", total_bytes));
      success = false;
      break;
    } else {
      assert(got <= kTransferSize);
      if (first_receive) {
        // Indicate to the parent thread that we're successfully streaming.
        first_receive = false;
        // TODO(jripley): Is this really necessary? The current policy
        // is to just go INTERACTIVE as soon as headers as parsed.
        // PostReadyState(INTERACTIVE);
      }
      // Copy the array data to the ByteStore.
      jbyte *pinned_array = env->GetByteArrayElements(byte_array.Get(), NULL);
      assert(pinned_array != NULL);
      {
        MutexLock lock(&mutex_);
        response_body_->AddData(pinned_array, got);
      }
      env->ReleaseByteArrayElements(byte_array.Get(), pinned_array, 0);
      total_bytes += got;
    }
  }
  MutexLock lock(&mutex_);
  if (success) {
    response_body_->Finalize();
    return true;
  } else {
    response_body_.reset(NULL);
    return false;
  }
}

std::string16 HttpRequestAndroid::GetRedirect() {
  // Returns the destination of a redirect, if any, and permitted by
  // the redirection policy.
  int status_code;
  if (!GetStatusNoCheck(&status_code)) {
    return std::string16();
  }
  if (status_code != 301 && status_code != 302) {
    // Not a redirection status code.
    LOG(("Status code %d is not a redirect\n", status_code));
    return std::string16();
  }
  // The redirect target URL is in the response header "location".
  std::string16 location;
  if (!GetResponseHeaderNoCheck(HttpConstants::kLocationHeader, &location) ||
      location.empty()) {
    LOG(("Redirect, but no Location header field\n"));
    return std::string16();
  }
  // Resolve the redirect URL as it may be relative.
  std::string16 redirect_url;
  LOG(("Resolving %s -> %s\n",
       String16ToUTF8(current_url_).c_str(),
       String16ToUTF8(location).c_str()));
  if (!ResolveAndNormalize(current_url_.c_str(),
                           location.c_str(),
                           &redirect_url)) {
    LOG(("Failed to resolve. Malformed.\n"));
    return std::string16();
  }
  LOG(("Resolved to %s\n", String16ToUTF8(redirect_url).c_str()));
  if (redirect_behavior_ == FOLLOW_NONE) {
    LOG(("Redirect to %s but policy disallows\n",
         String16ToUTF8(redirect_url).c_str()));
    return std::string16();
  } else if (redirect_behavior_ == FOLLOW_ALL) {
    LOG(("Redirect to %s, policy allows all\n",
         String16ToUTF8(redirect_url).c_str()));
    return redirect_url;
  } else {
    assert(redirect_behavior_ == FOLLOW_WITHIN_ORIGIN);
    if (IsSameOrigin(current_url_, redirect_url)) {
      LOG(("Redirect to %s, within same origin\n",
           String16ToUTF8(redirect_url).c_str()));
      return redirect_url;
    } else {
      LOG(("Cross-origin redirect to %s denied\n",
           String16ToUTF8(redirect_url).c_str()));
      return std::string16();
    }
  }
}

std::string16 HttpRequestAndroid::GetCookieForUrl(const std::string16 &url) {
  // Call a static helper function to call the CookieManager instance.
  JNIEnv *env = JniGetEnv();
  JavaLocalFrame frame;
  jobject java_string = env->CallStaticObjectMethod(
      java_class_.Get(),
      GetMethod(JAVA_METHOD_GET_COOKIE_FOR_URL),
      JavaString(url).Get());
  std::string16 result(
      JavaString(static_cast<jstring>(java_string)).ToString16());
#ifdef DEBUG
  std::string url8, result8;
  String16ToUTF8(url.c_str(), &url8);
  String16ToUTF8(result.c_str(), &result8);
  LOG(("URL:%s Cookie:%s\n", url8.c_str(), result8.c_str()));
#endif
  return result;
}

void HttpRequestAndroid::SetCookieForUrl(const std::string16 &url,
                                         const std::string16 &cookie) {
  // Call a static helper function to call the CookieManager instance.
  JniGetEnv()->CallStaticVoidMethod(
      java_class_.Get(),
      GetMethod(JAVA_METHOD_SET_COOKIE_FOR_URL),
      JavaString(url).Get(),
      JavaString(cookie).Get());
}

std::string16 HttpRequestAndroid::GetResponseCharset() {
  assert(IsMainThread());
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  // Extracted from the Content-Type header.
  return encoding_;
}

bool HttpRequestAndroid::GetAllResponseHeaders(std::string16 *out_string) {
  assert(IsMainThread());
  // Get a (big) string containing all response headers separated by
  // CR-LF, and ending with a blank line. This is implemented on the
  // Java side.
  assert(out_string != NULL);
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  JavaLocalFrame frame;
  jobject result = JniGetEnv()->CallObjectMethod(
      java_object_.Get(),
      GetMethod(JAVA_METHOD_GET_ALL_RESPONSE_HEADERS));
  return JavaString(static_cast<jstring>(result)).ToString16(out_string);
}

bool HttpRequestAndroid::GetResponseHeader(const char16* name,
                                           std::string16 *value) {
  assert(name != NULL);
  assert(value != NULL);
  if (!IsInteractiveOrComplete() || was_aborted_)
    return false;
  return GetResponseHeaderNoCheck(name, value);
}

bool HttpRequestAndroid::GetResponseHeaderNoCheck(const char16* name,
                                                  std::string16 *value) {
  // Get a single response header. This is implemented on the Java
  // side.
  assert(IsMainThread());
  JavaLocalFrame frame;
  jobject result = JniGetEnv()->CallObjectMethod(
      java_object_.Get(),
      GetMethod(JAVA_METHOD_GET_RESPONSE_HEADER),
      JavaString(name).Get());
  if (result != NULL) {
    return JavaString(static_cast<jstring>(result)).ToString16(value);
  } else {
    value->clear();
    return true;
  }
}

bool HttpRequestAndroid::Abort() {
  LOG(("Abort\n"));
  assert(IsMainThread());

  // Signal the child thread that it should stop any long term
  // blocking operations, e.g receiving or sending bulk data.
  was_aborted_ = true;
  // Wait for the state machine to go idle.  Unfortunately, the
  // java.net.* operations are not easy to atomically interrupt, so we
  // need to resort to an ugly back-off-retry loop.
  int sleep_period_ms = 10;
  // state_ is only changed by the main thread, which gives us a good
  // rendezvous point to know that the child has handed us back
  // control.
  while (state_ != STATE_MAIN_IDLE) {
    LOG(("Waiting for idle\n"));
    // Interrupt the Java-side in case it's stuck in one of the
    // java.net.* methods.
    LOG(("Calling interrupt()\n"));
    JniGetEnv()->CallVoidMethod(java_object_.Get(),
                                GetMethod(JAVA_METHOD_INTERRUPT));
    // Process the message queue.
    while (AndroidMessageLoop::RunOnceWithTimeout(sleep_period_ms)) {
      // No-op. Loop until out of messages and timed out, then resort
      // to another interrupt.
    }
    // Double the sleep period up to a maximum of 1.28 seconds.
    if (sleep_period_ms < 1280) {
      sleep_period_ms *= 2;
    }
  }
  SetReadyState(COMPLETE);
  LOG(("Abort complete\n"));
  return true;
}

bool HttpRequestAndroid::SetListener(HttpListener *listener,
                                     bool enable_data_available) {
  assert(IsMainThread());
  MutexLock lock(&mutex_);
  http_listener_ = listener;
  // TODO(jripley): listener_enable_data_available_ not implemented.
  listener_enable_data_available_ = enable_data_available;
  return true;
}

bool HttpRequestAndroid::IsSameOrigin(const std::string16 &from,
                                      const std::string16 &to) {
  // Check if two URLs are from the same origin using SecurityOrigin.
#ifdef DEBUG
  std::string from8, to8;
  String16ToUTF8(from.c_str(), &from8);
  String16ToUTF8(to.c_str(), &to8);
#endif
  // Initialize both origins.
  SecurityOrigin from_origin;
  if (!from_origin.InitFromUrl(from.c_str())) {
#ifdef DEBUG
    LOG(("'From' origin is bad: \"%s\"\n", from8.c_str()));
#endif
    return false;
  }
  SecurityOrigin to_origin;
  if (!to_origin.InitFromUrl(to.c_str())) {
#ifdef DEBUG
    LOG(("'To' origin is bad: \"%s\"\n", to8.c_str()));
#endif
    return false;
  }
  // Check the origins match.
  if (from_origin.IsSameOrigin(to_origin)) {
#ifdef DEBUG
    LOG(("%s is the same origin as %s\n", from8.c_str(), to8.c_str()));
#endif
    return true;
  } else {
#ifdef DEBUG
    LOG(("%s not the same origin as %s\n", from8.c_str(), to8.c_str()));
#endif
    return false;
  }
}

#ifdef DEBUG
// static
const char *HttpRequestAndroid::GetStateName(State state) {
  // Convert the State enumeration to a human-readable debug string.
  // Ensure that this array matches the enumeration.
  static const char *const state_names[] = {
    "Main thread idle",
    "Main thread request",
    "Child thread connect to remote",
    "Main thread connected",
    "Child thread POST",
    "Main thread parse headers",
    "Child thread receive data",
    "Main thread received all data",
    "Main thread complete"
  };
  assert(NELEM(state_names) == STATE_COUNT);
  if (state >= STATE_COUNT) {
    LOG(("Bad state %d\n", static_cast<int>(state)));
    assert(false);
  }
  return state_names[static_cast<int>(state)];
}
#endif

// static
bool HttpRequestAndroid::InitJni() {
  // This is called by the main thread. Remember its thread ID.
  main_thread_id_ = ThreadMessageQueue::GetInstance()->GetCurrentThreadId();
  JNIEnv *env = JniGetEnv();
  // Find our Java class that does most of the heavy lifting.
  if (!java_class_.FindClass(kHttpRequestAndroidClassName)) {
    LOG(("Couldn't get HttpRequestAndroid class\n"));
    return false;
  }
  // Grab the method IDs for all the calls we need.
  assert(NELEM(java_methods_) == JAVA_METHOD_COUNT);
  if (!java_class_.GetMultipleMethodIDs(java_methods_, NELEM(java_methods_))) {
    LOG(("Couldn't get HttpRequestAndroid methods\n"));
    return false;
  }
#ifdef DEBUG
  // Turn on logging in the Java class.
  env->CallStaticVoidMethod(java_class_.Get(),
                            GetMethod(JAVA_METHOD_ENABLE_LOGGING),
                            static_cast<jboolean>(true));
#endif
  return true;
}
