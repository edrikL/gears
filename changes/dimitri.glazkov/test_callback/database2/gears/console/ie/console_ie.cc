// Copyright 2008, Google Inc.
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

#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"

#include "gears/console/ie/console_ie.h"


STDMETHODIMP GearsConsole::log(BSTR type, BSTR message, VARIANT args) {
  Initialize();

  // Get required type and message parameters
  if (!type || !type[0]) {
    RETURN_EXCEPTION(STRING16(L"Type is required."));
  }
  if (!message || !message[0]) {
    RETURN_EXCEPTION(STRING16(L"Message is required."));
  }

  // Check for optional args array
  if (!ActiveXUtils::OptionalVariantIsPresent(&args)) {
    console_.get()->Log(type, message, NULL, EnvPageLocationUrl());
  } else {
    JsArray args_array;
    if (!args_array.SetArray(args, NULL)) {
      RETURN_EXCEPTION(STRING16(L"Args parameter must be an array."));
    }
    console_.get()->Log(type, message, &args_array, EnvPageLocationUrl());
  }
  RETURN_NORMAL();
}

STDMETHODIMP GearsConsole::put_onlog(const VARIANT *in_value) {
  Initialize();
  if (ActiveXUtils::VariantIsNullOrUndefined(in_value)) {
    console_.get()->ClearOnlog();
  } else if (in_value->vt == VT_DISPATCH) {
    console_.get()->SetOnlog(new JsRootedCallback(NULL, *in_value));
  } else {
    RETURN_EXCEPTION(STRING16(L"The onlog callback must be a function."));
  }
  RETURN_NORMAL();
}

void GearsConsole::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
  console_.reset();
}

void GearsConsole::Initialize() {
  // Initialize console
  if (!console_.get()) {
    console_.reset(new Console(EnvPageSecurityOrigin().url(),
                               GetJsRunner()));
  }
  // Create an event monitor to alert us when the page unloads.
  if (!unload_monitor_.get()) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD,
                                             this));
  }
}
