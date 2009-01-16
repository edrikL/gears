// Copyright 2006-2009, Google Inc.
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

#ifndef GEARS_LOCALSERVER_IE_HTTP_HANDLER_PATCHED_H__
#define GEARS_LOCALSERVER_IE_HTTP_HANDLER_PATCHED_H__

#include <urlmon.h>
#include "gears/base/common/scoped_refptr.h"
#include "gears/localserver/ie/http_handler_base.h"

//------------------------------------------------------------------------------
// HttpHandlerPatch overrides the the default HTTP and HTTPS protocol
// handler supplied by URLMON by way of vtable and iat patching.
//
// This class implements most of the interfaces supported by an
// Asyncronous Pluggable Protocol. Most of the methods are simple
// boilerplate to execute a no-op stub.  For those methods
// that require real work on our part when actually handling a
// request, we call a custom <<>>Impl method provided by HttpHandlerBase.
//------------------------------------------------------------------------------
class HttpHandlerPatch : public HttpHandlerBase, public RefCounted {
 public:
  // Installs our handler in the http/https namespace. Can be called multiple
  // times, if already installed this is a noop.
  static HRESULT Install();

  // Called by the vtable patching layer to find an instance (if any) that
  // corresponds with the urlmon handler instance.
  static HttpHandlerPatch *Find(IInternetProtocolRoot *urlmon_instance);
  static HttpHandlerPatch *Find(IUnknown *urlmon_instance) {
    CComQIPtr<IInternetProtocolRoot> pinterface(urlmon_instance);
    if (!pinterface) return NULL;
    return Find(pinterface);
  }

  HttpHandlerPatch(IInternetProtocolRoot *urlmon_instance);

  // IInternetProtocolEx
  STDMETHODIMP StartExMaybe(IUri *pUri,
    IInternetProtocolSink *pOIProtSink,
    IInternetBindInfo *pOIBindInfo,
    DWORD grfPI,
    HANDLE_PTR dwReserved,
    IInternetProtocolSink **replacement_sink);

  // IInternetProtocolRoot
  STDMETHODIMP StartMaybe(
    /* [in] */ LPCWSTR szUrl,
    /* [in] */ IInternetProtocolSink *pOIProtSink,
    /* [in] */ IInternetBindInfo *pOIBindInfo,
    /* [in] */ DWORD grfPI,
    /* [in] */ HANDLE_PTR dwReserved,
    /* [out] */ IInternetProtocolSink **replacement_sink);

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

 protected:
  virtual ~HttpHandlerPatch();

 private:
  // The instance of the default HTTP or HTTPS handler that we're patched
  // on top of.
  CComPtr<IInternetProtocolRoot> urlmon_instance_;

#ifdef DEBUG
  virtual void SelfTest();
#endif
};

#endif  // GEARS_LOCALSERVER_IE_HTTP_HANDLER_PATCHED_H__
