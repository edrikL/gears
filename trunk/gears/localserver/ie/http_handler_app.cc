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


#include <windows.h>
#include <wininet.h>
#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "gears/localserver/ie/http_handler_app.h"

#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/string_utils.h"
#ifdef OS_WINCE
#include "gears/base/common/wince_compatibility.h"  // For BrowserCache
#endif
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/ie_version.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/ie/http_intercept.h"
#include "gears/localserver/ie/urlmon_utils.h"

#ifdef OS_WINCE
// On WinCE, when a request is made for a network resource, the device first
// checks the cache. If the resource is not present in the cache, the device
// attempts to make a network connection. If this fails, a pop-up a dialog box
// is shown to inform the user. This all happens before our HTTP handler
// receives the request callback, so we're not able to disable the popup for
// resources which are in the LocalServer.
//
// A workaround for this problem is to add an empty entry to the browser cache
// for each resource in the LocalServer. This occurs at the following points ...
// - For a resource store, when capturing a new resource.
// - For a resource store, when copying or moving a resource.
// - For a managed resource store, when reading a new manifest file.
//
// Note that we can not guarantee that the cache entries will persists (cache
// entries may be removed by the system when the cache becomes full or manually
// by the user), so we re-insert the entries at the following points ...
// - For a resource store, when refreshing a resource, but it's unmodified.
// - For a managed resource store, when checking for a manifest update.
// - When serving a resource from the local server.
//
// Also, the LocalServer can contain multiple resources (from seperate stores)
// for a given url which are conditionally eligible for local serving based on
// the enable property of the containing store and the presence or absence of
// particular cookies. This obviously doesn't map very well to the browser
// cache, so our approach isn't completely foolproof.
//
// Note that SetSessionOption, which is used on Win32 with
// kBypassIInternetProtocolInfoOption to prevent the device from attempting to
// connect to the network for LocalServer resources, is not available on WinCE.
#else
// NOTE: Undocumented voodoo to kick IE into using IInternetProtocolInfo
// for well known protocols.  We depend on this to indicate that items
// in our webcapture database do not require the network to be accessed.
static const DWORD kBypassIInternetProtocolInfoOption = 0x40;
#endif

static bool has_installed_handler = false;
static CComPtr<IClassFactory> factory_http;
static CComPtr<IClassFactory> factory_https;
static CComQIPtr<IInternetProtocolInfo> factory_protocol_info;

static HRESULT RegisterNoLock();
static HRESULT UnregisterNoLock();


// static
HRESULT HttpHandlerAPP::Install() {
  if (has_installed_handler) return S_OK;
  MutexLock lock(&global_mutex_);
  return RegisterNoLock();
}

// static
HRESULT HttpHandlerAPP::Uninstall() {
  MutexLock lock(&global_mutex_);
  return UnregisterNoLock();
}


// Registers our handler with URLMON to intercept HTTP and HTTPS requests,
// and configures URLMON to call our implementation of IInternetProtocolInfo
// rather than using the default implementation.
static HRESULT RegisterNoLock() {
  typedef PassthroughAPP::CMetaFactory<HttpHandlerFactory, HttpHandlerAPP>
      PassthruMetaFactory;

  if (has_installed_handler) {
    return S_OK;
  }
  has_installed_handler = true;

  CComPtr<IInternetSession> session;
  HRESULT hr = CoInternetGetSession(0, &session, 0);
  if (FAILED(hr) || !session) {
    UnregisterNoLock();
    return FAILED(hr) ? hr : E_FAIL;
  }
  hr = PassthruMetaFactory::CreateInstance(CLSID_HttpProtocol, &factory_http);
  if (FAILED(hr) || !factory_http) {
    UnregisterNoLock();
    return FAILED(hr) ? hr : E_FAIL;
  }
  hr = PassthruMetaFactory::CreateInstance(CLSID_HttpSProtocol, &factory_https);
  if (FAILED(hr) || !factory_https) {
    UnregisterNoLock();
    return FAILED(hr) ? hr : E_FAIL;
  }
  factory_protocol_info = factory_http;
  if (!factory_protocol_info) {
    UnregisterNoLock();
    return E_FAIL;
  }
  hr = session->RegisterNameSpace(factory_http, CLSID_NULL,
                                  HttpConstants::kHttpScheme, 0, 0, 0);
  if (FAILED(hr)) {
    UnregisterNoLock();
    return hr;
  }
  hr = session->RegisterNameSpace(factory_https, CLSID_NULL,
                                  HttpConstants::kHttpsScheme, 0, 0, 0);
  if (FAILED(hr)) {
    UnregisterNoLock();
    return hr;
  }
#ifdef OS_WINCE
  // SetSessionOption is not implemented on WinCE.
#else
  BOOL bypass = FALSE;
  hr = session->SetSessionOption(kBypassIInternetProtocolInfoOption,
                                 &bypass, sizeof(BOOL), 0);
  if (FAILED(hr)) {
    UnregisterNoLock();
    return hr;
  }
#endif
  LOG16((L"HttpHandlerAPP::Registered\n"));
  return hr;
}


