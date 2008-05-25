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

#ifdef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#else  // OFFICIAL_BUILD

#import <Foundation/NSStream.h>

#include "gears/base/common/common.h"

class BlobInterface;

// BlobInputStream provides an NSInputStream interface for blobs.
@interface BlobInputStream: NSInputStream {
 @private
  BlobInterface* blob_;
  int64 offset_;
}

#pragma mark Public Instance methods

// Initializes a BlobInputStream with a blob.
- (id)initFromBlob:(BlobInterface *)blob;

// Replaces the stream's current blob (if any) with the argument.
// The argument can be NULL.
- (void)reset:(BlobInterface *)blob;

#pragma mark NSInputStream function overrides.

// From the current offset, take up to the number of bytes specified in |len|
// from the stream and place them in the client-supplied |buffer|.
// Return the actual number of bytes placed in the buffer.
- (NSInteger)read:(uint8_t *)buffer maxLength:(NSUInteger)len;

// This method is not appropriate to BlobInputStream, so we always return NO.
- (BOOL)getBuffer:(uint8_t **)buffer length:(NSUInteger *)len;

// Return YES if there is more data to read in the stream, NO if there is not.
- (BOOL)hasBytesAvailable;

@end

#endif  // OFFICIAL_BUILD
