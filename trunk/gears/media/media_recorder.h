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
// Media recorder class that provides recording abilities

#ifndef GEARS_MEDIA_MEDIA_RECORDER_H__
#define GEARS_MEDIA_MEDIA_RECORDER_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"

class GearsMediaRecorder {
 public:
  GearsMediaRecorder();

  // ---- ERROR STATE ----
  // readonly attribute MediaError error;
  void GetError(JsCallContext *context);

  // ---- RECORDING STATE ----
  // attribute DOMString destination;
  void GetDestination(JsCallContext *context);
  void SetDestination(JsCallContext *context);

  // readonly attribute boolean recording;
  void IsRecording(JsCallContext *context);

  // attribute int bitRate;
  void GetBitRate(JsCallContext *context);
  void SetBitRate(JsCallContext *context);

  // attribute int format;
  void GetFormat(JsCallContext *context);
  void SetFormat(JsCallContext *context);

  // attribute int bitsPerSample;
  void GetBitsPerSample(JsCallContext *context);
  void SetBitsPerSample(JsCallContext *context);

  // attribute boolean autoStream;
  void IsAutoStream(JsCallContext *context);
  void SetAutoStream(JsCallContext *context);

  // ---- CONTROLS ----
  //  attribute float volume;
  void GetVolume(JsCallContext *context);
  void SetVolume(JsCallContext *context);

  // readonly attribute float position;
  void GetPosition(JsCallContext *context);

  // attribute boolean muted;
  void IsMuted(JsCallContext *context);
  void SetMuted(JsCallContext *context);

  // readonly attribute boolean paused;
  void IsPaused(JsCallContext *context);

  // ---- CUE RANGES ----
  // void addCueRange(in DOMString className, in float start, in float end,
  //   in boolean pauseOnExit,
  //   in VoidCallback enterCallback, in VoidCallback exitCallback);
  void AddCueRange(JsCallContext *context);

  // void removeCueRanges(in DOMString className);
  void RemoveCueRanges(JsCallContext *context);

  // ---- METHODS ----
  void Record(JsCallContext *context);
  void Pause(JsCallContext *context);
  void Stop(JsCallContext *context);

  // ---- BLOB ----
  void GetMediaBlob(JsCallContext *context);

  // ---- EVENTS ----
  // canstream, pause, record, ended
  void GetEventCanStream(JsCallContext *context);
  void SetEventCanStream(JsCallContext *context);
  void GetEventPause(JsCallContext *context);
  void SetEventPause(JsCallContext *context);
  void GetEventRecord(JsCallContext *context);
  void SetEventRecord(JsCallContext *context);
  void GetEventEnded(JsCallContext *context);
  void SetEventEnded(JsCallContext *context);

 protected:
  virtual ~GearsMediaRecorder();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GearsMediaRecorder);
};
#endif  // GEARS_MEDIA_MEDIA_RECORDER_H__
