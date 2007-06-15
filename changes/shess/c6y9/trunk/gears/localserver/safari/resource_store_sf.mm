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
#import "gears/base/safari/factory.h"
#import "gears/base/safari/factory_utils.h"
#import "gears/base/safari/string_utils.h"
#import "gears/base/safari/browser_utils.h"
#import "gears/localserver/common/capture_task.h"
#import "gears/localserver/safari/file_submitter_sf.h"
#import "gears/localserver/safari/resource_store.h"

@interface GearsResourceStore(PrivateMethods)
- (NSNumber *)capture:(id)urlOrArray callback:(id)callback;
- (void)abortCapture:(NSNumber *)captureID;
- (void)remove:(NSString *)url;
- (void)renameSrc:(NSString *)src dest:(NSString *)dest;
- (void)copySrc:(NSString *)src dest:(NSString *)dest;
- (NSNumber *)isCaptured:(NSString *)url;
- (void)captureFile:(id)element url:(NSString *)url;
- (NSString *)getCapturedFileName:(NSString *)url;
- (NSString *)getHeaderURL:(NSString *)url name:(NSString *)name;
- (NSString *)getAllHeaders:(NSString *)url;
- (id)createFileSubmitter;
- (void)startCaptureTaskIfNeeded:(BOOL)fireEventsOnFailure;
- (void)fireFailedEvents:(SFCaptureRequest *)request;
- (void)invokeCompletionCallback:(SFCaptureRequest *)request
                             URL:(NSString *)urlStr success:(BOOL)success;
- (void)handleEventCode:(int)code parameter:(int)parameter task:(AsyncTask *)task;
- (void)onCaptureUrlAtIndex:(int)index success:(BOOL)success;
- (void)onCaptureTaskComplete;
- (BOOL)resolveAndAppendURL:(NSString *)urlStr 
                    request:(SFCaptureRequest *)request;
@end

static unsigned int next_capture_id_ = 1;

