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

#ifndef GEARS_LOCALSERVER_IE_HTTP_HANDLER_BASE_H__
#define GEARS_LOCALSERVER_IE_HTTP_HANDLER_BASE_H__

#include <urlmon.h>
#include "gears/base/common/mutex.h"
#include "gears/localserver/common/localserver_db.h"
#include "third_party/passthru_app/urlmon_ie7_extras.h"

//------------------------------------------------------------------------------
// HttpHandlerBase is a common base class for two the different implementations
// of intercepting http requests that we've developed.
//------------------------------------------------------------------------------
class HttpHandlerBase {
 public:
  // Returns the number of times the Start or StartEx method has been
  // called since system startup. Used to determine if we're successfully
  // intercepting requests.
  static int GetStartedCount();

  // HttpHandler supports a mechanism to bypass the cache on a
  // per request basis which is used when capturing resources during
  // an application update task or webcapture task. The handler queries
  // its service provider chain for the following SID / IID pair prior
  // to handling a request. If SetBypassCache is invoked as a side
  // effect of this query, the handler will bypass the webcache and
  // use the browser's default http handler.
  static void SetBypassCache();

  // Detects and handles queries about the cached'ness of URLs contained
  // in the localserver database. If the query is for another purpose,
  // returns INET_E_DEFAULT_ACTION.
  static HRESULT CoInternetQueryInfo(LPCWSTR url,
                                     QUERYOPTION option,
                                     DWORD flags,
                                     LPVOID buffer,
                                     DWORD buffer_len,
                                     DWORD *buffer_len_out,
                                     DWORD reserved);
 protected:
  HttpHandlerBase();

  // IInternetProtocol(Ex)
  HRESULT StartImpl(
    /* [in] */ const char16 *url,
    /* [in] */ IInternetProtocolSink *prot_sink,
    /* [in] */ IInternetBindInfo *bind_info,
    /* [in] */ DWORD flags,
    /* [in] */ HANDLE_PTR reserved);

  HRESULT ReadImpl(void *buffer, ULONG byte_count, ULONG *bytes_read);

  // IWinInetInfo
  HRESULT QueryOptionImpl(
    /* [in] */ DWORD dwOption,
    /* [in, out] */ LPVOID pBuffer,
    /* [in, out] */ DWORD *pcbBuf);

  // IWinInetHttpInfo
  HRESULT QueryInfoImpl(
    /* [in] */ DWORD dwOption,
    /* [in, out] */ LPVOID pBuffer,
    /* [in, out] */ DWORD *pcbBuf,
    /* [in, out] */ DWORD *pdwFlags,
    /* [in, out] */ DWORD *pdwReserved);

  // Helpers to call thru to the sink. These wrappers gaurd against calling
  // thru after we've been terminated or aborted. 
  HRESULT CallReportProgress(ULONG status_code, LPCWSTR status_text);
  HRESULT CallReportData(DWORD flags, ULONG progress, ULONG progress_max);
  HRESULT CallReportResult(HRESULT result, DWORD error, LPCWSTR result_text);
  HRESULT CallBeginningTransaction(LPCWSTR url,
                                   LPCWSTR headers,
                                   DWORD reserved,
                                   LPWSTR *additional_headers);
  HRESULT CallOnResponse(DWORD status_code,
                         LPCWSTR response_headers,
                         LPCWSTR request_headers,
                         LPWSTR *additional_request_headers);

  // True when we're passing thru to the default handler created by our base
  bool is_passingthru_;

  // True when we're directly handling the request as opposed to passing thru
  // to the default handler
  bool is_handling_;

  // True if the request is a HEAD request, only valid if 'started_'
  bool is_head_request_;

  // True once we've called sink->ReportResult()
  bool has_reported_result_;

  // True if Abort() was called
  bool was_aborted_;

  // True if Terminate() was called
  bool was_terminated_;

  // The response body we're returning, only valid if 'started_'
  WebCacheDB::PayloadInfo payload_;

  // Read position, only valid if 'is_handling_'
  size_t read_pointer_;

  // Sink related interface pointers
  CComPtr<IInternetProtocolSink> protocol_sink_;
  CComPtr<IHttpNegotiate> http_negotiate_;

  static Mutex global_mutex_;

#ifdef DEBUG
  virtual void SelfTest() {};
#endif
};

#endif  // GEARS_LOCALSERVER_IE_HTTP_HANDLER_BASE_H__
