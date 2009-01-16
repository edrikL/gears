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
//
// The Asyncronous Pluggable Protocol handler used to intercept
// HTTP and HTTPS requests and satisfy them from the WebCacheDB when
// appropiate.
//
// For an overview of APPs, see
// http://msdn.microsoft.com/workshop/networking/pluggable/overview/overview.asp
//
// This handler makes use of the passthruAPP framework to solve an issue with
// propertly intercepting 302 redirects from a network response to a resource
// that is in our cache. APPs have two techniques of deferring to the default
// handling for a request:
// 1) Return INET_E_USE_DEFAULT_PROTOCOLHANDLER from Start() or StartEx()
//    which causes URLMON to discard the APP, call the default instead.
// 2) Internally create an instance of the default handler and pass API
//    calls thru to it. Essentially wrap the default handler.
// When redirection occurs, generally a new APP is not started, a chain of
// redirects are handled within the context of a single URLMON transaction.
// This presents a problem for option 1 above where 302 responses will
// not be delivered back to the APP. The passthruAPP framework implements
// option 2 above and also wraps the upward flowing "sink" APIs. Our code
// monitors the sink API calls and detects when a 302 back into our cache
// occurs, then stops the default handling and intervenes to satisfy the
// request from our cache.

#ifndef GEARS_LOCALSERVER_IE_HTTP_HANDLER_APP_H__
#define GEARS_LOCALSERVER_IE_HTTP_HANDLER_APP_H__

#include "gears/base/common/security_model.h"
#include "gears/base/ie/atl_browser_headers.h"
#include "gears/localserver/common/localserver_db.h"
#include "gears/localserver/ie/http_handler_base.h"
#include "third_party/passthru_app/ProtocolCF.h"
#include "third_party/passthru_app/ProtocolImpl.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

class HttpHandlerAPP;

//------------------------------------------------------------------------------
// PassthruSink
// Defines a passthru framework required "Sink" object that forwards upward
// flowing API calls from the default handler to our outer layer.
//------------------------------------------------------------------------------
class PassthruSink
    : public PassthroughAPP::CInternetProtocolSinkWithSP<PassthruSink,
                                                         CComMultiThreadModel> {
 public:
  PassthruSink() : http_handler_(NULL) {}
  void SetHttpHandler(HttpHandlerAPP *handler) { http_handler_ = handler; }

  // The ReportProgress method is overriden to detect 302s back into our cache
  // and intervene to satisfy the request from our cache.
  STDMETHODIMP ReportProgress(
    /* [in] */ ULONG ulStatusCode,
    /* [in] */ LPCWSTR szStatusText);

 private:
  // Calling BaseClass::Foo() passes an API call thru to the caller provided
  // sink
  typedef PassthroughAPP::CInternetProtocolSinkWithSP<PassthruSink,
      CComMultiThreadModel> BaseClass;
  // A convenience pointer to our HttpHandlerAPP
  HttpHandlerAPP *http_handler_;
};


// A "StartPolicy" to satisfy the CInternetProtocol<> template
typedef PassthroughAPP::CustomSinkStartPolicy<HttpHandlerAPP, PassthruSink>
    PassthruStartPolicy;


