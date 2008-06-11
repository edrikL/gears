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

#include <cassert>

#import "gears/blob/blob_input_stream_sf.h"
#include "gears/blob/blob_interface.h"

@implementation BlobInputStream

#pragma mark Public Instance methods
//------------------------------------------------------------------------------
// BlobInputStream implementation
//------------------------------------------------------------------------------

- (id)initFromBlob:(BlobInterface *)blob {
  self = [super init];
  if (self != nil) {
    blob->Ref();
    blob_ = blob;
  }
  return self;
}

- (void)dealloc {
  if (blob_) {
    blob_->Unref();
  }
  [super dealloc];
}

- (void)reset:(BlobInterface *)blob {
  if (blob) {
    blob->Ref();
  }
  if (blob_) {
    blob_->Unref();
  }
  blob_ = blob;
  offset_ = 0;
}

#pragma mark NSInputStream function overrides.
//------------------------------------------------------------------------------
// NSInputStream implementation
//------------------------------------------------------------------------------

- (NSInteger)read:(uint8_t *)buffer maxLength:(NSUInteger)len {
  assert(blob_ != NULL);
  if (blob_ == NULL) {
    return 0;
  }
  int64 numBytesRead = blob_->Read(buffer, offset_, len);
  assert(numBytesRead >= 0);
  if (numBytesRead < 0) {
    return 0;
  }
  // Confirm that BlobInterface::Read returned <= bytes requested
  assert(numBytesRead <= static_cast<int64>(len));
  offset_ += numBytesRead;
  return static_cast<NSInteger>(numBytesRead);
}

- (BOOL)getBuffer:(uint8_t **)buffer length:(NSUInteger *)len {
  // From the NSInputStream documentation:
  // "If this method is not appropriate for your type of stream,
  //  you may return NO."
  return NO;
}

- (BOOL)hasBytesAvailable {
  if (blob_ == NULL) {
    return NO;
  }
  int64 remaining = blob_->Length() - offset_;
  if (remaining <= 0) {
    return NO;
  }
  return YES;
}

@end
