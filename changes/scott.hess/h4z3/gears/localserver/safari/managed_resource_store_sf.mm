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

#import "gears/base/safari/browser_utils.h"
#import "gears/base/safari/factory.h"
#import "gears/base/safari/factory_utils.h"
#import "gears/localserver/safari/managed_resource_store_sf.h"
#import "gears/localserver/safari/update_task_sf.h"

@interface GearsManagedResourceStore(PrivateMethods)
- (void)checkForUpdate;
- (void)updateStatusInfo;
- (void)handleEventCode:(int)code parameter:(int)parameter
                   task:(AsyncTask *)task;
@end

//------------------------------------------------------------------------------
void CaptureManagedStoreListener::HandleEvent(int msg_code, int msg_param, 
                                              AsyncTask *source) {
  SEL sel = @selector(handleEventCode:parameter:task:);
  NSMethodSignature *sig = [target_ methodSignatureForSelector:sel];
  NSInvocation *inv = [NSInvocation invocationWithMethodSignature:sig];
  
  [inv setTarget:target_];
  [inv setSelector:sel];
  [inv setArgument:&msg_code atIndex:2];
  [inv setArgument:&msg_param atIndex:3];
  [inv setArgument:&source atIndex:4];
  [inv invoke];
}

@implementation GearsManagedResourceStore
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Class ||
//------------------------------------------------------------------------------
+ (BOOL)hasStoreNamed:(NSString *)name cookie:(NSString *)cookie 
             security:(const SecurityOrigin *)security 
             existing:(int64 *)existing_store_id {
  std::string16 nameStr, cookieStr, domainStr;
  
  if (![name string16:&nameStr])
    ThrowExceptionKeyAndReturnNo(@"invalidParameter");
  
  if (![cookie string16:&cookieStr])
    ThrowExceptionKeyAndReturnNo(@"invalidParameter");
  
  return ManagedResourceStore::ExistsInDB(*security, nameStr.c_str(),
                                          cookieStr.c_str(), existing_store_id);
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Public ||
//------------------------------------------------------------------------------
- (id)initWithName:(NSString *)name cookie:(NSString *)cookie 
         factory:(SafariGearsFactory *)factory 
          existing:(int64)existing_store_id {
  if ((self = [super initWithFactory:factory])) {
    name_ = [name copy];
    requiredCookie_ = [cookie copy];
    lastErrorMessage_ = @"";
    cpp_ = new CPP_managed_resource_store;
    cpp_->listener_.reset(new CaptureManagedStoreListener(self));

    if (existing_store_id != WebCacheDB::kInvalidID) {
      if (!cpp_->store_.Open(existing_store_id)) {
        ThrowExceptionKey(@"unableToOpenStore");
        [self release];
        self = nil;
      }
    } else {
      std::string16 nameStr, cookieStr, domainStr;
      BOOL isValid = true;
      
      isValid |= [name string16:&nameStr];
      isValid |= [cookie string16:&cookieStr];
      
      if (!isValid)
        ThrowExceptionKey(@"invalidParameter");

      if (!cpp_->store_.CreateOrOpen(base_->EnvPageSecurityOrigin(),
                                     nameStr.c_str(),
                                     cookieStr.c_str()) || !isValid) {
        ThrowExceptionKey(@"unableToCreateStore");
        [self release];
        self = nil;
      }
    }
  } 
  
  return self;
}

//------------------------------------------------------------------------------
- (BOOL)removeStore {
  return cpp_ ? cpp_->store_.Remove() : NO;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || SafariGearsBaseClass ||
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"checkForUpdate", @"checkForUpdate",
    nil];
}

//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptKeys {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"name", @"name_",
    @"requiredCookie", @"requiredCookie_",
    @"enabled", @"enabled_",
    @"manifestUrl", @"manifestUrl_",
    @"lastUpdateCheckTime", @"lastUpdateCheckTime_",
    @"updateStatus", @"updateStatus_",
    @"lastErrorMessage", @"lastErrorMessage_",
    @"currentVersion", @"currentVersion_",
    nil];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject ||
