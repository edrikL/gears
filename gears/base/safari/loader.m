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

#import "genfiles/product_name_constants.h"
#import "gears/base/safari/loader.h"

@implementation GearsLoader
//------------------------------------------------------------------------------
+ (BOOL)canLoadGears {
  Class webViewClass = NSClassFromString(@"WebView");
  NSBundle *webkitBundle = [NSBundle bundleForClass:webViewClass];
  NSString *key = (NSString *)kCFBundleVersionKey;
  NSString *vers = [webkitBundle objectForInfoDictionaryKey:key];
  
  if ([vers floatValue] >= 522)
    return YES;
  
  NSLog(@"%s requires WebKit 522 or later (Current: %d)",
        PRODUCT_FRIENDLY_NAME_ASCII, [vers intValue]);
  return NO;
}

//------------------------------------------------------------------------------
+ (NSString *)gearsBundlePath {
  NSArray *paths = 
    NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, 
                                        NSUserDomainMask | NSLocalDomainMask,
                                        YES);
  
  // Find the first one that exists
  unsigned int i, count = [paths count];
  NSFileManager *fileManager = [NSFileManager defaultManager];
  for (i = 0; i < count; ++i) {
    NSString *path = [paths objectAtIndex:i];
    NSString *internetPlugins = @"Internet Plug-Ins";
    NSString *ident = [NSString stringWithFormat:@"%s.plugin",
      PRODUCT_SHORT_NAME_ASCII];
    
    path = [path stringByAppendingPathComponent:internetPlugins];
    path = [path stringByAppendingPathComponent:ident];
    
    BOOL isDir;
    if ([fileManager fileExistsAtPath:path isDirectory:&isDir] && isDir)
      return path;
  }
  
  return nil;
}

//------------------------------------------------------------------------------
+ (BOOL)loadGearsBundle {
  NSBundle *bundle = [NSBundle bundleWithPath:[GearsLoader gearsBundlePath]];
  
  return [bundle load];
}

//------------------------------------------------------------------------------

@end
