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

#ifndef GEARS_CONSOLE_COMMON_CONSOLE_H__
#define GEARS_CONSOLE_COMMON_CONSOLE_H__

#include "gears/base/common/js_runner.h"
#include "gears/base/common/string16.h"
#include "gears/console/common/js_callback_logging_backend.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"


class Console {
 public:
  Console(const std::string16 &security_origin, JsRunnerInterface* js_runner);
  ~Console();

  void Log(const std::string16 &type,
           const std::string16 &message,
           const std::string16 &sourceUrl);
  void SetOnlog(JsRootedCallback* callback);
  void ClearOnlog();

 private:
  void Initialize();
  
  std::string16 security_origin_;
  std::string16 observer_topic_;
  JsRunnerInterface* js_runner_;

  // Console provides a default backend for accessing the log
  // stream via a JavaScript callback
  scoped_ptr<JsCallbackLoggingBackend> callback_backend_;

  DISALLOW_EVIL_CONSTRUCTORS(Console);
};

#endif // GEARS_CONSOLE_COMMON_CONSOLE_H__
