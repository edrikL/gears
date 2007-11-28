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

#include <assert.h> // TODO(cprince): use DCHECK() when have google3 logging
#include <queue>

#include "gears/workerpool/npapi/workerpool.h"
#include "gears/workerpool/common/workerpool_utils.h"

//
// GearsWorkerPool -- handles the browser glue.
//

GearsWorkerPool::GearsWorkerPool() {
}

GearsWorkerPool::~GearsWorkerPool() {
}

void GearsWorkerPool::SetOnmessage() {
  // TODO(mpcomplete): implement for 1.0.
  Initialize();
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

void GearsWorkerPool::GetOnmessage() {
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

void GearsWorkerPool::SetOnerror() {
  Initialize();
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

void GearsWorkerPool::GetOnerror() {
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

void GearsWorkerPool::CreateWorker() {
  // TODO(mpcomplete): implement for 1.0.
  Initialize();
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

void GearsWorkerPool::CreateWorkerFromUrl() {
  Initialize();
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

void GearsWorkerPool::AllowCrossOrigin() {
  Initialize();
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

void GearsWorkerPool::SendMessage() {
  // TODO(mpcomplete): implement for 1.0.
  Initialize();
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

#ifdef DEBUG
void GearsWorkerPool::ForceGC() {
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}
#endif // DEBUG

void GearsWorkerPool::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
}

void GearsWorkerPool::Initialize() {
  // Monitor 'onunload' to shutdown threads when the page goes away.
  //
  // A thread that keeps running after the page changes can cause odd problems,
  // if it continues to send messages. (This can happen if it busy-loops.)  On
  // Firefox, such a thread triggered the Print dialog after the page changed!
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD,
                                             this));
  }
}
