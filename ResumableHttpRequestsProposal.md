﻿#summary A proposal for supporting resumable POST/PUT HTTP requests in HTTP/1.0.
#labels DesignDoc

# Introduction #

Current web standards provide no reliable mechanism to facilitate the HTTP upload of large files. As a result, file uploads at Google and other sites have traditionally been limited to moderate sizes (e.g. 100 MB), with browser-external clients being required to upload larger files. Several such clients have been created for Google Video, YouTube, and Google Books, all using HTTP to upload large files by POSTing one segment (a.k.a. chunk) at a time, with chunk sizes on the order of 1 MB; this facilitates recovery from transient errors that occur during upload, by providing well-defined points at which the upload could be resumed (i.e. chunk boundaries). We propose an HTTP/1.0 protocol which provides native support for resuming HTTP requests which aren't currently resumable, such as POST and PUT. While our protocol adheres to HTTP/1.1, it avoids constructs which can not be dealt with properly by HTTP/1.0 proxies. For cases where HTTP/1.0 proxies are not a constraint, an optional checkpoint can be sent as an optimization.

If an HTTP connection is closed prior to the client receiving a response from the server, the client has no way of knowing how much of the content was successfully received by the server, or whether the operation completed. When this occurs, idempotent operations can be automatically retried, but in the case of non-idempotent operations HTTP/1.1 section 8.1.4 explicitly forbids the client from retrying the operation without user interaction. In order to reattempt the operation without concern for introducing unintended side effects, the server and client require a mechanism for uniquely identifying the operation across multiple requests. We thus require that each upload operation be assigned a unique, per-upload URL, whose generation is outside the scope of the resumable upload protocol itself.

When an HTTP _request_ is resumed, the initial request combined with all subsequent resume requests together constitute a single logical _operation_. As such, we use the terms request and operation, respectively, to distinguish between individual HTTP requests and the combined unit of requests (the original request plus any retries) that are logically associated with each other.

# Protocol Overview #

  1. Prior to initiating an operation, the client must first obtain a unique per-operation identifier which the server can use to group requests that are part of the same operation. This could be a unique URL, an ETag, or some other mechanism.
  1. The client initiates a request (e.g. POST/PUT). Normally the client sends all data bytes in the initial request, but when necessary it can send partial data, in which case a 308 (Resume Incomplete) response code is expected.
  1. Should the connection be closed prior to the arrival of a final response code (possibly before all bytes of the request have even been sent), or if the client receives a 503 (Service Unavailable) status code, then the client queries the server by sending a resume request (e.g. POST/PUT) with no body.
  1. If the server has succesfully received all bytes from the operation, it responds with a final status code; otherwise it responds with a 308 (Resume Incomplete), indicating which bytes of the operation it has successfully received.
  1. If the client receives a 308 (Resume Incomplete), it sends a resume request, which resembles the original request from step (2), but contains only a subset of the data bytes (specifically those not yet received by the server). A Content-Range header is added to specify the byte range which is being transfered.

# Unique Operation Identifier #

The core logic of this protocol specification requires a unique identifier of each operation. Two standard means of accomplishing this are the use of a unique per-upload URL, or the assignment of an ETag. The _Initial Handshake_ section proposes one mechanism whereby unique operation identifiers can be established.

# Initial Request #

Normally there is nothing special about the initial request in a resumable operation. Specifically, it should contain the entire byte range, and not include a Content-Range header. While the protocol itself supports sending a partial range in the initial request, this is recommended against unless explicitly required.

# Status Code: 308 Resume Incomplete #

Upon receiving the content body of a resume request, the server may still not posses the complete byte range, requiring further action (e.g. additional requests) from the client. In such cases, the server SHOULD return status code 308 (Resume Incomplete) if it is still willing to continue the operation.

308 (Resume Incomplete) responses MAY include a Range header. The Range header MUST specify the last-byte-pos (e.g. it is not optional). The presence of a Range header in this context indicates the byte ranges of the content body which the server has stored for this operation. In future resume requests, the client SHOULD use this information to minimize the amount of data that needs to be retransmitted. This header overrides the value of any previous Range headers, even if the new Range is not a superset of previous ranges (which allows for the possibility of the server losing or discarding data). In addition, the server MAY specify disjoint byte ranges but SHOULD represent the ranges in canonical form. In the absence of any such Range header over the course of the operation, the client should assume that the server has no stored bytes.

