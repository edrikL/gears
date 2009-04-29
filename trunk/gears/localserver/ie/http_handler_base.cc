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

#include "gears/localserver/ie/http_handler_base.h"

#include <wininet.h>

#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/string_utils.h"
#ifdef OS_WINCE
#include "gears/base/common/wince_compatibility.h"  // For BrowserCache
#endif
#include "gears/base/ie/activex_utils.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/ie/http_intercept.h"
#include "gears/localserver/ie/urlmon_utils.h"

Mutex HttpHandlerBase::global_mutex_;
static bool bypass_cache = false;
static int started_count = 0;
static const std::string16 kMimeImagePrefix(L"image/");

// static
int HttpHandlerBase::GetStartedCount() {
  return AtomicIncrement(&started_count, 0);
}

// static
void HttpHandlerBase::SetBypassCache() {
  bypass_cache = true;
}


#ifdef DEBUG
// To support debugging, this class can be configured via the registry to
// display an alert when an uncaptured resource is requested while the
// browser is working in offline mode.
// See StartImpl()
static bool InitAlertCacheMiss() {
  bool default_value = false;
  CRegKey key;
  LONG res = key.Open(HKEY_CURRENT_USER,
                      L"Software\\" PRODUCT_FRIENDLY_NAME,
                      KEY_READ);
  if (res != ERROR_SUCCESS)
    return default_value;
  DWORD val;
  res = key.QueryDWORDValue(_T("AlertCacheMiss"), val);
  if (res != ERROR_SUCCESS)
    return default_value;
  return val != 0;
}
static const bool kAlertCacheMiss = InitAlertCacheMiss();
#else
static const bool kAlertCacheMiss = false;
#endif


HttpHandlerBase::HttpHandlerBase()
    : is_passingthru_(false), is_handling_(false),
      is_head_request_(false), has_reported_result_(false),
      was_aborted_(false), was_terminated_(false),
      read_pointer_(0) {
}


//------------------------------------------------------------------------------
// Helpers to call methods of the Sink object
//------------------------------------------------------------------------------

HRESULT HttpHandlerBase::CallReportProgress(ULONG status_code,
                                            LPCWSTR status_text) {
  if (was_terminated_ || was_aborted_ || !protocol_sink_)
    return E_UNEXPECTED;
#ifdef DEBUG
  LOG16((L"Calling ReportProgress( %s, %s )\n", GetBindStatusLabel(status_code),
         status_text ? status_text : L""));
#endif
  HRESULT hr = protocol_sink_->ReportProgress(status_code, status_text);
  if (FAILED(hr))
    LOG16((L"  protocol_sink->ReportProgress error = %d\n", hr));
  return hr;
}

HRESULT HttpHandlerBase::CallReportData(DWORD flags, ULONG progress,
                                        ULONG progress_max) {
  if (was_terminated_ || was_aborted_ || !protocol_sink_)
    return E_UNEXPECTED;
  HRESULT hr = protocol_sink_->ReportData(flags, progress, progress_max);
  if (FAILED(hr))
    LOG16((L"  protocol_sink->ReportData error = %d\n", hr));
  return hr;
}

HRESULT HttpHandlerBase::CallReportResult(HRESULT result,
                                          DWORD error,
                                          LPCWSTR result_text) {
  if (was_terminated_ || has_reported_result_ || !protocol_sink_)
    return E_UNEXPECTED;
  has_reported_result_ = true;
  HRESULT hr = protocol_sink_->ReportResult(result, error, result_text);
  if (FAILED(hr))
    LOG16((L"  protocol_sink->ReportResult error = %d\n", hr));
  return hr;
}

HRESULT HttpHandlerBase::CallBeginningTransaction(LPCWSTR url,
                                                  LPCWSTR headers,
                                                  DWORD reserved,
                                                  LPWSTR *additional_headers) {
  if (was_terminated_ || was_aborted_ || !http_negotiate_)
    return E_UNEXPECTED;
  HRESULT hr = http_negotiate_->BeginningTransaction(url, headers, reserved,
                                                     additional_headers);
  if (FAILED(hr))
    LOG16((L"  http_negotiate->BeginningTransaction error = %d\n", hr));
  return hr;
}

