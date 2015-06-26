# Problem #

Web applications have no way to interact with binary data. For example, it is not possible to get the binary content of a user-selected file or XMLHttpRequest response. The lack of any concept of binary data severely limits web applications' capabilities in many ways:

  * You can't build a large file uploader (like for YouTube or Flickr) in JavaScript because there's no way to get access to the file(s) selected through `<input type="file">` element and break them into smaller pieces.
  * You can't support attachments in an offline-enabled mail client because you can't store a selected file for later synchronization.
  * It's not practical to build a client side image editor using `<canvas>` because there isn't a good way to export the result. [getImageData()](http://www.whatwg.org/specs/web-apps/current-work/multipage/section-the-canvas.html#getimagedata) requires manually encoding the image one byte at a time, which would be unacceptably slow for large files.
  * Etc...

# Considerations #

  1. There are many potential provides and consumers of binary data in a web application. For example, network connections, file upload controls, audio/video, cameras, and any sort of embedded resource could all naturally be extended with APIs to accept or provide binary data.
  1. The web model is successful partly because users can safely assume that web applications cannot violate their privacy or security. Adding APIs to manipulate binary data must maintain this model.

# Solution #

We propose the addition of a new _Blob_ interface to serve as a way to exchange binary data between APIs. Blobs are complementary to ECMAScript-style ByteArrays and other in-memory representations of data. The primary difference is that Blobs are immutable[\*](http://code.google.com/p/google-gears/wiki/BlobWebAPIPropsal#Appendix_1:_Blob_Immutability_Details), and can therefore represent large objects.

Having a common object to represent and manipulate binary data across all APIs provides a lot of power with minimum footprint. For example:

  * Get a blob from a large file, chop it up into smaller blobs, and upload the pieces using XMLHttpRequest to get a large file uploader.
  * Get a blob from a file while offline, store it in the database or application cache, and upload it later when connected.
  * Get a blob from the network, send it into canvas, then get the modifications back out and re-upload them.
  * Etc...

# API Summary #

```
// Represents an immutable chunk of binary data
interface Blob {
  readonly attribute int64 length;
  Blob slice(int64 start, int64 length);

  // ... APIs for reading blob data...
  // See "Reading Blob Data" below for details.
};


// Extensions to the existing <input type="file"> DOM interface to
// allow getting Blobs for selected files.
interface HTMLFileInputElement {
  readonly FileList files; // Reuse Mozilla's existing property in a
                           // compatible way.
};
interface FileList {
  readonly int length;
  File item(int index);
};
interface File {
  readonly string fileName; // only the name portion, not the path (for privacy)
  readonly Blob contents;
};


// Extensions to XMLHttpRequest to allow sending and receiving blobs.
interface XMLHttpRequest {
  readonly Blob responseBlob;
  void send(Blob data); // overloads the existing send methods
};


// Extension to the Window object to prompt the user to save a blob to a file.
interface Window {
  // prompts the user to save the file to disk.
  void exportBlobToFile(Blob blob);
};
```


# Example #
```
<input id="picker" type="file" max="5" accept="image/*">
<button onclick="upload()">Go!</button>
<script>
function upload() {
  var files = document.getElementById("picker").files;
  for (var i = 0; i < files.length; i++) {
    uploadFile(files[i]);
  }
}

function uploadFile(file) {
  var chunkSize = 1024*1024; // 1 MB
  var pos = 0;
  var partNum = 0;
  while (pos < file.contents.length) {
    var part = file.contents.slice(pos, chunkSize);
    var req = new XMLHttpReqest();
    req.open("POST",
        "upload?name= " + file.fileName + "&part=" + partNum,
        true);
    req.send(file.contents);
    pos += part.length;
    partNum++;
  }
}
</script>
```


# Reading Blob Data #

Sometimes it is useful to manipulate raw data, even though it is slow in JavaScript.

```
// Additional APIs for reading the contents of a blob.
interface Blob {
  ...

  void readAsText(ReadStreamSuccessCallback<string> onsuccess,
      ReadStreamFailureCallback on fail, optional string encoding,
      optional int64 start, optional int64 desiredLength);
  void readAsBase64(ReadStreamSuccessCallback<string> onsuccess,
      ReadStreamFailureCallback onfail,
      optional int64 start, optional int64 desiredLength);
  void readAsBytes(ReadStreamSuccessCallback<ByteArray> onsuccess,
      ReadStreamFailureCallback onfail,
      optional int64 start, optional int64 desiredLength);
  void readAsByteString(ReadStreamSuccessCallback<string> onsuccess,
      ReadStreamFailureCallback onfail,
      optional int64 start, optional int64 desiredLength);
};
void ReadStreamSuccessCallback<T>(T data); // assumes T has a "length" property.
void ReadStreamFailureCallback(Error e); // e contains some sort of data about
                                         // what went wrong.
```

# Creating Blobs #

_TODO: There should be some way to build up a blob from other formats, needs more thought._


# More #

Blob support can be added to many existing and proposed APIs, including `<canvas>`, `<audio>`, `<video>`, `<img>`, Databases, Offline Applications, etc.


# Appendix 1: Blob Immutability Details #

Although it is impossible to guarantee that the data backing a blob does not ever change, this proposal assumes that such changes are rare. The UA should attempt to detect such situations and call ReadStreamFailureCallback with an error code that lets the application try again.


# Appendix 2: Alternate File Picker API #

Instead of extending the existing `<input type="file">` element, we also considered designing an entirely new, and purely programmatic, _Filesystem_ interface for selecting files, exporting files, and potentially other operations in the future.

This solution is elegant because all the file-related APIs can be together, and some additional flexibility is granted by having a purely programmatic interface.

It also has the advantage that it would finally allow web developers to style their "Browse..." buttons as they please.