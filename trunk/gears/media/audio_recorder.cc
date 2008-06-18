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

#include "gears/base/common/common.h"
#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/media/audio_recorder.h"
#include "gears/media/audio_recorder_constants.h"

DECLARE_GEARS_WRAPPER(GearsAudioRecorder);

template<>
void Dispatcher<GearsAudioRecorder>::Init() {
  RegisterProperty("error", &GearsAudioRecorder::GetError, NULL);

  RegisterProperty("recording", &GearsAudioRecorder::GetRecording, NULL);
  RegisterProperty("paused", &GearsAudioRecorder::GetPaused, NULL);
  RegisterProperty("activityLevel", &GearsAudioRecorder::GetActivityLevel,
                   NULL);
  RegisterProperty("duration", &GearsAudioRecorder::GetDuration, NULL);

  REGISTER_CONSTANT(MONO, GearsAudioRecorder);
  REGISTER_CONSTANT(STEREO, GearsAudioRecorder);
  RegisterProperty("channelType", &GearsAudioRecorder::GetChannelType,
                   &GearsAudioRecorder::SetChannelType);
  RegisterProperty("sampleRate", &GearsAudioRecorder::GetSampleRate,
                   &GearsAudioRecorder::SetSampleRate);
  RegisterProperty("bitsPerSample", &GearsAudioRecorder::GetBitsPerSample,
                   &GearsAudioRecorder::SetBitsPerSample);
  RegisterProperty("format", &GearsAudioRecorder::GetFormat,
                   &GearsAudioRecorder::SetFormat);

  RegisterMethod("record", &GearsAudioRecorder::Record);
  RegisterMethod("pause", &GearsAudioRecorder::Pause);
  RegisterMethod("unpause", &GearsAudioRecorder::UnPause);
  RegisterMethod("stop", &GearsAudioRecorder::Stop);

  RegisterProperty("volume", &GearsAudioRecorder::GetVolume,
                   &GearsAudioRecorder::SetVolume);
  RegisterProperty("muted", &GearsAudioRecorder::GetMuted,
                   &GearsAudioRecorder::SetMuted);
  RegisterProperty("silenceLevel", &GearsAudioRecorder::GetSilenceLevel,
                   &GearsAudioRecorder::SetSilenceLevel);

  RegisterMethod("addCueRange", &GearsAudioRecorder::AddCueRange);
  RegisterMethod("removeCueRanges", &GearsAudioRecorder::RemoveCueRanges);

  RegisterMethod("getBlob", &GearsAudioRecorder::GetBlob);

  RegisterProperty("onrecord", &GearsAudioRecorder::GetEventRecord,
                   &GearsAudioRecorder::SetEventRecord);
  RegisterProperty("onprogress", &GearsAudioRecorder::GetEventProgress,
                   &GearsAudioRecorder::SetEventProgress);
  RegisterProperty("onerror", &GearsAudioRecorder::GetEventError,
                   &GearsAudioRecorder::SetEventError);
  RegisterProperty("onpaused", &GearsAudioRecorder::GetEventPause,
                   &GearsAudioRecorder::SetEventPause);
  RegisterProperty("onunpaused", &GearsAudioRecorder::GetEventUnPause,
                   &GearsAudioRecorder::SetEventUnPause);
  RegisterProperty("onvolumechange", &GearsAudioRecorder::GetEventVolumeChange,
                   &GearsAudioRecorder::SetEventVolumeChange);
  RegisterProperty("onended", &GearsAudioRecorder::GetEventEnded,
                   &GearsAudioRecorder::SetEventEnded);
}

GearsAudioRecorder::GearsAudioRecorder()
    : ModuleImplBaseClassVirtual("GearsAudioRecorder"),
      last_error_(AudioRecorderConstants::AUDIO_RECORDER_NO_ERROR),
      recording_(false),
      paused_(false),
      activity_level_(0),
      duration_(0.0),
      // TODO(vamsikrishna): Set with the default value, once spec is done.
      format_(STRING16(L"")),
      volume_(0.5),
      muted_(false),
      silence_level_(0) {
  // TODO(vamsikrishna): Initialize based on device.
  channel_type_ = AudioRecorderConstants::CHANNEL_TYPE_MONO;
  sample_rate_ = 16000;
  bits_per_sample_ = 16;

  // TODO(vamsikrishna): Implement me
}

GearsAudioRecorder::~GearsAudioRecorder() {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetError(JsCallContext *context) {
  if (last_error_ == AudioRecorderConstants::AUDIO_RECORDER_NO_ERROR) {
    context->SetReturnValue(JSPARAM_NULL,
        AudioRecorderConstants::AUDIO_RECORDER_NO_ERROR);
    return;
  }
  JsRunnerInterface *js_runner = this->GetJsRunner();
  scoped_ptr<JsObject> error_object(js_runner->NewObject());
  if (!error_object.get()) {
    context->SetException(STRING16(L"Failed to create new javascript object."));
  }
  error_object->SetPropertyInt(STRING16(L"code"), last_error_);
  error_object->SetPropertyInt(STRING16(L"AUDIO_RECORDER_ERR_ENCODE"),
      AudioRecorderConstants::AUDIO_RECORDER_ERR_ENCODE);
  error_object->SetPropertyInt(STRING16(L"AUDIO_RECORDER_ERR_DEVICE"),
      AudioRecorderConstants::AUDIO_RECORDER_ERR_DEVICE);
  context->SetReturnValue(JSPARAM_OBJECT, error_object.get());
}

void GearsAudioRecorder::GetRecording(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_BOOL, &recording_);
}

void GearsAudioRecorder::GetPaused(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_BOOL, &paused_);
}

void GearsAudioRecorder::GetActivityLevel(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_INT, &activity_level_);
}

void GearsAudioRecorder::GetDuration(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_DOUBLE, &duration_);
}

void GearsAudioRecorder::GetChannelType(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_INT, &channel_type_);
}

void GearsAudioRecorder::SetChannelType(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetSampleRate(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_INT, &sample_rate_);
}

void GearsAudioRecorder::SetSampleRate(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetBitsPerSample(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_INT, &bits_per_sample_);
}

void GearsAudioRecorder::SetBitsPerSample(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetFormat(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16, &format_);
}

void GearsAudioRecorder::SetFormat(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::Record(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::Pause(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::UnPause(JsCallContext *context)  {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::Stop(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetVolume(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_DOUBLE, &volume_);
}

void GearsAudioRecorder::SetVolume(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetMuted(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_BOOL, &muted_);
}

void GearsAudioRecorder::SetMuted(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetSilenceLevel(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_INT, &silence_level_);
}

void GearsAudioRecorder::SetSilenceLevel(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::AddCueRange(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::RemoveCueRanges(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetBlob(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetEventRecord(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::SetEventRecord(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetEventProgress(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::SetEventProgress(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetEventError(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::SetEventError(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetEventPause(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::SetEventPause(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetEventUnPause(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::SetEventUnPause(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetEventVolumeChange(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::SetEventVolumeChange(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::GetEventEnded(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

void GearsAudioRecorder::SetEventEnded(JsCallContext *context) {
  // TODO(vamsikrishna): Implement me
}