HRESULT HttpHandlerBase::CallOnResponse(DWORD status_code,
                                        LPCWSTR response_headers,
                                        LPCWSTR request_headers,
                                        LPWSTR *additional_request_headers) {
  if (was_terminated_ || was_aborted_ || !http_negotiate_)
    return E_UNEXPECTED;
  HRESULT hr = http_negotiate_->OnResponse(status_code, response_headers,
                                           request_headers,
                                           additional_request_headers);
  if (FAILED(hr))
    LOG16((L"  http_negotiate->OnResponse error = %d\n", hr));
  return hr;
}


//------------------------------------------------------------------------------
// Called from both Start and StartEx, this method determines whether or not
// our handler will run in one of three possible modes. The return value and
// the state of the flags is_passingthru_ and is_handling_ indicate which mode
// should be used.
//
// 1) passthru - We delegate all method calls to an instance of the default
//      handler created by the passthru framework. The return value is
//      INET_E_USE_DEFAULT_PROTOCOLHANDLER and is_passingthru_ == true
// 2) bypassed - The Start or StartEx method will return
//      INET_E_USE_DEFAULT_PROTOCOLHANDLER which prevents this instance from
//      being used. The return value is INET_E_USE_DEFAULT_PROTOCOLHANDLER and
//      is_passingthru_ == false
// 3) handling - Our handler will be used to satisfy the request from our cache
//      The return value is S_OK and is_handling_ == true.
//------------------------------------------------------------------------------

class BindInfoReleaser {
 public:
  BindInfoReleaser(BINDINFO *info) : info_(info) {
  }
  ~BindInfoReleaser() {
    ReleaseBindInfo(info_);
  }
 private:
  BINDINFO *info_;
};

#ifdef DEBUG
static std::string16 kDisableUrl(L"http://gears_intercept_disable/");
static std::string16 kEnableUrl(L"http://gears_intercept_enable/");
static std::string16 kResetWarningUrl(L"http://gears_intercept_reset_warning/");
static std::string16 kSelfTestUrl(L"http://gears_self_test/");
static bool g_intercept_enabled = true;
#endif

