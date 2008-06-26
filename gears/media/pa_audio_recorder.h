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
//
// The PortAudio based audio recorder for Gears.

#ifndef GEARS_MEDIA_PA_AUDIO_RECORDER_H__
#define GEARS_MEDIA_PA_AUDIO_RECORDER_H__

#include "gears/base/common/basictypes.h"
#include "third_party/portaudio/include/portaudio.h"

// TODO(vamsikrishna): Replace the portaudio calls with calls to thread-safe
// wrapper functions once aprasath's CL 7307473 is submitted (this CL has
// implemetation for thread-safe wrappers).

// TODO(vamsikrishna): Remove these once the recording state attributes
// (channelType, ...) are implemented.
#define NUM_CHANNELS  (2)
#define SAMPLE_FORMAT paInt16
#define SAMPLE_RATE   (16000)
#define SAMPLE_SIZE   (2)
#define FRAME_SIZE    (NUM_CHANNELS * SAMPLE_SIZE)
#define SILENCE       (0)

class GearsAudioRecorder;

// TODO(vamsikrishna): Abstract out the generic AudioRecorder class,
// (when need comes) ?

class PaAudioRecorder {
 public:
  PaAudioRecorder(GearsAudioRecorder *gears_audio_recorder);
  ~PaAudioRecorder();

  // Initialize the devices, buffers, etc.,.
  void Init();

  // Terminate the recording by releasing the devices, buffers, etc.,.
  void Terminate();

  // The recorder will start receiving the media data after this
  // function completes.
  void StartCapture();

  // The recorder will stop receiving the media data after this
  // function completes.
  void StopCapture();

private:
  PaStream *stream_;
  GearsAudioRecorder *gears_audio_recorder_;

  // The callback function invoked by PortAudio. This method is where the input
  // buffer is to be read for the raw audio signal values. Read this buffer for
  // the signal values and lo-and-behold, the audio is recorded on all
  // platforms. Portaudio does all the magic behind the scenes, reading into
  // this buffer from the appropriate audio recording device/driver internally.
  // Look at portaudio.h for more details on the method parameters.
  static int PaCallback(const void *input, void *output, \
                        unsigned long frame_count, \
                        const PaStreamCallbackTimeInfo *time_info, \
                        PaStreamCallbackFlags status_flags, void *user_data);

  DISALLOW_EVIL_CONSTRUCTORS(PaAudioRecorder);
};
#endif  // GEARS_MEDIA_PA_AUDIO_RECORDER_H__
