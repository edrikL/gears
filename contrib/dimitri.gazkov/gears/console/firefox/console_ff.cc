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

#include <nsServiceManagerUtils.h> // for NS_IMPL_* and NS_INTERFACE_*
#include <gecko_internal/nsIDOMClassInfo.h> // for *_DOM_CLASSINFO

#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"

#include "gears/console/firefox/console_ff.h"

// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_ADDREF(GearsConsole)
NS_IMPL_RELEASE(GearsConsole)
NS_INTERFACE_MAP_BEGIN(GearsConsole)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsConsoleInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsConsoleInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsConsole)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsConsoleClassName = "GearsConsole";
const nsCID kGearsConsoleClassId = {0xd1df4fba, 0x0ad0, 0x4270, {0x89, 0x1d,
                                     0x4e, 0xbf, 0x33, 0xaf, 0xd8, 0x3b}};
                                    // {d1df4fba-0ad0-4270-891d-4ebf33afd83b}


NS_IMETHODIMP GearsConsole::Log(// const nsAString &type,
                                // const nsAString &message
                                // OPTIONAL string array args
                                ) {
  Initialize();
  std::string16 type;
  std::string16 message;
  JsParamFetcher js_params(this);

  // Get required type and message parameters
  if (js_params.GetCount(false) < 2) {
    RETURN_EXCEPTION(STRING16(L"Type and message parameters required."));
  }
  if (js_params.GetCount(false) > 3) {
    RETURN_EXCEPTION(STRING16(L"Too many parameters."));
  }
  if (!js_params.GetAsString(0, &type)) {
    RETURN_EXCEPTION(STRING16(L"Type parameter must be a string."));
  }
  if (!js_params.GetAsString(1, &message)) {
    RETURN_EXCEPTION(STRING16(L"Message parameter must be a string."));
  }
  if (!type[0]) {
    RETURN_EXCEPTION(STRING16(L"Type is required."));
  }
  if (!message[0]) {
    RETURN_EXCEPTION(STRING16(L"Message is required."));
  }

  // Check for optional args array
  if (!js_params.IsOptionalParamPresent(2, false)) {
    console_.get()->Log(type, message, NULL, EnvPageLocationUrl());
  } else {
    JsArray args;
    if (!js_params.GetAsArray(2, &args)) {
      RETURN_EXCEPTION(STRING16(L"Args parameter must be an array."));
    }
    console_.get()->Log(type, message, &args, EnvPageLocationUrl());
  }
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsConsole::SetOnlog(nsIVariant *in_value) {
  Initialize();
  JsParamFetcher js_params(this);
  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"A valid callback for onlog is required."));
  }
  JsRootedCallback* callback = 0;
  if (!js_params.GetAsNewRootedCallback(0, &callback)) {
    RETURN_EXCEPTION(STRING16(L"A valid callback for onlog is required."));
  }
  console_.get()->SetOnlog(callback);
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsConsole::GetOnlog(nsIVariant **out_value) {
  Initialize();
  *out_value = NULL;
  RETURN_EXCEPTION(STRING16(L"This property is write only."));
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
