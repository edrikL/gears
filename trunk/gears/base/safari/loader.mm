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

#import "genfiles/product_constants.h"
#import "gears/base/safari/loader.h"

@implementation GearsLoader

+ (int)webKitBuildNumber {
  static int webkit_version = 0;

  if (webkit_version != 0) {
    return webkit_version;
  }

  Class webViewClass = NSClassFromString(@"WebView");
  NSBundle *webkitBundle = [NSBundle bundleForClass:webViewClass];
  NSString *key = (NSString *)kCFBundleVersionKey;
  NSString *vers = [webkitBundle objectForInfoDictionaryKey:key];

  // Check the version of WebKit that the current application is using and
  // see if it's new enough to load with.

  // A (somewhat dated list) of Safari Version strings and equivalent
  // CFBundleVersions can be found here:
  // http://developer.apple.com/internet/safari/uamatrix.html
  // It seems that the most significant digit is the OS Version, and the
  // rest denote the real safari version number.
  // Some values we've seen empirically:
  // 5523.15.1 - Safari 3.0.4 on 10.5.2
  // 4525.18   - Safari 3.1.1 on 10.4
  // 5525.18   - Safari 3.1.1 on 10.5
  // 4525.22   - Safari 3.1.2 on 10.4
  // 5525.18   - Safari 3.1.2 on 10.5
  // 5528.1    - Savari 4 public beta on 10.5
  int version = floor([vers floatValue]);

  // Strip off MSD, apparently corresponding to OS Version.
  webkit_version = version % 1000;

  return webkit_version;
}
//------------------------------------------------------------------------------
+ (BOOL)canLoadGears {
  int version = [GearsLoader webKitBuildNumber];

  // 525 is Safari 3.1.1
  if (version >= 525)
    return YES;

  NSLog(@"%s requires WebKit X525 or later (Current: %d)",
        PRODUCT_FRIENDLY_NAME_ASCII, version);
  return NO;
}

//------------------------------------------------------------------------------
+ (NSString *)gearsBundlePath {
  // There are a couple of interesting twists here surrounding case-sensitive/
  // insensitive file systems.
  // The HFS+ file system used on Macs has traditionally been case insensitive
  // however there is now an option to format HFS+ disks to be case sensitive.
  // (http://support.apple.com/kb/TA21400?viewlocale=en_US).
  // Now for the fun part:
  // * On case insensitive HFS+ volumes we need to be careful to load the Gears
  // plugin using exactly the same case as that used on the disk.  If we don't
  // we run into a bizarre bug by which DYLD can load the bundle twice into our
  // process, since it can't disambiguate 'gears.plugin' and 'Gears.plugin'.
  // This causes all kinds of mayhem, e.g. pthread_once will fire twice...
  // * On case sensitive HFS+ volumes, if we don't specify the correct case
  // the plugin won't load since the file won't be found.
  NSArray *paths =
    NSSearchPathForDirectoriesInDomains(
        NSLibraryDirectory,
        (NSSearchPathDomainMask)(NSUserDomainMask | NSLocalDomainMask),
        YES);

  // This must match the case of the Gears.plugin name exactly!  See above.
  NSString *gears_plugin_basename = @"Gears.plugin"; // [naming]

  // Find the first one that exists
  unsigned int i, count = [paths count];
  NSFileManager *fileManager = [NSFileManager defaultManager];
  for (i = 0; i < count; ++i) {
    NSString *path = [paths objectAtIndex:i];
    NSString *internetPlugins = @"Internet Plug-Ins";

    path = [path stringByAppendingPathComponent:internetPlugins];

    BOOL isDir;
    NSString *tmp_path = [path stringByAppendingPathComponent:
                                   gears_plugin_basename];
    if (![fileManager fileExistsAtPath:tmp_path isDirectory:&isDir] || !isDir) {
      continue;
    } else {
      return tmp_path;
    }
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