308 (Resume Incomplete) responses MAY include a Location header as defined in HTTP/1.1 section 14.30. The presence of a Location header in this context specifies the URI to which future resumable requests should be sent for this operation. The header overrides the value of any previous Location headers. In the absence of any such Location header over the course of the operation, the client should send resume requests to the original Request-URI.

A 308 (Resume Incomplete) MAY include a Retry-After header as defined in HTTP/1.1 section 14.37. A client which receives a Retry-After SHOULD delay further resume operations for the specified time period. If the delay is not feasible, the client MUST fail the operation. The server MAY send a 308 (Resume Incomplete) before receiving the entire request body, but as specified in section 8.2.3 of HTTP/1.1 the server SHOULD NOT close the transport connection until it has read the entire request, or until the client closes the connection.

A 308 (Resume Incomplete) MAY include an ETag header, uniquely identifying the operation. If a client receives an ETag then it MUST resend the ETag in an If-Match header on resuming operations.

Servers MUST NOT send 308 (Resume Incomplete) responses in the face of fatal errors. By definition, the 308 (Resume Incomplete) response indicates that the client can rectify the current error condition by sending the bytes which the server is missing, and thus the respose code MUST only be sent in this scenario.

# Content-Range Header #

The Content-Range header MUST be sent by clients on all requests that do not contain the complete byte range, and SHOULD NOT be sent on requests that include the complete byte range. Specifically, Content-Range need not be set on initial requests containing the entire byte range, and must be set on resume requests and query requets.

Clients MAY omit last-byte-pos in cases where the length of the range being transferred is unknown or difficult to determine. Clients SHOULD specify instance-length, unless it is unknown or difficult to determine (in which case "`*`" should be used). In cases where the instance-length is "`*`", the client MUST either use chunked transfer coding or specify the instance-length in the final chunk transferred. Once a client specifies instance-length on one request, all subsequent requests to the same URL MUST use the same instance-length.

# Resume Requests #

When resuming an operation, clients MUST send a byte range that overlaps with the earliest missing range of bytes on the server. If a client received an ETag in a previous 308 (Resume Incomplete) then it MUST resend the ETag in an If-Match header on resuming operations.

The server SHOULD process the data from a resumable operation in the same way it would have processed a single request with the entire content. In particular, the server may need to cache the partial data from an operation, and only process it (e.g. pass it to a CGI application) once the data is complete. Servers which support a streaming mode of processing may do so for a resumable operation as long as the streaming can be resumed normally when the client sends more data in a future resume request.

If a client's connection to the server is terminated prior to the receipt of a final response code, or if the client receives a 503 (Service Unavailable), exponential backoff should be used to separate series of sequential retries, per HTTP/1.1 section 8.2.4.

If the client sends a resume request which refers to an operation that has already completed, the server MUST NOT re-process the data, and SHOULD return the same or equivalent response to the client as was returned from the original processing. If the server cannot compose an equivalent response, it MAY return 202 or some other response to indicate that the operation was processed with unknown results, keeping in mind that this response might be confusing when resuming requests occur transparently to the user. Regardless of this possibility, any sequence of resume requests with respect to the same operation are guaranteed idempotent, and may be retried without user interaction, despite the considerations of section 8.1.4 of HTTP/1.1.

# Query Requests #

Clients can query the server for the status of an operation by sending a request with no body. This is the canonical polling mechanism whereby the client can determine which bytes the server has received under failure modes where this information is not already expossed. This would be desired, for example, when the connection is closed prior to the arrival of a final response code, or if the client receives a 503 (Service Unavailable) status code.

Query requests MUST contain a Content-Range header with a byte-range-resp-spec of "`*`".

# Initial Handshake #

Query requests could be used by clients to probe servers for resumability support prior to sending additional bytes. They can also be used to implement resumability with a non-unique starting URL that either redirects to a unique URL, or provides a unique ETag, either of which could then be used to uniquely identify the operation.

# Use Cases #

The client would begin by sending a single POST/PUT containing the entire (potentially large) body, and then if the connection were prematurely terminated it could issue a query request to determine at which byte range to recommence, at which point all remaining bytes would again be sent in a single POST/PUT. This process would repeat until the last byte was successfully transferred and a response was received from the server. While this should be the standard mode of operation, alternatively a client could dynamically or deterministically break a large POST/PUT into multiple pieces, sending each one separately with an appropriate Content-Range header.

