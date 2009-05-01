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

#include "gears/localserver/ie/http_handler_patch.h"

#include <map>
#include <wininet.h>

#include "gears/base/common/mutex.h"
#include "gears/base/common/string_utils.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/ie/http_intercept.h"
#include "gears/localserver/ie/http_patches.h"
#include "gears/localserver/ie/urlmon_utils.h"

namespace {

//------------------------------------------------------------------------------
// ActiveHandlers is a collection of live HttpHandlerPatch instances in the
// process. This map is used to quickly lookup the HttpHandlerPatch associated 
// with an instance of the URLMON's default handler. This is frequently NULL.
// Each patched method call involves a lookup. For this reason, we cache the
// most recent lookup to avoid calling map find repeadly while streaming in a
// response.
//
// TODO(michaeln): Look into http://code.google.com/p/google-gears/issues/detail?id=182
// and this new impl?
//------------------------------------------------------------------------------
class ActiveHandlers {
 public:
  ActiveHandlers() : cached_urlmon_instance_(NULL), cached_handler_(NULL) {
  }

  ~ActiveHandlers() {
    assert(map_.empty());
  }

  void Add(IInternetProtocolRoot *urlmon_instance, HttpHandlerPatch *handler) {
    assert(urlmon_instance && handler);
    MutexLock lock(&mutex_);
    assert(map_.find(urlmon_instance) == map_.end());
    map_[urlmon_instance] = handler;
    if (cached_urlmon_instance_ == urlmon_instance) {
      cached_handler_ = handler;
    }
  }

  void Remove(IInternetProtocolRoot *urlmon_instance) {
    assert(urlmon_instance);
    MutexLock lock(&mutex_);
    assert(map_.find(urlmon_instance) != map_.end());
    map_.erase(urlmon_instance);
    if (cached_urlmon_instance_ == urlmon_instance) {
      cached_handler_ = NULL;
    }
  }

  HttpHandlerPatch* Lookup(IInternetProtocolRoot *urlmon_instance) {
    assert(urlmon_instance);
    MutexLock lock(&mutex_);
    if (urlmon_instance == cached_urlmon_instance_) {
      return cached_handler_;
    }
    HandlerMap::iterator iter = map_.find(urlmon_instance);
    cached_urlmon_instance_ = urlmon_instance;
    cached_handler_ = (iter == map_.end()) ? NULL : iter->second;
    return cached_handler_;
  }

 private:
  typedef std::map<IInternetProtocolRoot*, HttpHandlerPatch*> HandlerMap;
  Mutex mutex_;
  HandlerMap map_;
  IInternetProtocolRoot *cached_urlmon_instance_;
  HttpHandlerPatch *cached_handler_;
};

}  // namespace

static ActiveHandlers active_handlers;
static bool has_installed_handler = false;

// static
HRESULT HttpHandlerPatch::Install() {
  MutexLock lock(&global_mutex_);
  if (has_installed_handler)
    return S_OK;
  has_installed_handler = true;
  return InitializeHttpPatches();
}

// static
HttpHandlerPatch *HttpHandlerPatch::Find(
                      IInternetProtocolRoot *urlmon_instance) {
  return active_handlers.Lookup(urlmon_instance);
}


//------------------------------------------------------------------------------
// ReplacementSink is used to hijack redirects from URLs not in our cache to
// URLs that are in our cache.
//
// When redirection occurs, generally a new handler is not started, a chain of
// redirects are handled within the context of a single URLMON instance.
// This presents a problem for us since Start or StartEx will not be called
// for the new location of the redirect. When we defer to the default handler,
// we wrap the caller's "sink" with an instance of our ReplacementSink. Our 
// code monitors the ReportProgress calls and detects when a 302 back into 
// our cache occurs and intervenes to satisfy the request from our cache.
//------------------------------------------------------------------------------
class ATL_NO_VTABLE ReplacementSink :
  public CComObjectRootEx<CComMultiThreadModel>,
  public IInternetProtocolSink {
 public:
  BEGIN_COM_MAP(ReplacementSink)
    COM_INTERFACE_ENTRY(IInternetProtocolSink)
    COM_INTERFACE_ENTRY_AGGREGATE_BLIND(sink_.p)
  END_COM_MAP()

  void Initialize(IInternetProtocolSink *sink) {
    assert(sink);
    sink_ = sink;
  }

  STDMETHOD(Switch)(PROTOCOLDATA *data) {
    return sink_->Switch(data);
  }

  STDMETHOD(ReportData)(DWORD bscf, ULONG progress, ULONG progress_max) {
    return sink_->ReportData(bscf, progress, progress_max);
  }

  STDMETHOD(ReportResult)(HRESULT result, DWORD error, LPCWSTR result_text) {
    return sink_->ReportResult(result, error, result_text);
  }

  STDMETHOD(ReportProgress)(ULONG status, LPCWSTR text);

  static void CreateReplacement(IInternetProtocolSink *orig,
                                IInternetProtocolSink **replacement) {
    CComObject<ReplacementSink>* replacement_sink = NULL;
    HRESULT hr = CComObject<ReplacementSink>::CreateInstance(&replacement_sink);
    if (SUCCEEDED(hr)) {
      replacement_sink->Initialize(orig);
      replacement_sink->AddRef();
      *replacement = replacement_sink;
    }
  }

 private:
  CComPtr<IInternetProtocolSink> sink_;
};


