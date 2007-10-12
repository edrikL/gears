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

#import "gears/base/common/common_sf.h"
#import "gears/base/common/factory_utils.h"
#import "gears/base/common/product_version.h"
#import "gears/base/common/string_utils.h"
#import "gears/base/safari/base_class.h"
#import "gears/base/safari/browser_utils.h"
#import "gears/base/safari/factory.h"
#import "gears/base/safari/factory_utils.h"
#import "gears/base/safari/loader.h"
#import "gears/base/safari/string_utils.h"
#import "gears/database/safari/database.h"
#import "gears/workerpool/safari/workerpool.h"

@class GearsDatabase;
@class GearsLocalServer;
@class GearsWorkerPool;

@interface SafariGearsFactory(PrivateMethods)
// Return an autoreleased instance of a SafariGearsBaseClass that has previously
// registered itself with this class. The |version| string should match the
// style described for the SafariGearsBaseClass protocol.  The script visible
// "create" function comes through here.
- (id)createModule:(NSString *)name versionString:(NSString *)versionStr;

// Return the info about this build.  Maps to the script visible "getBuildInfo" 
// function.
- (NSString *)buildInfo;
@end

@implementation SafariGearsFactory
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Public ||
//------------------------------------------------------------------------------
- (id)initWithArguments:(NSDictionary *)arguments {
  if ((self = [super init])) {
    arguments_ = [arguments retain];
    
    if (![GearsLoader canLoadGears]) {
      [self release];
      self = nil;
      return self;
    }
    
    factory_ = new GearsFactory();
    NSURL *baseURL = [arguments objectForKey:WebPlugInBaseURLKey];

    if (!factory_->InitBaseFromDOM([[baseURL absoluteString] UTF8String])) {
      [self release];
      self = nil;
      return self;
    }
  }
  
  return self;
}

//------------------------------------------------------------------------------
- (void)setArguments:(NSDictionary *)arguments {
  [arguments_ autorelease];
  arguments_ = [arguments retain];
}

//------------------------------------------------------------------------------
- (NSDictionary *)arguments {
  return arguments_;
}

//------------------------------------------------------------------------------
- (GearsFactory *)gearsFactory {
  return factory_;
}

//------------------------------------------------------------------------------
- (GearsWorkerSupervisor *)supervisor {
  if (!supervisor_)
    supervisor_ = [[GearsWorkerSupervisor alloc] init];
  
  return supervisor_;
}

//------------------------------------------------------------------------------
- (NSString *)resolveURLString:(NSString *)urlString {
  return [GearsURLUtilities resolveURLString:urlString 
                             usingPluginArgs:arguments_];
}

//------------------------------------------------------------------------------
- (void)shutdown {
  [supervisor_ release];
  supervisor_ = nil;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject (WebScripting) ||
//------------------------------------------------------------------------------
+ (BOOL)isKeyExcludedFromWebScript:(const char *)key {
  // No public keys
  return YES;
}

//------------------------------------------------------------------------------
+ (NSString *)webScriptNameForKey:(const char *)key {
  // No public keys
  return nil;
}

//------------------------------------------------------------------------------
+ (BOOL)isSelectorExcludedFromWebScript:(SEL)sel {
  if (sel == @selector(createModule:versionString:))
    return NO;
  else if (sel == @selector(buildInfo))
    return NO;
  
  return YES;
}

//------------------------------------------------------------------------------
+ (NSString *)webScriptNameForSelector:(SEL)sel {
  if (sel == @selector(createModule:versionString:))
    return @"create";
  else if (sel == @selector(buildInfo))
    return @"getBuildInfo";
  
  return nil;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject ||
//------------------------------------------------------------------------------
- (void)dealloc {
  [arguments_ release];
  delete factory_;
  [super dealloc];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Private ||
//------------------------------------------------------------------------------
- (id)createModule:(NSString *)name versionString:(NSString *)version {
  if (version == (NSString *)[WebUndefined undefined])
    version = nil;

#ifdef TIMEBOMB_EXPIRATION
  if (HasTimebombExpired())
    ThrowExceptionKeyAndReturnNil(@"expired");
#endif
  
  std::string16 versionStr;
  if (![version string16:&versionStr])
    ThrowExceptionKeyAndReturnNil(@"invalidVersionFmt", version);
  
  if (!HasPermissionToUseScour(factory_)) 
    ThrowExceptionKeyAndReturnNil(@"permissionDenied");

  // Initialize to be the current version
  int major_version_desired;
  int minor_version_desired;
  if (!ParseMajorMinorVersion(versionStr.c_str(), 
                              &major_version_desired, 
                              &minor_version_desired)) {
    ThrowExceptionKeyAndReturnNil(@"invalidVersionFmt", version);
  }
  
  // Determine the class for this name/version
  Class moduleClass = nil;
  if ([name isEqualToString:@"beta.database"]) {
    if (major_version_desired == kGearsDatabaseVersionMajor &&
        minor_version_desired <= kGearsDatabaseVersionMinor)
      moduleClass = [GearsDatabase class];
  } else if ([name isEqualToString:@"beta.localserver"]) {
    if (major_version_desired == kGearsLocalServerVersionMajor &&
        minor_version_desired <= kGearsLocalServerVersionMinor)
      moduleClass = [GearsLocalServer class];
  } else if ([name isEqualToString:@"beta.workerpool"]) {
    if (major_version_desired == kGearsWorkerPoolVersionMajor &&
        minor_version_desired <= kGearsWorkerPoolVersionMinor)
      moduleClass = [GearsWorkerPool class];
  } else {
    ThrowExceptionKeyAndReturnNil(@"invalidModuleNameFmt", name);
  }
  
  if (!moduleClass)
    ThrowExceptionKeyAndReturnNil(@"unableToCreateComponentFmt",
                                  name, version);

  // Allocate the class
  return [[[moduleClass alloc] initWithFactory:self] autorelease];
}

//------------------------------------------------------------------------------
- (NSString *)buildInfo {
  std::string16 info;
  AppendBuildInfo(&info);

  return [NSString stringWithString16:info.c_str()];
}

@end
