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

#include "gears/database2/error.h"

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"

#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "gears/base/common/base_class.h"

DECLARE_GEARS_WRAPPER(Database2Error);

void Dispatcher<Database2Error>::Init() {
  RegisterProperty("code", &Database2Error::GetCode, NULL);
  RegisterProperty("message", &Database2Error::GetErrorMessage, NULL);
}

void Database2Error::GetCode(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_INT, code_);
}

void Database2Error::GetErrorMessage(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16, &message_);
}

bool Database2Error::Create(const ModuleImplBaseClass *sibling,
                            int code,
                            std::string16 message,
                            Database2Error **instance) {
  Database2Error *error = 
    CreateModule<Database2Error>(sibling->GetJsRunner());
  if (error && error->InitBaseFromSibling(sibling)) {
    error->code_ = code;
    error->message_ = message;
    *instance = error;
    return true;
  }
  return false;
}