# Introduction #
The purpose of this module is to provide an effective (and simple) Audio API through Gears.

# Requirements #
  1. Playback & Recording (Basic)
  1. Multiple channels and mixing (Basic)
  1. Editing (Adv)
  1. 3D positioned audio (Adv)

# Scope #
The scope of this document is limited to items (1) and (2) above. Items (3) and (4) are briefly discussed.

# High level Design #
The Audio API is designed similar to the [HTMLAudioElement](http://www.whatwg.org/specs/web-apps/current-work/#audio) in the HTML5 specification. The audio playback, transform, mixing and recording all happen in the CPU (since CPUs today are fast and basic audio mixing/processing takes very little usage).

  * **_Libraries:_** For the actual platform dependent audio play and record capabilities, we will build on top of the open source [PortAudio](http://www.portaudio.com) project. To know the stack portaudio is supported on, look at http://www.portaudio.com/status.html. SDL seems to be another possible candidate.

  * **_Formats:_**
    * _WAV_: As proposed by the HTML5 spec, the WAV format will be supported with audio in the PCM format.
    * _OGG_: The OGG format will be supported with audio encoded using the SPEEX format. The opensource libOGG and libSPEEX projects will be used, pending discussion about patent and license issues.
    * _MP3_: There is no clear plan for MP3 yet. No open source or free integer-only MP3 encoder available. LAME codec is LGPL, and not integer-only and hence not suitable. Madplay codec is integer-only decode, but GPL. Also not suitable. Probably no suitable codecs exist for us.
> > Integer-only encoding needs to be supported so that this can work on mobiles (where FP emulation is way slower than realtime). The only open/free codec known with integer-only encoding seems to be _speex_. If royalty and license issues cause trouble with codecs, speex can be the fallback since most audio recorded will be human speech.

  * **_Blobs:_** The Blob is used as a data structure to hold audio data during playback and recording. Streaming in and out capabilities need to be added to such blobs, i.e. get callbacks from blobs as the data is being received from a url, and the ability to give data in chunks to a blob (as it becomes available) when it sends out to a remote url.
    * Blobs can be either in-memory or backed by a file or a stream.
    * For a given URL, the LocalServer can stream data into it and expose a handle to the blob to the programmer, if needed.
    * The LocalServer can also expose contents of a blob (initiated by other means) via HTTP through an URL.

  * **_Classes:_** 'Audio' and AudioRecorder' classes are defined separately to handle playback and recording respectively. The 'Audio' class is designed in line with the HTML5 _HTMLAudioElement_.

  * **_Mixing:_** The 'audio.play()' methods need to be asynchronous, so that, multiple channels/audio files can be played at the same time. It is assumed that the underlying library/drivers will handle the mixing. If large number of channels could not be mixed at the same time by the OS audio interface, a software mixer which works under the hood and does the mixing of all played audio into a single stream for the platform audio interface, will need to be added.

  * **_Security considerations:_**
    * Get permission from the user before recording, to avoid snooping into conversations happening at the user end. Also, provide the user a visible/audible indication whenever recording is triggered on.
    * Streaming content onto player - interactive files could be malicious.

# JS interface #

## Audio Playback ##

**_Media_ class (borrowed from HTML5 spec):**
We model the API based on _HTMLAudioElement_ in the HTML5 spec. Look at http://www.whatwg.org/specs/web-apps/current-work/#media5 for an explanation of the members of this class. We include an additional method to get a handle to the Blob, and an additional metadata field.
```
Media class
{  
  // error state
  readonly attribute MediaError error;

  // network state
           attribute DOMString src;
  readonly attribute DOMString currentSrc;
  const unsigned short EMPTY = 0;
  const unsigned short LOADING = 1;
  const unsigned short LOADED_METADATA = 2;
  const unsigned short LOADED_FIRST_FRAME = 3;
  const unsigned short LOADED = 4;
  readonly attribute unsigned short networkState;
  readonly attribute float bufferingRate;
  readonly attribute TimeRanges buffered;
  void load();

  // ready state
  const unsigned short DATA_UNAVAILABLE = 0;
  const unsigned short CAN_SHOW_CURRENT_FRAME = 1;
  const unsigned short CAN_PLAY = 2;
  const unsigned short CAN_PLAY_THROUGH = 3;
  readonly attribute unsigned short readyState;
  readonly attribute boolean seeking;

  // playback state
           attribute float currentTime;
  readonly attribute float duration;
  readonly attribute boolean paused;
           attribute float defaultPlaybackRate;
           attribute float playbackRate;
  readonly attribute TimeRanges played;
  readonly attribute TimeRanges seekable;
  readonly attribute boolean ended;
           attribute boolean autoplay;
  void play();
  void pause();

  // looping
           attribute float start;
           attribute float end;
           attribute float loopStart;
           attribute float loopEnd;
           attribute unsigned long playCount;
           attribute unsigned long currentLoop;

  // cue ranges
  void addCueRange(in DOMString className, in float start, in float end, 
        in boolean pauseOnExit, 
        in VoidCallback enterCallback, in VoidCallback exitCallback);
  void removeCueRanges(in DOMString className);

  // controls
           attribute boolean controls;
           attribute float volume;
           attribute boolean muted;

  // load from blob
  void loadBlob(blob);
   
  // metadata
           attribute map metadata;
};
```

**_Events from Media class:_**
The media class also fires the following events (HTML5 does not exactly define some of these events and the handling framework yet). Section _'3.2.9.12 - Event summary'_ in http://www.whatwg.org/specs/web-apps/current-work/ helps understand what these events are, and when they are fired. Handlers of these events can decide what to do on event notifications:
_begin, progress, loadedmetadata, loadedfirstframe, load, abort, error, emptied, stalled, play, pause, waiting, timeupdate, ended, dataunavailable, canshowcurrentframe, canplay, canplaythrough, ratechange, durationchange, volumechange_

**_Audio class : Media_**

The additional properties below are sound channel specific properties.
```

// an object can be got from 
// - google.gears.factory.create('beta.audio')
Audio class : Media
{
  // TODO: prefer int (number of channels) over enumeration ?
  // Channel type
  const unsigned short MONO = 1;
  const unsigned short STEREO = 2;
  readonly attribute short channelType;

  // current amplitude of left channel [0 to 1]
  readonly attribute float leftPeak;

  // current amplitude of right channel [0 to 1]
  readonly attribute float rightPeak;

  // tranform array attributes follow:
  // how much of left input goes to the left output
  const unsigned short LEFT_TO_LEFT = 1;
  // how much of left input goes to the right output
  const unsigned short LEFT_TO_RIGHT = 2;
  // how much of right input goes to the left output
  const unsigned short RIGHT_TO_LEFT = 3;
  // how much of right input goes to the right output
  const unsigned short RIGHT_TO_RIGHT = 4;
  // bass control [1-10]
  const unsigned short BASS = 5;
  // treble control [1-10]
  const unsigned short TREBLE = 6;
  // An array specifying the transform to be applied to this audio. 
  // Array has a list of 'attribute:numeric' value pairs.
  //     Example. [RIGHT_TO_RIGHT:12, BASS:15]
         attribute Array transform;
};
```

## Audio Recording ##

**_AudioRecorder class_**
```
// an object of this class can be got from 
// google.gears.factory.create('beta.audiorecorder')
AudioRecorder class
{
  // ---- error state ----
  readonly attribute AudioRecorderError error;
 
  // ---- recording state ----
  // says whether recorder is currently recording or not
  readonly attribute boolean recording;
  // says whether recorder is paused or not
  readonly attribute boolean paused;
  // the amount of sound detected by the microphone
  // 0 - no sound detected to 100 - maximum sound detected
  readonly attribute int activityLevel;
  // specifies the length (in milli seconds) of the audio recorded
  readonly attribute float duration;
 
  // number of channels, currently can be 1 (mono) or 2 (stereo)
           attribute int numberOfChannels;
  // sample rate for the recording
           attribute float sampleRate;
  // sample type for the recording, possible values need to be defined
  // signed 16 bit little endian linear PCM
  const unsigned short S16_LE = 0;
           attribute short sampleFormat;
  // audio file type (container and codec), possible values need to be defined
           attribute string type; 
 
  void record();
  void pause();
  void unpause();
  void stop();
 
  // ---- controls ----
  // 0.0 - silent to 1.0 - loudest
           attribute float volume;
           attribute boolean muted;
  // the amount of sound required to activate the microphone
  // 0 - capture even minutest sound to 100 - capture only loudest sound
           attribute int silenceLevel;
 
  // ---- cue ranges ----
  // provides ability to set callbacks at specific points in playback time.
  // similar to API in Audio class. Look at HTML5 spec for explanation.
  void addCueRange(in DOMString className, in float start, in float end, 
                  in boolean pauseOnExit,
                  in VoidCallback enterCallback, in VoidCallback exitCallback);
  void removeCueRanges(in DOMString className);
 
  // ---- access blob ----
  // returns handle to the blob object containing the audio data
  Blob getBlob();
};     
```

**_AudioRecorderError class_**
```
AudioRecorderError class
{
  // could not record with the specified recording state (channelType, ...)
  const unsigned short AUDIO_RECORDER_ERR_ENCODE = 1;
  // there is problem accessing the device or there is no device
  const unsigned short AUDIO_RECORDER_ERR_DEVICE = 2;
  readonly attribute unsigned short code;
}
```

**_Events from AudioRecorder class:_**
The AudioRecorder class throws the following events, under the stated preconditions:
  * _record_ - recording started
  * _progress_ - is fired periodically during recording
  * _error_ - an error occurred while recording
  * _pause_ - when recorder becomes paused
  * _unpause_ - when recorder becomes unpaused
  * _volumechange_ - either the recorder's volume is changed or is muted/unmuted
  * _ended_ - finished recording

**_Description of the various attributes_**


> TODO: Describe other attributes also.

  * _sampleRate_ - sample rate for recording specified in Hz. On setting with an arbitrary value, the AudioRecorder tries to pick the nearest possible sample rate. On getting returns the sample rate actually set. Cannot be modified while recording.

  * _sampleFormat_ - Possible values need to be defined. Currently, only S16\_LE (signed 16 bit little endian linear PCM) is supported. Cannot be modified while recording.

  * _type_ - Possible values need to be defined. Currenlty, only "audio/wav" is supported. Cannot be modified while recording.

  * _volume_ - volume ranges from 0.0 (silent) to 1.0 (loudest) and intialized to 0.5 by default. The range, 0.0 - 1.0, and the default value, 0.5, have been taken from HTML5 spec. Note that this is a soft volume and is no way concerned with system/device volume. Can be modified while recording.

**_Default values for the various attributes_**

| **Attribute**   | **New recorder object** | **Previously recorded**      |
|:----------------|:------------------------|:-----------------------------|
|                 |                         | **(currently stopped)**      |
| error           | null                    | last error occurred          |
| recording       | false                   | false                        |
| paused          | false                   | false                        |
| activityLevel   | 0                       | 0                            |
| duration        | 0ms                     | previous recorded duration   |
| channelType     | based on device         | previous channelType         |
| sampleRate      | based on device         | previous sampleRate          |
| sampleFormat    | based on device         | previous sampleFormat        |
| type            | audio/wav               | previous type                |
| volume          | 0.5                     | previous volume              |
| muted           | false                   | false                        |
| silenceLevel    | 0                       | previous silenceLevel        |

**_Recording the audio_**

The **recording** attribute represents whether the audio recorder is recording or not.
The **paused** attribute represents whether the audio recorder is paused or not.
The **muted** attribute indicates whether the audio recorder is capturing silence.

The audio data that is captured by the audio recorder is according to the recording-
state (channelType, sampleRate, bitsPerSample, format) and the recording controls
(volume, muted).

**record()**
  1. If audio recorder is already recording, it has to be aborted and all the audio data that is already recorded can be discarded.
  1. Check that recording can be done according to the given recording state (channelType, ...) else set error to AUDIO\_RECORDER\_ERR\_ENCODE and return.
  1. Get handle to the recording device. If failed to get the device set error to AUDIO\_RECORDER\_ERR\_DEVICE and return.
  1. The recording attribute should be set to true, paused attribute should be set to false, duration should to set to 0, muted attribute should be set to false. [the 'record' event is fired, 'progress' starts firing periodically ](.md)
  1. The audio recorder starts capturing the audio data.

**pause()**
  1. If the audio recorder is not recording, then return.
  1. If the audio recorder is already paused, then return.
  1. The paused attribute should be set to true. [the 'pause' event is fired](.md)
  1. The audio recorder stops capturing the audio data.

**unpause()**
  1. If the audio recorder is not recording, the return.
  1. If the audio recorder is not paused, the return.
  1. The paused attribute should be set to false. [the 'unpause' event is fired ](.md)
  1. The audio recorder starts capturing the audio data again.

**stop()**
  1. If the audio recorder is not recording, then return.
  1. The audio recorder stops capturing the audio data.
  1. The recording attribute should be set to false. [the 'ended' event is fired ](.md)
  1. Release the recording device.

**_Privacy_**

It must be clear to users when an application is using the AudioRecorder API. We could implement one or both of the following UI elements:

  1. _A separate dialog from the Gears security dialog to enable the AudioRecorder API._ If the general-purpose dialog gave access to the microphone, it would be easy for users to forget they allowed access to Gears, or to fail to realize enabling Gears also gives access to their microphone.

  1. _Some persistent UI that indicates the AudioRecorder API is being used._ For example, there could be a large red dot across the bottom of the browser. Perhaps this red dot should blink, indicating that the audio is being recorded, so that the user cannot forget it is being used.

**_Use cases_**

We want to be able to support the following use-cases w.r.t. recording:
  * Send the recorded audio to a HTTP post URL/webservice (upload) - The destination attribute can be used to set the post URL to which the recorded data need to be streamed to.
  * Save the recorded audio to a local file (after compressing to some format specified) - One can get a handle to the blob and export it to a local file (using the export method in the blob class below).
  * Add the recorded audio as an attachment in a mail - The mail application should be able to access the blob data in chunks through XHR calls that invoke the slice() method (after it is compressed and uuencoded).

## Others (borrowed from HTML5 spec) ##

```
interface TimeRanges {
  readonly attribute unsigned long length;
  float start(in unsigned long index);
  float end(in unsigned long index);
};

interface VoidCallback {
  void handleEvent();
};
```

# Deviations from HTML5 #
  * an additional metadata field in Media class
  * additional channel properties specified in the Audio class
  * separate class for AudioRecorder

# Sample code #

**_Simple example - 2D playback flow_**:
```
var audio = google.gears.factory.create('beta.audio');
audio.src = 'http://blahblahblob.com/sampleaudio.wav';
audio.load();
audio.play();

//a handle to the blob can be got by invoking, if needed.
var blob = audio.getBlob();
```

**_Simple example - 2D record flow_**:
```
var recorder = google.gears.factory.create('beta.audiorecorder');
recorder.record(); //asynchronous call
...
recorder.stop();
// handle to the blob containing the recorded data can be obtained
// after the recorder is stopped, the blob may then be uploaded to
// a server. 
var blob = recorder.getBlob();
```

# Advanced topics #
## Thoughts on 3D positioned audio ##
> For 3D, it is essential to take advantage of hardware, and also provide a more sophisticated abstraction as compared to that of regular audio.
    * [DirectSound3D](http://en.wikipedia.org/wiki/DirectSound3D) was quite useful in the past, it was available on all latest windows versions. But with Windows Vista, there have been changes in the audio driver model and DS3D audio is not fully supported now. There have been talks that MS is looking to bring in XAudio2 (xbox audio engine) to the PC soon and it may replace DirectSound, so uncertain at the moment. Also, this is Windows only.
    * [OpenAL](http://en.wikipedia.org/wiki/Open_Al) is specifically designed for 3D positioned audio, has implementations available for the popular OSes. However these are binary distributions which are drivers installed by the user, and software which come along with the hardware bought from sound device manufacturers such as Creative. There is an SDK available which we can use to code for this API. (See http://www.openal.org/platforms.html)

For supporting 3D audio, we can provide a minimal JS wrapper on top of the [OpenAL 1.1 API](http://www.openal.org/openal_webstf/specs/OpenAL11Specification.pdf). We could add 3D position and velocity information to the **Audio** class, and make it similar to an OpenAL source. This should be an easy extension of the above API.

**_Audio3D class : Audio_**

A _Audio3D_ object is similar to the notion of an openAL source. The current model assumes the **AL\_SOURCE\_RELATIVE** model where all positions and velocities of sources are with respect to the listener. This API needs to be extended to an absolute model too (This is what would be very useful in most games).
```
// an object can be got from 
// - google.gears.factory.create('beta.audio3D')
Audio3D class : Audio
{
   // position of the source - in 3D space
   attribute float xposition;
   attribute float yposition;
   attribute float zposition;

   // velocity of the source - in 3D space
   attribute float xspeed;
   attribute float yspeed;
   attribute float zspeed;
};
```

**_Simple example  - 3D sound play (Bill versus Mike)_**

```
// Bill is on the left, moving towards Mike
var audio1 = google.gears.factory.create('beta.audio3D');
audio1.src = 'http://blahblahblob.com/Bill.wav';
audio1.xposition = -50;
audio1.yposition = 10;
audio1.xvelocity = 20;
audio1.load();

// Mike is on the right, moving towards Bill
var audio2 = google.gears.factory.create('beta.audio3D');
audio2.src = 'http://blahblahblob.com/Mike.wav';
audio2.load();
audio2.xposition = 50;
audio2.yposition = 10;
audio2.xvelocity = -25;

// mixing automatically happens when play (being asynchronous) is invoked - One can play these audio sources in separate event handlers.
audio1.play();
audio2.play();
```

## Editing ##
An utility class is added, to expose features like editing.

```
interface MediaUtils
{
  // extracts segment @copyStartTime to @copyEndTime of the @sourceBlob
  // and inserts that into @insertTime of @destBlob
  // supported only on raw uncompressed media
  void insert(in Blob sourceBlob,
              in Blob destBlob, 
              in int insertTime,
              in int copyStartTime, 
              in int copyEndTime);

  // deletes segment @startTime to @endTime in the @sourceBlob
  // supported only on raw uncompressed media
  void delete(in Blob sourceBlob,
              in int startTime,
              in int endTime); 

  // when passed a list of audioObjects, this method queues up play requests and 
  // fires them in one go - to avoid phasing effects.
  // This method can be used to synchronize play of multiple objects.
  void play(in Array mediaObjects);

  // returns currently active/playing media objects.
  Array getActiveMedia();
};

interface AudioUtils : MediaUtils
{
    // no new members
};
```


# C++ design #

## Threads: ##

Consider a single `GearsAudio` object created from the `GearsFactory`. This might end up in 3 parallel threads of execution over the course of time. Find them below (along with a high level description of their broad functionality).

  1. **Main thread (controlled from JS)**
    * creates `GearsAudio` object.
    * load() from network - by setting 'src' to a URL -> spawns Network thread. (A parallel flow exists for loading from a blob as well).
    * play() - starts portaudio stream -> creation of Portaudio thread
  1. **Network thread (one per load request)**
    * load from HTTP asynchronously
  1. **Player thread (one per play request)**
    * responsible for playing the audio data streamed in by the network thread.
    * In the case of a portaudio player, once `StartStream()` is called by `player.play()`, this thread invokes `paCallback()` module (defined by us) every now and then, until `StopStream()` is called.

## Class Design: ##
Please note that only the important fields are described here. The class members less crucial to the understanding of how the various elements interplay are omitted in this discussion, for reasons of brevity.

### 1. `MediaData` class: ###

A `MediaData` object holds the audio buffer data, and also states/properties that are shared by the various threads concurrently. This is also the protocol object passed to the player's callback function (referred to, as `userData` in portaudio documentation).
This is a `RefCounted` object, as it is shared by multiple threads. Hence, all threads should access this object via scoped\_refptrs. Note that, for a given `GearsAudio` object, a single `MediaData` object exists in memory for the scope of that `GearsAudio` object. The `MediaData` object is shared by the `GearsMedia` object in the main thread, the `NetworkMediaRequest` object in the network thread and the `PaPlayer` object in the player thread. Each of these objects hold a `scoped_refptr` to the shared `MediaData` object.

```
class MediaData : public RefCounted {

 private:
  // The lock used for accessing or updating properties of this object in a thread safe way.
  Mutex media_data_lock_;

  // The media buffer. This is initially NULL. 
  // Either the network thread or the main thread (in case of load from blob) 
  // writes to this.
  scoped_ptr<std::vector<uint8>> media_buffer_;
  
 public:
  // Method to append new data (possibly available from the stream) to the media buffer.
  // Acquires media_data_lock_.
  // Reads values in new_data and appends them to media_buffer_.
  void AppendToMediaBuffer(std::vector<uint8>* new_data);

  // Method to reset buffer data.
  // Acquires media_data_lock_ and sets media_buffer_ to NULL
  void ResetMediaBuffer();

  // Method to get data present in the media buffer
  // Acquires media_data_lock_.
  // Gets data from start_pos of media_buffer of size length and appends onto the 'output'
  // as long as index into buffer is valid.
  // If index exceeds length of media_buffer_, fills zero thereafter till 'length' values
  // are filled in 'out'
  std::vector<uint8>* MediaData::GetMediaBufferData(int start_pos, int length)
  
  // ....
  // other common properties accessible by all threads
  // for instance, last_error_.
}
```

### 2. `GearsMedia` class: ###

Abstracts HTML5 media, of which we are only implementing audio for now.
`GearsAudio`, naturally extends from `GearsMedia`. `GearsMedia` handles all the Media functionality mentioned in the HTML5 spec. Since this functionality would be common to audio and video, no changes are expected here, if we add a `GearsVideo` class later.

```
class GearsMedia {
 
 public:
  scoped_refptr<MediaData> media_data_;  
 protected:
  virtual void play() = 0;

  // The asynchronous request for media data
  scoped_ptr<NetworkMediaRequest> network_media_request_;

  // other properties required to be exposed as specified in the spec...
}
```

### 3. `GearsAudio` class: ###

This is the object created when passing `'beta.audio'` to Factory.

```
class GearsAudio
    : public GearsMedia,
      public ModuleImplBaseClassVirtual {

 private:

  // instantiated with a PaPlayer object for now through the PlayerFactory
  scoped_ptr<PlayerInterface> player_;

  // play method is delegated as below. Similarly for other methods/properties of the player,
  // such as volume, playback rate, playback position (seek), etc..
  virtual void play() {
    player_->play();
  }
}
```

### 4. `NetworkMediaRequest` class: ###

This is the class that takes care of asynchronously loading data from the network into the `media_buffer_` field of the `MediaData` object.

This class also implements the HttpRequest::Listener interface and provides implementations for the `DataAvailable()` and the `ReadyStateChanged` methods. Whenever new data comes in, this is extracted from the response body and appended to the `media_data_` through the `AppendToMediaBuffer()` method.

This class has a scoped\_refptr to the media\_data_which is initialized on creation._

```
class NetworkMediaRequest : HttpRequest::Listener {
 public:
  NetworkMediaRequest(std::string16 url, MediaData* media_data);
  virtual ~NetworkMediaRequest();

  // calls open of the httprequest asynchronously.  
  void AsyncOpen();

 private: 
  HttpRequest* media_request_;
  MediaListener* listener_;
  
  scoped_refptr<MediaData> media_data_;
  
  // Listener implementation
  void DataAvailable(HttpRequest * source);
  void ReadyStateChanged(HttpRequest *source);
}
```

### 5. `PlayerInterface` class: ###

The abstract player interface. `GearsAudio` class is abstracted from how the player works.
It just holds a `scoped_ptr` to an object that implements `PlayerInterface` and makes calls such as `play()`, `pause()`, etc.... The concrete player instance take care of the actual implementation.

```
class PlayerInterface {
 protected:
  virtual void play() = 0;
  virtual void pause() = 0;
  // .... and other such methods and properties that the player should implement.
  // eg., seek(), volume, player state (current playback position), playbackRate, etc...

  // This is called by the player factory when the player instance is created.
  // It does an addref to the media_data object of GearsMedia.
  Init(MediaData* media_data) {
    media_data_.reset(media_data);
  }

 private:
  scoped_refptr<MediaData> media_data_;
}
```

### 6. `PlayerFactory` class: ###

The factory singleton class that returns the instance of the player that we want to use.
Right now, only possible value is `"portaudio"`. Later, new players can figure in here,
based on different libraries.

```
class PlayerFactory {
public:
  PlayerInterface* GetPlayerInstance (std::string16 player_name, MediaData* media_data)
     // instantiate the correct player.
     // if (player_name is 'portaudio') {
     //   PlayerInterface *my_player = new PaPlayer();
     // }

     // AddRef to media_data
     my_player->Init(media_data);
     return my_player;
  }

  // private singleton constructor and public getInstance method for the PlayerFactory class.
}
```

### 7. `PaManager` class: ###

This is a portaudio specific module. A singleton class, that does two things:

  1. Provide a thread safe wrapper for portaudio stream methods.
  1. Keep track of player/recorder threads in memory.

```
class PaManager {
 public:
  // Player and recorder threads need to register with the manager.
  // When first thread registers, PaManager calls Pa_Init()
  void RegisterAudioThread();
  
  // When last thread unregisters, PaManager calls Pa_Terminate().
  // Both these methods use a lock to update thread_count.
  void UnregisterAudioThread();
  
  // Thread safe wrapper for portaudio close stream.
  PaError Pa_SafeCloseStream( PaStream *stream ) {
    MutexLock locker(&pa_lock_);
    return Pa_CloseStream(stream);
  }

  // ....
  // Similarly define safe methods for other stream methods such as 
  // OpenDefaultStream, StartStream, StopStream, etc...

 private:
  Mutex pa_lock_;
  // Also have private constructor and a public getInstance method for the
  // singleton PaManager instance
}
```

### 8. `PaPlayer` class: ###

This is a portaudio specific module. It represents the portaudio based audio player.

```
class PaPlayer : public PlayerInterface {
public:
 // PlayerFactory is a friend. See below.
 friend class PlayerFactory;

 // methods inherited from the interface...
 // These use the safe methods on the PaManager
 // virtual void play();
 // virtual void pause();

 // The portaudio callback that does all the magic... 
 void PaCallback(<params>) {
   // code to copy audio buffer into portaudio's 'out' buffer.
 }

 // This method has a static signature (as required by portaudio).
 // We delegate this call to the non-static method above using 
 // the object handle
 static void paCallback(<params>, void *userData) {
   PaPlayer *player = reinterpret_cast<PaPlayer>(userData);
   player->PaCallback(<params>);
 }

private:
 // private constructor that only PlayerFactory can access.
 PaPlayer() {
   // RegisterAudioThread() with global pa_manager
 }

 ~PaPlayer() {
   // UnregisterAudioThread() with global pa_manager
 }
}
```

## Locking semantics: ##
### The `media_data`: ###
`media_data_` is not NULL whenever a player thread or a network thread (or in general, whoever needs to read/write `media_buffer_`) is executing because they use a `scoped_refptr` to reference the `media_data_` and hence have `addRef()`-ed it. (In other words, whenever `PaCallback` executes, `media_data_` cannot be NULL. This can in fact be asserted in the callback function).

### Semantics for locking `media_buffer`: ###
  * The only way to read/dereference the `media_buffer_` is through the `GetMediaBufferData` method. This acquires the `media_data_lock_` (but reads only valid data based on the buffer's current size).
  * The only way to set the `media_buffer_` to NULL is through the `ResetMediaBuffer` method. This acquires the `media_data_lock_` too.
  * The only way to append onto the buffer is through the `AppendToMediaBuffer` method. This too acquires the `media_data_lock`.

## How they all work together: ##

### Main thread: ###
  * Through the factory, the main thread (JS) creates a `GearsAudio` object. The constructor of `GearsAudio` does the following:
    * Instantiate a new `MediaData` object, and assign it to its `scoped_refptr media_data_`. The `media_buffer_` in the `media_data_` is initially NULL.
    * Instantiates a new player object (`PlayerInterface`) passing this `media_data_` to it. The player is referenced by a `scoped_ptr` (so that it gets automatically cleaned up when this `GearsMedia` object is cleaned up).
    * `network_media_request_` is initially NULL.
  * The javascript now sets `'src'`. This also initiates a `load()` implicitly.
  * `'load()'` does the following:
    * If there is an existing NetworkMediaRequest object, it cleans it.
    * It creates a new NetworkMediaRequest object by passing the `media_data_` and the 'url' to it.
    * It calls `AsyncOpen()` on this object and returns.
  * If `'play()'` is called, the main thread calls `player->play()`;

### Network thread (NetworkMediaRequest object): ###
  * At the time of creation, `AddRef()`s the passed `media_data_`. (via `scoped_refptr<MediaData>.reset()`).
  * The `AsyncOpen` method loads the data over the network from the `url_`.
  * On `DataAvailable()` new data available in the response body is appended to the `media_buffer_` in `media_data_` through the `AppendToMediaBuffer()` method.
  * On deletion, this class resets the `media_buffer_` by invoking `ResetMediaBuffer()`.

### Player thread: ###
  * `PaPlayer`, on creation, `AddRef()`s `media_data_` (by virtue of using `scoped_refptr`) and also registers itself with the `PaManager`.
  * `PaPlayer::play()` defines `OpenDefaultStream(<params>, this)` and `StartStream()` - (call safe methods defined on `PaManager`).
  * `PaPlayer::PaCallback()` does the following:
    * Invokes `GetMediaBufferData()` to get data for the current playback position.
    * Decode if necessary.
    * Fill the 'out' buffer with the next few elements (internally player tracks the current read position on the buffer) till the buffer is filled, till `buffer_size_` is exceeded. Values are filled as is, in case of raw audio data, or decoded to the raw values appropriately.
  * If the 'current position' exceeds the buffer size, fill silence values ('0') onto the 'out' buffer.
  * On destruction, the player unregisters with the `PaManager`.