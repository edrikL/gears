// Copyright 2007, Google Inc.
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

#ifndef GEARS_WORKERPOOL_NPAPI_WORKERPOOL_H__
#define GEARS_WORKERPOOL_NPAPI_WORKERPOOL_H__

#include <vector>

#include "gears/base/common/base_class.h"
#include "gears/base/common/js_runner.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

class GearsWorkerPool
    : public ModuleImplBaseClass,
      public JsEventHandlerInterface {
 public:
  // Need a default constructor to instance objects from the Factory.
  GearsWorkerPool();
  ~GearsWorkerPool();

  // IN: string full_script
  // OUT: int retval
  void CreateWorker(JsCallContext *context);

  // IN: string url
  // OUT: int retval
  void CreateWorkerFromUrl(JsCallContext *context);

  void AllowCrossOrigin(JsCallContext *context);

  // IN: string message
  // IN: int dest_worker_id
  void SendMessage(JsCallContext *context);

  // IN: function_ptr handler
  void SetOnmessage(JsCallContext *context);

  // OUT: function_ptr handler
  void GetOnmessage(JsCallContext *context);

  // IN: function_ptr handler
  void SetOnerror(JsCallContext *context);

  // OUT: function_ptr handler
  void GetOnerror(JsCallContext *context);

#ifdef DEBUG
  void ForceGC(JsCallContext *context);
#endif

 private:
  void Initialize(); // okay to call this multiple times

  void HandleEvent(JsEventType event_type);

  scoped_ptr<JsEventMonitor> unload_monitor_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsWorkerPool);
};

#endif // GEARS_WORKERPOOL_NPAPI_WORKERPOOL_H__
