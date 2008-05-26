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

#include "gears/media/media_recorder.h"

GearsMediaRecorder::GearsMediaRecorder() {
}

GearsMediaRecorder::~GearsMediaRecorder() {
}

void GearsMediaRecorder::GetError(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetDestination(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetDestination(JsCallContext *context) {
  std::string16 value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::IsRecording(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetBitRate(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetBitRate(JsCallContext *context) {
  int64 value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_INT64, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetFormat(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetFormat(JsCallContext *context) {
  int value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetBitsPerSample(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetBitsPerSample(JsCallContext *context) {
  int64 value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_INT64, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetVolume(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetVolume(JsCallContext *context) {
  double value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::IsAutoStream(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetAutoStream(JsCallContext *context) {
  bool value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_BOOL, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::Record(JsCallContext *context) {
  const int argc = 0;
  JsArgument argv[1];
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::Pause(JsCallContext *context) {
  const int argc = 0;
  JsArgument argv[1];
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::Stop(JsCallContext *context) {
  const int argc = 0;
  JsArgument argv[1];
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::AddCueRange(JsCallContext *context) {
  const int argc = 6;
  std::string16 className;
  double start = 0.0;
  double end = 0.0;
  bool pauseOnExit = false;
  JsObject enterCallback;
  JsObject exitCallback;

  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &className },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &start },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &end },
    { JSPARAM_REQUIRED, JSPARAM_BOOL, &pauseOnExit },
    { JSPARAM_REQUIRED, JSPARAM_OBJECT, &enterCallback },
    { JSPARAM_REQUIRED, JSPARAM_OBJECT, &exitCallback }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  JsRootedCallback* enterHandleFunction = NULL;
  JsRootedCallback* exitHandleFunction = NULL;
  if (!(enterCallback.GetPropertyAsFunction(STRING16(L"handleEvent"),
    &enterHandleFunction))) {
    context->SetException(STRING16
      (L"Callback object does not implement the VoidCallback interface"));
  }
  if (!(exitCallback.GetPropertyAsFunction(STRING16(L"handleEvent"),
    &exitHandleFunction))) {
    context->SetException(STRING16
     (L"Callback object does not implement the VoidCallback interface"));
  }
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::RemoveCueRanges(JsCallContext *context) {
  std::string16 className;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &className }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::IsPaused(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::IsMuted(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetMuted(JsCallContext *context) {
  bool value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_BOOL, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetPosition(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetMediaBlob(JsCallContext *context) {
  const int argc = 0;
  JsArgument argv[1];
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetEventCanStream(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetEventCanStream(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetEventRecord(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetEventRecord(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetEventPause(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetEventPause(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::GetEventEnded(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMediaRecorder::SetEventEnded(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  // TODO(aprasath): Implement me
}
