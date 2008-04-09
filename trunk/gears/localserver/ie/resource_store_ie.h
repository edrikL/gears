// Copyright 2006, Google Inc.
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

#ifndef GEARS_LOCALSERVER_IE_RESOURCE_STORE_IE_H__
#define GEARS_LOCALSERVER_IE_RESOURCE_STORE_IE_H__

#include <deque>
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/localserver/common/resource_store.h"
#include "gears/localserver/ie/capture_task_ie.h"
#include "genfiles/interfaces.h"

//------------------------------------------------------------------------------
// GearsResourceStore
//------------------------------------------------------------------------------
class ATL_NO_VTABLE GearsResourceStore
    : public ModuleImplBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsResourceStore>,
      public CWindowImpl<GearsResourceStore>,
      public IDispatchImpl<GearsResourceStoreInterface> {
 public:
  BEGIN_COM_MAP(GearsResourceStore)
    COM_INTERFACE_ENTRY(GearsResourceStoreInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsResourceStore)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT FinalConstruct();
  void FinalRelease();
  // End boilerplate code. Begin interface.

  // need a default constructor to CreateInstance objects in IE
  GearsResourceStore() {}

  // GearsResourceStoreInterface
  // This is the interface we expose to JavaScript.

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_name(
      /* [retval][out] */ BSTR *name);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_requiredCookie(
      /* [retval][out] */ BSTR *cookie);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_enabled(
      /* [retval][out] */ VARIANT_BOOL *enabled);

  virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_enabled(
      /* [in] */ VARIANT_BOOL enabled);

  virtual HRESULT STDMETHODCALLTYPE capture(
      /* [in] */ const VARIANT *urls,
      /* [in] */ VARIANT *completion_callback,
      /* [retval][out] */ long *capture_id);

  virtual HRESULT STDMETHODCALLTYPE abortCapture(
      /* [in] */ long capture_id);

  virtual HRESULT STDMETHODCALLTYPE isCaptured(
      /* [in] */ const BSTR url,
      /* [retval][out] */ VARIANT_BOOL *retval);

  virtual HRESULT STDMETHODCALLTYPE remove(
      /* [in] */ const BSTR url);

  virtual HRESULT STDMETHODCALLTYPE rename(
      /* [in] */ const BSTR src_url,
      /* [in] */ const BSTR dst_url);

  virtual HRESULT STDMETHODCALLTYPE copy(
      /* [in] */ const BSTR src_url,
      /* [in] */ const BSTR dst_url);

  virtual HRESULT STDMETHODCALLTYPE getHeader(
      /* [in] */ const BSTR url,
      /* [in] */ const BSTR header,
      /* [retval][out] */ BSTR *value);

  virtual HRESULT STDMETHODCALLTYPE getAllHeaders(
      /* [in] */ const BSTR url,
      /* [retval][out] */ BSTR *all_headers);

#ifdef WINCE
  // No BLOB support on WINCE yet
#else
#ifdef OFFICIAL_BUILD
  // Blob support is not ready for prime time yet
#else
  virtual HRESULT STDMETHODCALLTYPE captureBlob(
      /* [in] */ IUnknown *blob,
      /* [in] */ const BSTR url);
#endif  // OFFICIAL_BUILD
#endif  // WINCE

  virtual HRESULT STDMETHODCALLTYPE captureFile(
      /* [in] */ IDispatch *file_input_element,
      /* [in] */ const BSTR url);

  virtual HRESULT STDMETHODCALLTYPE getCapturedFileName(
      /* [in] */ const BSTR url,
      /* [retval][out] */ BSTR *file_name);

#ifdef WINCE
  // It looks like it will be difficult/impossible to implement FileSubmitter
  // for WinCE. This API will likely be deprecated once we have binary POST
  // functionality for HttpRequest, so we won't implement it for WinCE, at least
  // for now.
#else
  virtual HRESULT STDMETHODCALLTYPE createFileSubmitter(
      /* [retval][out] */ GearsFileSubmitterInterface **file_submitter);
#endif

 private:
  // CWindowImpl related members to receive messages from our CaptureTasks
  static const int kCaptureTaskMessageBase = WM_USER;
  static const int
      WM_CAPTURE_TASK_COMPLETE = CaptureTask::CAPTURE_TASK_COMPLETE
                                 + kCaptureTaskMessageBase;
  static const int
      WM_CAPTURE_URL_SUCCEEDED = CaptureTask::CAPTURE_URL_SUCCEEDED
                                 + kCaptureTaskMessageBase;
  static const int
      WM_CAPTURE_URL_FAILED = CaptureTask::CAPTURE_URL_FAILED
                              + kCaptureTaskMessageBase;

  BEGIN_MSG_MAP(GearsResourceStore)
    MESSAGE_HANDLER(WM_CAPTURE_TASK_COMPLETE, OnCaptureTaskComplete)
    MESSAGE_HANDLER(WM_CAPTURE_URL_SUCCEEDED, OnCaptureUrlComplete)
    MESSAGE_HANDLER(WM_CAPTURE_URL_FAILED, OnCaptureUrlComplete)
  END_MSG_MAP()

 private:  // BEGIN_MSG_MAP expansion switches to public scope
  LRESULT OnCaptureTaskComplete(UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam,
                                BOOL& bHandled);

  LRESULT OnCaptureUrlComplete(UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam,
                               BOOL& bHandled);

  HRESULT CreateWindowIfNeeded();

  // other private helper methods

  HRESULT ResolveAndAppendUrl(const char16 *url, IECaptureRequest *request);
  HRESULT ResolveUrl(const char16 *url, std::string16 *resolved_url);
  HRESULT StartCaptureTaskIfNeeded(bool fire_events_on_failure);
  void FireFailedEvents(IECaptureRequest* request);
  void FireEvent(IDispatch *handler, const char16 *url, bool success, int id);

  int next_capture_id_;
  std::deque<IECaptureRequest*> pending_requests_;
  scoped_ptr<IECaptureRequest> current_request_;
  scoped_ptr<CaptureTask> capture_task_;
  ResourceStore store_;
  // This flag is set to true until the first OnCaptureUrlComplete is called by 
  // the capture_task_. If current_request_ is aborted before it gets a chance
  // to begin the capture, OnCaptureUrlComplete will not be called for any of
  // its urls, which implies that none of the corresponding capture callbacks
  // will be called, either. To prevent this, we inspect this flag in
  // OnCaptureTaskComplete and, if set, we call FireFailedEvents for
  // current_request_.
  bool need_to_fire_failed_events_;

  friend class GearsLocalServer;

  DISALLOW_EVIL_CONSTRUCTORS(GearsResourceStore);
};


#endif  // GEARS_LOCALSERVER_IE_RESOURCE_STORE_IE_H__
