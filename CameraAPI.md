# Summary #

The goal of this API is to allow a Web application to launch the native camera application and obtain the resulting media file.

On devices with built-in camera, all of the UI interaction is left to the corresponding native camera application, not Gears.

On devices that do not have a built-in camera (e.g. laptop or desktop computers),  Gears will scan for attached Web cameras and will provide a basic viewport that allows the users to grab a picture or record a movie. If several cameras are attached, Gears will provide a way for the users to select the device they want to use.

This API is based on the "action-driven permission model" - the user is involved in the action and hence a separate permission prompt is not required.

# API Proposal #

```
interface Camera {
  // Acquisition methods

  // Triggers the camera viewfinder and invokes the callback after the user
  // has either captured the image or canceled the operation.
  void captureImage(CaptureCallback callback, optional CaptureOptions options);
  // Triggers the camera viewfinder and invokes the callback after the user
  // has either captured the video, canceled the operation or maxDuration milliseconds
  // have elapsed.
  void captureVideo(CaptureCallback callback, double maxDuration, optional CaptureOptions options);

  // Query methods

  Array availableImageFormats();   
  Array availableVideoFormats();   
};

void CaptureCallback(Media media);

interface CaptureOptions {
  string format;  // MIME type
  int width;   // desired image or video frame width
  int height;  // desired image or video frame height
};

interface Media {
  readonly string format;  // MIME type
  readonly double width;   
  readonly double height;
  readonly Blob content;   // reference to captured media
};

```

# Example #

```
// Create a camera object
var camera = google.gears.factory.create("beta.camera");

// Capture an image.
var image = camera.captureImage(function(media) {
    uploadImage(media.content);
    });

// Capture of a video, specifying a desired resolution and duration.
var video = camera.captureVideo(function(media) {
    uploadVideo(media.content);
    }, 5000);
```

# Implementation details #

The format parameter in Camera.captureImage(), Camera.captureVideo(), and MediaData.format refers to a MIME type in lower case. The following types are supported video/mp4, video/3gpp, video/3gp2, image/jpeg, image/png. The bucket codec parameter (RFC 4281) may be used to specify a particular codec to use.

The arguments passed to the Camera object to specify the resolution or the format of the image or video are only hints to set defaults - the final result may be different (i.e., the resolution or the format can be constrained by the device capabilities or the user may change the default settings). If the developer specifies an unsupported resolution or format, fallback resolutions or formats will be used.

To avoid situations where the content that was produced by the camera cannot be used by an application due to media format incompatibilities, this API provides two methods that allow the developer to query the device encoding capabilities. These functions return an array of strings, denoting the MIME type of the content that can be produced by the camera (TBD: plus the codec required to render the content, as in RFC 4281?).