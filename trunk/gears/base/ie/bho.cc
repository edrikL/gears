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


//------------------------------------------------------------------------------
// The WinCE BHO implementation is considerably different from the
// implementation for windows desktop. We keep them segregated.
//------------------------------------------------------------------------------
#ifdef OS_WINCE
#include "gears/base/ie/bho.h"
#include <piedocvw.h>
#inlcude "gears/base/common/common.h"
#include "gears/base/common/detect_version_collision.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/installer/common/cab_updater.h"
#include "gears/localserver/ie/http_intercept.h"

// TODO(steveblock): Fix this GUID. See bug 406.
const char16* kGuid = L"%7Bc3fc95dBb-cd75-4f3d-a586-bcb7D004784c%7D";

HWND BrowserHelperObject::browser_window_ = NULL;

// static
HWND BrowserHelperObject::GetBrowserWindow() {
  return browser_window_;
}

STDAPI BrowserHelperObject::SetSite(IUnknown *pUnkSite) {
  LOG16((L"BHO::SetSite() %p, %p\n", this, pUnkSite));
  if (DetectedVersionCollision())
    return S_OK;

  if (pUnkSite == NULL) {
    // We never get here on WinCE so we cannot know when we are
    // uninitialized. SetSite should be called with pUnkSite = NULL
    // when the BHO is unitialized. It seems that this call 
    // is not made on WinCE.
  } else {
    InitializeHttpInterception();

    CComQIPtr<IWebBrowser2> site = pUnkSite;
    ASSERT(site);
    assert(NULL == browser_window_);
    site->get_HWND(reinterpret_cast<long*>(&browser_window_));

    static CabUpdater updater;
    updater.Start(browser_window_, kGuid);
  }
  return S_OK;
}

//------------------------------------------------------------------------------
// Win32 Desktop BHO implementation
//------------------------------------------------------------------------------
#else
#include "gears/base/ie/bho.h"
#include "gears/base/common/common.h"
#include "gears/base/common/detect_version_collision.h"
#include "gears/base/common/exception_handler.h"
#include "gears/base/common/trace_buffers_win32/trace_buffers_win32.h"
#include "gears/base/common/user_config.h"
#include "third_party/googleurl/src/gurl.h"

STDAPI BrowserHelperObject::SetSite(IUnknown *pUnkSite) {
// Only send crash reports for offical builds.  Crashes on an engineer's machine
// during internal development are confusing false alarms.
#ifdef OFFICIAL_BUILD
  if (IsUsageReportingAllowed()) {
    static ExceptionManager exception_manager(false);  // false == only our DLL
    exception_manager.StartMonitoring();
    // Trace buffers only exist in dbg official builds.
#ifdef DEBUG
    exception_manager.AddMemoryRange(g_trace_buffers,
                                     sizeof(g_trace_buffers));
    exception_manager.AddMemoryRange(g_trace_positions,
                                     sizeof(g_trace_positions));
#endif  // DEBUG
  }
#endif  // OFFICIAL_BUILD

  LOG16((L"BHO::SetSite() %p, %p\n", this, pUnkSite));
  if (DetectedVersionCollision())
    return S_OK;

  if (pUnkSite == NULL) {
    BrowserListener::Teardown();
  } else {
    InitializeHttpInterception();
    CComQIPtr<IWebBrowser2> browser2 = pUnkSite;
    if (browser2) {
      BrowserListener::Init(browser2);
    }
  }
  return S_OK;
}

static void StripFragment(GURL *url) {
  if (url->has_ref()) {
    GURL::Replacements strip_fragment;
    strip_fragment.SetRef("", url_parse::Component());
    *url = url->ReplaceComponents(strip_fragment);
  }
}

void BrowserHelperObject::OnBeforeNavigate2(IWebBrowser2 *window,
                                            const CString &url,
                                            bool *cancel) {
  LOG16((L"BHO::OnBeforeNavigate2() %p %p %d %s\n",
         this, window, BrowserBusy(), url.GetString()));

  if (window != BrowserListener::browser()) {
    // Skip subframe navigations.
    return;
  }
  if (handler_check_.IsChecking()) {
    // Sometimes BrowserListener doesn't deliver the completion event.
    handler_check_.CancelCheck();
  }

  CComBSTR location;
  if (FAILED(window->get_LocationURL(&location))) {
    return;
  }
  if (location.m_str && location.m_str[0]) {
    GURL current_url(location.m_str);
    GURL new_url(url.GetString());
    if (!current_url.is_valid() || !new_url.is_valid()) {
      return;
    }
    StripFragment(&current_url);
    StripFragment(&new_url);
    if (current_url == new_url) {
      // Skip navigations between anchors within the same document.
      return;
    }
  }

  handler_check_.StartCheck(url.GetString());
}

void BrowserHelperObject::OnDocumentComplete(IWebBrowser2 *window,
                                             const CString &url) {
  LOG16((L"BHO::OnDocumentComplete() %p %p %d %s\n",
         this, window, BrowserBusy(), url.GetString()));
  if (window != BrowserListener::browser() || BrowserListener::BrowserBusy()) {
    // Skip subframe navigations and ignore spurious calls that can happen
    // prior to actual completion.
    return;
  }
  if (handler_check_.IsChecking()) {
    handler_check_.FinishCheck();
  }
}
#endif  // OS_WINCE