HRESULT HttpHandlerBase::StartImpl(LPCWSTR url,
                                   IInternetProtocolSink *protocol_sink,
                                   IInternetBindInfo *bind_info,
                                   DWORD flags,  // PI_FLAGS
                                   HANDLE_PTR reserved) {
#ifdef DEBUG
  if (kEnableUrl == url) {
    g_intercept_enabled = true;
    return E_FAIL;
  }
  if (kDisableUrl == url) {
    g_intercept_enabled = false;
    return E_FAIL;
  }
  if (kResetWarningUrl == url) {
    HttpInterceptCheck::ResetHasWarned();
    return E_FAIL;
  }
  if (kSelfTestUrl == url) {
    SelfTest();
    return E_FAIL;
  }
  if (!g_intercept_enabled) {
    LOG16((L"  intercept disabled - using default handler\n"));
    return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
  }
#endif
  if (!protocol_sink || !bind_info) {
    ATLASSERT(protocol_sink);
    ATLASSERT(bind_info);
    return E_INVALIDARG;
  }

  AtomicIncrement(&started_count, 1);

  protocol_sink_ = protocol_sink;

  BINDINFO bindinfo = {0};
  DWORD bindinfoflags = 0;
  bindinfo.cbSize = sizeof(bindinfo);
  HRESULT hr = bind_info->GetBindInfo(&bindinfoflags, &bindinfo);
  if (FAILED(hr)) {
    LOG16((L"GetBindInfo failed, error = %d\n", hr));
    return hr;
  }
  BindInfoReleaser releaser(&bindinfo);

  // TODO(michaeln): Better understand flags, bindinfoFlags, and bindinfoOptions
  // TODO(michaeln): Better understand how our method calls should respond
  // at various points in the 'bind process'. For example, what should
  // IWinInetHttpInfo::QueryInfo return when invoked from within the caller's
  // implementation of IHttpNegotiate::BeginningTransaction?

  LOG16((L"HttpHandlerBase::StartImpl( %s ) "
         L"flags=0x%x, bindinfoflags=0x%x, dwOptions=0x%x\n",
         url, flags, bindinfoflags, bindinfo.dwOptions));
#ifdef DEBUG
  TraceBindFlags(bindinfoflags);
#endif

  // We only directly handle GET and HEAD requests, but we run in
  // passthru mode for all requests to detect when redirects back
  // into our cache occur, see ReportProgress.
  is_head_request_ = false;
  if (bindinfo.dwBindVerb != BINDVERB_GET) {
    if ((bindinfo.dwBindVerb == BINDVERB_CUSTOM) &&
        (wcscmp(HttpConstants::kHttpHEAD, bindinfo.szCustomVerb) == 0)) {
      LOG16((L" HEAD request"));
      is_head_request_ = true;
    } else {
      LOG16((L"  not a GET or HEAD request - "
             L"passing thru to default handler\n"));
      is_passingthru_ = true;
      return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
    }
  }

  // When capturing resources and fetching manifest files we bypass our cache
  CComQIPtr<IServiceProvider> service_provider = protocol_sink;
  if (service_provider) {
    MutexLock lock(&global_mutex_);
    CComPtr<IUnknown> not_used;
    bypass_cache = false;
    service_provider->QueryService(SID_HttpInterceptBypass,
                                   IID_HttpInterceptBypass,
                                   reinterpret_cast<void**>(&not_used));
    if (bypass_cache) {
      LOG16((L"  by passing cache - using default protocol handler\n"));
      return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
    }
  }

  // Fetch a response from our DB for this url
  WebCacheDB* db = WebCacheDB::GetDB();
  if (!db) {
    return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
  }
  bool is_captured = db->Service(url, NULL, is_head_request_, &payload_);

  if (!is_captured) {
    LOG16((L"  cache miss - passing thru to default protocol handler\n"));
    if (kAlertCacheMiss && !ActiveXUtils::IsOnline()) {
      MessageBoxW(NULL, url, L"LocalServer cache miss",
                  MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
    }
    is_passingthru_ = true;
    return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
  }

#ifdef OS_WINCE
  // Re-insert the cache entry to make sure it's there. We add an entry whether
  // or not this is a redirect.
  BrowserCache::EnsureBogusEntry(url);
#endif

  // The requested url may redirect to another location
  std::string16 redirect_url;
  if (payload_.IsHttpRedirect()) {
    // We collapse a chain of redirects and hop directly to the final
    // location for which we have a cache entry.
    int num_redirects = 0;
    while (payload_.IsHttpRedirect()) {
      if (!payload_.GetHeader(HttpConstants::kLocationHeader, &redirect_url)) {
        LOG16((L"  redirect with no location - using default handler\n"));
        is_passingthru_ = true;
        return INET_E_USE_DEFAULT_PROTOCOLHANDLER;
      }
      // Fetch a response for redirect_url from our DB
      if (!db->Service(redirect_url.c_str(), NULL, is_head_request_, &payload_)) {
        // We don't have a response for redirect_url. So report
        // INET_E_REDIRECT_FAILED which causes the aggregating outer
        // object (URLMON) to create a new inner APP and release us.
        LOG16((L"  cache hit, almost - redirect out of cache ( %s )\n",
               redirect_url.c_str()));
        is_handling_ = true;  // set to true so we respond to QueryInfo calls
        return CallReportResult(INET_E_REDIRECT_FAILED,
                                HttpConstants::HTTP_FOUND,
                                redirect_url.c_str());
      }
      
      // Make sure we don't get stuck in an infinite redirect loop.
      ++num_redirects;
      if (num_redirects > HttpConstants::kMaxRedirects) {
        LOG16((STRING16(L"Redirect chain too long %s -> %s"),
               url, redirect_url.c_str()));
        return E_FAIL;
      }
    }
    LOG16((L"  cache hit - redirect ( %s )\n", redirect_url.c_str()));
  } else {
    LOG16((L"  cache hit\n"));
  }

  // Ok, we're going to satisfy this request from our cache
  is_handling_ = true;

  if (service_provider) {
    service_provider->QueryService(__uuidof(IHttpNegotiate),
                                   __uuidof(IHttpNegotiate),
                                   reinterpret_cast<void**>(&http_negotiate_));
  }

  if (http_negotiate_) {
    WCHAR *extra_headers = NULL;
    hr = CallBeginningTransaction(url, L"", 0, &extra_headers);
    CoTaskMemFree(extra_headers);
    if (FAILED(hr)) return hr;
  }

  if (!redirect_url.empty()) {
    hr = CallReportProgress(BINDSTATUS_REDIRECTING, redirect_url.c_str());
    if (FAILED(hr)) return hr;
  }

  // Drive the IHttpNegotiate interface if this interface is provided
  // by the client, this is how XmlHttpRequest accesses header information
  if (http_negotiate_) {
    WCHAR *extra_headers = NULL;
    std::string16 statusline_and_headers;
    statusline_and_headers = payload_.status_line;
    statusline_and_headers += HttpConstants::kCrLf;
    statusline_and_headers += payload_.headers;
    hr = CallOnResponse(HttpConstants::HTTP_OK, statusline_and_headers.c_str(),
                        L"", &extra_headers);
    CoTaskMemFree(extra_headers);
    if (FAILED(hr)) return hr;
  }

  // Drive the IInternetProtocolSink interface
  std::string16 mimetype;
  payload_.GetHeader(HttpConstants::kContentTypeHeader, &mimetype);
  if (!mimetype.empty()) {
#ifdef USE_HTTP_HANDLER_APP
    // TODO(andreip): if the below works for wince, we don't need this ifdef
    hr = CallReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, mimetype.c_str());
    if (FAILED(hr)) return hr;
#else
    if (!StartsWith(mimetype, kMimeImagePrefix)) {
      // We're careful to not report image/png when in fact the resource is gif
      // data as this can cause IE's image handling to get very wedged.
      hr = CallReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, mimetype.c_str());
      if (FAILED(hr)) return hr;
    }
    hr = CallReportProgress(BINDSTATUS_RAWMIMETYPE, mimetype.c_str());
    if (FAILED(hr)) return hr;
#endif
  }

  // The cached_filepath is not provided for head requests
  if (!is_head_request_ && !payload_.cached_filepath.empty()) {
#ifdef OS_WINCE
    // WinCE does not support user-initiated file downloads from the browser.
    //
    // Passing to the browser the filepath to the Gears cache entry can cause
    // the cache entry to get deleted once it has been read. This has been
    // observed when Media Player is used by the browser to open the file.
#else
    // One way to support file downloads is to provide a file path, I don't
    // know if there are other options?
    hr = CallReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE,
                            payload_.cached_filepath.c_str());
    if (FAILED(hr)) return hr;