# Optional Status Code: 103 Checkpoint #

When interacting with HTTP/1.1 clients (through HTTP/1.1 proxies), servers can optimistically push checkpoints to clients, allowing them to resume failed operations without having to first poll the server to determine which bytes it had. This is accomplished with a new status code: 103 (Checkpoint), which is exactly the same as the 308 (Resume Incomplete) status code except that it is sent as a provisional response rather than a final response. In other words, zero or more 103 (Checkpoint) responses can be sent in response to a request that is still being processed, after which a final response code (e.g. 200 or 308) must be sent.

# ETag Example #

Client sends initial handshake:

```
POST /upload HTTP/1.1
Host: example.com
Content-Length: 0
Content-Range: bytes */100
```

Server assigns ETag:

```
HTTP/1.1 308 Resume Incomplete
ETag: "vEpr6barcD"
Content-Length: 0
```

Client initiates upload:

```
POST /upload HTTP/1.1
Host: example.com
If-Match: "vEpr6barcD"
Content-Length: 100
Content-Range: bytes 0-99/100

[bytes 0-99]
```

The request is terminated prior to receiving a response from the server.

Client polls the server to determine which bytes it has received:

```
POST /upload HTTP/1.1
Host: example.com
If-Match: "vEpr6barcD"
Content-Length: 0
Content-Range: bytes */100
```

Server responds with the current byte range:

```
HTTP/1.1 308 Resume Incomplete
ETag: "vEpr6barcD"
Content-Length: 0
Range: 0-42
```

Client resumes where the server left off:

```
POST /upload HTTP/1.1
Host: example.com
If-Match: "vEpr6barcD"
Content-Length: 57
Content-Range: bytes 43-99/100

[bytes 43-99]
```

This time the server receives everything and sends its response:

```
HTTP/1.1 200 OK
ETag: "vEpr6barcD"
Content-Length: 10

[response]
```

# Location Example #

Client sends initial handshake:

```
POST /upload HTTP/1.1
Host: example.com
Content-Length: 0
Content-Range: bytes */100
```

Server assigns unique URL:

```
HTTP/1.1 308 Resume Incomplete
Location: http://example.com/upload/eCn14NjNAy
Content-Length: 0
```

Client initiates upload:

```
POST /upload/eCn14NjNAy HTTP/1.1
Host: example.com
Content-Length: 100
Content-Range: bytes 0-99/100

[bytes 0-99]
```

The request is terminated prior to receiving a response from the server.

Client polls the server to determine which bytes it has received:

```
POST /upload/eCn14NjNAy HTTP/1.1
Host: example.com
Content-Length: 0
Content-Range: bytes */100
```

Server responds with the current byte range:

```
HTTP/1.1 308 Resume Incomplete
Content-Length: 0
Range: 0-42
```

Client resumes where the server left off:

```
POST /upload/eCn14NjNAy HTTP/1.1
Host: example.com
Content-Length: 57
Content-Range: bytes 43-99/100

[bytes 43-99]
```

This time the server receives everything and sends its response:

```
HTTP/1.1 200 OK
Content-Length: 10

[response]
```

# Related Work #

## RFC 2616: HTTP/1.1 ##

http://www.w3.org/Protocols/rfc2616/rfc2616.html

| 3.6.1 | Protocol Parameters: Chunked Transfer Coding |
|:------|:---------------------------------------------|
| 8.1.4 | Connections: Practical Considerations        |
| 8.2.4 | Connections: Client Behavior if Server Prematurely Closes Connection |
| 9.5   | Method: POST                                 |
| 9.6   | Method: PUT                                  |
| 10.3  | Redirection 3xx                              |
| 14.16 | Header Field: Content-Range                  |
| 14.21 | Header Field: Expires                        |
| 14.24 | Header Field: If-Match                       |
| 14.27 | Header Field: If-Range                       |
| 14.30 | Header Field: Location                       |
| 14.35 | Header Field: Range                          |
| 14.41 | Header Field: Transfer-Encoding              |
| 19.2  | Appendix: Internet Media Type multipart/byteranges |

## XMLHttpRequest ##

http://www.w3.org/TR/XMLHttpRequest/

## Related Google Gears APIs ##

  * ContentRangePostProposal
  * GearsHttpRequestProposal
  * [BlobAPI](BlobAPI.md)