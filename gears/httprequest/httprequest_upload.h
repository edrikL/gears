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

#ifndef GEARS_OPENSOURCE_GEARS_HTTPREQUEST_HTTPREQUEST_UPLOAD_H__
#define GEARS_OPENSOURCE_GEARS_HTTPREQUEST_HTTPREQUEST_UPLOAD_H__

#include <string>

#include "gears/base/common/base_class.h"
#include "gears/base/common/js_types.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

class GearsHttpRequestUpload
    : public ModuleImplBaseClassVirtual,
      public JsEventHandlerInterface {
 public:
  static const std::string kModuleName;

  GearsHttpRequestUpload() : ModuleImplBaseClassVirtual(kModuleName) {}

  // IN: -
  // OUT: function
  void GetOnProgress(JsCallContext *context);

  // IN: function
  // OUT:-
  void SetOnProgress(JsCallContext *context);

  // For use by GearsHttpRequest
  void ReportProgress(int64 position, int64 total);
  void ResetOnProgressHandler();

 private:
  scoped_ptr<JsRootedCallback> onprogress_handler_;
  scoped_ptr<JsEventMonitor> unload_monitor_;

  // JsEventHandlerInterface implementation.
  virtual void HandleEvent(JsEventType event_type);

  DISALLOW_EVIL_CONSTRUCTORS(GearsHttpRequestUpload);
};

#endif  // GEARS_OPENSOURCE_GEARS_HTTPREQUEST_HTTPREQUEST_UPLOAD_H__