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

function testDefaultAttributeValues() {
  if (isOfficial) {
    // Audio API is not available on official builds yet.
  } else {
    var recorder = google.gears.factory.create('beta.audiorecorder');

    assertEqual(null, recorder.error);

    assertEqual(false, recorder.recording);
    assertEqual(false, recorder.paused);
    assertEqual(0, recorder.activityLevel);
    assertEqual(0.0, recorder.duration);
    // TODO(vamsikrishna): Test the value of format.

    assertEqual(0.5, recorder.volume);
    assertEqual(false, recorder.muted);
    assertEqual(0, recorder.silenceLevel);
  }
}

// TODO(vamsikrishna): Enable the below tests after figuring out how to
// mock the audio recording device. 

/*function testBasicRecording() {
  if (isOfficial) {
    // Audio API is not available on official builds yet.
  } else {
    var recorder = google.gears.factory.create('beta.audiorecorder');
    
    assertEqual(false, recorder.recording);
    recorder.record();
    assertEqual(true, recorder.recording);
    recorder.stop();
    assertEqual(false, recorder.recording);
    // TODO(vamsikrishna): Compare the blob from the recorder, once the
    // getBlob() and a mock recorder are implemented.
  }
}

function testPauseWhileNotRecording() {
  if (isOfficial) {
    // Audio API is not available on official builds yet.
  } else {
    var recorder = google.gears.factory.create('beta.audiorecorder');
    
    var old_paused_value = recorder.paused;
    recorder.pause();
    assertEqual(old_paused_value, recorder.paused);
    // TODO(vamsikrishna): Assert that the 'pause' event is not raised. 
  }
}

function testPauseWhileRecording() {
  if (isOfficial) {
    // Audio API is not available on official builds yet.
  } else {
    var recorder = google.gears.factory.create('beta.audiorecorder');
    
    recorder.record();
    recorder.pause();
    assertEqual(true, recorder.recording);
    assertEqual(true, recorder.paused);
    // TODO(vamsikrishna): Assert that the 'pause' event is raised.
    recorder.unpause();
    assertEqual(true, recorder.recording);
    assertEqual(false, recorder.paused);
    // TODO(vamsikrishna): Assert that the 'unpause' event is raised.
    recorder.stop();
  }
}

// TODO(vamsikrishna): Add a test that tests that calling pause repeatedly
// is equivalent calling it once (the 'pause' event should not be raised
// repeatedly).

function testMuteWhileNotRecording() {
  if (isOfficial) {
    // Audio API is not available on official builds yet.
  } else {
    var recorder = google.gears.factory.create('beta.audiorecorder');
    
    var old_muted_value = recorder.muted;
    recorder.muted = true;
    assertEqual(old_muted_value, recorder.muted);
    // TODO(vamsikrishna): Assert that the 'volumechange' event is not raised.
  }
}

function testMuteWhileRecording() {
  if (isOfficial) {
    // Audio API is not available on official builds yet.
  } else {
    var recorder = google.gears.factory.create('beta.audiorecorder');

    recorder.record();
    recorder.muted = true;
    assertEqual(true, recorder.recording);
    assertEqual(true, recorder.muted);
    // TODO(vamsikrishna): Assert that the 'volumechange' event is raised.
    recorder.muted = false;
    assertEqual(true, recorder.recording);
    assertEqual(false, recorder.muted);
    // TODO(vamsikrishna): Assert that the 'volumechange' event is raised.
    recorder.stop();
  }
}

// TODO(vamsikrishna): Add a test that tests that setting muted repeatedly
// is equivalent to setting it once (the 'volumechange' event should not be raised
// repeatedly).

// TODO(vamsikrishna): Add tests that test the pause and mute functionalities
// when used together.*/