static HRESULT UnregisterNoLock() {
  if (!has_installed_handler) {
    return S_OK;
  }
  has_installed_handler = false;

  CComPtr<IInternetSession> session;
  HRESULT rv = CoInternetGetSession(0, &session, 0);
  if (FAILED(rv) || !session) {
    return FAILED(rv) ? rv : E_FAIL;
  }
  if (factory_http != NULL) {
    session->UnregisterNameSpace(factory_http, HttpConstants::kHttpScheme);
    factory_http.Release();
  }
  if (factory_https != NULL) {
    session->UnregisterNameSpace(factory_https, HttpConstants::kHttpScheme);
    factory_https.Release();
  }
  factory_protocol_info.Release();
#ifdef OS_WINCE
  // SetSessionOption is not implemented on WinCE.
#else
  BOOL bypass = TRUE;
  session->SetSessionOption(kBypassIInternetProtocolInfoOption,
                            &bypass, sizeof(BOOL), 0);
#endif
  LOG16((L"HttpHandlerAPP::Unregistered\n"));
  return S_OK;
}

//------------------------------------------------------------------------------
// class PassthruSink
//------------------------------------------------------------------------------

STDMETHODIMP PassthruSink::ReportProgress(
    /* [in] */ ULONG status_code,
    /* [in] */ LPCWSTR status_text) {
  if (!http_handler_)
    return E_UNEXPECTED;
  if (!http_handler_->is_passingthru_)
    return E_FAIL;

#ifdef DEBUG
  LOG16((L"PassthruSink::ReportProgress( %s, %s )\n",
         GetBindStatusLabel(status_code),
         status_text ? status_text : L""));
#endif

  if (status_code == BINDSTATUS_REDIRECTING) {
    // status_text contains the redirect url in this case
    WebCacheDB* db = WebCacheDB::GetDB();
    if (db && db->CanService(status_text, NULL)) {
      // Here we detect 302s into our cache here and intervene to hijack
      // handling of the request. Reporting a result of INET_E_REDIRECT_FAILED
      // causes URMLON to abandon this handler instance an create a new one
      // to follow the redirect. When that new instance of our handler is
      // created, it will satisfy the request locally.
      LOG16((L"PassthruSink::ReportProgress - hijacking redirect\n"));
      return BaseClass::ReportResult(INET_E_REDIRECT_FAILED,
                                     HttpConstants::HTTP_FOUND,
                                     status_text);
    }
  }
  return BaseClass::ReportProgress(status_code, status_text);
}


//------------------------------------------------------------------------------
// ActiveHandlers
//
// This class is here to workaround a crash in IE6SP2 when the browser process
// is exiting. In some circumstances, HttpHandlers are not terminated as they
// should be. Some time after our DLL is unloaded during shutdown, IE
// invokes methods on these orphaned handlers resulting in a crash. Note that
// WinCE does not suffer from this problem.
// See http://code.google.com/p/google-gears/issues/detail?id=182
//
// To avoid this problem, we maintain a collection of the active HttpHandlers
// when running in IE6 or earlier. When our DLL is unloaded, we explicitly
// Terminate() any orphaned handlers. This prevents the crash.
// TODO(michaeln): If and when we find and fix the source of this bug,
// remove this workaround code.
//------------------------------------------------------------------------------

namespace {

#ifdef OS_WINCE
class ActiveHandlers {
 public:
  void Add(HttpHandlerAPP *handler) {}
  void Remove(HttpHandlerAPP *handler) {}
};
#else
class ActiveHandlers : private std::set<HttpHandlerAPP*> {
 public:
  ActiveHandlers()
      : has_determined_ie_version_(false), is_at_least_version_7_(false) {}

