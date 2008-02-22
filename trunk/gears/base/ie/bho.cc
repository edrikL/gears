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
#ifdef WINCE
#include "gears/base/ie/activex_utils.h"
#endif
#include "gears/base/common/exception_handler_win32.h"
#include "gears/base/ie/bho.h"
#include "gears/base/ie/detect_version_collision.h"
#include "gears/factory/common/factory_utils.h"
#ifdef WINCE
#include "gears/installer/iemobile/cab_updater.h"
#endif
#include "gears/localserver/ie/http_handler_ie.h"

STDAPI BrowserHelperObject::SetSite(IUnknown *pUnkSite) {
#ifdef WIN32
// Only send crash reports for offical builds.  Crashes on an engineer's machine
// during internal development are confusing false alarms.
#ifdef OFFICIAL_BUILD
  static ExceptionManager exception_manager(false);  // false == only our DLL
  exception_manager.StartMonitoring();
#endif
#endif

  if (DetectedVersionCollision())
    return S_OK;

  if (pUnkSite == NULL) {
    // We never get here on WinCE so we cannot know when we are
    // uninitialized. SetSite should be called with pUnkSite = NULL
    // when the BHO is unitialized. It seems that this call 
    // is not made on WinCE.
    LOG16((L"SetSite(): pUnkSite is NULL\n"));
  } else {
    HttpHandler::Register();
#ifdef WINCE
    static CabUpdater updater;
    CComQIPtr<IWebBrowser2> site = pUnkSite;
    ASSERT(site);
    updater.SetSiteAndStart(site);    
#endif
  }
  return S_OK;
}
