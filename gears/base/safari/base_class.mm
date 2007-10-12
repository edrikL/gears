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

#import "gears/base/common/base_class.h"
#import "gears/base/common/common_sf.h"
#import "gears/base/common/security_model.h"
#import "gears/base/safari/base_class.h"
#import "gears/base/safari/browser_utils.h"
#import "gears/base/safari/string_utils.h"
#import "gears/factory/safari/factory.h"
#import "gears/factory/safari/factory_utils.h"

@implementation SafariGearsBaseClass
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Class ||
//------------------------------------------------------------------------------
+ (NSArray *)convertWebScriptArray:(WebScriptObject *)array {
  if (array == (WebScriptObject *)[WebUndefined undefined])
    return nil;
  
  if (array == nil)
    return nil;
  
  // The |object| must be a WebScriptObject
  if ([array class] != [WebScriptObject class])
    return (NSArray *)[NSNull null];
  
  NSNumber *lengthProperty = [array valueForKey:@"length"];
  
  if (!lengthProperty)
    return nil;
  
  NSMutableArray *converted = [NSMutableArray array];
  int length = [lengthProperty intValue];
  
  for (int i = 0; i < length; ++i) {
    id obj = [array webScriptValueAtIndex:i];
    
    // Remap nil object to NSNull
    if (obj == nil)
      obj = [NSNull null];
    
    [converted addObject:obj];
  }
  
  return converted;
}
  
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || SafariGearsBaseClass (protocol) ||
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings {
  return nil;
}

//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptKeys {
  return nil;
}

//------------------------------------------------------------------------------
- (id)initWithFactory:(SafariGearsFactory *)factory {
  if ((self = [super init])) {
    factory_ = [factory retain];
    factory_ = factory;
    base_ = new GearsBaseClass();
    
    if (!base_->InitBaseFromSibling([factory gearsFactory])) {
      [self release];
      self = nil;
    }

    // When building the WorkerPool application, GEARS_WORKER will be
    // defined to be 1
#if GEARS_WORKER
    isWorker_ = YES;
#endif
  }
  
  return self;
}

//------------------------------------------------------------------------------
- (SafariGearsFactory *)factory {
  return factory_;
}

//------------------------------------------------------------------------------
- (GearsBaseClass *)gearsBaseClass {
  return base_;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject ||
//------------------------------------------------------------------------------
- (void)dealloc {
  [factory_ release];
  delete base_;
  [super dealloc];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || WebScripting (informal protocol) ||
//------------------------------------------------------------------------------
+ (BOOL)isSelectorExcludedFromWebScript:(SEL)sel {
  // If there's a non-nil selector, we want to expose it
  return [self webScriptNameForSelector:sel] ? NO : YES;
}

//------------------------------------------------------------------------------
+ (NSString *)webScriptNameForSelector:(SEL)sel {
  NSString *selStr = NSStringFromSelector(sel);
  NSDictionary *strs = [self webScriptSelectorStrings];
  NSString *name = [strs objectForKey:selStr];

  return name;
}

//------------------------------------------------------------------------------
+ (BOOL)isKeyExcludedFromWebScript:(const char *)key {
  // If there's a non-nil key, we want to expose it
  return [self webScriptNameForKey:key] ? NO : YES;
}

//------------------------------------------------------------------------------
+ (NSString *)webScriptNameForKey:(const char *)key {
  return [[self webScriptKeys] objectForKey:
    [NSString stringWithCString:key encoding:NSUTF8StringEncoding]];
}

@end
