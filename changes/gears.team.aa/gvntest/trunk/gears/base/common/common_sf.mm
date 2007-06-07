// Copyright 2007, Google Inc.
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

#import "gears/base/common/string_utils.h"
#import "gears/base/common/common_sf.h"
#import "gears/base/safari/browser_utils.h"

//------------------------------------------------------------------------------
void SafariGearsLog(const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
}

//------------------------------------------------------------------------------
static NSString *StringWithLocalizedKeyAndList(NSString *key, va_list list) {
  Class factory = NSClassFromString(@"SafariGearsFactory");
  NSBundle *bundle = [NSBundle bundleForClass:factory];
  NSString *str = [bundle localizedStringForKey:key value:@"" table:nil];
  NSString *msg = [[NSString alloc] initWithFormat:str arguments:list];

  return [msg autorelease];
}

//------------------------------------------------------------------------------
NSString *StringWithLocalizedKey(NSString *key, ...) {
  va_list list;
  NSString *msg;
  
  va_start(list, key);
  msg = StringWithLocalizedKeyAndList(key, list);
  va_end(list);
  
  return msg;
}

//------------------------------------------------------------------------------
void ThrowExceptionKey(NSString *key, ...) {
  va_list list;
  NSString *msg;
  
  va_start(list, key);
  msg = StringWithLocalizedKeyAndList(key, list);
  va_end(list);
  
  [WebScriptObject throwException:msg];
}

//------------------------------------------------------------------------------
bool ResolveRelativeUrl(const char16 *base, const char16 *url,
                        std::string16 *resolved) {
  assert(base);
  assert(url);
  assert(resolved);
  
  std::string baseUTF8;
  if (!String16ToUTF8(base, &baseUTF8))
    return false;
  
  std::string urlUTF8;
  if (!String16ToUTF8(url, &urlUTF8))
    return false;
  
  NSString *baseStr = [NSString stringWithUTF8String:baseUTF8.c_str()];
  NSString *urlStr = [NSString stringWithUTF8String:urlUTF8.c_str()];
  NSString *resolvedStr = [GearsURLUtilities resolveURLString:urlStr
                                                   baseURLStr:baseStr];

  if (UTF8ToString16([resolvedStr UTF8String], resolved))
    return true;
  
  return false;
}

//------------------------------------------------------------------------------
int64 GetCurrentTimeMillis() {
  // Return millisecond time based on 1970 epoch
  return (CFAbsoluteTimeGetCurrent() + 
          kCFAbsoluteTimeIntervalSince1970) * 1000.0;
}
