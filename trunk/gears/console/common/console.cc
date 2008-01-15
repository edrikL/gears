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

#include "gears/base/common/message_service.h"
#include "gears/console/common/log_event.h"

#include "gears/console/common/console.h"


Console::Console(const std::string16 &security_origin,
                 JsRunnerInterface* js_runner)
    : security_origin_(security_origin),
      js_runner_(js_runner) {
}

Console::~Console() { }

void Console::Log(const std::string16 &type,
                  const std::string16 &message,
                  const std::string16 &sourceUrl) { 
  Initialize();
  LogEvent *log_event = new LogEvent(message, type, sourceUrl);
  MessageService::GetInstance()->NotifyObservers(observer_topic_.c_str(),
                                                 log_event);
}

void Console::SetOnlog(JsRootedCallback* callback) {
  Initialize();
  callback_backend_.get()->SetCallback(callback);
}

void Console::ClearOnlog() {
  Initialize();
  callback_backend_.get()->ClearCallback();
}

void Console::Initialize() {
  // Initialize JS logging backend
  if (!callback_backend_.get()) {
    observer_topic_ = STRING16(L"console:logstream-");
    observer_topic_ += security_origin_;

    JsCallbackLoggingBackend *js = new JsCallbackLoggingBackend(
        observer_topic_);
    js->SetJsRunner(js_runner_);
    callback_backend_.reset(js);
  }
}
