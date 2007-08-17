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

#ifndef GEARS_LOCALSERVER_IE_HTTP_REQUEST_IE_H__
#define GEARS_LOCALSERVER_IE_HTTP_REQUEST_IE_H__

#include "gears/base/ie/atl_headers.h"
#include "gears/localserver/common/http_request.h"
#include "gears/localserver/common/localserver_db.h"


class IEHttpRequest
    : public CComObjectRootEx<CComMultiThreadModel::ThreadModelNoCS>,
      public IBindStatusCallback,
      public IHttpNegotiate,
      public IServiceProviderImpl<IEHttpRequest>,
      public HttpRequest {
 public:
  // HttpRequest interface

  // refcounting
  virtual int AddReference();
  virtual int ReleaseReference();

  // Get or set whether to use or bypass caches, the default is USE_ALL_CACHES
  virtual CachingBehavior GetCachingBehavior() {
    return caching_behavior_;
  }

  virtual void SetCachingBehavior(CachingBehavior behavior) {
    if (IsUninitialized() || IsOpen()) {
      caching_behavior_ = behavior;
    }
  }

  // properties
  virtual bool GetReadyState(ReadyState *state);
  virtual bool GetResponseBodyAsText(std::string16 *text);
  virtual bool GetResponseBody(std::vector<uint8> *body);
  virtual std::vector<uint8> *GetResponseBody();
  virtual bool GetStatus(int *status);
  virtual bool GetStatusText(std::string16 *status_text);
  virtual bool GetStatusLine(std::string16 *status_line);

  virtual bool SetFollowRedirects(bool follow);
  virtual bool WasRedirected();
  virtual bool GetFinalUrl(std::string16 *full_url);
  virtual bool GetInitialUrl(std::string16 *full_url);

  // methods
  virtual bool Open(const char16 *method, const char16* url, bool async);
  virtual bool SetRequestHeader(const char16* name, const char16* value);
  virtual bool Send();
  virtual bool SendString(const char16 *data);
  virtual bool GetAllResponseHeaders(std::string16 *headers);
  virtual bool GetResponseHeader(const char16* name, std::string16 *header);
  virtual bool Abort();

  // events
  virtual bool SetOnReadyStateChange(ReadyStateListener *listener);

  // IE implementation specific

  BEGIN_COM_MAP(IEHttpRequest)
    COM_INTERFACE_ENTRY(IBindStatusCallback)
    COM_INTERFACE_ENTRY(IHttpNegotiate)
    COM_INTERFACE_ENTRY(IServiceProvider)
  END_COM_MAP()

  BEGIN_SERVICE_MAP(IEHttpRequest)
    SERVICE_ENTRY(__uuidof(IHttpNegotiate))
  END_SERVICE_MAP()

  IEHttpRequest();  
  HRESULT FinalConstruct();
  void FinalRelease();

  // IBindStatusCallback

  virtual HRESULT STDMETHODCALLTYPE OnStartBinding( 
      /* [in] */ DWORD dwReserved,
      /* [in] */ IBinding *pib);

  virtual HRESULT STDMETHODCALLTYPE GetPriority( 
      /* [out] */ LONG *pnPriority);

  virtual HRESULT STDMETHODCALLTYPE OnLowResource( 
      /* [in] */ DWORD reserved);

  virtual HRESULT STDMETHODCALLTYPE OnProgress( 
      /* [in] */ ULONG ulProgress,
      /* [in] */ ULONG ulProgressMax,
      /* [in] */ ULONG ulStatusCode,
      /* [in] */ LPCWSTR szStatusText);

  virtual HRESULT STDMETHODCALLTYPE OnStopBinding( 
      /* [in] */ HRESULT hresult,
      /* [unique][in] */ LPCWSTR szError);

  virtual HRESULT STDMETHODCALLTYPE GetBindInfo( 
      /* [out] */ DWORD *grfBINDF,
      /* [unique][out][in] */ BINDINFO *pbindinfo);

  virtual HRESULT STDMETHODCALLTYPE OnDataAvailable( 
      /* [in] */ DWORD grfBSCF,
      /* [in] */ DWORD dwSize,
      /* [in] */ FORMATETC *pformatetc,
      /* [in] */ STGMEDIUM *pstgmed);

  virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable( 
      /* [in] */ REFIID riid,
      /* [iid_is][in] */ IUnknown *punk);

  // IHttpNegotiate

  virtual HRESULT STDMETHODCALLTYPE BeginningTransaction( 
      /* [in] */ LPCWSTR szURL,
      /* [unique][in] */ LPCWSTR szHeaders,
      /* [in] */ DWORD dwReserved,
      /* [out] */ LPWSTR *pszAdditionalHeaders);

  virtual HRESULT STDMETHODCALLTYPE OnResponse( 
      /* [in] */ DWORD dwResponseCode,
      /* [unique][in] */ LPCWSTR szResponseHeaders,
      /* [unique][in] */ LPCWSTR szRequestHeaders,
      /* [out] */ LPWSTR *pszAdditionalRequestHeaders);

  // IServiceProvider

  virtual HRESULT STDMETHODCALLTYPE QueryService( 
      /* [in] */ REFGUID guidService,
      /* [in] */ REFIID riid,
      /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

 private:
  bool SendImpl();
  HRESULT OnRedirect(const char16 *redirect_url);
  void SetReadyState(ReadyState state);
  bool IsUninitialized() { return ready_state_ == HttpRequest::UNINITIALIZED; }
  bool IsOpen() { return ready_state_ == HttpRequest::OPEN; }
  bool IsSent() { return ready_state_ == HttpRequest::SENT; }
  bool IsInteractive() { return ready_state_ == HttpRequest::INTERACTIVE; }
  bool IsComplete() { return ready_state_ == HttpRequest::COMPLETE; }
  bool IsInteractiveOrComplete() { return IsInteractive() || IsComplete(); }
  bool IsPostOrPut() {
    return bind_verb_ == BINDVERB_POST ||
           bind_verb_ == BINDVERB_PUT;
  }

  // The (non-relative) request url
  std::string16 url_;

  // Whether to bypass caches
  CachingBehavior caching_behavior_;

  // The request method
  std::string16 method_;
  int bind_verb_;

  // The POST data
  std::string post_data_string_;

  // Additional request headers we've been asked to send with the request
  std::string16 additional_headers_;

  // Our XmlHttpRequest like ready state, 0 thru 4
  ReadyState ready_state_;

  // Whether this request was aborted
  bool was_aborted_;

  // Whether we should follow redirects
  bool follow_redirects_;

  // Whether or not we have been redirected
  bool was_redirected_;

  // If we've been redirected, the location of the redirect. If we experience
  // a chain of redirects, this will be the last in the chain upon completion.
  std::string16 redirect_url_;

  // Our listener
  ReadyStateListener *listener_;

  // We populate this structure with various pieces of response data:
  // status code, status line, headers, data
  WebCacheDB::PayloadInfo response_payload_;

  // The amount of data we've read into the response_payload_.data
  // Initially the stl vector is allocated to a large size. We keep
  // track of how much of that allocated space is actually used here. 
  size_t actual_data_size_;

  // URLMON object references
  CComPtr<IMoniker> url_moniker_;
  CComPtr<IBindCtx> bind_ctx_;
  CComPtr<IBinding> binding_;
};

#endif  // GEARS_LOCALSERVER_IE_HTTP_REQUEST_IE_H__
