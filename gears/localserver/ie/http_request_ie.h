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

  // properties
  virtual bool getReadyState(long *state);
  virtual bool getResponseBody(std::vector<unsigned char> *body);
  virtual std::vector<unsigned char> *getResponseBody();
  virtual bool getStatus(long *status);
  virtual bool getStatusText(std::string16 *status_text);
  virtual bool getStatusLine(std::string16 *status_line);

  virtual bool setFollowRedirects(bool follow);
  virtual bool wasRedirected();
  virtual bool getRedirectUrl(std::string16 *full_redirect_url);

  // methods
  virtual bool open(const char16 *method, const char16* url, bool async);
  virtual bool setRequestHeader(const char16* name, const char16* value);
  virtual bool send();
  virtual bool getAllResponseHeaders(std::string16 *headers);
  virtual bool getResponseHeader(const char16* name, std::string16 *header);
  virtual bool abort();

  // events
  virtual bool setOnReadyStateChange(ReadyStateListener *listener);

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
  HRESULT OnRedirect(const char16 *redirect_url);
  void SetReadyState(int state);

  // The url we've been asked to get
  std::string16 url_;

  // Additional request headers we've been asked to send with the request
  std::string16 additional_headers_;

  // Our XmlHttpRequest like ready state, 0 thru 4
  // TODO(michaeln): change the interface to just report 'complete'
  int ready_state_;

  // Whether this request has been sent
  bool was_sent_;

  // Whether this request is complete
  bool is_complete_;

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