#endif

    // Report content disposition, including a file path
    // TODO(michaeln): a better parser for this header value, perhaps CAtlRegExp
    // The value uses a character encoding that is particular to this header
    // The value has multiple semi-colon delimited fields
    // See the following for an implementation
    // '//depot/google3/java/com/google/httputil/ContentDisposition.java'
    // '//depot/google3/java/com/google/parser/Parser.java'

#ifdef OS_WINCE
    // On WinCE, it seems that different file types are treated in one of three
    // ways (when not served by Gears) ...
    // 1 - Displayed directly in browser, eg text/html, text/plain
    // 2 - Prompt user for download, eg audio/mpeg
    // 3 - Automatically download and run in other app, eg video/x-ms-wmv
    // Setting the header 'Content-Disposition: attachment' makes no difference
    // to any of these cases.
    //
    // However, when files are served from the local server, reporting
    // BINDSTATUS_CONTENTDISPOSITIONATTACH and supplying the path to the Gears
    // cache entry here causes a change in behaviour ...
    // 1 - No change
    // 2 & 3 - Prompt for download, using the Gears cache entry as the default
    // save location. This then fails.
    //
    // So to maintain the normal behaviour, we do not do anything special for
    // 'Content-Disposition: attachment' on WinCE.
#else
    std::string16 content_disposition;
    payload_.GetHeader(L"Content-Disposition", &content_disposition);
    if (content_disposition.find(L"attachment") != std::string16::npos) {
      hr = CallReportProgress(BINDSTATUS_CONTENTDISPOSITIONATTACH,
                              payload_.cached_filepath.c_str());
      if (FAILED(hr)) return hr;
    }
