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

// The main entry point for a process that implements the WorkerPool class
// "thread" for Gears

#import <stdlib.h>
#import <unistd.h>

#import "gears/workerpool/safari/worker.h"

static const NSTimeInterval kRunLoopTimeInterval = 0.5;

@interface NSObject(GearsWorkerInterface)
- (id)initWithIdentifier:(NSString *)ident poolName:(NSString *)poolName;
- (BOOL)shouldCancel;
- (NSString *)identifier;
@end

//------------------------------------------------------------------------------
static id CreateWorker(NSNumber *identifier, NSString *poolName) {
  Class workerClass = NSClassFromString(@"GearsWorker");
  return [[workerClass alloc] initWithIdentifier:identifier poolName:poolName];
}

//------------------------------------------------------------------------------
int main(int argc, const char *argv[]) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // argv[1]: unique identifier
  // argv[2]: worker pool
  if (argc != 3)
    exit(1);

  // Load the bundle that contains all of Gears
  if (![GearsLoader loadGearsBundle])
    exit(1);

  NSAutoreleasePool *loopPool = nil;
  id worker = nil;

  do {
    // Create an autorelease pool to purge each loop
    if (!loopPool)
      loopPool = [[NSAutoreleasePool alloc] init];

    // Loop for a little time
    [[NSRunLoop currentRunLoop] runUntilDate:
      [NSDate dateWithTimeIntervalSinceNow:kRunLoopTimeInterval]];

    if (!worker) {
      unsigned int identifierVal = strtol(argv[1], NULL, 10);
      NSNumber *identifier = [NSNumber numberWithUnsignedInt:identifierVal];
      NSString *poolName = [NSString stringWithUTF8String:argv[2]];
      worker = CreateWorker(identifier, poolName);
      [worker registerWithPool];
    }

    [loopPool release];
    loopPool = nil;
  }
  while (![worker shouldCancel]);
  
  // Cleanup
  [worker release];
  
  [pool release];
  return 0;
}