//------------------------------------------------------------------------------
- (void)dealloc {
  delete cpp_;
  [super dealloc];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject (NSKeyValueCoding) ||
//------------------------------------------------------------------------------
- (void)setValue:(id)value forKey:(NSString *)key {
  if ([key isEqualToString:@"enabled_"]) {
    if ([value respondsToSelector:@selector(boolValue)])
      cpp_->store_.SetEnabled([value boolValue]);
  } else if ([key isEqualToString:@"manifestUrl_"]) {
    NSString *resolved = [factory_ resolveURLString:value];
    std::string16 url;
    [resolved string16:&url];
    
    if (!base_->EnvPageSecurityOrigin().IsSameOriginAsUrl(url.c_str()))
      ThrowExceptionKeyAndReturn(@"invalidDomainAccess");
    
    if (!cpp_->store_.SetManifestUrl(url.c_str()))
      ThrowExceptionKeyAndReturn(@"unableToSetManifest");
  }
}

//------------------------------------------------------------------------------
- (id)valueForKey:(NSString *)key {
  if ([key isEqualToString:@"enabled_"]) {
    return [NSNumber numberWithBool:cpp_->store_.IsEnabled()];
  } else if ([key isEqualToString:@"manifestUrl_"]) {
    std::string16 url;
    if (!cpp_->store_.GetManifestUrl(&url))
      ThrowExceptionKeyAndReturnNil(@"unableToGetManifest");
        
    return [NSString stringWithString16:url.c_str()];
  } else if ([key isEqualToString:@"currentVersion_"]) {
    std::string16 version;
    cpp_->store_.GetVersionString(WebCacheDB::VERSION_CURRENT, &version);
    return [NSString stringWithString16:version.c_str()];
  } else {
    // Lookup the internal key name: name, cookie
    NSString *foundKey = [[[self class] webScriptKeys] objectForKey:key];
    
    if (foundKey) {
      // Refresh the status info
      [self updateStatusInfo];
      return [super valueForKey:key];
    }
  }
  
  return nil;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Private ||
//------------------------------------------------------------------------------
- (void)checkForUpdate {
  if (cpp_->update_task_.get()) {
    // We're already running an update task, just return
    return;
  }
    
  cpp_->update_task_.reset(UpdateTask::CreateUpdateTask());
  if (!cpp_->update_task_->Init(&cpp_->store_)) {
    ThrowExceptionKeyAndReturn(@"unableToInitializeUpdate");
  }
  
  cpp_->update_task_->SetListener(cpp_->listener_.get());
  if (!cpp_->update_task_->Start()) {
    cpp_->update_task_.reset(NULL);
    ThrowExceptionKeyAndReturn(@"unableToStartUpdate");
  }
  
  cpp_->update_task_->AwaitStartup();
}

//------------------------------------------------------------------------------
- (void)updateStatusInfo {
  std::string16 error_msg;
  WebCacheDB::UpdateStatus update_status = WebCacheDB::UPDATE_OK;
  int64 time64 = 0;
  
  if (!cpp_->store_.GetUpdateInfo(&update_status, &time64, NULL, &error_msg)) {
    NSLog(@"** Unable to get status **");
    return;
  }
  
  [lastErrorMessage_ release];
  lastErrorMessage_ = [[NSString stringWithString16:error_msg.c_str()] retain];
  updateStatus_ = update_status;
  lastUpdateCheckTime_ = (int)(time64 / 1000);
}

//------------------------------------------------------------------------------
- (void)handleEventCode:(int)code parameter:(int)parameter
                   task:(AsyncTask *)task {
  UpdateTask *update_task = reinterpret_cast<UpdateTask*>(task);

  if (update_task && (update_task == cpp_->update_task_.get())) {
    cpp_->update_task_->SetListener(NULL);
    cpp_->update_task_.release()->DeleteWhenDone();
  }
}

@end
