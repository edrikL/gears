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
#import "gears/base/common/product_version.h"
#import "gears/base/safari/browser_utils.h"
#import "gears/factory/safari/factory.h"
#import "gears/factory/safari/factory_utils.h"
#import "gears/localserver/safari/localserver_sf.h"
#import "gears/localserver/safari/localserver_db_proxy.h"
#import "gears/localserver/safari/managed_resource_store_sf.h"
#import "gears/localserver/safari/resource_store_sf.h"

#ifdef DEBUG
#include "gears/localserver/common/localserver_tests.h" // for testing
#endif

@interface GearsLocalServer(PrivateMethods)
- (BOOL)validateName:(NSString **)name cookie:(NSString **)requiredCookie;
- (NSNumber *)canServeLocally:(NSString *)urlStr;
- (id)createStore:(NSString *)name cookie:(id)requiredCookie;
- (id)openStore:(NSString *)name cookie:(id)requiredCookie;
- (void)removeStore:(NSString *)name cookie:(id)requiredCookie;
- (id)createManagedStore:(NSString *)name cookie:(id)requiredCookie;
- (id)openManagedStore:(NSString *)name cookie:(id)requiredCookie;
- (void)removeManagedStore:(NSString *)name cookie:(id)requiredCookie;
@end

@implementation GearsLocalServer
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || SafariGearsBaseClass ||
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"canServeLocally", @"canServeLocally:",
    @"createStore", @"createStore:cookie:",
    @"openStore", @"openStore:cookie:",
    @"removeStore", @"removeStore:cookie:",
    @"createManagedStore", @"createManagedStore:cookie:",
    @"openManagedStore", @"openManagedStore:cookie:",
    @"removeManagedStore", @"removeManagedStore:cookie:",
    nil];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Private ||
//------------------------------------------------------------------------------
- (BOOL)validateName:(NSString **)name cookie:(NSString **)requiredCookie {
  // Ensure that there's a valid name
  if (!name || ![*name respondsToSelector:@selector(length)])
    ThrowExceptionKeyAndReturnNo(@"invalidParameter");
  
  if (![*name length])
    ThrowExceptionKeyAndReturnNo(@"invalidParameter");
  
  if (*requiredCookie == (id)[WebUndefined undefined])
    *requiredCookie = @"";
  
  return YES;
}

//------------------------------------------------------------------------------
- (NSNumber *)canServeLocally:(NSString *)urlStr {
  
  if (urlStr == (NSString *)[WebUndefined undefined]) 
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");
  
  if (![urlStr length])
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");
  
  NSString *fullURLStr = [factory_ resolveURLString:urlStr];

#ifdef DEBUG
  if ([fullURLStr isEqualToString:@"test:webcache"])
    return [NSNumber numberWithBool:TestWebCacheAll()];
#endif
  
  NSURL *url = [NSURL URLWithString:fullURLStr];
  
  if (!url)
    return [NSNumber numberWithBool:NO];
  
  std::string16 full_url;
  [fullURLStr string16:&full_url];
  if (!base_->EnvPageSecurityOrigin().IsSameOriginAsUrl(full_url.c_str())) {
    ThrowExceptionKey(@"invalidDomainAccess");
    return [NSNumber numberWithBool:NO];
  }
  
  return [NSNumber numberWithBool:[GearsWebCacheDB canService:url]];
}

//------------------------------------------------------------------------------
- (id)createStore:(NSString *)name cookie:(id)requiredCookie {
  NSString *cookie = requiredCookie;
  
  if (![self validateName:&name cookie:&cookie])
    return nil;
  
  GearsResourceStore *store = [[[GearsResourceStore alloc] initWithName:name
    cookie:cookie factory:factory_ existing:WebCacheDB::kInvalidID]
    autorelease];
  
  if (!store)
    ThrowExceptionKeyAndReturnNil(@"unableToInitializeResourceStore");
  
  return store;
}

//------------------------------------------------------------------------------
- (id)openStore:(NSString *)name cookie:(id)requiredCookie {
  NSString *cookie = requiredCookie;

  if (![self validateName:&name cookie:&cookie])
    return nil;

  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (![GearsResourceStore hasStoreNamed:name cookie:cookie 
                                security:&base_->EnvPageSecurityOrigin()
                                existing:&existing_store_id]) {
    return nil;
  }
 
  GearsResourceStore *store = [[[GearsResourceStore alloc] initWithName:name
    cookie:cookie factory:factory_ existing:existing_store_id]
    autorelease];
  
  if (!store)
    ThrowExceptionKeyAndReturnNil(@"unableToInitializeResourceStore");
  
  return store;
}

//------------------------------------------------------------------------------
- (void)removeStore:(NSString *)name cookie:(id)requiredCookie {
  GearsResourceStore *store = [self openStore:name cookie:requiredCookie];
  
  if (store) {
    if (![store removeStore])
      ThrowExceptionKeyAndReturn(@"unableToRemoveStoreFmt", name,
                                 requiredCookie);
  }
}

//------------------------------------------------------------------------------
- (id)createManagedStore:(NSString *)name cookie:(id)requiredCookie {
  NSString *cookie = requiredCookie;
  
  if (![self validateName:&name cookie:&cookie])
    return nil;
  
  GearsManagedResourceStore *store = 
    [[[GearsManagedResourceStore alloc] initWithName:name cookie:cookie
    factory:factory_ existing:WebCacheDB::kInvalidID] autorelease];
  
  if (!store)
    ThrowExceptionKeyAndReturnNil(@"unableToInitializeManagedResourceStore");
  
  return store;
}

//------------------------------------------------------------------------------
- (id)openManagedStore:(NSString *)name cookie:(id)requiredCookie {
  NSString *cookie = requiredCookie;
  
  if (![self validateName:&name cookie:&cookie])
    return nil;
 
  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (![GearsManagedResourceStore hasStoreNamed:name cookie:cookie 
                                       security:&base_->EnvPageSecurityOrigin()
                                       existing:&existing_store_id]) {
    return nil;
  }

  GearsManagedResourceStore *store = 
    [[[GearsManagedResourceStore alloc] initWithName:name cookie:cookie
    factory:factory_ existing:existing_store_id] autorelease];
  
  if (!store)
    ThrowExceptionKeyAndReturnNil(@"unableToInitializeManagedResourceStore");
  
  return store;
}

//------------------------------------------------------------------------------
- (void)removeManagedStore:(NSString *)name cookie:(id)requiredCookie {
  GearsResourceStore *store = [self openManagedStore:name 
                                              cookie:requiredCookie];
  
  if (store) {
    if (![store removeStore])
      ThrowExceptionKeyAndReturn(@"unableToRemoveStoreFmt", name,
                                 requiredCookie);
  }
}

@end
