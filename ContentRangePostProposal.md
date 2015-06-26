# Introduction #

The primary primitive necessary to enable large file uploads is the ability to specify byte ranges in POST and PUT requests. Byte ranges are already standardized for GET requests, and there are implementations of byte-range PUT used by WebDAV, but to our knowledge there has been little effort to use them for POST. Byte-range POST/PUT could be used to resume incomplete transfers, or to explicitly break transfers into smaller sized chunks. We propose standardizing byte range POST/PUT in a manner analogous to byte range GET.

The use of Content-Range headers in POST/PUT is allowed by the standard.  Section 9.6 explicitly suggests the possibility of using Content-Range headers in PUTs: "The recipient of the entity MUST NOT ignore any Content-`*` (e.g. Content-Range) headers that it does not understand or implement and MUST return a 501 (Not Implemented) response in such cases."  However, such functionality has not been widely deployed, and therefore there exists no reference implementation or standardized semantics for how it should be used.

# Proposal #

Content-Range values used for POST/PUT should be used as described by HTTP/1.1 section 14.16 with the following differences:

  * Clients MUST NOT send a byte-range-resp-spec of "`*`" when transferring bytes, as the server would have no way to determine the byte range being transferred.
  * Clients MAY omit last-byte-pos of if the length of the range being transferred is unknown or difficult to determine.
  * Clients SHOULD specify the instance-length, unless it is unknown or difficult to determine (in which case "`*`" should be used).

# Use Cases #

The use of Content-Range POST/PUT could be used to compose a chunked transfer protocol where a large POST/PUT was broken up into many small POST/PUTs. This would be similar to the protocols used by several current resumable uploaders. The basic algorithm for this model is to continue sending each chunk (probably in serial order) until it is successfully acknowledged. Note that if used for POSTs, some server-specific mechanism would be required to uniquely identify the the set of chunks constituting a single logical transfer. If it is necessary to accommodate server-side chunk loss (e.g. due to failures partially masked by replication) some additional custom protocol components would be necessary for the server to indicate to the client which chunks it had successfully received.

For some web applications, it may be desirable to upload a subset of a file to the server to reduce the latency between when a file is selected, and when the user can manipulate it in the application.  A concrete example of this might be an image hosting website, which would like to upload the 64KB EXIF data segment from a JPEG file, so that the user can quickly view a thumbnail version of the file in the application while the rest of the image is transferred in the background.

# Gears Modifications #

It is already feasible to write JavaScript which emits POST or PUT with a Content-Range header, by utilizing the `setRequestHeader` method of `XMLHttpRequest` or `Gears.HttpRequest`. However, those objects currently only support string or XML document payloads, which are not that interesting from our perspective, since they are unlikely to be large enough to warrant segmented upload. There is an existing proposal for supporting upload of files and other Binary Large OBjects via the `Gears.Blob` API, which is very relevant to this proposal. The Blob API and it's integration with `HttpRequest` promises a programmatic way to upload large content. To support segmentation, we propose an extension to `Gears.Blob` which would enable segmentation of an existing Blob.  The interface is as follows:

```
interface Blob {
  ...
  Blob slice(int64 offset);
  Blob slice(int64 offset, int64 length);
  ...
}
```

The resulting Blob would be equivalent to the target Blob, except that it begin with the data at the byte position `offset`, and continue for `length` bytes (or the end of data if not specified).

Utilizing this interface, along with `Gears.HttpRequest.send(Blob)` and `setRequestHeader`, one can implement this proposal for the upload of segments of files or other large objects to which the script has access.

# Related Work #

## RFC 2616: HTTP/1.1 ##

http://www.w3.org/Protocols/rfc2616/rfc2616.html

| 9.6 | Method: PUT |
|:----|:------------|
| 14.16 | Header Field: Content-Range |

## XMLHttpRequest ##

http://www.w3.org/TR/XMLHttpRequest/

## Related Google Gears APIs ##

  * GearsHttpRequestProposal
  * [BlobAPI](BlobAPI.md)

## Apache mod\_dav ##

http://www.webdav.org/mod_dav/

Apache mod\_dav added support for Content-Range PUT's in version 1.0.0-1.3.6, released June 13, 2000.