STDMETHODIMP ReplacementSink::ReportProgress(
    /* [in] */ ULONG status_code,
    /* [in] */ LPCWSTR status_text) {
#ifdef DEBUG
  LOG16((L"ReplacementSink::ReportProgress( %s, %s )\n",
         GetBindStatusLabel(status_code),
         status_text ? status_text : L""));
#endif
  if (status_code == BINDSTATUS_REDIRECTING) {
    // status_text contains the redirect url in this case
    WebCacheDB* db = WebCacheDB::GetDB();
    if (db && db->CanService(status_text, NULL)) {
      // Here we detect redirect into our cache here and intervene to hijack
      // handling of the request. Reporting a result of INET_E_REDIRECT_FAILED
      // causes URMLON to abandon the current handler instance and create a new
      // instance to follow the redirect. When that new instance of our handler
      // is created, it will satisfy the request locally.
      LOG16((L"ReplacementSink::ReportProgress - hijacking redirect\n"));
      return sink_->ReportResult(INET_E_REDIRECT_FAILED,
                                 HttpConstants::HTTP_FOUND,
                                 status_text);
    }
  }
  return sink_->ReportProgress(status_code, status_text);
}


//------------------------------------------------------------------------------
// HttpHandlerPatch is used to retrieve resources from our cache. When
// StartMaybe or StartExMaybe is called, we determine whether
// to satisfy the request or to defer to the default handler. If we're handling
// the request subsequent method calls on the default handler will be delivered
// to our instance, otherwise our instance will be deleted upon return from
// StartMaybe or StartExMaybe.
//------------------------------------------------------------------------------

HttpHandlerPatch::HttpHandlerPatch(IInternetProtocolRoot *urlmon_instance)
    : urlmon_instance_(urlmon_instance) {
  active_handlers.Add(urlmon_instance, this);
}

HttpHandlerPatch::~HttpHandlerPatch() {
  LOG16((L"~HttpHandlerPatch\n"));
  active_handlers.Remove(urlmon_instance_);
}

// IInternetProtocolEx
STDMETHODIMP HttpHandlerPatch::StartExMaybe(
    /* [in] */ IUri *uri,
    /* [in] */ IInternetProtocolSink *protocol_sink,
    /* [in] */ IInternetBindInfo *bind_info,
    /* [in] */ DWORD flags,
    /* [in] */ HANDLE_PTR reserved,
    /* [out] */IInternetProtocolSink **replacement_sink) {
  CComBSTR uri_bstr;
  HRESULT rv = uri->GetAbsoluteUri(&uri_bstr);
  if (FAILED(rv)) {
    return rv;
  }
  LOG16((L"HttpHandlerPatch::StartEx( %s )\n", uri_bstr.m_str));
  Ref();
  rv = StartImpl(uri_bstr.m_str, protocol_sink, bind_info, flags, reserved);
  if (rv == INET_E_USE_DEFAULT_PROTOCOLHANDLER) {
    if (is_passingthru_) {
      ReplacementSink::CreateReplacement(protocol_sink, replacement_sink);
    }
    protocol_sink_.Release();
    http_negotiate_.Release();
  }
  if (rv != S_OK) {
    Unref();
  }
  return rv;
}

// IInternetProtocolRoot
STDMETHODIMP HttpHandlerPatch::StartMaybe(
    /* [in] */ LPCWSTR url,
    /* [in] */ IInternetProtocolSink *protocol_sink,
    /* [in] */ IInternetBindInfo *bind_info,
    /* [in] */ DWORD flags,
    /* [in] */ HANDLE_PTR reserved,
    /* [out] */IInternetProtocolSink **replacement_sink) {
  LOG16((L"HttpHandlerPatch::Start( %s )\n", url));
  Ref();
  HRESULT rv = StartImpl(url, protocol_sink, bind_info, flags, reserved);
  if (rv == INET_E_USE_DEFAULT_PROTOCOLHANDLER) {
    if (is_passingthru_) {
      ReplacementSink::CreateReplacement(protocol_sink, replacement_sink);
    }
    protocol_sink_.Release();
    http_negotiate_.Release();
  }
  if (rv != S_OK) {
    Unref();
  }
  return rv;
}

STDMETHODIMP HttpHandlerPatch::Continue(
    /* [in] */ PROTOCOLDATA *data) {
  LOG16((L"HttpHandlerPatch::Continue(data)\n"));
  return S_OK;
}

STDMETHODIMP HttpHandlerPatch::Abort(
    /* [in] */ HRESULT reason,
    /* [in] */ DWORD options) {
  LOG16((L"HttpHandlerPatch::Abort()\n"));
  was_aborted_ = true;
  // We intentionally don't propogate the return value from this method,
  // our handler is aborted regardless of the sink's return value.
  CallReportResult(reason, E_ABORT, L"Aborted");
  return S_OK;
}

