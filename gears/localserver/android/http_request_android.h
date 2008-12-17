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

#ifndef GEARS_LOCALSERVER_ANDROID_HTTP_REQUEST_ANDROID_H__
#define GEARS_LOCALSERVER_ANDROID_HTTP_REQUEST_ANDROID_H__

#include <assert.h>
#include "gears/base/android/java_class.h"
#include "gears/base/android/java_global_ref.h"
#include "gears/base/common/async_router.h"
#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/byte_store.h"
#include "gears/base/common/common_np.h"
#include "gears/base/common/message_queue.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/thread.h"
#include "gears/localserver/common/http_request.h"
#include "gears/localserver/common/progress_event.h"

class BrowsingContext;

class HttpRequestAndroid : public HttpRequest,
                           public ProgressEvent::Listener {
 public:
  virtual void Ref();
  virtual void Unref();
  virtual int GetRef() { return ref_count_.Value(); }

  // Initialization of JNI bindings, called by module.cc on startup.
  static bool InitJni();
  // Get or set whether to use or bypass caches, the default is
  // USE_ALL_CACHES. May only be set prior to calling Send.
  virtual CachingBehavior GetCachingBehavior();
  virtual bool SetCachingBehavior(CachingBehavior behavior);
  // Get or set the redirect behavior, the default is FOLLOW_ALL. May
  // only be set prior to calling Send.
  virtual RedirectBehavior GetRedirectBehavior();
  virtual bool SetRedirectBehavior(RedirectBehavior behavior);
  // Set whether browser cookies are sent with the request. The default
  // is SEND_BROWSER_COOKIES. May only be set prior to calling Send.
  virtual bool SetCookieBehavior(CookieBehavior behavior);
  // Get the current state of the HTTP connection.
  virtual bool GetReadyState(ReadyState *state);
  // Get the response body, wrapped in a blob.
  virtual bool GetResponseBody(scoped_refptr<BlobInterface> *blob);
  // Get the status code of a completed request, e.g 200.
  virtual bool GetStatus(int *status);
  // Get the status informational text, e.g "OK".
  virtual bool GetStatusText(std::string16 *status_text);
  // Get the entire status line, e.g "HTTP/1.1 200 OK".
  virtual bool GetStatusLine(std::string16 *status_line);
  // Whether or not this request has followed a redirect
  virtual bool WasRedirected();
  // Sets full_url to final location of the request, including any
  // redirects. Returns false on failure (e.g. final redirect url not
  // yet known).
  virtual bool GetFinalUrl(std::string16 *full_url);
  // Similar to GetFinalUrl, but retrieves the initial URL requested.
  // Always succeeds.
  virtual bool GetInitialUrl(std::string16 *full_url);
  // Open an HTTP connection with the given method (GET/POST) to the
  // destination resource "url" (e.g http://www.google.com). If async
  // is true, the proceeding call to Send() will initiate the transfer
  // but return without blocking. If false, Send() will not return
  // until the entire transfer is complete, or fails.
  virtual bool Open(const char16 *method,
                    const char16* url,
                    bool async,
                    BrowsingContext *context);
  // Set a header to send to the server with the request.
  virtual bool SetRequestHeader(const char16 *name, const char16 *value);
  // Get a header that was previous set in SetRequestHeader(). Returns
  // an empty string if not set.
  std::string16 GetRequestHeader(const char16 *name);
  // Initiate the HTTP connection. If in asynchronous mode, this will
  // immediately return without blocking. If synchronous, this will
  // block until completion or failure. If blob is non-NULL, send the
  // blob to the server (by POST) with the request.
  virtual bool Send(BlobInterface* blob);
  // Get the character set encoding the response body is using. This
  // is the value returned in a "Content-Type" header.
  virtual std::string16 GetResponseCharset();
  // Get all responses from the server, encoded as UTF-16, with key
  // and value separated by a colon, and entries separated by CR-LF
  // line breaks.
  virtual bool GetAllResponseHeaders(std::string16 *headers);
  // Get an individual response. Returns true and fills in *header if
  // found, otherwise returns false.
  virtual bool GetResponseHeader(const char16* name, std::string16 *header);
  // Abort a request currently in flight. Returns true if background
  // processing has been successfully halted before returning.
  virtual bool Abort();
  // Set a listening object which will be called on state
  // transitions. If enable_data_available is true, it will also be
  // called as data becomes available.
  virtual bool SetListener(HttpListener *listener,
                           bool enable_data_available);
  // Implements the ProgressEvent::Listener interface. This is called
  // on the main thread by an AsyncFunctor fired by
  // ProgressEvent::Update.
  virtual void OnUploadProgress(int64 position, int64 total);  
  // Helper function to parse a "Content-Type:" response haeader, also
  // used by UrlInterceptAndroid. Extracts the MIME type and encoding
  // from it. mime_type and/or encoding can be skipped by passing NULL
  // for the return pointer. Returns true if parsing was at least
  // partially successful and the returned values are valid.
  static bool ParseContentType(const std::string16 &content_type,
                               std::string16 *mime_type,
                               std::string16 *encoding);
  // Returns the cookie value corresponding with this URL, or an empty
  // string if none is set.
  static std::string16 GetCookieForUrl(const std::string16 &url);
  // Returns true if the URL we're coming from is in the same origin
  // as the URL we're going to. Used to check against cross-domain.
  static bool IsSameOrigin(const std::string16 &from, const std::string16 &to);

 private:
  // Worker thread which performs blocking network I/O operations.
  class HttpThread : public Thread {
   public:
    HttpThread(HttpRequestAndroid *instance) : instance_(instance) { }
   protected:
    virtual void Run() { instance_->ThreadRun(); LOG(("Child exiting."));}
   private:
    HttpRequestAndroid *instance_;
  };

  // The HttpRequest state machine. State always increases, with some
  // steps optional (e.g POST). State reverts to STATE_MAIN_IDLE after
  // completion of a request. The words 'MAIN' and 'CHILD' refer to
  // the thread that is in charge when in this state. It is asserted
  // in debug builds that only the main thread runs STATE_MAIN_* and
  // only the child thread runs STATE_CHILD_*.
  enum State {
    STATE_MAIN_IDLE = 0,                // Initialization or exit state.
    STATE_MAIN_REQUEST,                 // Entered on Send().
    STATE_CHILD_CONNECT_TO_REMOTE,      // Connect to remote site.
    STATE_MAIN_CONNECTED,               // Child thread finished connecting.
    STATE_CHILD_POST,                   // Child thread sending POST data.
    STATE_MAIN_PARSE_HEADERS,           // Parsing headers.
    STATE_CHILD_RECEIVE,                // Child thread receiving data.
    STATE_MAIN_RECEIVED,                // Child thread finished data phase.
    STATE_MAIN_COMPLETE,                // All data received, clean up.
    STATE_COUNT                         // Count of states.
  };
#ifdef DEBUG
  // Convert the State enumeration to a human-readable debug string.
  // Ensure that the enumeration matches the array of strings.
  static const char *GetStateName(State State);
#endif

  // Functor which executes a message on either the main or child
  // thread, on behalf of the other.
  class HttpThreadFunctor : public AsyncFunctor {
   public:
    enum TargetThread {
      TARGET_THREAD_MAIN = 0,
      TARGET_THREAD_CHILD,
    };

    HttpThreadFunctor(HttpRequestAndroid *instance,
                      TargetThread target,
                      State state)
        : instance_(instance),
          target_(target),
          new_state_(state) {
      if (target == TARGET_THREAD_MAIN) {
        // Avoid a potential race in the order of sending this message
        // and running the main thread message loop. This defers any
        // deletion that may occur on the main thread until after this
        // functor has been removed from the queue.
        instance_->Ref();
      }
    }
    virtual void Run() {
      switch (target_) {
        case TARGET_THREAD_MAIN:
          // If we are already aborted, any state transition set via a
          // functor comes too late so we ignore it. Transitions out
          // IDLE are only made directly on the main thread in Send().
          if (!instance_->was_aborted_) {
            // Set the state now that we're in the main thread.
            instance_->state_ = new_state_;
            instance_->HandleStateMachine();
          } else {
            assert(instance_->state_ == STATE_MAIN_IDLE);
          }
          // Now potentially delete the instance, if this functor
          // invoked final processing.
          instance_->Unref();
          break;
        case TARGET_THREAD_CHILD:
          // Lock the mutex before looking at was_aborted_;
          instance_->was_aborted_mutex_.Lock();
          if (!instance_->was_aborted_) {
            // The main thread should have set the state before
            // switching to the child thread's control.
            assert(instance_->state_ == new_state_);
            instance_->was_aborted_mutex_.Unlock();
            // Avoid holding a mutex while in the state machine.
            // This thread never changes the state directly. Instead
            // it will queue a state-changing-functor onto the main
            // thread. That will run and get ignored if the was_aborted_
            // flag was meanwhile set to true (see above).
            instance_->HandleStateMachine();
          } else {
            instance_->was_aborted_mutex_.Unlock();
          }
          break;
      }
    }

   private:
    HttpRequestAndroid *instance_;
    TargetThread target_;
    State new_state_;
  };

  // No public constructors. These Create() functions are used
  // instead.
  friend bool HttpRequest::Create(scoped_refptr<HttpRequest> *);
  friend bool HttpRequest::CreateSafeRequest(scoped_refptr<HttpRequest> *);
  HttpRequestAndroid();
  virtual ~HttpRequestAndroid();

  // Start the child thread. Cannot be invoked while a child thread is
  // already running.
  void StartChild();

  // Stop the child thread. Cannot be invoked if the child thread is
  // not already running.
  void StopChild();

  // Method invoked on the background thread. This invokes a message
  // loop and persists until explicitly quit by the main thread.
  void ThreadRun();

  // Set a new HTTP state, notifying listeners if progress was
  // made. Can only be called by the main thread.
  void SetReadyState(ReadyState new_state);

  // Update the state machine and transfer control to the main thread
  // by tickling its message loop.
  void SwitchToMainThreadState(State state);
  // Update the state machine and transfer control to the child thread
  // by tickling its message loop.
  void SwitchToChildThreadState(State state);
  // Run the state machine. Called by both main and child thread.
  // Each thread checks that it does not enter a state that it does
  // not own.
  void HandleStateMachine();
  // Attempt to serve the request locally, through LocalServer or
  // through the browser's cache. Returns true if successful, or false
  // if the request needs to go to the network.
  bool ServeLocally();
  // Attempt to perform the HTTP request using LocalServer. Returns
  // true on success, false if the URL is not present in LocalServer.
  bool UseLocalServerResult();
  // Insert the result of an HTTP request into the cache, using the
  // URL in initial_url_.
  bool InsertIntoCache();
  // Initiate an HTTP connection to the remote server in current_url_.
  bool ConnectToRemote();
  // After connecting, send the blob provided in Send() as POST data
  // to the server.
  bool SendPostData();
  // After receiving headers, parse them to split out status and all
  // key:value pairs.
  bool ParseHeaders();
  // Receive all data from the body of an HTTP request, into the
  // request_body_ ByteStore.
  bool Receive();
  // Returns the URL the previous HTTP request is redirecting us to,
  // or an empty string on error or if not a redirect response.
  std::string16 GetRedirect();
  // Sets the cookie value corresponding with this URL.
  void SetCookieForUrl(const std::string16 &url, const std::string16 &cookie);
  // Get the status code of a completed request, e.g 200. This is for
  // internal use and does not check the current state of the request.
  bool GetStatusNoCheck(int *status);
  // Get the entire status line, e.g "HTTP/1.1 200 OK". This is for
  // internal use and does not check the current state of the request.
  bool GetStatusLineNoCheck(std::string16 *status_line);
  // Get an individual response. Returns true and fills in *header if
  // found, otherwise returns false. This is for internal use and does
  // not check the current state of the request.
  bool GetResponseHeaderNoCheck(const char16* name, std::string16 *header);
  // Atomically check aborting_.
  bool CheckAborting();
  // TODO(jripley): This is copied from http_request_chrome.h, but
  // actually needs moving to the base class.
  bool IsUninitialized() { return ready_state_ == HttpRequest::UNINITIALIZED; }
  bool IsOpen() { return ready_state_ == HttpRequest::OPEN; }
  bool IsInteractive() { return ready_state_ == HttpRequest::INTERACTIVE; }
  bool IsComplete() { return ready_state_ == HttpRequest::COMPLETE; }
  bool IsInteractiveOrComplete() { return IsInteractive() || IsComplete(); }
  bool IsPostOrPut() {
    return method_ == HttpConstants::kHttpPOST ||
           method_ == HttpConstants::kHttpPUT;
  }
  // Call the Java useCacheResult() method on the main thread to
  // retrieve a result from the local cache.
  bool UseCacheResult();
  // Call the Java saveCacheResult() method on the main thread to save
  // the result into the local cache.
  bool SaveCacheResult();
  // Returns whether the currently executing thread is the main thread.
  static bool IsMainThread();
  // Returns whether the currently executing thread is the child thread.
  bool IsChildThread() const;
  // A helper function which sets the state_ member and also checks
  // that this is only performed by the main thread. The child thread
  // is not allowed to modify state_.
  void SetState(State state) {
    assert(IsMainThread());
    state_ = state;
  }

  // Method IDs. Must match the order in java_methods_.
  enum JavaMethod {
    JAVA_METHOD_CONSTRUCTOR = 0,
    JAVA_METHOD_ENABLE_LOGGING,
    JAVA_METHOD_INIT_CHILD_THREAD,
    JAVA_METHOD_SET_REQUEST_HEADER,
    JAVA_METHOD_GET_REQUEST_HEADER,
    JAVA_METHOD_OPEN,
    JAVA_METHOD_USE_LOCAL_SERVER_RESULT,
    JAVA_METHOD_USE_CACHE_RESULT,
    JAVA_METHOD_CREATE_CACHE_RESULT,
    JAVA_METHOD_APPEND_CACHE_RESULT,
    JAVA_METHOD_SAVE_CACHE_RESULT,
    JAVA_METHOD_CONNECT_TO_REMOTE,
    JAVA_METHOD_PARSE_HEADERS,
    JAVA_METHOD_RECEIVE,
    JAVA_METHOD_SEND_POST_DATA,
    JAVA_METHOD_GET_RESPONSE_LINE,
    JAVA_METHOD_GET_COOKIE_FOR_URL,
    JAVA_METHOD_SET_COOKIE_FOR_URL,
    JAVA_METHOD_GET_RESPONSE_HEADER,
    JAVA_METHOD_GET_ALL_RESPONSE_HEADERS,
    JAVA_METHOD_INTERRUPT,
    JAVA_METHOD_COUNT,
  };
  // Simple method ID array accessor.
  static jmethodID GetMethod(JavaMethod i) {
    assert(java_methods_[i].id != 0);
    return java_methods_[i].id;
  }

  // Container for the Java-side counterpart of this class.
  static JavaClass java_class_;
  // Array containing the Java method signatures and IDs, matching the
  // order in JavaMethodIndex.
  static JavaClass::Method java_methods_[JAVA_METHOD_COUNT];
  // The thread ID of the main thread. Some calls can only be
  // performed on this thread.
  static ThreadId main_thread_id_;

  // Reference count for this object. If decremented to zero, this
  // object will self-delete.
  RefCount ref_count_;
  // Short-term mutex for atomically updating non-static members.
  Mutex mutex_;
  // Child thread which performs blocking operations.
  HttpThread child_thread_;
  // Caching strategy, set by setCachingBehavior().
  CachingBehavior caching_behavior_;
  // Redirection policy, set by setRedirectBehavior().
  RedirectBehavior redirect_behavior_;
  // SendCookie policy, set by SetCookieBehavior().
  CookieBehavior cookie_behavior_;
  // The current state machine state. Only the main thread is allowed
  // to change this value, and only through SetState().
  State state_;
  // The externally visible state of the HTTP connection.
  ReadyState ready_state_;
  // The Java object instance.
  JavaGlobalRef<jobject> java_object_;
  // If non-NULL, this is an object that will be called whenever the
  // HTTP state (e.g ReadyState) changes.
  HttpListener *http_listener_;
  // If true, the listener will be notified as soon as data is first
  // available.
  bool listener_enable_data_available_;
  // If true, the current transfer direction is upload (POST). If
  // false, the current transfer direction is download (GET, or body
  // of POST). This is only modified in the main thread at a point
  // when there are no ProgressEvent messages on the queue.
  bool listener_in_post_;
  // The total number of bytes that will be by the child thread, or 0
  // if this cannot be determined.
  int64_t total_bytes_to_send_;
  // True if this is an asynchronous (non-blocking) request.
  bool asynchronous_;
  // True if Abort() was used on this instance at any point, false
  // otherwise.
  bool was_aborted_;
  // This mutex protects both was_aborted_ and state_, since
  // the latter is written from the main thread and read
  // concurrently from the child thread.
  Mutex was_aborted_mutex_;
  // The response body for a completed request. This is a ByteStore so
  // it can be easily wrapped in a BlobInterface.
  scoped_refptr<ByteStore> response_body_;
  // The Blob being posted during Send(), or NULL if not a POST
  // request.
  scoped_refptr<BlobInterface> post_blob_;
  // True if we were redirected along the way.
  bool was_redirected_;
  // True if the result was served out of cache or LocalServer, and
  // should not be saved back into the cache.
  bool served_locally_;
  // The number of times redirected. Used to prevent endless loops.
  int redirect_count_;
  // The HTTP request method, e.g GET or POST.
  std::string16 method_;
  // The initial URL fetched.
  std::string16 initial_url_;
  // The URL currently being fetched.
  std::string16 current_url_;
  // The MIME type for the received document, e.g "text/html".
  std::string16 mime_type_;
  // The encoding for the received document, e.g "utf-8".
  std::string16 encoding_;
};

#endif  // GEARS_LOCALSERVER_ANDROID_HTTP_REQUEST_ANDROID_H__
