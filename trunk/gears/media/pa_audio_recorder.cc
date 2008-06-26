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

#include "gears/media/pa_audio_recorder.h"

#include <assert.h>
#include <vector>

#include "gears/media/audio_recorder.h"

#include "third_party/portaudio/include/portaudio.h"

PaAudioRecorder::PaAudioRecorder(GearsAudioRecorder *gears_audio_recorder)
  : stream_(NULL), gears_audio_recorder_(gears_audio_recorder){
  // TODO(vamsikrishna): Use pa_manager framework once aprasath's CL is submitted.
  // Initialize portaudio.
  Pa_Initialize();
  // TODO(vamsikrishna): Check the portaudio error.
}

PaAudioRecorder::~PaAudioRecorder() {
  if (stream_ != NULL) {
    Terminate();
  }
  // TODO(vamsikrishna): Use pa_manager framework once aprasath's CL is submitted.
  // Terminate portaudio.
  Pa_Terminate();
  // TODO(vamsikrishna): Check the portaudio error.
}

void PaAudioRecorder::Init() {
  assert(stream_ == NULL);
  // TODO(vamsikrishna): Open the stream according to the
  // attributes bitrate, etc.,  currently using default stream with
  // default values).
  Pa_OpenDefaultStream(&stream_, NUM_CHANNELS, 0, SAMPLE_FORMAT,
                       SAMPLE_RATE, 1024, PaCallback, this);
  // TODO(vamsikrishna): Set error based on result.
}

void PaAudioRecorder::Terminate() {
  assert(stream_ != NULL);
  Pa_CloseStream(stream_);
  // TODO(vamsikrishna): Set error based on result.

  stream_ = NULL;
}

void PaAudioRecorder::StartCapture() {
  assert(stream_ != NULL);
  Pa_StartStream(stream_);
  // TODO(vamsikrishna): Set error based on result.
}

void PaAudioRecorder::StopCapture() {
  assert(stream_ != NULL);
  Pa_StopStream(stream_);
  // TODO(vamsikrishna): Set error based on result.
}

int PaAudioRecorder::PaCallback(const void *input, void *output, \
                                unsigned long frame_count, \
                                const PaStreamCallbackTimeInfo *time_info, \
                                PaStreamCallbackFlags status_flags, \
                                void *user_data) {
  // TODO(vamsikrishna): Add some preprocessing like noise cancelling ?

  PaAudioRecorder *p_pa_audio_recorder = reinterpret_cast<PaAudioRecorder *>(user_data);
  std::vector<uint8> *p_buffer = &(p_pa_audio_recorder->gears_audio_recorder_->buffer_);

  // TODO(vamsikrishna): Remove FRAME_SIZE and SILENCE once the
  // recording state attributes (channelType, ...) are implemented.

  // Append the recorded data from stream to the buffer.
  // If the recorder is muted then append 'silence'.
  std::vector<uint8>::size_type length = p_buffer->size();

  p_buffer->resize(length + FRAME_SIZE*frame_count, SILENCE);

  bool muted = p_pa_audio_recorder->gears_audio_recorder_->muted_;
  if (!muted) {
    memcpy(&((*p_buffer)[length]), input, FRAME_SIZE*frame_count);
  }

  return paContinue;
}
