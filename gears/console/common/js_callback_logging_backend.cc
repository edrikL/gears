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

#include "gears/console/common/js_callback_logging_backend.h"


JsCallbackLoggingBackend::JsCallbackLoggingBackend(const std::string16 &topic)
    : observer_topic_(topic), js_runner_(NULL) {
  LogEvent::RegisterLogEventClass();
}

JsCallbackLoggingBackend::~JsCallbackLoggingBackend() {
  ClearCallback();
}

void JsCallbackLoggingBackend::OnNotify(MessageService *service,
                            const char16 *topic,
                            const NotificationData *data) {
  if (!callback_.get() || js_runner_ == NULL) return;
    
  const LogEvent *log_event = static_cast<const LogEvent *>(data);
  
  scoped_ptr<JsObject> callback_params;
  callback_params.reset(js_runner_->NewObject(NULL));
  if (!callback_params.get()) return;

  callback_params.get()->SetPropertyString(STRING16(L"message"),
                                           log_event->message().c_str());
  callback_params.get()->SetPropertyString(STRING16(L"type"),
                                           log_event->type().c_str());
  callback_params.get()->SetPropertyString(STRING16(L"sourceUrl"),
                                           log_event->sourceUrl().c_str());
  // TODO(aa): date property here should be a JavaScript Date object that has
  // been set to the number of ms as returned by log_event->date(), however
  // JsRunner::NewObject(STRING16(L"Date")) returns null which makes it
  // impossible to construct a Date object to pass back. For now, we just
  // return the raw number of ms.
  // (Note that this cast is technically not safe, but with 2^53 bits in a JS
  // Number (i.e. double), it won't wrap until long after the year 142,000.)
  callback_params.get()->SetPropertyDouble(STRING16(L"date"),
                                           static_cast<const double>(
                                               log_event->date()));
  
  const int argc = 1;
  JsParamToSend argv[argc] = {
    { JSPARAM_OBJECT, callback_params.get() },
  };
  js_runner_->InvokeCallback(callback_.get(), argc, argv, NULL);
}

void JsCallbackLoggingBackend::SetCallback(JsRootedCallback* callback) {
  if (!callback) {
    ClearCallback();
    return;
  }
  // If callback has not been set yet, we need to attach ourself as an
  // observer to log stream
  if (!callback_.get()) {
    callback_.reset(callback);
    MessageService::GetInstance()->AddObserver(this,
                                               observer_topic_.c_str());
  } else {
    callback_.reset(callback);
  }
}

void JsCallbackLoggingBackend::SetJsRunner(JsRunnerInterface* js_runner) {
  js_runner_ = js_runner;
}

void JsCallbackLoggingBackend::ClearCallback() {
  // If a callback is set, we need to remove ourself as an observer from the
  // log stream
  if (callback_.get()) {
    MessageService::GetInstance()->RemoveObserver(this,
                                                  observer_topic_.c_str());
  }
  callback_.reset(NULL);
}
