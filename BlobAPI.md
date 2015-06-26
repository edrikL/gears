# Introduction #

A Blob represents an arbitrary chunk of (binary) data.  Examples include an image file on disk, a resource from a LocalServer, an entry in a Database, or something computed client-side to be uploaded via HTTP to a server.  The JavaScript interface is minimal and does not expose its constituent data - it is merely a handle, allowing JavaScript code to nominate the inputs into, and represent the outputs from, other Gears APIs, such as the [ImageManipulationAPI](ImageManipulationAPI.md).

The C++ interface is essentially a random-access input stream of bytes.  Blobs are read-only and immutable, a la Strings in the Java language.  For C++ Gears modules, there will be an analog of Java's StringBuffer class for creating mutable in-memory Blobs.

# Details #

## JS Interface ##

IDL definition:

```
interface Blob {
  readonly attribute int64 length;
  Blob slice(int64 start, int64 length);
}
```

### length ###

The length of the Blob, in bytes.  For example, length could be used to estimate the time taken to upload the Blob.  Note that a Blob's length may be volatile, such as for a Blob representing a file on disk, since that file could be concurrently modified elsewhere.

### slice() ###

Creates a new blob as a subset of this blob.

## C++ Interface ##

```
class BlobInterface {
  int const Read(uint8 *destination, int max_bytes, int64 position);
  int64 const Length();
}
```

### int const Read(uint8 `*`destination, int max\_bytes, int64 position) ###

Fills the _destination_ buffer with data from this Blob, starting at _position_, up to _max\_bytes_ bytes.  This returns the number of bytes actually read, or zero on error.

### int64 const Length() ###

This is as per the JS interface.

# Discussion #

## Content Type ##

Previous design iterations had a Blob also exposing (in its JavaScript interface) its content type, such as "image/png", but this was deemed unnecessary for now.  If a future use case would benefit from that, we could resurrect this idea.

## Serialization ##

The intention is to be able to serve a Blob's contents via the LocalServer (e.g. by binding a URL to a Blob).  For example, you could construct an <img> object and point its src to that URL.  Additionally, you should be able to upload a Blob's contents via a HTTP request, or save it to a Database.<br>
<br>
<h2>Worker Interaction</h2>

Workers communicate by passing String messages.  One proposal for Workers to pass Blobs to each other is to have a global BlobRepository where Blobs can be checked in (in exchange for a String ticket ID) by one Worker (called A) to be collected only by another, nominated worker (called B), A would then pass this ticket ID to B via sendMessage, and B would ask the BlobRepository to exchange that ticket ID for the Blob object.