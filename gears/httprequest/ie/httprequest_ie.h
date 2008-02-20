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

#ifndef GEARS_HTTPREQUEST_IE_HTTPREQUEST_IE_H__
#define GEARS_HTTPREQUEST_IE_HTTPREQUEST_IE_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/localserver/common/http_request.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"
#include "ie/genfiles/interfaces.h" // from OUTDIR

//------------------------------------------------------------------------------
// GearsHttpRequest
//------------------------------------------------------------------------------
class ATL_NO_VTABLE GearsHttpRequest
    : public ModuleImplBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsHttpRequest>,
      public IDispatchImpl<GearsHttpRequestInterface>,
      public HttpRequest::ReadyStateListener,
      public JsEventHandlerInterface {
 public:
  BEGIN_COM_MAP(GearsHttpRequest)
    COM_INTERFACE_ENTRY(GearsHttpRequestInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsHttpRequest)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  // End boilerplate code. Begin interface.

  // need a default constructor to CreateInstance objects in IE
  GearsHttpRequest();
  virtual ~GearsHttpRequest();

  // GearsHttpRequestInterface
  // This is the interface we expose to JavaScript.
  // Note: lifted from the midl generated .h file

  virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_onreadystatechange( 
      /* [in] */ VARIANT *handler);
  
  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_onreadystatechange( 
      /* [retval][out] */ VARIANT *handler);
  
  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_readyState( 
      /* [retval][out] */ int *state);
  
  virtual HRESULT STDMETHODCALLTYPE open( 
      /* [in] */ const BSTR method,
      /* [in] */ const BSTR url,
      /* [optional][in] */ const VARIANT *async);
  
  virtual HRESULT STDMETHODCALLTYPE setRequestHeader( 
      /* [in] */ const BSTR header,
      /* [in] */ const BSTR value);
  
  virtual HRESULT STDMETHODCALLTYPE send( 
      /* [optional][in] */ const VARIANT *data);
  
  virtual HRESULT STDMETHODCALLTYPE abort(void);
  
  virtual HRESULT STDMETHODCALLTYPE getAllResponseHeaders( 
      /* [retval][out] */ BSTR *headers);
  
  virtual HRESULT STDMETHODCALLTYPE getResponseHeader( 
      /* [in] */ const BSTR headerName,
      /* [retval][out] */ BSTR *headerValues);
  
  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_responseText( 
      /* [retval][out] */ BSTR *body);

#ifdef WINCE
  // No BLOB support on WINCE yet
#else
#ifndef OFFICIAL_BUILD
  // The blob API has not been finalized for official builds
  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_responseBlob( 
      /* [retval][out] */ IUnknown **blob);
#endif  // not OFFICIAL_BUILD
#endif  // WINCE
  
  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_status( 
      /* [retval][out] */ int *statusCode);

  virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_statusText(
      /* [retval][out] */ BSTR *statusText);

 private:
  CComPtr<IDispatch> onreadystatechangehandler_;
  HttpRequest *request_;

  // Null if response_text_ has not yet been cached
  scoped_ptr<std::string16> response_text_;

#ifdef WINCE
  // BLOB not yet supported on WINCE
#else
#ifndef OFFICIAL_BUILD
  // The blob API has not been finalized for official builds
  // Valid only after a successful get_responseBlob()
  CComPtr<GearsBlobInterface> response_blob_;
#endif  // not OFFICIAL_BUILD
#endif  // WINCE

  bool content_type_header_was_set_;
  bool has_fired_completion_event_;
  scoped_ptr<JsEventMonitor> unload_monitor_;

  void CreateRequest();
  void ReleaseRequest();

  HttpRequest::ReadyState GetState();
  bool IsUninitialized() { return GetState() == HttpRequest::UNINITIALIZED; }
  bool IsOpen()          { return GetState() == HttpRequest::OPEN; }
  bool IsSent()          { return GetState() == HttpRequest::SENT; }
  bool IsInteractive()   { return GetState() == HttpRequest::INTERACTIVE; }
  bool IsComplete()      { return GetState() == HttpRequest::COMPLETE; }
  bool IsValidResponse();

  bool ResolveUrl(const char16 *url, std::string16 *resolved_url,
                  std::string16 *exception_message);

  // HttpRequest::ReadyStateListener impl
  virtual void DataAvailable(HttpRequest *source);
  virtual void ReadyStateChanged(HttpRequest *source);

  // JsEventHandlerInterface impl
  virtual void HandleEvent(JsEventType event_type);

  DISALLOW_EVIL_CONSTRUCTORS(GearsHttpRequest);
};


#endif  // GEARS_HTTPREQUEST_IE_HTTPREQUEST_IE_H__
