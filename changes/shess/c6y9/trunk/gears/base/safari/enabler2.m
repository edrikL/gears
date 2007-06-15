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

//=============================================================================
//  ScourEnabler.m
//  ScourSafari
//=============================================================================

#import "gears/base/safari/enabler.h"
//UNUSED? #import "GOBSCache.h"
//UNUSED? #import "URLProtocolHandler.h"

@implementation ScourEnabler
//=============================================================================
#pragma mark -
#pragma mark || NSObject ||
//=============================================================================
- (id)init {
  if (self = [super init]) {
    NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
    
    // Setup things if we're running in Safari
    if ([bundleID isEqualToString:@"com.apple.Safari"]) {
      // Register our protocol handler
      BOOL result = [NSURLProtocol registerClass:[URLProtocolHandler class]];
      
      NSLog(@"Protocol handler Result: %d", result);
      
      cache_ = [[GOBSCache alloc] init];
      [cache_ installHandlers];
    } else {
      // Otherwise, remove from memory
      [self release];
      self = nil;
    }
  }
  
  return self;
}

//=============================================================================
- (void)dealloc {
  [cache_ release];
  [super dealloc];
}

@end
