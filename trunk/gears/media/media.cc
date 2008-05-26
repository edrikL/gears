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

#include "gears/media/media.h"

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/module_wrapper.h"

GearsMedia::GearsMedia() {
}

GearsMedia::~GearsMedia() {
}

// API methods

void GearsMedia::GetError(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::GetSrc(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetSrc(JsCallContext *context) {
  std::string16 value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  // TODO(aprasath): Implement me
}

void GearsMedia::GetCurrentSrc(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::GetNetworkState(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::GetBufferingRate(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::GetBuffered(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::Load(JsCallContext *context) {
  const int argc = 0;
  JsArgument argv[1];
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetReadyState(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::IsSeeking(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

// The name 'GetCurrentTime' collides with macro of similar name from winbase.h
void GearsMedia::CurrentTime(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetCurrentTime(JsCallContext *context) {
  double value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetDuration(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::IsPaused(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::GetDefaultPlaybackRate(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetDefaultPlaybackRate(JsCallContext *context) {
  double value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &value },
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetPlaybackRate(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetPlaybackRate(JsCallContext *context) {
  double value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetPlayedTimeRanges(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::GetSeekableTimeRanges(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::IsEnded(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::IsAutoPlay(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetAutoPlay(JsCallContext *context) {
  bool value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_BOOL, &value },
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::Play(JsCallContext *context) {
  const int argc = 0;
  JsArgument argv[1];
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::Pause(JsCallContext *context) {
  const int argc = 0;
  JsArgument argv[1];
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetStart(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetStart(JsCallContext *context) {
  double value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEnd(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEnd(JsCallContext *context) {
  double value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetLoopStart(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetLoopStart(JsCallContext *context) {
  double value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetLoopEnd(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetLoopEnd(JsCallContext *context) {
  double value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetPlayCount(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetPlayCount(JsCallContext *context) {
  int64 value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_INT64, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetCurrentLoop(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetCurrentLoop(JsCallContext *context) {
  int64 value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_INT64, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::AddCueRange(JsCallContext *context) {
  const int argc = 6;
  std::string16 className;
  double start = 0.0;
  double end = 0.0;
  bool pauseOnExit = false;
  JsRootedCallback* enterCallback = NULL;
  JsRootedCallback* exitCallback = NULL;

  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &className },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &start },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &end },
    { JSPARAM_REQUIRED, JSPARAM_BOOL, &pauseOnExit },
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &enterCallback },
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &exitCallback }
  };

  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  // TODO(aprasath): Implement me
}

void GearsMedia::RemoveCueRanges(JsCallContext *context) {
  std::string16 className;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &className }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::HasControls(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

void GearsMedia::SetControls(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
}

void GearsMedia::GetVolume(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetVolume(JsCallContext *context) {
  double value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::IsMuted(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsMedia::SetMuted(JsCallContext *context) {
  bool value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_BOOL, &value },
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetMediaBlob(JsCallContext *context) {
  const int argc = 0;
  JsArgument argv[1];
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventCanShowCurrentFrame(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventCanShowCurrentFrame(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventCanPlay(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventCanPlay(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventCanPlayThrough(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventCanPlayThrough(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventDataUnavailable(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventDataUnavailable(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventEnded(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventEnded(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventTimeUpdate(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventTimeUpdate(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventVolumeChange(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventVolumeChange(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventRateChange(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventRateChange(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventDurationChange(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventDurationChange(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventWaiting(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventWaiting(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventPause(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventPause(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventPlay(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventPlay(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventAbort(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventAbort(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventError(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventError(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventStalled(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventStalled(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventEmptied(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventEmptied(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventLoad(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventLoad(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventLoadedFirstFrame(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventLoadedFirstFrame(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventLoadedMetadata(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventLoadedMetadata(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventProgress(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventProgress(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::GetEventBegin(JsCallContext *context) {
  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}

void GearsMedia::SetEventBegin(JsCallContext *context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  context->SetException(STRING16(L"Not Implemented"));
  // TODO(aprasath): Implement me
}
