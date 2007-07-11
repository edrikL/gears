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

// Implement the ResourceStore for Safari
#import <deque>

#import "gears/base/safari/base_class.h"
#import "gears/localserver/common/async_task.h"
#import "gears/localserver/common/resource_store.h"
#import "gears/localserver/safari/capture_task_sf.h"

class SecurityOrigin;

class CaptureStoreListener : public AsyncTask::Listener {
 public:
  CaptureStoreListener(id target) : target_(target) {}
  void HandleEvent(int msg_code, int msg_param, AsyncTask *source);
 private:
  id target_;
};

typedef struct CPP_resource_store {
  ResourceStore store_;
  std::deque<SFCaptureRequest*> pending_requests_;
  scoped_ptr<SFCaptureRequest> current_request_;
  scoped_ptr<CaptureTask> capture_task_;
  scoped_ptr<CaptureStoreListener> listener_;
} CPP_resource_store;

@interface GearsResourceStore : SafariGearsBaseClass {
 @protected
  CPP_resource_store *cpp_;  // Structure to hold c++ things (STRONG)
  
  // These are placeholders used to provice Javascript access to the underlying
  // keys in |store_|
  NSString *name_;
  NSString *requiredCookie_;
  BOOL enabled_;
}

//------------------------------------------------------------------------------
// Class
//------------------------------------------------------------------------------
+ (BOOL)hasStoreNamed:(NSString *)name cookie:(NSString *)cookie 
             security:(const SecurityOrigin *)security 
             existing:(int64 *)existing_store_id;

//------------------------------------------------------------------------------
// Public
//------------------------------------------------------------------------------
- (id)initWithName:(NSString *)name cookie:(NSString *)cookie 
         factory:(SafariGearsFactory *)factory
          existing:(int64)existing_store_id;

- (BOOL)removeStore;

- (ResourceStore *)resourceStore;

//------------------------------------------------------------------------------
// SafariGearsBaseClass
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings;
+ (NSDictionary *)webScriptKeys;

@end
