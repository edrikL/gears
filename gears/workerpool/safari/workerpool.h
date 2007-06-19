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
// Implements the Google Gears WorkerPool class.  It will create a separate
// processes to execute Javascript.  It uses DistributedObjects to communicate
// with (and vice versa) the GearsWorker processes.  It relies on the 
// GearsWorkerSupervisor to hand out unique identifiers.

#import <Cocoa/Cocoa.h>

#import "gears/base/safari/base_class.h"

@class GearsWorker;

// Keys for processMessage: method             Object
extern NSString *kGearsWorkerMessageKey; // NSString, message 
extern NSString *kGearsWorkerTargetKey;  // NSNumber, GearsWorker identifier
extern NSString *kGearsWorkerSenderKey;  // NSNumber, GearsWorker identifier

// The WorkerPool allows Workers to register with an identifier. When a message
// is to be sent between Safari and workers, it will provide a conduit for those
// messages.  The message is packaged as a NSDictionary with the keys above
// defined.  Finally, the arguments will be the arguments passed into the Gears
// WebKit plugin for the current page.  They are provided to allow the Worker to
// initialize the GearsFactory with the proper permissions.
@protocol GearsWorkerPoolProtocol
// The |worker| will register itself and provide an |identifier| to be used
// when trying to communicate with it
- (oneway void)registerWorker:(in GearsWorker *)worker
                   identifier:(in bycopy NSNumber *)identifier;

// |message| will be dispatched by the WorkerPool.  Either to another worker
// or for the WorkerPool itself
- (oneway void)processMessage:(in bycopy NSDictionary *)message;

// The |arguments| that were specified by the creation of the web page.  Used
// when creating the GearsFactory object in the worker
- (out bycopy NSDictionary *)arguments;
@end

// See detailed descriptions above
@interface GearsWorkerPool : SafariGearsBaseClass <GearsWorkerPoolProtocol> {
 @private
  NSMutableSet *workers_;             // Workers owned by this manager (STRONG)
  NSNumber *identifier_;              // Unique identifier (STRONG)
  id onmessage_;                      // onmessage property (STRONG)
  NSConnection *serverConnection_;    // Distributed object connection (STRONG)
  NSMutableDictionary *sources_;      // Pending sources (STRONG)
  NSMutableDictionary *identifierMap_;// Map: identifier to Worker (STRONG)
  NSMutableDictionary *messageQueue_; // Pending Messages (STRONG)
  BOOL cancelMessageQueueSends_;      // YES, if we should cancel sending
  BOOL isDisconnected_;               // YES, if we're disconnected
}

//------------------------------------------------------------------------------
// Public
//------------------------------------------------------------------------------
// Return the Javascript exposed selectors and keys
+ (NSDictionary *)webScriptSelectorStrings;
+ (NSDictionary *)webScriptKeys;

- (id)initWithFactory:(SafariGearsFactory *)factory;

// identifier is the unique number provided by the GearsWorkerSupervisor to
// identify a WorkerPool or Safari
- (BOOL)canProcessMessageToIdentifier:(NSNumber *)identifier;
- (NSNumber *)identifier;

- (void)disconnect;

@end
