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

if (isOfficial) {
// Audio API is not available on official builds yet.
} else {
  var player = google.gears.factory.create('beta.audio');
  var recorder = google.gears.factory.create('beta.audiorecorder');
}

function testAudioPlayerStub() {
  if (isOfficial) {
  // Audio API is not available on official builds yet.
  } else {
    // error state
    var err = player.error;
    assertEqual(1, err.MEDIA_ERR_ABORTED);
    assertEqual(2, err.MEDIA_ERR_NETWORK);
    assertEqual(3, err.MEDIA_ERR_DECODE);

    // network state
    player.src = "http://testurl";
    var currentSrc = player.currentSrc;
    assertEqual(0, player.EMPTY);
    assertEqual(1, player.LOADING);
    assertEqual(2, player.LOADED_METADATA);
    assertEqual(3, player.LOADED_FIRST_FRAME);
    assertEqual(4, player.LOADED);
    var networkState = player.networkState;
    var bufferingRate = player.bufferingRate;
    var tRanges = player.buffered;
    var numRanges = tRanges.length;
    var startIndex = tRanges.start(4);
    var endIndex = tRanges.end(4);
    player.load();

    // ready state
    assertEqual(0, player.DATA_UNAVAILABLE);
    assertEqual(1, player.CAN_SHOW_CURRENT_FRAME);
    assertEqual(2, player.CAN_PLAY);
    assertEqual(3, player.CAN_PLAY_THROUGH);
    var readyState = player.readyState;
    var seek = player.isSeeking;

    // playback state
    player.currentTime = 20.5;
    var duration = player.duration;
    var paused = player.paused;
    player.defaultPlaybackRate = 20.5;
    player.playbackRate = 20.5;
    var played = player.played;
    var seekable = player.seekable;
    var ended = player.ended;
    player.autoplay = true;
    player.play();
    player.pause();

    // looping
    player.start = 1.0;
    player.end = 3.5;
    player.loopStart = 5.0;
    player.loopEnd = 6.0;
    player.playCount = 100;
    player.currentLoop = 5;

    // cue ranges
    var enterCallback = function() { alert('Hello, from the future!'); }
    var exitCallback = function() { alert('Hello, from the future!'); }
    player.addCueRange("myCue", 1.0, 5.0, true, enterCallback, exitCallback);
    player.removeCueRanges("myCue");

    //controls
    player.volume = 5.0;
    player.muted = false;

    // blob
    var blob = player.getMediaBlob(); 
  }
}

function testAudioRecorderStub() {
  if (isOfficial) {
  // Audio API is not available on official builds yet.
  } else {
    // error state
    var error = recorder.error;

    // recording state
    var recording = recorder.recording;
    recorder.bitRate = 4;
    recorder.format = 3;
    recorder.bitsPerSample = 64;
    recorder.autoStream = false;
    recorder.record();
    recorder.stop();
    recorder.pause();

    // controls
    recorder.volume = 5.0;
    var pos = recorder.position;
    recorder.muted = false;
    var paused = recorder.paused;

    // cue ranges
    var enterCallback = function() { alert('Hello, from the future!'); }
    var exitCallback = function() { alert('Hello, from the future!'); }
    player.addCueRange("myCue", 1.0, 5.0, true, enterCallback, exitCallback);
    player.removeCueRanges("myCue");

    // blob
    var blob = recorder.getMediaBlob(); 

    // audio specific
    var activityLevel = recorder.activityLevel;
    recorder.silenceLevel = 3;
  }
}