#endif
  }

  size_t response_size = payload_.data.get() ? payload_.data->size() : 0;

  hr = CallReportData(BSCF_DATAFULLYAVAILABLE |
                      BSCF_FIRSTDATANOTIFICATION |
                      BSCF_LASTDATANOTIFICATION,
                      static_cast<ULONG>(response_size),
                      static_cast<ULONG>(response_size));
  if (FAILED(hr)) return hr;

  std::string16 status_text;
  if (!ParseHttpStatusLine(payload_.status_line, NULL, NULL, &status_text)) {
    // We never expect to get here because when inserting payloads into
    // the database, we verify that they can be parsed w/o error. To make
    // ReportResults happy we return a made up value.
    assert(false);
    status_text = L"UNKNOWN";
  }

  hr = CallReportResult(S_OK, payload_.status_code, status_text.c_str());
  if (FAILED(hr)) return hr;

  LOG16((L"HttpHandlerBase::StartImpl( %s, %d ): YES\n", url, response_size));
  return S_OK;
}

//------------------------------------------------------------------------------
// Called from Read, this method implements Read when we're satisfying
// a request from our cache
//------------------------------------------------------------------------------
HRESULT HttpHandlerBase::ReadImpl(void *buffer,
                                  ULONG byte_count,
                                  ULONG *bytes_read) {
  LOG16((L"HttpHandlerBase::ReadImpl(%d)\n", byte_count));
  if (is_handling_) {
    std::vector<uint8> *data = payload_.data.get();
    size_t bytes_available = data ? (data->size() - read_pointer_) : 0;
    size_t bytes_to_copy = std::min<size_t>(byte_count, bytes_available);

    if (bytes_to_copy != 0) {
      memcpy(buffer, &(*data)[read_pointer_], bytes_to_copy);
      read_pointer_ += bytes_to_copy;
    }

    if (bytes_read != NULL) {
      *bytes_read = static_cast<ULONG>(bytes_to_copy);
    }

    if (bytes_available - bytes_to_copy == 0) {
      LOG16((L"----> HttpHandlerBase::ReadImpl() complete\n"));
      return S_FALSE;
    } else {
      return S_OK;
    }
  } else {
    LOG16((L"----> HttpHandlerBase::ReadImpl() E_UNEXPECTED\n"));
    assert(false);
    return E_UNEXPECTED;
  }
}

//------------------------------------------------------------------------------
// IWinInetInfo::QueryOptionImpl
// Called from QueryOption, this method implements QueryOption when we're
// satisfying a request from our cache
//------------------------------------------------------------------------------
HRESULT HttpHandlerBase::QueryOptionImpl(/* [in] */ DWORD dwOption,
                                         /* [in, out] */ LPVOID pBuffer,
                                         /* [in, out] */ DWORD *pcbBuf) {
#ifdef DEBUG
  LOG16((L"HttpHandlerBase::QueryOption(%s (%d))\n",
         GetWinInetInfoLabel(dwOption), dwOption));
#endif

  if (!is_handling_) {
    return E_UNEXPECTED;
  }

  // TODO(michaeln): determine which options we need to support and how?
  // Options of interests
  //   INTERNET_OPTION_DATAFILE_NAME
  //   INTERNET_OPTION_REQUEST_FLAGS
  //   INTERNET_OPTION_SECURITY_FLAGS
  //   unknown option with dwOption value of 66
  //
  //switch (dwOption) {
  //  case INTERNET_OPTION_REQUEST_FLAGS:
  //  case INTERNET_OPTION_SECURITY_FLAGS:
  //    if (*pcbBuf < sizeof(DWORD)) {
  //      *pcbBuf = sizeof(DWORD);
  //    } else {
  //      DWORD *number_out = reinterpret_cast<DWORD*>(pBuffer);
  //      *number_out = 0;
  //      *pcbBuf = sizeof(DWORD);
  //      return S_OK;
  //    }
  //    break;
  //}

  return S_FALSE;
}


