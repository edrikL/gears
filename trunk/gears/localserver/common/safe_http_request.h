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

#ifndef GEARS_LOCALSERVER_COMMON_SAFE_HTTP_REQUEST_H__
#define GEARS_LOCALSERVER_COMMON_SAFE_HTTP_REQUEST_H__


#include "gears/base/common/async_router.h"
#include "gears/base/common/common.h"
#include "gears/base/common/http_utils.h"
#include "gears/base/common/message_queue.h"
#include "gears/base/common/scoped_refptr.h"
#ifndef OFFICIAL_BUILD
#include "gears/blob/blob_interface.h"
#endif
#include "gears/localserver/common/http_request.h"
#include "third_party/scoped_ptr/scoped_ptr.h"


// "Safe" means that it can be used from a background thread. "Safe" does not
// mean that an single instance can be used from multiple threads of control.
//
// Specifically, compare this to the Firefox port's native HttpRequest,
// which can only be used from the main (safe) thread.
//
// For browsers that have this constraint, a SafeHttpRequest can co-ordinate
// with the safe thread to execute a native HttpRequest on a background thread's
// behalf. Clients should use HttpRequest::CreateSafeRequest factory method
// to create an instance of an object that implements the HttpRequest interface
// and is safe to use from any thread. However clients should not assume that
// the instance returned is an instance of this class. The "native" HttpRequest
// class for some ports work equally well on background and foreground threads.
//
// This helper class is effectively an implementation detail of the native
// HttpRequest implementation for a given port.

class SafeHttpRequest
    : public HttpRequest,
      public HttpRequest::ReadyStateListener {
 public:
  virtual void Ref() {
    ref_count_.Ref();
  }

  virtual void Unref() {
    if (ref_count_.Unref())
      delete this;
  }

  virtual bool Open(const char16 *method, const char16* url, bool async);

  virtual bool GetResponseBodyAsText(std::string16 *text);
  virtual bool GetResponseBody(std::vector<uint8> *body);
  virtual std::vector<uint8> *GetResponseBody();

  virtual bool GetAllResponseHeaders(std::string16 *headers);
  virtual bool GetResponseHeader(const char16* name, std::string16 *header);

  virtual bool GetStatus(int *status);
  virtual bool GetStatusText(std::string16 *status_text);
  virtual bool GetStatusLine(std::string16 *status_line);

  virtual bool WasRedirected();
  virtual bool GetFinalUrl(std::string16 *full_url);
  virtual bool GetInitialUrl(std::string16 *full_url);

  virtual bool SetRequestHeader(const char16 *name, const char16 *value);

  virtual bool Send();
#ifndef OFFICIAL_BUILD
  virtual bool SendBlob(BlobInterface* blob);
#endif  // OFFICIAL_BUILD
  virtual bool SendString(const char16 *name);

  virtual bool Abort();

  virtual bool GetReadyState(ReadyState *state);

  virtual CachingBehavior GetCachingBehavior();
  virtual bool SetCachingBehavior(CachingBehavior behavior);
  virtual RedirectBehavior GetRedirectBehavior();
  virtual bool SetRedirectBehavior(RedirectBehavior behavior);
  virtual bool SetOnReadyStateChange(HttpRequest::ReadyStateListener *listener);

 private:
  struct ResponseInfo {
    int status;
    bool was_redirected;
    std::string16 status_text;
    std::string16 status_line;
    std::string16 final_url;
    std::string16 headers;
    scoped_ptr<HTTPHeaders> parsed_headers;

    // Valid when the ReadyState is either INTERACTIVE or COMPLETE.
    std::string16 text;

    // Only valid when COMPLETE.
    scoped_ptr< std::vector<uint8> > body;

    ResponseInfo() : status(0), was_redirected(false) {}
  };

  struct RequestInfo {
    ReadyState ready_state;
    ReadyState upcoming_ready_state;
    CachingBehavior caching_behavior;
    RedirectBehavior redirect_behavior;
    std::string16 method;
    std::string16 full_url;
    std::vector< std::pair<std::string16, std::string16> > headers;

    std::string16 post_data_string;
#ifndef OFFICIAL_BUILD
    scoped_refptr<BlobInterface> post_data_blob;
#endif

    ResponseInfo response;

    RequestInfo()
        : ready_state(UNINITIALIZED),
          upcoming_ready_state(UNINITIALIZED),
          caching_behavior(USE_ALL_CACHES),
          redirect_behavior(FOLLOW_WITHIN_ORIGIN) {}
  };

  friend bool TestHttpRequest();
  friend bool HttpRequest::CreateSafeRequest(
                               scoped_refptr<HttpRequest>* request);
  
  SafeHttpRequest(ThreadId safe_thread_id);
  virtual ~SafeHttpRequest();

  bool IsSafeThread() {
     return safe_thread_id_ ==
        ThreadMessageQueue::GetInstance()->GetCurrentThreadId();
 }

  bool IsApartmentThread() {
    return apartment_thread_id_ ==
        ThreadMessageQueue::GetInstance()->GetCurrentThreadId();
  }

  // Apartment / Safe thread ping-pong methods.
  enum AsyncCallType {
    kSend,   // invoked from apartment to execute on safe thread
    kAbort,  // invoked from apartment to execute on safe thread
    kReadyStateChanged,  // invoked from safe to execute on apartment thread
    kDataAvailable  // invoked from safe to execute on apartment thread
  };
  bool CallAbortOnSafeThread();
  bool CallSendOnSafeThread();
  bool CallReadyStateChangedOnApartmentThread();
  bool CallDataAvailableOnApartmentThread();
  void OnAbortCall();
  void OnSendCall();
  void OnReadyStateChangedCall();
  void OnDataAvailableCall();

  // Async ping-pong support.
  typedef void (SafeHttpRequest::*Method)();
  class AsyncMethodCall;
  void CallAsync(ThreadId thread_id, Method method);

  // ReadyStateListener implementation.
  void DataAvailable(HttpRequest *source);
  void ReadyStateChanged(HttpRequest *source);

  // Other private methods
  ReadyState GetState();
  bool IsValidResponse();
  bool SendImpl();
  void CreateNativeRequest();
  void RemoveNativeRequest();

  // Terminology, the apartment thread is the thread of control that the
  // request instance was created on.
  //
  // The following members represent the state of the current request.
  // Methods of this class running in both the apartment and safe threads
  // access this shared state. The apartment thread manages the life-cycle
  // of the RequestInfo ptr.  
  Mutex request_info_lock_;
  RequestInfo request_info_;
  bool was_aborted_;
  bool was_sent_;

  RefCount ref_count_;
  HttpRequest::ReadyStateListener *listener_;

  // Used only on the safe thread 
  scoped_refptr<HttpRequest> native_http_request_;

  ThreadId safe_thread_id_;
  ThreadId apartment_thread_id_;

  DISALLOW_EVIL_CONSTRUCTORS(SafeHttpRequest);
};


#endif // GEARS_LOCALSERVER_COMMON_SAFE_HTTP_REQUEST_H__
