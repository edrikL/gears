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

#ifndef GEARS_LOCALSERVER_SAFARI_PROGRESS_INPUT_STREAM_H__
#define GEARS_LOCALSERVER_SAFARI_PROGRESS_INPUT_STREAM_H__

#import <Foundation/NSStream.h>
#import "gears/base/common/basictypes.h"

class SFHttpRequest;

// ProgressInputStream is a wrapper around an NSInputStream that provides
// notification to the native HttpRequest object as the stream is read.
//
// This class should only be created by a SFHttpRequest, and that SFHttpRequest
// should call OnSFHttpRequestDetached whenever the SFHttpRequest no longer
// points to this ProgressInputStream (e.g. during SFHttpRequest's destructor).
// This constraint is in order to let the two objects point to each other
// without causing a reference cycle that prohibits deleting them after use.
@interface ProgressInputStream: NSInputStream {
 @private
  SFHttpRequest *request_;
  NSInputStream *input_stream_;
  int64 position_;
  int64 total_;
}

#pragma mark Public instance methods

// Initializes a ProgressInputStream.  |total| is the total number of bytes
// contained within |input_stream|.
- (id)initFromStream:(NSInputStream *)input_stream
             request:(SFHttpRequest *)request
               total:(int64)total;

- (void)onSFHttpRequestDetached:(SFHttpRequest *)request;

@end
#endif  // GEARS_LOCALSERVER_SAFARI_PROGRESS_INPUT_STREAM_H__