  virtual ~ActiveHandlers() {
    while (HttpHandlerAPP* handler = GetAndRemoveFirstHandler()) {
      handler->Terminate(0);
      handler->Release();
    }
  }

  void Add(HttpHandlerAPP *handler) {
    assert(handler);
    if (!IsIEAtLeastVersion7()) {
      MutexLock lock(&mutex_);
      insert(handler);
    }
  }

  void Remove(HttpHandlerAPP *handler) {
    assert(handler);
    if (!IsIEAtLeastVersion7()) {
      MutexLock lock(&mutex_);
      erase(handler);
    }
  }

 private:
  HttpHandlerAPP *GetAndRemoveFirstHandler() {
    MutexLock lock(&mutex_);
    if (empty()) return NULL;
    HttpHandlerAPP *handler = *begin();
    erase(handler);
    handler->AddRef();  // released by our caller
    return handler;
  }

  bool IsIEAtLeastVersion7() {
    if (!has_determined_ie_version_) {
      MutexLock lock(&mutex_);
      is_at_least_version_7_ = IsIEAtLeastVersion(7, 0, 0, 0);
      has_determined_ie_version_ = true;
    }
    return is_at_least_version_7_;
  }

  Mutex mutex_;
  bool has_determined_ie_version_;
  bool is_at_least_version_7_;
};
#endif

}  // namespace

static ActiveHandlers g_active_handlers;

//------------------------------------------------------------------------------
// class HttpHandlerAPP
//------------------------------------------------------------------------------

HttpHandlerAPP::HttpHandlerAPP() : passthru_sink_(NULL) {
}

HttpHandlerAPP::~HttpHandlerAPP() {
  LOG16((L"~HttpHandlerAPP\n"));
  g_active_handlers.Remove(this);   // okay to Remove() multiple times from set
}

HRESULT HttpHandlerAPP::FinalConstruct() {
  passthru_sink_ = PassthruStartPolicy::GetSink(this);
  passthru_sink_->SetHttpHandler(this);
  return S_OK;
}

// IInternetProtocolEx
STDMETHODIMP HttpHandlerAPP::StartEx(
    /* [in] */ IUri *uri,
    /* [in] */ IInternetProtocolSink *protocol_sink,
    /* [in] */ IInternetBindInfo *bind_info,
    /* [in] */ DWORD flags,
    /* [in] */ HANDLE_PTR reserved) {
  CComBSTR uri_bstr;
  HRESULT rv = uri->GetAbsoluteUri(&uri_bstr);
  if (FAILED(rv)) {
    return rv;
  }
  LOG16((L"HttpHandlerAPP::StartEx( %s )\n", uri_bstr.m_str));
  g_active_handlers.Add(this);
  rv = StartImpl(uri_bstr.m_str, protocol_sink, bind_info, flags, reserved);
  if (rv == INET_E_USE_DEFAULT_PROTOCOLHANDLER) {
    protocol_sink_.Release();
    http_negotiate_.Release();
    if (is_passingthru_) {
      return BaseClass::StartEx(uri, protocol_sink, bind_info, flags, reserved);
    } else {
      return rv;
    }
  } else {
    return rv;
  }
}

// IInternetProtocolRoot
STDMETHODIMP HttpHandlerAPP::Start(
    /* [in] */ LPCWSTR url,
    /* [in] */ IInternetProtocolSink *protocol_sink,
    /* [in] */ IInternetBindInfo *bind_info,
    /* [in] */ DWORD flags,
    /* [in] */ HANDLE_PTR reserved) {
  LOG16((L"HttpHandlerAPP::Start( %s )\n", url));
  g_active_handlers.Add(this);
  HRESULT rv = StartImpl(url, protocol_sink, bind_info, flags, reserved);
  if (rv == INET_E_USE_DEFAULT_PROTOCOLHANDLER) {
    protocol_sink_.Release();
    http_negotiate_.Release();
    if (is_passingthru_) {
      return BaseClass::Start(url, protocol_sink, bind_info, flags, reserved);
    } else {
      return rv;
    }
  } else {
    return rv;
  }
}

STDMETHODIMP HttpHandlerAPP::Continue(
    /* [in] */ PROTOCOLDATA *data) {
  LOG16((L"HttpHandlerAPP::Continue(data)\n"));
  if (is_passingthru_)
    return BaseClass::Continue(data);
  else if (is_handling_)
    return S_OK;
  else
    return E_UNEXPECTED;
}

