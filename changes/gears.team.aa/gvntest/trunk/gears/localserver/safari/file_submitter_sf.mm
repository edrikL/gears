// Copyright 2006, Google Inc.
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

#import <WebKit/WebKit.h>

#import "gears/localserver/safari/file_submitter_sf.h"

@interface GearsFileSubmitter(PrivateMethods)
- (void)setFileInput:(id)input url:(NSString *)urlStr;
@end

@implementation GearsFileSubmitter
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || GearsBase ||
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"setFileInputElement", @"setFileInput:url:",
    nil];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Private ||
//------------------------------------------------------------------------------
- (void)setFileInput:(id)input url:(NSString *)urlStr {
  // TODO(waylonis): It seems like this is supposed to insert the data
  // from a previously captured |urlStr| into |input|.  The problem is that
  // WebKit will not allow you to set the file parameter of a html file input
  // element.
  [WebScriptObject throwException:@"setFileInputElement() not implemented"];
}

@end
