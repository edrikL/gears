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

// TODO(playmobil): Move IE & FF implementations to be dispatcher based.

#include "gears/console/npapi/console_np.h"

#include "gears/base/common/module_wrapper.h"

DECLARE_GEARS_WRAPPER(GearsConsole);

template<>
void Dispatcher<GearsConsole>::Init() {
  RegisterMethod("log", &GearsConsole::Log);
  RegisterProperty("onlog", &GearsConsole::GetOnLog, &GearsConsole::SetOnLog);
}

void GearsConsole::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
  console_.reset();
}

void GearsConsole::Log(JsCallContext *context) {
  Initialize();
  
  // Get and sanitize parameters.
  std::string16 type_str;
  std::string16 message;
  JsArray args_array;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &type_str },
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &message },
    { JSPARAM_OPTIONAL, JSPARAM_ARRAY, &args_array},
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  
  if (context->is_exception_set())
    return;
  
  // Check input validity.
  if (type_str.length() == 0) {
    context->SetException(STRING16(L"type cannot be an empty string."));
    return;
  }
  
  if (message.length() == 0) {
    context->SetException(STRING16(L"message cannot be an empty string."));
    return;
  }
  
  // Log
  console_.get()->Log(type_str, message, &args_array, EnvPageLocationUrl());
}

void GearsConsole::GetOnLog(JsCallContext *context) {
  // TODO(playmobil): Make this parameter readable.
  context->SetException(STRING16(L"onlog property is write only."));
  return;
}

void GearsConsole::SetOnLog(JsCallContext *context) {
  Initialize();
  
  // Get & sanitize parameters.
  JsRootedCallback *function = NULL;
  JsArgument argv[] = {
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &function },
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  scoped_ptr<JsRootedCallback> scoped_function(function);
  
  if (context->is_exception_set())
    return;
    
  console_.get()->SetOnlog(scoped_function.release());
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
