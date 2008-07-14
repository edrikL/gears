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

#import <Cocoa/Cocoa.h>

#import "gears/base/common/common_osx.h"
#import "gears/base/safari/nsstring_utils.h"

void *InitAutoReleasePool() {
  return [[NSAutoreleasePool alloc] init];
}

void DestroyAutoReleasePool(void *pool) {
  assert(pool);
  [(NSAutoreleasePool *)pool release];
}

void OSXGearsLog(const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  NSLogv([NSString stringWithCString:msg], args);
  va_end(args);
}

void OSXGearsLog16(const char16 *msg_utf16, ...) {
  va_list args;
  va_start(args, msg_utf16);
  
  NSMutableString *msg = [[NSMutableString alloc] init]; 
  [msg setString:[NSString stringWithString16:msg_utf16]];
  
  // LOG16 takes char16 literals.
  [msg replaceOccurrencesOfString:@"%s" 
                       withString:@"%S "
                          options:NSLiteralSearch
                            range:NSMakeRange(0, [msg length])];
  
  NSLogv(msg, args);
  va_end(args);
  [msg release];
}
