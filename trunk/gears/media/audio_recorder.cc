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

#include "gears/media/audio_recorder.h"

#include "gears/base/common/module_wrapper.h"

DECLARE_GEARS_WRAPPER(GearsAudioRecorder);

template<>
void Dispatcher<GearsAudioRecorder>::Init() {
  RegisterProperty("oncanstream", &GearsAudioRecorder::GetEventCanStream,
                   &GearsAudioRecorder::SetEventCanStream);
  RegisterProperty("onrecord", &GearsAudioRecorder::GetEventRecord,
                   &GearsAudioRecorder::SetEventRecord);
  RegisterProperty("onpaused", &GearsAudioRecorder::GetEventPause,
                   &GearsAudioRecorder::SetEventPause);
  RegisterProperty("onended", &GearsAudioRecorder::GetEventEnded,
                   &GearsAudioRecorder::SetEventEnded);
  RegisterProperty("recording", &GearsAudioRecorder::IsRecording, NULL);
  RegisterProperty("paused", &GearsAudioRecorder::IsPaused, NULL);
  RegisterProperty("error", &GearsAudioRecorder::GetError, NULL);
  RegisterProperty("bitRate", &GearsAudioRecorder::GetBitRate,
                   &GearsAudioRecorder::SetBitRate);
  RegisterProperty("format", &GearsAudioRecorder::GetFormat,
                   &GearsAudioRecorder::SetFormat);
  RegisterProperty("bitsPerSample", &GearsAudioRecorder::GetBitsPerSample,
                   &GearsAudioRecorder::SetBitsPerSample);
  RegisterProperty("autoStream", &GearsAudioRecorder::IsAutoStream,
                   &GearsAudioRecorder::SetAutoStream);
  RegisterProperty("volume", &GearsAudioRecorder::GetVolume,
                   &GearsAudioRecorder::SetVolume);
  RegisterProperty("muted", &GearsAudioRecorder::IsMuted,
                   &GearsAudioRecorder::SetMuted);
  RegisterProperty("activityLevel", &GearsAudioRecorder::GetActivityLevel,
                   NULL);
  RegisterProperty("silenceLevel", &GearsAudioRecorder::GetSilenceLevel,
                   &GearsAudioRecorder::SetSilenceLevel);

  RegisterMethod("record", &GearsAudioRecorder::Record);
  RegisterMethod("pause", &GearsAudioRecorder::Pause);
  RegisterMethod("stop", &GearsAudioRecorder::Stop);
  RegisterMethod("getMediaBlob", &GearsAudioRecorder::GetMediaBlob);
  RegisterMethod("addCueRange", &GearsAudioRecorder::AddCueRange);
  RegisterMethod("removeCueRanges", &GearsAudioRecorder::RemoveCueRanges);
}

GearsAudioRecorder::GearsAudioRecorder()
    : ModuleImplBaseClassVirtual("GearsAudioRecorder") {
}

GearsAudioRecorder::~GearsAudioRecorder() {
}

void GearsAudioRecorder::GetActivityLevel(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsAudioRecorder::GetSilenceLevel(JsCallContext *context) {
  // TODO(aprasath): Implement me
}

void GearsAudioRecorder::SetSilenceLevel(JsCallContext *context) {
  int value;
  const int argc = 1;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;
  // TODO(aprasath): Implement me
}