//------------------------------------------------------------------------------
// IWinInetHttpInfo::QueryInfoImpl
// Called from QueryInfo, this method implements QueryInfo when we're
// satisfying a request from our cache
//------------------------------------------------------------------------------
HRESULT HttpHandlerBase::QueryInfoImpl(/* [in] */ DWORD dwOption,
                                       /* [in, out] */ LPVOID pBuffer,
                                       /* [in, out] */ DWORD *pcbBuf,
                                       /* [in, out] */ DWORD *pdwFlags,
                                       /* [in, out] */ DWORD *pdwReserved) {
  DWORD flags = dwOption & HTTP_QUERY_MODIFIER_FLAGS_MASK;
  dwOption &= HTTP_QUERY_HEADER_MASK;
  bool flag_request_headers = (flags & HTTP_QUERY_FLAG_REQUEST_HEADERS) != 0;
  bool flag_systemtime = (flags & HTTP_QUERY_FLAG_SYSTEMTIME) != 0;
  bool flag_number = (flags & HTTP_QUERY_FLAG_NUMBER) != 0;
  bool flag_coalesce = (flags & HTTP_QUERY_FLAG_COALESCE) != 0;

#ifdef DEBUG
  LOG16((L"HttpHandlerBase::QueryInfo(%s (%d), %d)\n",
         GetWinInetHttpInfoLabel(dwOption),
         dwOption,
         pdwFlags ? *pdwFlags : -1));
#endif

  if (!is_handling_) {
    return E_UNEXPECTED;
  }

  // We don't respond to queries about request headers
  if (flag_request_headers) {
    SetLastError(ERROR_HTTP_HEADER_NOT_FOUND);
    return S_FALSE;
  }

  // Most queries have to do with a particular header, but not all.
  // We special case those that don't map to a header, and by default
  // lookup a header value
  const char16 *value = NULL;
  std::string16 value_str;
  int value_len = -1;
  switch (dwOption) {
    case HTTP_QUERY_STATUS_CODE:
      if (IsValidResponseCode(payload_.status_code)) {
        value_str = IntegerToString16(payload_.status_code);
        value = value_str.c_str();
        value_len = value_str.length();
      }
      break;

    case HTTP_QUERY_STATUS_TEXT:
      if (ParseHttpStatusLine(payload_.status_line, NULL, NULL, &value_str)) {
        value = value_str.c_str();
        value_len = value_str.length();
      }
      break;

    case HTTP_QUERY_VERSION:
      if (ParseHttpStatusLine(payload_.status_line, &value_str, NULL, NULL)) {
        value = value_str.c_str();
        value_len = value_str.length();
      }
      break;

    case HTTP_QUERY_REQUEST_METHOD:
      if (is_head_request_) {
        value = HttpConstants::kHttpHEAD;
      } else {
        value = HttpConstants::kHttpGET;
      }
      break;

    case HTTP_QUERY_RAW_HEADERS:
      // The returned value includes the status line followed by each header.
      // Each line is terminated by "\0" and an additional "\0" terminates
      // the list of headers
      value_str = payload_.status_line;
      value_str += HttpConstants::kCrLf;
      value_str += payload_.headers;
      ReplaceAll(value_str,
                 std::string16(HttpConstants::kCrLf),
                 std::string16(1, '\0'));  // string containing an embedded NULL
      value_str.append('\0');
      value = value_str.c_str();
      value_len = value_str.length();
      break;

    case HTTP_QUERY_RAW_HEADERS_CRLF:
      // The returned value includes the status line followed by each header.
      // Each line is separated by a carriage return/line feed sequence
      value_str = payload_.status_line;
      value_str += HttpConstants::kCrLf;
      value_str += payload_.headers;
      value = value_str.c_str();
      value_len = value_str.length();
      break;

    default:
      // Lookup a particular header value
      // TODO(michaeln): flesh out the table of options to header values
      // contained in GetWinInetHttpInfoHeaderName, there are many options
      // not mapped to a header_name yet
      const char16* header_name = GetWinInetHttpInfoHeaderName(dwOption);
      if (header_name && payload_.GetHeader(header_name, &value_str)) {
        value = value_str.c_str();
        value_len = value_str.length();
      }
      break;
  }

  if (!value) {
    SetLastError(ERROR_HTTP_HEADER_NOT_FOUND);
    return S_FALSE;
  }

  // The high-bits of dwOptions contain flags that influence how values
  // are returned

  if (!flags) {
    // Caller is asking for a char string value
    // TODO(michaeln): what character encoding should be used, for now UTF8
    if (value_len < 0) {
      value_len = wcslen(value);
    }
    std::string value8;
    if (!String16ToUTF8(value, value_len, &value8)) {
      return E_FAIL;
    }
    if (!pBuffer) {
      *pcbBuf = value8.length() + 1;
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return S_FALSE;
    } else {
      if (*pcbBuf <= value8.length()) {
        *pcbBuf = value8.length() + 1;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return S_FALSE;
      } else {
        *pcbBuf = value8.length();
        strcpy(reinterpret_cast<char*>(pBuffer), value8.c_str());
        return S_OK;
      }
    }
  } else if (flag_number) {
    // Caller is asking for a 32-bit integer value
    if (*pcbBuf < sizeof(int32)) {
      *pcbBuf = sizeof(int32);
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return S_FALSE;
    } else {
      *pcbBuf = sizeof(int32);
      int32 *number_out = reinterpret_cast<int32*>(pBuffer);
      *number_out = static_cast<int32>(_wtol(value));
      return S_OK;
    }
  } else if (flag_systemtime) {
    // Caller is asking for a SYSTEMTIME value
    if (*pcbBuf < sizeof(SYSTEMTIME)) {
      *pcbBuf = sizeof(SYSTEMTIME);
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return S_FALSE;
    } else {
      *pcbBuf = sizeof(SYSTEMTIME);
      SYSTEMTIME *system_time = reinterpret_cast<SYSTEMTIME*>(pBuffer);
      if (!InternetTimeToSystemTimeW(value, system_time, 0)) {
        return E_FAIL;
      }
      return S_OK;
    }
  } else if (flag_coalesce) {
    // MSDN says this flag is not implemented
    return E_NOTIMPL;
  } else {
    // shouldn't get here
    assert(false);
    return E_FAIL;
  }
}


