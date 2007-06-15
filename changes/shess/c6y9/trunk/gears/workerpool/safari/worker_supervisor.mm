// Copyright 2006, Google Inc.
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
#import "gears/workerpool/safari/workerpool.h"
#import "gears/workerpool/safari/worker_supervisor.h"

@interface GearsWorkerSupervisor(PrivateMethods)
- (void)factoryWillTerminate:(NSNotification *)note;
@end

@implementation GearsWorkerSupervisor
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Public ||
//------------------------------------------------------------------------------
- (id)init {
  if ((self = [super init])) {
    pools_ = [[NSMutableSet alloc] init];
  }
  
  return self;
}

//------------------------------------------------------------------------------
- (void)dealloc {
  // Ensure that the worker pools disconnect from connected worker processes
  [pools_ makeObjectsPerformSelector:@selector(disconnect)];
  [pools_ release];
  [super dealloc];
}

//------------------------------------------------------------------------------
- (NSNumber *)uniqueIdentifier {
  return [NSNumber numberWithUnsignedInt:++nextIdentifier_];
}

//------------------------------------------------------------------------------
- (NSNumber *)superviseWorkerPool:(GearsWorkerPool *)pool {
  if (!pool)
    return nil;
  
  [pools_ addObject:pool];
  
  return [self uniqueIdentifier];
}

//------------------------------------------------------------------------------
- (void)unsuperviseWorkerPool:(GearsWorkerPool *)pool {
  [pools_ removeObject:pool];
}

//------------------------------------------------------------------------------
- (GearsWorkerPool *)workerPoolContainingIdentifier:(NSNumber *)ident {
  NSArray *pools = [pools_ allObjects];
  int count = [pools count];
  
  for (int i = 0; i < count; ++i) {
    GearsWorkerPool *pool = [pools objectAtIndex:i];
    
    if ([pool canProcessMessageToIdentifier:ident])
      return pool;
  }
  
  return nil;
}

@end
