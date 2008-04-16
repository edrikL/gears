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
//
// A Javascript worker object.  It uses Distributed Objects to communicate with
// the GearsWorkerPool (which implements the GearsWorkerPoolProtocol).  It is
// created in worker_process.mm using arguments specified on the command line
// by the calling WorkerPool.  The object will register with the WorkerPool and
// then wait for the source to be set and any incoming messages.  It will
// automatically cancel itself when the DO connection to the server breaks.  It
// uses WebKit as the Javascript interpreter.

#import <Foundation/Foundation.h>

@protocol GearsWorkerPoolProtocol;
@class SafariGearsFactory;
@class WebView;

// Defines the required "Worker" interface.  The intention is that the worker 
// will use DO in an asynchronous way for best performance.
@protocol GearsWorkerProtocol
// A unique number for each worker.  This will be used to direct the message
- (out bycopy NSNumber *)identifier;

// Specify the JS source for the worker to execute
- (oneway void)setSource:(in bycopy NSString *)source;

// A message sent to the worker to be processed
- (oneway void)processMessage:(in bycopy NSDictionary *)message;

// Indicate that the parent is done with the worker and it should cancel any
// processing
- (oneway void)cancel;
@end

// See Description above
@interface GearsWorker : NSObject <GearsWorkerProtocol> {
 @private
  SafariGearsFactory *factory_;         // The Factory (WEAK)
  id <GearsWorkerPoolProtocol> pool_;   // The worker pool (STRONG);
  WebView *view_;                       // Holds the webkit view (STRONG)
  NSNumber *identifier_;                // The unique identifier (STRONG)
  id onmessage_;                        // onmessage property (STRONG)
  BOOL shouldCancel_;                   // YES, if we should cancel
  NSString *source_;                    // The source for this worker (STRONG)
}

//------------------------------------------------------------------------------
// Public
//------------------------------------------------------------------------------
- (id)initWithIdentifier:(NSNumber *)identifier poolName:(NSString *)pool;

- (BOOL)shouldCancel;

- (void)registerWithPool;

@end