STDMETHODIMP HttpHandlerPatch::Terminate(
    /* [in] */ DWORD options) {
  LOG16((L"HttpHandlerPatch::Terminate()\n"));
  protocol_sink_.Release();
  http_negotiate_.Release();
  was_terminated_ = true;
  Unref();
  return S_OK;
}

STDMETHODIMP HttpHandlerPatch::Suspend() {
  LOG16((L"HttpHandlerPatch::Suspend()\n"));
  return S_OK;
}

STDMETHODIMP HttpHandlerPatch::Resume() {
  LOG16((L"HttpHandlerPatch::Resume()\n"));
  return S_OK;
}

// IInternetProtocol
STDMETHODIMP HttpHandlerPatch::Read(
    /* [in, out] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead) {
  return ReadImpl(pv, cb, pcbRead);
}

STDMETHODIMP HttpHandlerPatch::Seek(
    /* [in] */ LARGE_INTEGER dlibMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULARGE_INTEGER *plibNewPosition) {
  LOG16((L"HttpHandlerPatch::Seek()\n"));
  return S_OK;
}

STDMETHODIMP HttpHandlerPatch::LockRequest(
    /* [in] */ DWORD dwOptions) {
  LOG16((L"HttpHandlerPatch::LockRequest()\n"));
  Ref();
  return S_OK;
}

STDMETHODIMP HttpHandlerPatch::UnlockRequest() {
  LOG16((L"HttpHandlerPatch::UnlockRequest()\n"));
  Unref();
  return S_OK;
}

// IWinInetInfo
STDMETHODIMP HttpHandlerPatch::QueryOption(
    /* [in] */ DWORD option,
    /* [in, out] */ LPVOID buffer,
    /* [in, out] */ DWORD *len) {
  LOG16((L"HttpHandlerPatch::QueryOption(%d)\n", option));
  return QueryOptionImpl(option, buffer, len);
}

// IWinInetHttpInfo
STDMETHODIMP HttpHandlerPatch::QueryInfo(
    /* [in] */ DWORD option,
    /* [in, out] */ LPVOID buffer,
    /* [in, out] */ DWORD *len,
    /* [in, out] */ DWORD *flags,
    /* [in, out] */ DWORD *reserved) {
  LOG16((L"HttpHandlerPatch::QueryInfo(%d)\n", option));
  return QueryInfoImpl(option, buffer, len, flags, reserved);
}


#ifdef DEBUG
void HttpHandlerPatch::SelfTest() {
  // We probe the behavior of these interfaces that we have not patched.
  // The start method of the underlying urlmon instance has not been called,
  // and we want to observe their behavior when these methods get called while
  // the object is in that state. This is invoked when handling a request for
  // http://gears_self_test/
  CComQIPtr<IInternetPriority> priority(urlmon_instance_);
  CComQIPtr<IInternetThreadSwitch> thread_switch(urlmon_instance_);
  CComQIPtr<IWinInetCacheHints> cache_hints(urlmon_instance_);
  CComQIPtr<IWinInetCacheHints2> cache_hints2(urlmon_instance_);
  CComQIPtr<IUriContainer> uri_container(urlmon_instance_);

  HRESULT hr;
  LONG value;
  hr = priority->GetPriority(&value);
  LOG16((L"IInternetPriority::GetPrioriy = %d\n", hr));
  hr = priority->SetPriority(value);
  LOG16((L"IInternetPriority::SetPriority = %d\n", hr));
  hr = thread_switch->Prepare();
  LOG16((L"IInternetThreadSwitch::Prepare = %d\n", hr));
  hr = thread_switch->Continue();
  LOG16((L"IInternetThreadSwitch::Continue = %d\n", hr));

  if (cache_hints) {
    WCHAR buffer[1024];
    DWORD buffer_size = sizeof(buffer);
    DWORD error = 0;
    DWORD reserved = 0;
    hr = cache_hints->SetCacheExtension(L".bla",
                                        buffer,
                                        &buffer_size,
                                        &error,
                                        &reserved);
    LOG16((L"IWinInetCacheHints::SetCacheExtension = %d, %d\n", hr, error));
  }
  if (cache_hints2) {
    WCHAR buffer[1024];
    DWORD buffer_size = sizeof(buffer);
    DWORD error = 0;
    DWORD reserved = 0;
    hr = cache_hints2->SetCacheExtension2(L".bla",
                                          buffer,
                                          &buffer_size,
                                          &error,
                                          &reserved);
    LOG16((L"IWinInetCacheHints2::SetCacheExtension2 = %d, %d\n", hr, error));
  }

  if (uri_container) {
    CComPtr<IUri> uri;
    hr = uri_container->GetIUri(&uri);
    LOG16((L"IUriContainer::GetIUri = %d\n", hr));
    if (uri) {
      CComBSTR url;
      hr = uri->GetAbsoluteUri(&url);
      LOG16((L"uri->GetAbsoluteUri = %d, %s\n", hr,
             url.m_str ? url.m_str : L"null"));
    }
  }
}
#endif