// Helper to pass BOOL return values from CoInternetQueryInfo
static HRESULT ReturnBoolean(bool value, LPVOID pBuffer,
                             DWORD cbBuffer, DWORD *pcbBuf) {
  if (cbBuffer < sizeof(BOOL)) {
    return S_FALSE;
  }
  if (pBuffer != NULL) {
    *static_cast<BOOL*>(pBuffer) = value ? TRUE : FALSE;
  }
  if (pcbBuf != NULL) {
    *pcbBuf = sizeof(BOOL);
  }
  return S_OK;
}

HRESULT HttpHandlerBase::CoInternetQueryInfo(LPCWSTR pwzUrl,
                                             QUERYOPTION queryOption,
                                             DWORD dwQueryFlags,
                                             LPVOID pBuffer,
                                             DWORD cbBuffer,
                                             DWORD *pcbBuf,
                                             DWORD reserved) {
  // We are careful to only intercept queries about the cachedness of urls.
  // As a page loads, this method is called multiple times for each embedded
  // resource with a queryOption unrelated to cachedness. We don't want to
  // hit the database in those unrelated cases.
  bool result;
  switch (queryOption) {
    case QUERY_USES_NETWORK:
      // Checks if the URL needs to access the network
      result = false;
      break;

    case QUERY_IS_CACHED:
      // Checks if the resource is cached locally
      result = true;
      break;

    case QUERY_IS_CACHED_OR_MAPPED:
      // Checks if this resource is stored in the cache or if it is on a
      // mapped drive (in a cache container)
      result = true;
      break;

    default:
      return INET_E_DEFAULT_ACTION;
  }

  // Determine if we would serve this url from our cache at this time. If
  // not, defer to the default handling.
  WebCacheDB* db = WebCacheDB::GetDB();
  if (!db || !db->CanService(pwzUrl, NULL)) {
    return INET_E_DEFAULT_ACTION;
  }

#ifdef DEBUG
  LOG16((L"HttpHandlerBase::CoInternetQueryInfo(%s (%d), %s)\n",
         GetProtocolInfoLabel(queryOption), queryOption, pwzUrl));
#endif

  return ReturnBoolean(result, pBuffer, cbBuffer, pcbBuf);
}
