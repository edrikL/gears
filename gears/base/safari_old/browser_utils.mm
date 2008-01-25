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

#import "gears/base/common/security_model.h"
#import "gears/base/safari/string_utils.h"
#import "gears/base/safari/browser_utils.h"
#import "gears/base/safari/scoped_cf.h"

@interface NSString(GearsUsedWebKitPrivate)
// WebKit/Misc/WebNSURLExtras.m
- (NSString *)_web_mapHostNameWithRange:(NSRange)range encode:(BOOL)encode
                             makeString:(BOOL)makeString;
@end

@implementation GearsURLUtilities
//------------------------------------------------------------------------------
+ (NSString *)resolveURLString:(NSString *)relativeURLStr
               usingPluginArgs:(NSDictionary *)args {
  NSURL *baseURL = [args objectForKey:WebPlugInBaseURLKey];
  
  return [GearsURLUtilities resolveURLString:relativeURLStr 
                                  baseURLStr:[baseURL absoluteString]];
}

//------------------------------------------------------------------------------
+ (NSString *)resolveURLString:(NSString *)relativeURLStr
                    baseURLStr:(NSString *)baseURLStr {
  NSURL *baseURL = [NSURL URLWithString:baseURLStr];
  NSURL *url = [NSURL URLWithString:relativeURLStr relativeToURL:baseURL];
  NSString *fragment = [url fragment];
  NSString *urlStr = [url absoluteString];
  
  if (fragment) {
    NSMutableString *urlStrCopy = [urlStr mutableCopy];
    
    // Tack on the #
    fragment = [NSString stringWithFormat:@"#%@", fragment];
    [urlStrCopy replaceOccurrencesOfString:fragment withString:@"" options:0 
                                     range:NSMakeRange(0, [urlStrCopy length])];
    urlStr = [NSString stringWithString:urlStrCopy];
  }
  
  return urlStr;
}

@end

//------------------------------------------------------------------------------
bool CFURLRefToString16(CFURLRef url, std::string16 *out16) {
  if (!url || !out16)
    return false;
  
  scoped_CFURL absolute(CFURLCopyAbsoluteURL(url));
  CFStringRef absoluteStr = CFURLGetString(absolute.get());
  
  return CFStringRefToString16(absoluteStr, out16);
}

//------------------------------------------------------------------------------
CFURLRef CFURLCreateWithString16(const char16 *url_str) {
  scoped_CFString url(CFStringCreateWithString16(url_str));
  
  return CFURLCreateWithString(kCFAllocatorDefault, url.get(), NULL);
}

//------------------------------------------------------------------------------
bool SafariURLUtilities::GetPageOrigin(const char *url_str, 
                                       SecurityOrigin *security_origin) {
  std::string16 location;
  UTF8ToString16(url_str, &location);

  return security_origin->InitFromUrl(location.c_str());
}

//------------------------------------------------------------------------------
bool SafariURLUtilities::GetPageOriginFromArguments(
                             CFDictionaryRef dict, 
                             SecurityOrigin *security_origin) {
  NSURL *baseURL = [(NSDictionary *)dict objectForKey:WebPlugInBaseURLKey];
  
  if (baseURL)
    return GetPageOrigin([[baseURL absoluteString] UTF8String], 
                         security_origin);
  
  return false;
}