STDMETHODIMP HttpHandlerAPP::Abort(
    /* [in] */ HRESULT reason,
    /* [in] */ DWORD options) {
  LOG16((L"HttpHandlerAPP::Abort()\n"));
  if (is_passingthru_) {
    return BaseClass::Abort(reason, options);
  } else if (is_handling_) {
    was_aborted_ = true;
    // We intentionally don't propogate the return value from this method,
    // our handler is aborted regardless of the sink's return value.
    CallReportResult(reason, E_ABORT, L"Aborted");
    return S_OK;
  } else {
    return E_UNEXPECTED;
  }
}

STDMETHODIMP HttpHandlerAPP::Terminate(
    /* [in] */ DWORD options) {
  LOG16((L"HttpHandlerAPP::Terminate()\n"));
  protocol_sink_.Release();
  http_negotiate_.Release();
  g_active_handlers.Remove(this);
  if (is_passingthru_) {
    return BaseClass::Terminate(options);
  } else if (is_handling_) {
    was_terminated_ = true;
    return S_OK;
  } else {
    return E_UNEXPECTED;
  }
}

STDMETHODIMP HttpHandlerAPP::Suspend() {
  LOG16((L"HttpHandlerAPP::Suspend()\n"));
  if (is_passingthru_)
    return BaseClass::Suspend();
  else if (is_handling_)
    return S_OK;
  else
    return E_UNEXPECTED;
}

STDMETHODIMP HttpHandlerAPP::Resume() {
  LOG16((L"HttpHandlerAPP::Resume()\n"));
  if (is_passingthru_)
    return BaseClass::Resume();
  else if (is_handling_)
    return S_OK;
  else
    return E_UNEXPECTED;
}

// IInternetProtocol
STDMETHODIMP HttpHandlerAPP::Read(
    /* [in, out] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead) {
  if (is_passingthru_) {
    HRESULT hr = BaseClass::Read(pv, cb, pcbRead);
    LOG16((L"HttpHandlerAPP::Read() - passing thru, %d bytes\n", *pcbRead));
    return hr;
  } else if (is_handling_) {
    return ReadImpl(pv, cb, pcbRead);
  } else {
    LOG16((L"HttpHandlerAPP::Read() - unexpected\n"));
    return E_UNEXPECTED;
  }
}

STDMETHODIMP HttpHandlerAPP::Seek(
    /* [in] */ LARGE_INTEGER dlibMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULARGE_INTEGER *plibNewPosition) {
  LOG16((L"HttpHandlerAPP::Seek()\n"));
  if (is_passingthru_)
    return BaseClass::Seek(dlibMove, dwOrigin, plibNewPosition);
  else if (is_handling_)
    return S_OK;
  else
    return E_UNEXPECTED;
}

STDMETHODIMP HttpHandlerAPP::LockRequest(
    /* [in] */ DWORD dwOptions) {
  LOG16((L"HttpHandlerAPP::LockRequest()\n"));
  if (is_passingthru_)
    return BaseClass::LockRequest(dwOptions);
  else if (is_handling_)
    return S_OK;
  else
    return E_UNEXPECTED;
}

STDMETHODIMP HttpHandlerAPP::UnlockRequest() {
  LOG16((L"HttpHandlerAPP::UnlockRequest()\n"));
  if (is_passingthru_)
    return BaseClass::UnlockRequest();
  else if (is_handling_)
    return S_OK;
  else
    return E_UNEXPECTED;
}

// IInternetProtocolInfo
// This interface can be used out of the context of handling a request, this
// is why we don't return E_UNEXPECTED in that case
STDMETHODIMP HttpHandlerAPP::ParseUrl(
    /* [in] */ LPCWSTR pwzUrl,
    /* [in] */ PARSEACTION ParseAction,
    /* [in] */ DWORD dwParseFlags,
    /* [out] */ LPWSTR pwzResult,
    /* [in] */ DWORD cchResult,
    /* [out] */ DWORD *pcchResult,
    /* [in] */ DWORD dwReserved) {
  LOG16((L"HttpHandlerAPP::ParseUrl()\n"));
  if (is_passingthru_)
    return BaseClass::ParseUrl(pwzUrl, ParseAction, dwParseFlags,
                               pwzResult, cchResult, pcchResult,
                               dwReserved);
  else
    return factory_protocol_info->ParseUrl(pwzUrl, ParseAction, dwParseFlags,
                                           pwzResult, cchResult, pcchResult,
                                           dwReserved);
}

