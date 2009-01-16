// Copyright 2009, Google Inc.
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

#ifndef GEARS_LOCALSERVER_IE_HTTP_INTERCEPT_H__
#define GEARS_LOCALSERVER_IE_HTTP_INTERCEPT_H__

#include <windows.h>
#include "gears/base/common/security_model.h"

// Initializes the http intercept layer.
HRESULT InitializeHttpInterception();

// The intercept layer supports a mechanism to bypass the cache on a
// per request basis which is used when capturing resources during
// an application update task or webcapture task. The intercept layer
// queries the service provider chain for the following SID / IID pair prior
// to handling a request. If SetBypassHttpInterception is invoked as a side
// effect of this query, the intercept layer will delegate handling to the
// the browser's default http handling and bypass the LocalServer's cache.
extern const GUID SID_HttpInterceptBypass;
extern const GUID IID_HttpInterceptBypass;
void SetBypassHttpInterception();

// Used to detect when our interception layer is not being called by the
// system and to inform the user if not.
class HttpInterceptCheck {
 public:
  HttpInterceptCheck() : is_checking_(false), handler_started_count_(0) {}
  void StartCheck(const char16 *url);
  void FinishCheck();
  void CancelCheck() { is_checking_ = false; }
  bool IsChecking() const { return is_checking_; }
#ifdef DEBUG
  static void ResetHasWarned();
#endif
 private:
  bool is_checking_;
  int handler_started_count_;
  SecurityOrigin origin_;
};

#endif  // GEARS_LOCALSERVER_IE_HTTP_INTERCEPT_H__
