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

#import "gears/base/safari/factory.h"
#import "gears/base/safari/plugin.h"

@implementation ScourPlugin
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Public ||
//------------------------------------------------------------------------------
- (id)initWithArguments:(NSDictionary *)arguments {
  if (self = [super initWithFrame:NSZeroRect]) {
    factory_ = [[GearsFactory alloc] initWithArguments:arguments];
  }
  
  return self;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || WebPlugInViewFactory (Protocol) ||
//------------------------------------------------------------------------------
+ (NSView *)plugInViewWithArguments:(NSDictionary *)arguments {
  return [[[ScourPlugin alloc] initWithArguments:arguments] autorelease];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject (WebPlugIn) ||
//------------------------------------------------------------------------------
- (id)objectForWebScript {
  return factory_;
}

//------------------------------------------------------------------------------
- (void)webPlugInDestroy {
  // WebKit will call this method just before page teardown or reload.  By
  // releasing the factory here, we can have greater control over how the
  // allocated components are cleaned up.  Specifically, the 
  // GearsWorkerSupervisor, which is listening for the notification that is 
  // posted by the factory (GearsFactoryWillTermiateNotification) before
  // releasing.

  [factory_ release];
  factory_ = NULL;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject ||
//------------------------------------------------------------------------------
- (void)dealloc {
  [factory_ release];
  [super dealloc];
}

@end