STDMETHODIMP HttpHandlerAPP::CombineUrl(
    /* [in] */ LPCWSTR pwzBaseUrl,
    /* [in] */ LPCWSTR pwzRelativeUrl,
    /* [in] */ DWORD dwCombineFlags,
    /* [out] */ LPWSTR pwzResult,
    /* [in] */ DWORD cchResult,
    /* [out] */ DWORD *pcchResult,
    /* [in] */ DWORD dwReserved) {
  LOG16((L"HttpHandlerAPP::CombineUrl()\n"));
  if (is_passingthru_)
    return BaseClass::CombineUrl(pwzBaseUrl, pwzRelativeUrl,
                                 dwCombineFlags, pwzResult,
                                 cchResult, pcchResult, dwReserved);
  else
    return factory_protocol_info->CombineUrl(pwzBaseUrl, pwzRelativeUrl,
                                             dwCombineFlags, pwzResult,
                                             cchResult, pcchResult, dwReserved);
}

STDMETHODIMP HttpHandlerAPP::CompareUrl(
    /* [in] */ LPCWSTR pwzUrl1,
    /* [in] */ LPCWSTR pwzUrl2,
    /* [in] */ DWORD dwCompareFlags) {
  LOG16((L"HttpHandlerAPP::CompareUrl()\n"));
  if (is_passingthru_)
    return BaseClass::CompareUrl(pwzUrl1, pwzUrl2, dwCompareFlags);
  else
    return factory_protocol_info->CompareUrl(pwzUrl1, pwzUrl2, dwCompareFlags);
}

STDMETHODIMP HttpHandlerAPP::QueryInfo(
    /* [in] */ LPCWSTR pwzUrl,
    /* [in] */ QUERYOPTION QueryOption,
    /* [in] */ DWORD dwQueryFlags,
    /* [in, out] */ LPVOID pBuffer,
    /* [in] */ DWORD cbBuffer,
    /* [in, out] */ DWORD *pcbBuf,
    /* [in] */ DWORD dwReserved) {
  LOG16((L"HttpHandlerAPP::QueryInfo()\n"));
  if (is_passingthru_)
    return BaseClass::QueryInfo(pwzUrl, QueryOption, dwQueryFlags,
                                pBuffer, cbBuffer, pcbBuf,
                                dwReserved);
  else
    return factory_protocol_info->QueryInfo(pwzUrl, QueryOption, dwQueryFlags,
                                            pBuffer, cbBuffer, pcbBuf,
                                            dwReserved);
}

// IInternetPriority
// TODO(michaeln): our handler implements this interface for the purpose
// of passing thru only, when not in passthru mode what should we do?
// We call thru to the default handler in all cases so Set/Get sequence
// will return the value expected by the caller.
STDMETHODIMP HttpHandlerAPP::SetPriority(
    /* [in] */ LONG nPriority) {
  LOG16((L"HttpHandlerAPP::SetPriority()\n"));
  return BaseClass::SetPriority(nPriority);
}

STDMETHODIMP HttpHandlerAPP::GetPriority(
    /* [out] */ LONG *pnPriority) {
  LOG16((L"HttpHandlerAPP::GetPriority()\n"));
  return BaseClass::GetPriority(pnPriority);
}

// IInternetThreadSwitch
// TODO(michaeln): our handler implements this interface for the purpose
// of passing thru only, when not in passthru mode what should we do?
STDMETHODIMP HttpHandlerAPP::Prepare() {
  LOG16((L"HttpHandlerAPP::Prepare()\n"));
  if (is_passingthru_)
    return BaseClass::Prepare();
  else if (is_handling_)
    return S_OK;
  else
    return E_UNEXPECTED;
}

STDMETHODIMP HttpHandlerAPP::Continue() {
  LOG16((L"HttpHandlerAPP::Continue()\n"));
  if (is_passingthru_)
    return BaseClass::Continue();
  else if (is_handling_)
    return S_OK;
  else
    return E_UNEXPECTED;
}

