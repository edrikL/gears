// Copyright 2005, Google Inc.
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

#include "gears/localserver/npapi/resource_store_np.h"

#include "gears/localserver/npapi/file_submitter_np.h"


//------------------------------------------------------------------------------
// GetName
//------------------------------------------------------------------------------
void GearsResourceStore::GetName(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetRequiredCookie
//------------------------------------------------------------------------------
void GearsResourceStore::GetRequiredCookie(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetEnabled
//------------------------------------------------------------------------------
void GearsResourceStore::GetEnabled(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// SetEnabled
//------------------------------------------------------------------------------
void GearsResourceStore::SetEnabled(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// Capture
//------------------------------------------------------------------------------
void GearsResourceStore::Capture(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// AbortCapture
//------------------------------------------------------------------------------
void GearsResourceStore::AbortCapture(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// IsCaptured
//------------------------------------------------------------------------------
void GearsResourceStore::IsCaptured(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// Remove
//------------------------------------------------------------------------------
void GearsResourceStore::Remove(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// Rename
//------------------------------------------------------------------------------
void GearsResourceStore::Rename(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// Copy
//------------------------------------------------------------------------------
void GearsResourceStore::Copy(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}


//------------------------------------------------------------------------------
// CaptureFile
//------------------------------------------------------------------------------
void GearsResourceStore::CaptureFile(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetCapturedFileName
//------------------------------------------------------------------------------
void GearsResourceStore::GetCapturedFileName(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetHeader
//------------------------------------------------------------------------------
void GearsResourceStore::GetHeader(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetAllHeaders
//------------------------------------------------------------------------------
void GearsResourceStore::GetAllHeaders(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// CreateFileSubmitter
//------------------------------------------------------------------------------
void GearsResourceStore::CreateFileSubmitter(JsCallContext *context) {
  if (EnvIsWorker()) {
    context->SetException(
        STRING16(L"createFileSubmitter cannot be called in a worker."));
    return;
  }

  GComPtr<GearsFileSubmitter> submitter(
        CreateModule<GearsFileSubmitter>(EnvPageJsContext()));
  if (!submitter.get())
    return;  // Create function sets an error message.

  if (!submitter->InitBaseFromSibling(this)) {
    context->SetException(STRING16(L"Error initializing base class."));
    return;
  }

  context->SetReturnValue(JSPARAM_MODULE, submitter.get());
  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// HandleEvent
//------------------------------------------------------------------------------
void GearsResourceStore::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
}