//------------------------------------------------------------------------------
// HttpHandlerAPP
//  An Asynchronous Pluggable Protocol that we register as an http/https
//  namespace handler to satisfy http requests with the contents or our cache.
//------------------------------------------------------------------------------
class HttpHandlerAPP
    : public HttpHandlerBase,
      public PassthroughAPP::CInternetProtocol<PassthruStartPolicy,
                                               CComMultiThreadModel> {
 public:
  // Registers and unregisters our handler in the http/https namespace.
  static HRESULT Install();
  static HRESULT Uninstall();

  HttpHandlerAPP();
  ~HttpHandlerAPP();

  HRESULT FinalConstruct();

  // This class implements all of the interfaces supported by an
  // Asyncronous Pluggable Protocol. Most of the methods are simple
  // boilerplate to conditionally pass the API call through to the
  // default handler or to execute a no-op stub.  For those methods
  // that require real work on our part when actually handling a
  // request, we call a private <<>>Impl method.

  // IInternetProtocolEx
  STDMETHODIMP StartEx(IUri *pUri,
    IInternetProtocolSink *pOIProtSink,
    IInternetBindInfo *pOIBindInfo,
    DWORD grfPI,
    HANDLE_PTR dwReserved);

  // IInternetProtocolRoot
  STDMETHODIMP Start(
    /* [in] */ LPCWSTR szUrl,
    /* [in] */ IInternetProtocolSink *pOIProtSink,
    /* [in] */ IInternetBindInfo *pOIBindInfo,
    /* [in] */ DWORD grfPI,
    /* [in] */ HANDLE_PTR dwReserved);

  STDMETHODIMP Continue(
    /* [in] */ PROTOCOLDATA *pProtocolData);

  STDMETHODIMP Abort(
    /* [in] */ HRESULT hrReason,
    /* [in] */ DWORD dwOptions);

  STDMETHODIMP Terminate(
    /* [in] */ DWORD dwOptions);

  STDMETHODIMP Suspend();

  STDMETHODIMP Resume();

  // IInternetProtocol
  STDMETHODIMP Read(
    /* [in, out] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead);

  STDMETHODIMP Seek(
    /* [in] */ LARGE_INTEGER dlibMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULARGE_INTEGER *plibNewPosition);

  STDMETHODIMP LockRequest(
    /* [in] */ DWORD dwOptions);

  STDMETHODIMP UnlockRequest();

  // IInternetProtocolInfo
  STDMETHODIMP ParseUrl(
    /* [in] */ LPCWSTR pwzUrl,
    /* [in] */ PARSEACTION ParseAction,
    /* [in] */ DWORD dwParseFlags,
    /* [out] */ LPWSTR pwzResult,
    /* [in] */ DWORD cchResult,
    /* [out] */ DWORD *pcchResult,
    /* [in] */ DWORD dwReserved);

  STDMETHODIMP CombineUrl(
    /* [in] */ LPCWSTR pwzBaseUrl,
    /* [in] */ LPCWSTR pwzRelativeUrl,
    /* [in] */ DWORD dwCombineFlags,
    /* [out] */ LPWSTR pwzResult,
    /* [in] */ DWORD cchResult,
    /* [out] */ DWORD *pcchResult,
    /* [in] */ DWORD dwReserved);

  STDMETHODIMP CompareUrl(
    /* [in] */ LPCWSTR pwzUrl1,
    /* [in] */ LPCWSTR pwzUrl2,
    /* [in] */ DWORD dwCompareFlags);

  STDMETHODIMP QueryInfo(
    /* [in] */ LPCWSTR pwzUrl,
    /* [in] */ QUERYOPTION QueryOption,
    /* [in] */ DWORD dwQueryFlags,
    /* [in, out] */ LPVOID pBuffer,
    /* [in] */ DWORD cbBuffer,
    /* [in, out] */ DWORD *pcbBuf,
    /* [in] */ DWORD dwReserved);

  // IInternetPriority
  STDMETHODIMP SetPriority(
    /* [in] */ LONG nPriority);

  STDMETHODIMP GetPriority(
    /* [out] */ LONG *pnPriority);

  // IInternetThreadSwitch
  STDMETHODIMP Prepare();

  STDMETHODIMP Continue();

  // IWinInetInfo
  STDMETHODIMP QueryOption(
    /* [in] */ DWORD dwOption,
    /* [in, out] */ LPVOID pBuffer,
    /* [in, out] */ DWORD *pcbBuf);

  // IWinInetHttpInfo
  STDMETHODIMP QueryInfo(
    /* [in] */ DWORD dwOption,
    /* [in, out] */ LPVOID pBuffer,
    /* [in, out] */ DWORD *pcbBuf,
    /* [in, out] */ DWORD *pdwFlags,
    /* [in, out] */ DWORD *pdwReserved);

  // IWinInetCacheHints
  STDMETHODIMP SetCacheExtension(
      /* [in] */ LPCWSTR pwzExt,
      /* [size_is][out][in] */ LPVOID pszCacheFile,
      /* [out][in] */ DWORD *pcbCacheFile,
      /* [out][in] */ DWORD *pdwWinInetError,
      /* [out][in] */ DWORD *pdwReserved);

  // IWinInetCacheHints2
  STDMETHODIMP SetCacheExtension2(
      /* [in] */ LPCWSTR pwzExt,
      /* [size_is][out] */ WCHAR *pwzCacheFile,
      /* [out][in] */ DWORD *pcchCacheFile,
      /* [out] */ DWORD *pdwWinInetError,
      /* [out] */ DWORD *pdwReserved);

 private:
  // Calling BaseClass::Foo() passes an API call thru to the default handler
  typedef PassthroughAPP::CInternetProtocol<PassthruStartPolicy,
                                            CComMultiThreadModel> BaseClass;

  friend PassthruSink;

  // A convenience pointer to our PassthruSink. The PassthruSink is only
  // used when the default handler is in use.
  PassthruSink *passthru_sink_;
};


//------------------------------------------------------------------------------
// HttpHandlerFactory
//  A custom class factory that implements IInternetProtocolInfo. Rather
//  than creating a new handler instance in order to call methods of this
//  interface, IE will call these methods through the interface provided on
//  the factory. This greatly reduces the number of HttpHandlerAPP instances.
//------------------------------------------------------------------------------
class HttpHandlerFactory
    : public PassthroughAPP::CComClassFactoryProtocol,
      public IInternetProtocolInfo {
 public:
  BEGIN_COM_MAP(HttpHandlerFactory)
    COM_INTERFACE_ENTRY(IInternetProtocolInfo)
    COM_INTERFACE_ENTRY_CHAIN(PassthroughAPP::CComClassFactoryProtocol)
  END_COM_MAP()

  // IInternetProtocolInfo methods
  STDMETHODIMP ParseUrl(LPCWSTR pwzUrl,
      PARSEACTION ParseAction,
      DWORD dwParseFlags,
      LPWSTR pwzResult,
      DWORD cchResult,
      DWORD *pcchResult,
      DWORD reserved);

  STDMETHODIMP CombineUrl(LPCWSTR pwzBaseUrl,
      LPCWSTR pwzRelativeUrl,
      DWORD dwCombineFlags,
      LPWSTR pwzResult,
      DWORD cchResult,
      DWORD *pcchResult,
      DWORD reserved);

  STDMETHODIMP CompareUrl(LPCWSTR pwzUrl1,
      LPCWSTR pwzUrl2,
      DWORD dwCompareFlags);

  STDMETHODIMP QueryInfo(LPCWSTR pwzUrl,
      QUERYOPTION QueryOption,
      DWORD dwQueryFlags,
      LPVOID pBuffer,
      DWORD cbBuffer,
      DWORD *pcbBuf,
      DWORD reserved);
};

#endif  // GEARS_LOCALSERVER_IE_HTTP_HANDLER_APP_H__