// IWinInetInfo
STDMETHODIMP HttpHandlerAPP::QueryOption(
    /* [in] */ DWORD option,
    /* [in, out] */ LPVOID buffer,
    /* [in, out] */ DWORD *len) {
  LOG16((L"HttpHandlerAPP::QueryOption(%d)\n", option));
  if (is_passingthru_)
    return BaseClass::QueryOption(option, buffer, len);
  else if (is_handling_)
    return QueryOptionImpl(option, buffer, len);
  else
    return E_UNEXPECTED;
}

// IWinInetHttpInfo
STDMETHODIMP HttpHandlerAPP::QueryInfo(
    /* [in] */ DWORD option,
    /* [in, out] */ LPVOID buffer,
    /* [in, out] */ DWORD *len,
    /* [in, out] */ DWORD *flags,
    /* [in, out] */ DWORD *reserved) {
  LOG16((L"HttpHandlerAPP::QueryInfo(%d)\n", option));
  if (is_passingthru_)
    return BaseClass::QueryInfo(option, buffer, len, flags, reserved);
  else if (is_handling_)
    return QueryInfoImpl(option, buffer, len, flags, reserved);
  else
    return E_UNEXPECTED;
}

// IWinInetCacheHints
// TODO(michaeln): our handler implements this interface for the purpose
// of passing thru only, when not in passthru mode what should we do?
STDMETHODIMP HttpHandlerAPP::SetCacheExtension(
    /* [in] */ LPCWSTR pwzExt,
    /* [size_is][out][in] */ LPVOID pszCacheFile,
    /* [out][in] */ DWORD *pcbCacheFile,
    /* [out][in] */ DWORD *pdwWinInetError,
    /* [out][in] */ DWORD *pdwReserved) {
  LOG16((L"HttpHandlerAPP::SetCacheExtension()\n"));
  if (is_passingthru_)
    return BaseClass::SetCacheExtension(pwzExt, pszCacheFile, pcbCacheFile,
                                        pdwWinInetError, pdwReserved);
  else if (is_handling_)
    return E_FAIL;
  else
    return E_UNEXPECTED;
}

// IWinInetCacheHints2
// TODO(michaeln): our handler implements this interface for the purpose
// of passing thru only, when not in passthru mode what should we do?
STDMETHODIMP HttpHandlerAPP::SetCacheExtension2(
    /* [in] */ LPCWSTR pwzExt,
    /* [size_is][out] */ WCHAR *pwzCacheFile,
    /* [out][in] */ DWORD *pcchCacheFile,
    /* [out] */ DWORD *pdwWinInetError,
    /* [out] */ DWORD *pdwReserved) {
  LOG16((L"HttpHandlerAPP::SetCacheExtension2()\n"));
  if (is_passingthru_)
    return BaseClass::SetCacheExtension2(pwzExt, pwzCacheFile, pcchCacheFile,
                                         pdwWinInetError, pdwReserved);
  else if (is_handling_)
    return E_FAIL;
  else
    return E_UNEXPECTED;
}

//------------------------------------------------------------------------------
// HttpHandlerFactory
//------------------------------------------------------------------------------

STDMETHODIMP HttpHandlerFactory::ParseUrl(LPCWSTR pwzUrl,
                                          PARSEACTION ParseAction,
                                          DWORD dwParseFlags,
                                          LPWSTR pwzResult,
                                          DWORD cchResult,
                                          DWORD *pcchResult,
                                          DWORD reserved) {
  return INET_E_DEFAULT_ACTION;
}

STDMETHODIMP HttpHandlerFactory::CombineUrl(LPCWSTR baseUrl,
                                            LPCWSTR relateiveUrl,
                                            DWORD dwCombineFlags,
                                            LPWSTR pwzResult,
                                            DWORD cchResult,
                                            DWORD *pcchResult,
                                            DWORD reserved) {
  return INET_E_DEFAULT_ACTION;
}

STDMETHODIMP HttpHandlerFactory::CompareUrl(LPCWSTR pwzUrl1,
                                            LPCWSTR pwzUrl2,
                                            DWORD dwCompareFlags) {
  return INET_E_DEFAULT_ACTION;
}

STDMETHODIMP HttpHandlerFactory::QueryInfo(LPCWSTR url,
                                           QUERYOPTION option,
                                           DWORD flags,
                                           LPVOID buffer,
                                           DWORD buffer_size,
                                           DWORD *buffer_size_out,
                                           DWORD reserved) {
  return HttpHandlerBase::CoInternetQueryInfo(url, option, flags, buffer,
                                              buffer_size, buffer_size_out,
                                              reserved);
}