//------------------------------------------------------------------------------
void CaptureStoreListener::HandleEvent(int msg_code, int msg_param, AsyncTask *source) {
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

@implementation GearsResourceStore
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Class ||
//------------------------------------------------------------------------------
+ (BOOL)hasStoreNamed:(NSString *)name cookie:(NSString *)cookie 
             security:(SecurityOrigin *)security 
             existing:(int64 *)existing_store_id {
  std::string16 nameStr, cookieStr, domainStr;
  
  if (![name string16:&nameStr])
    ThrowExceptionKeyAndReturnNo(@"invalidParameter");
  
  if (![cookie string16:&cookieStr])
    ThrowExceptionKeyAndReturnNo(@"invalidParameter");
  
  return ResourceStore::ExistsInDB(*security, nameStr.c_str(),
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
    cpp_ = new CPP_resource_store;
    cpp_->listener_.reset(new CaptureStoreListener(self));
    
    std::string16 nameStr, cookieStr, domainStr;
    BOOL isValid = true;
    
    isValid |= [name string16:&nameStr];
    isValid |= [cookie string16:&cookieStr];

    if (!isValid)
      ThrowExceptionKey(@"invalidParameter");

    if (existing_store_id != WebCacheDB::kInvalidID) {
      if (!cpp_->store_.Open(existing_store_id)) {
        [self release];
        self = nil;
      }
    } else {
      if (!cpp_->store_.CreateOrOpen(*envPageOrigin_, nameStr.c_str(),
                                     cookieStr.c_str()) || !isValid) {
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
#pragma mark || GearsComponent ||
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"capture", @"capture:callback:",
    @"abortCapture", @"abortCapture:",
    @"remove", @"remove:",
    @"rename", @"renameSrc:dest:",
    @"copy", @"copySrc:dest:",
    @"isCaptured", @"isCaptured:",
    @"captureFile", @"captureFile:url:",
    @"getCapturedFileName", @"getCapturedFileName:",
    @"getHeader", @"getHeaderURL:name:",
    @"getAllHeaders", @"getAllHeaders:",
    @"createFileSubmitter", @"createFileSubmitter",
    nil];
}

//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptKeys {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"name", @"name_",
    @"requiredCookie", @"requiredCookie_",
    @"enabled", @"enabled_",
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
  }
}

//------------------------------------------------------------------------------
- (id)valueForKey:(NSString *)key {
  if ([key isEqualToString:@"name_"]) {
    return [NSString stringWithString16:cpp_->store_.GetName()];
  }
  
  if ([key isEqualToString:@"enabled_"]) {
    return [NSNumber numberWithBool:cpp_->store_.IsEnabled()];
  }

  if ([key isEqualToString:@"requiredCookie_"]) {
    return [NSString stringWithString16:cpp_->store_.GetRequiredCookie()];
  }
  
  return nil;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || GearsWebCaptureStore(PrivateMethods) ||
//------------------------------------------------------------------------------
- (NSNumber *)capture:(id)urlOrArray callback:(id)callback {
  scoped_ptr<SFCaptureRequest> request(new SFCaptureRequest);
  BOOL valid = YES;
  
  request->id = next_capture_id_++;
  request->completion_callback = [callback retain];
  NSDictionary *arguments = [factory_ arguments];
  SafariURLUtilities::GetPageOriginFromArguments((CFDictionaryRef)arguments,
                                                 &request->security_origin);
 
  // Determine what was passed as arguments
  if ([urlOrArray isKindOfClass:[WebScriptObject class]]) {
    for (unsigned int i = 0; valid; ++i) {
      id obj = [(WebScriptObject *)urlOrArray webScriptValueAtIndex:i];

      if (obj == (id)[WebUndefined undefined])
        break;

      valid = [self resolveAndAppendURL:obj request:request.get()];
    }
  } else {
    valid = [self resolveAndAppendURL:urlOrArray request:request.get()];
  }
  
  if (!valid)
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");
  
  NSNumber *result = [NSNumber numberWithInt:valid ? request->id : 0];
  cpp_->pending_requests_.push_back(request.release());

  [self startCaptureTaskIfNeeded:false];
  
  return result;
}

//------------------------------------------------------------------------------
- (void)abortCapture:(NSNumber *)captureNum {
  int capture_id = [captureNum intValue];
  
  // Stop the current task (if matching)
  if (cpp_->current_request_.get() && 
      cpp_->current_request_->id == capture_id) {
    if (cpp_->capture_task_.get())
      cpp_->capture_task_->Abort();
    return;
  }
  
  // Look for a pending task
  std::deque<SFCaptureRequest*>::iterator iter;
  for (iter = cpp_->pending_requests_.begin();
       iter < cpp_->pending_requests_.end();
       ++iter) {
    if ((*iter)->id == capture_id) {
      // Remove it from the queue and fire completion events
      SFCaptureRequest *request = (*iter);
      cpp_->pending_requests_.erase(iter);
      delete request;
      // Note: the deque.erase() call is safe here since we return and
      // do not continue the iteration
      return;
    }
  }
}

//------------------------------------------------------------------------------
- (void)remove:(NSString *)url {
  NSString *fullURL = [factory_ resolveURLString:url];
  std::string16 url16;
  
  if ([fullURL string16:&url16])
    cpp_->store_.Delete(url16.c_str());
  else
    ThrowExceptionKeyAndReturn(@"invalidParameter");
}

//------------------------------------------------------------------------------
- (void)renameSrc:(NSString *)src dest:(NSString *)dest {
  NSString *fullSrc = [factory_ resolveURLString:src];
  NSString *fullDest = [factory_ resolveURLString:dest];
  std::string16 src16;
  
  if (![fullSrc string16:&src16])
    ThrowExceptionKeyAndReturn(@"invalidParameter");
    
    std::string16 dest16;
    
  if (![fullDest string16:&dest16])
    ThrowExceptionKeyAndReturn(@"invalidParameter");
    
  if (!cpp_->store_.Rename(src16.c_str(), dest16.c_str()))
    ThrowExceptionKeyAndReturn(@"unableToRenameFmt", src, dest);
}

//------------------------------------------------------------------------------
- (void)copySrc:(NSString *)src dest:(NSString *)dest {
  NSString *fullSrc = [factory_ resolveURLString:src];
  NSString *fullDest = [factory_ resolveURLString:dest];
  std::string16 src16;
  
  if (![fullSrc string16:&src16])
    ThrowExceptionKeyAndReturn(@"invalidParameter");
  
  std::string16 dest16;
  
  if (![fullDest string16:&dest16])
    ThrowExceptionKeyAndReturn(@"invalidParameter");

  if (!cpp_->store_.Copy(src16.c_str(), dest16.c_str()))
    ThrowExceptionKeyAndReturn(@"unableToCopyFmt", src, dest);
}

//------------------------------------------------------------------------------
- (NSNumber *)isCaptured:(NSString *)url {
  NSString *fullURL = [factory_ resolveURLString:url];
  std::string16 url16;
  
  if (![fullURL string16:&url16])
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");

  return [NSNumber numberWithBool:cpp_->store_.IsCaptured(url16.c_str())];
}

//------------------------------------------------------------------------------
- (void)captureFile:(id)element url:(NSString *)url {
  // TODO(waylonis)
  [WebScriptObject throwException:@"Invalid file input parameter."];
}

//------------------------------------------------------------------------------
- (NSString *)getCapturedFileName:(NSString *)url {
  NSString *fullURL = [factory_ resolveURLString:url];
  std::string16 url16;

  if (![fullURL string16:&url16])
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");
    
  std::string16 result;
  
  if (!cpp_->store_.GetCapturedFileName(url16.c_str(), &result))
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");

  return [NSString stringWithString16:result.c_str()];
}

//------------------------------------------------------------------------------
- (NSString *)getHeaderURL:(NSString *)url name:(NSString *)name {
  NSString *fullURL = [factory_ resolveURLString:url];
  std::string16 url16;
  
  if (![fullURL string16:&url16])
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");

  std::string16 name16;
  
  if (![name string16:&name16])
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");
    
  std::string16 result;
    
  if (!cpp_->store_.GetHeader(url16.c_str(), name16.c_str(), &result))
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");
    
  return [NSString stringWithString16:result.c_str()];
}

//------------------------------------------------------------------------------
- (NSString *)getAllHeaders:(NSString *)url {
  NSString *fullURL = [factory_ resolveURLString:url];
  std::string16 url16;
  
  if (![fullURL string16:&url16])
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");

  std::string16 result;
  
  if (!cpp_->store_.GetAllHeaders(url16.c_str(), &result))
    ThrowExceptionKeyAndReturnNil(@"invalidParameter");
  
  return [NSString stringWithString16:result.c_str()];
}

//------------------------------------------------------------------------------
- (id)createFileSubmitter {
  GearsFileSubmitter *submitter = 
    [[[GearsFileSubmitter alloc] initWithFactory:factory_] autorelease];

  return submitter;
}

//------------------------------------------------------------------------------
- (void)startCaptureTaskIfNeeded:(BOOL)sendEventsOnFailure {
  if (cpp_->capture_task_.get()) {
    assert(cpp_->current_request_.get());
    return;
  }
  
  if (cpp_->pending_requests_.empty())
    return;
  
  assert(!cpp_->current_request_.get());
  cpp_->current_request_.reset(cpp_->pending_requests_.front());
  cpp_->pending_requests_.pop_front();
  cpp_->capture_task_.reset(new CaptureTask());
    
  if (!cpp_->capture_task_->Init(&cpp_->store_, cpp_->current_request_.get())) {
    scoped_ptr<SFCaptureRequest> failed_request(cpp_->current_request_.release());
    cpp_->capture_task_.reset(NULL);
    if (sendEventsOnFailure)
      [self fireFailedEvents:failed_request.get()];

    ThrowExceptionKeyAndReturn(@"unableToInitializeCapture");
  }
  
  cpp_->capture_task_->SetListener(cpp_->listener_.get());
  if (!cpp_->capture_task_->Start()) {
    scoped_ptr<SFCaptureRequest> failed_request(cpp_->current_request_.release());
    cpp_->capture_task_.reset(NULL);
    if (sendEventsOnFailure)
      [self fireFailedEvents:failed_request.get()];
      
    ThrowExceptionKeyAndReturn(@"unableToStartCapture");
  }
}

//------------------------------------------------------------------------------
- (void)fireFailedEvents:(SFCaptureRequest *)request {
  if (request) {
    for (size_t i = 0; i < request->urls.size(); ++i) {
      NSString *urlStr = [NSString stringWithString16:request->urls[i].c_str()];
      [self invokeCompletionCallback:request URL:urlStr success:NO];
    }
  }
}

//------------------------------------------------------------------------------
- (void)invokeCompletionCallback:(SFCaptureRequest *)request 
                             URL:(NSString *)urlStr success:(BOOL)success {
  if (request && request->completion_callback) {
    // The first argument is ignored
    [request->completion_callback callWebScriptMethod:@"call" withArguments:
      [NSArray arrayWithObjects:@"placeholder", urlStr, 
        [NSNumber numberWithBool:success], 
        [NSNumber numberWithUnsignedInt:request->id], nil]];
  }
}

//------------------------------------------------------------------------------
- (void)handleEventCode:(int)code parameter:(int)parameter 
                   task:(AsyncTask *)task {
  CaptureTask *capture_task = reinterpret_cast<CaptureTask*>(task);
  if (capture_task && capture_task == cpp_->capture_task_.get()) {
    if (code == CaptureTask::CAPTURE_TASK_COMPLETE) {
      [self onCaptureTaskComplete];      
    } else {
      // param = the index of the url that has been processed
      BOOL success = (code == CaptureTask::CAPTURE_URL_SUCCEEDED);
      [self onCaptureUrlAtIndex:parameter success:success];
    }
  }
}

//------------------------------------------------------------------------------
- (void)onCaptureUrlAtIndex:(int)index success:(BOOL)success {
  SFCaptureRequest *request = cpp_->current_request_.get();
  NSString *urlStr = 
    [NSString stringWithString16:request->urls[index].c_str()];
  
  [self invokeCompletionCallback:request URL:urlStr success:success];
}

//------------------------------------------------------------------------------
- (void)onCaptureTaskComplete {
  SFCaptureRequest *request = cpp_->current_request_.get();
  cpp_->capture_task_.release()->DeleteWhenDone();
  [request->completion_callback release];
  cpp_->current_request_.reset(NULL);
  [self startCaptureTaskIfNeeded:YES];  
}

//------------------------------------------------------------------------------
- (BOOL)resolveAndAppendURL:(NSString *)urlStr 
                    request:(SFCaptureRequest *)request {
  NSString *fullURLStr = [factory_ resolveURLString:urlStr];
  std::string16 url;
  if (![urlStr string16:&url])
    return NO;
  
  std::string16 full_url;
  if (![fullURLStr string16:&full_url])
    return NO;
  
  if (!envPageOrigin_->IsSameOriginAsUrl(full_url.c_str())) {
    ThrowExceptionKeyAndReturnNo(@"invalidDomainAccess");
  }
  
  request->urls.push_back(std::string16());
  request->urls.back() = url;
  request->full_urls.push_back(std::string16());
  request->full_urls.back() = full_url; 
  
  return YES;
}

@end
