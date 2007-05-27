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

#import <unistd.h>

#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>

#import "gears/base/common/product_version.h"
#import "gears/base/safari/factory_utils.h"
#import "gears/base/safari/factory.h"
#import "gears/workerpool/safari/worker.h"
#import "gears/workerpool/safari/workerpool.h"
#import "gears/workerpool/safari/worker_supervisor.h"

NSString *kGearsWorkerMessageKey = @"message";
NSString *kGearsWorkerTargetKey = @"target";
NSString *kGearsWorkerSenderKey = @"sender";

// Time to wait before processing the queue
static const float kQueueProcessingDelay = 0.05;

// Time to wait before timing out
static const float kTimeoutDelay = 0.5;

@interface GearsWorkerPool(PrivateMethods)
- (NSString *)serverName;
- (void)installServer;
- (NSNumber *)createWorker:(NSString *)source;
- (GearsWorker *)workerWithIdentifier:(NSNumber *)identifier;
- (void)sendMessage:(NSString *)message target:(NSNumber *)targetID;
- (void)queueMessage:(NSDictionary *)message;
- (void)processQueue;
- (void)forceGC;
@end

@implementation GearsWorkerPool
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Private ||
//------------------------------------------------------------------------------
- (NSString *)serverName {
  return [NSString stringWithFormat:@"%sWorkerPool-%d-%@", 
    PRODUCT_SHORT_NAME_ASCII, getpid(), identifier_];
}

//------------------------------------------------------------------------------
- (void)installServer {
  NSMachPort *receive = (NSMachPort *)[NSMachPort port];
  Protocol *proto = @protocol(GearsWorkerPoolProtocol);
  NSProtocolChecker *checker =
    [NSProtocolChecker protocolCheckerWithTarget:self
                                        protocol:proto];

  serverConnection_ = [[NSConnection alloc] initWithReceivePort:receive 
                                                       sendPort:nil];
  [serverConnection_ setRootObject:checker];
  [serverConnection_ setDelegate:self];
  
  // We're willing to timeout in a much shorter time
  [serverConnection_ setRequestTimeout:kTimeoutDelay];
  [serverConnection_ setReplyTimeout:kTimeoutDelay];

  if (![serverConnection_ registerName:[self serverName]]) {
    MethodLog("Unable to register server: %@", [self serverName]);
    [serverConnection_ setRootObject:nil];
    [serverConnection_ setDelegate:nil];
    [serverConnection_ release];
    serverConnection_ = nil;
  }
}

//------------------------------------------------------------------------------
- (NSNumber *)createWorker:(NSString *)source {
  NSBundle *bundle = [NSBundle bundleForClass:[self class]];
  NSString *pluginPath = [bundle executablePath];
  NSString *exeDir = [pluginPath stringByDeletingLastPathComponent];
  NSString *path = [exeDir stringByAppendingPathComponent:@"GearsWorker.app"];
  NSString *exe = 
    [path stringByAppendingPathComponent:@"Contents/MacOS/GearsWorker"];
  NSNumber *identifier = [[factory_ supervisor] uniqueIdentifier];

  // Save the source to send after the thread registers
  if (!sources_)
    sources_ = [[NSMutableDictionary alloc] init];
  
  [sources_ setObject:source forKey:identifier];

  // Spawn off a new process with the identifier and server name as the 
  // arguments. The ThreadProcess will connect to the server and call
  // registerThread:
  const char *argv[4];
  
  argv[0] = [exe fileSystemRepresentation];
  argv[1] = [[identifier description] UTF8String];
  argv[2] = [[self serverName] UTF8String];
  argv[3] = NULL;

  pid_t pid = fork();

  // If we're in the child, load in our new executable and run.  The parent
  // is ignoring the children because the child will install an observer for
  // NSConnectionDidDieNotification and will exit if/when the parent exits.
  if (pid == 0) {
    // Invoke a new instance so that we'll no longer be the parent of the child
    // and so that it won't be a Zombie process since we're not going to 
    // wait() on the child.  jgm says that fork() / fork() is a common pattern
    // in Unix when you want to disassociate the child from the parent.
    pid_t newPid = fork();
    
    if (newPid == 0) {
      execv(argv[0], (char * const *)argv);
      _exit(errno);
    }

    _exit(errno);
  }
  
  return identifier;
}

//------------------------------------------------------------------------------
- (GearsWorker *)workerWithIdentifier:(NSNumber *)identifier {
  return [identifierMap_ objectForKey:identifier];
}

//------------------------------------------------------------------------------
- (void)sendMessage:(NSString *)message target:(NSNumber *)targetID {
  if (isDisconnected_)
    return;
  
  NSDictionary *messageDict = [NSDictionary dictionaryWithObjectsAndKeys:
    message, kGearsWorkerMessageKey, targetID, kGearsWorkerTargetKey,
    identifier_, kGearsWorkerSenderKey, nil];
  
  // If there's a message queue for this target, queue it here
  NSMutableArray *identifierQueue = [messageQueue_ objectForKey:targetID];
  
  if (identifierQueue) {
    [self queueMessage:messageDict];
    return;
  }
  
  [self processMessage:messageDict];
}

//------------------------------------------------------------------------------
- (void)queueMessage:(NSDictionary *)message {
  if (isDisconnected_)
    return;
  
  if (!messageQueue_)
    messageQueue_ = [[NSMutableDictionary alloc] init];
  
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  
  // Each message queue object will have an identifier and the mutable array
  // of pending messages.  The messages will be sent in order
  NSNumber *identifier = [message objectForKey:kGearsWorkerTargetKey];
  NSMutableArray *queueForIdentifier = [messageQueue_ objectForKey:identifier];
  
  if (!queueForIdentifier) {
    queueForIdentifier = [[NSMutableArray alloc] init];
    [messageQueue_ setObject:queueForIdentifier forKey:identifier];
    [queueForIdentifier release];
  }
  
  [queueForIdentifier addObject:message];
  [self performSelector:@selector(processQueue) withObject:nil 
             afterDelay:kQueueProcessingDelay];
}

//------------------------------------------------------------------------------
- (void)processQueue {
  cancelMessageQueueSends_ = NO;
  
  // If there are messages in the queue, process the the messages for each
  // identifier in order
  if ([messageQueue_ count]) {
    NSArray *identifiers = [messageQueue_ allKeys];
    unsigned int i, count = [identifiers count];

    // Loop over each identifier
    for (i = 0; (i < count) && (!cancelMessageQueueSends_); ++i) {
      NSNumber *identifier = [identifiers objectAtIndex:i];

      if ([self canProcessMessageToIdentifier:identifier]) {
        NSMutableArray *queueForIdentifier = 
          [messageQueue_ objectForKey:identifier];
        unsigned int j, messageCount = [queueForIdentifier count];
        
        for (j = 0; j < messageCount; ) {
          NSDictionary *message = [queueForIdentifier objectAtIndex:j];
          [self processMessage:message];
          
          // If there was a problem sending, cancel and reschedule later
          if (cancelMessageQueueSends_)
            break;

          [queueForIdentifier removeObjectAtIndex:j];
          messageCount = [queueForIdentifier count];
          
          // Only send one message at a time to a particular identifier.
          // This is a hack, but keeps us from hiting a timeout on the DO.
          // When worker pools become real threads, we'll have a much better 
          // implementation
          break;
        }
      }
    }
    
    // If we exhausted the objects for an identifier, remove it
    for (i = 0; (i < count); ++i) {
      NSNumber *identifier = [identifiers objectAtIndex:i];
      NSMutableArray *queueForIdentifier = 
        [messageQueue_ objectForKey:identifier];
      
      if (![queueForIdentifier count])
        [messageQueue_ removeObjectForKey:identifier];
    }
    
    // If there are still messages, schedule this to be performed again
    if ([messageQueue_ count])
      [self performSelector:@selector(processQueue) withObject:nil 
                 afterDelay:kQueueProcessingDelay];    
  }
}

//------------------------------------------------------------------------------
- (void)forceGC {
  // No-op
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Public ||
//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptSelectorStrings {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"sendMessage", @"sendMessage:target:",
    @"createWorker", @"createWorker:",
    @"forceGC", @"forceGC",
    nil];
}

//------------------------------------------------------------------------------
+ (NSDictionary *)webScriptKeys {
  return [NSDictionary dictionaryWithObjectsAndKeys:
    @"onmessage", @"onmessage_",
    nil];
}

//------------------------------------------------------------------------------
- (id)initWithFactory:(SafariGearsFactory *)factory {
  if ((self = [super initWithFactory:factory])) {
    GearsWorkerSupervisor *supervisor = [factory_ supervisor];
    identifier_ = [[supervisor superviseWorkerPool:self] retain];
    [self installServer];
  }
  
  return self;
}

//------------------------------------------------------------------------------
- (BOOL)canProcessMessageToIdentifier:(NSNumber *)identifier {
  if ([identifier_ isEqualToNumber:identifier])
    return YES;
  
  return [self workerWithIdentifier:identifier] ? YES : NO;
}

//------------------------------------------------------------------------------
- (NSNumber *)identifier {
  return identifier_;
}

//------------------------------------------------------------------------------
- (void)disconnect {
  // Skip if we're already disconnected
  if (!serverConnection_)
    return;
  
  isDisconnected_ = YES;
  [[factory_ supervisor] unsuperviseWorkerPool:self];
  factory_ = nil;
  [workers_ makeObjectsPerformSelector:@selector(cancel)];
  [workers_ release];
  workers_ = nil;
  [sources_ release];
  sources_ = nil;
  [identifierMap_ release];
  identifierMap_ = nil;
  [messageQueue_ release];
  messageQueue_ = nil;

  // Unroll the connection.  The oddity is that you need to invalidate the
  // receive and send ports manually for the name server to actually disconnect
  // rather than just calling [<NSConnection> invalidate].
  [[serverConnection_ receivePort] invalidate];
  [[serverConnection_ sendPort] invalidate];
  [serverConnection_ release];
  serverConnection_ = nil;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || GearsWorkerPoolProtocol ||
//------------------------------------------------------------------------------
- (void)registerWorker:(GearsWorker *)worker identifier:(NSNumber *)identifier {
  // Set the saved source for the thread
  [worker setSource:[sources_ objectForKey:identifier]];
  [sources_ removeObjectForKey:identifier];

  if (!identifierMap_)
    identifierMap_ = [[NSMutableDictionary alloc] init];

  // Keep the cache mapping identifier to worker
  [identifierMap_ setObject:worker forKey:identifier];
  
  // Keep track of the workers
  if (!workers_)
    workers_ = [[NSMutableSet alloc] init];
 
  [workers_ addObject:worker];  
}

//------------------------------------------------------------------------------
- (void)processMessage:(NSDictionary *)message {
  if (isDisconnected_)
    return;
  
  NSString *messageStr = [message objectForKey:kGearsWorkerMessageKey];
  NSNumber *target = [message objectForKey:kGearsWorkerTargetKey];
  NSNumber *sender = [message objectForKey:kGearsWorkerSenderKey];
  
  if ([target isEqualToNumber:identifier_]) {
    // The 0th element seems to be ignored when setting the arguments so
    // insert a placeholder
    [onmessage_ callWebScriptMethod:@"call" withArguments:
      [NSArray arrayWithObjects:@"placeholder", messageStr, sender, nil]];
  }
  else {
    GearsWorker *worker = [self workerWithIdentifier:target];

    // If the worker can't be found (e.g., is starting up), queue it and
    // try again later.  Otherwise, send now.
    if (!worker) {
      [self queueMessage:message];
    } else {
      @try {
        [worker processMessage:message];
      }
      @catch (id exception) {
        cancelMessageQueueSends_ = YES;
        [self queueMessage:message];
        MethodLog("Requeuing message %@ to worker %@ (%@)", messageStr, target, 
              exception);
      }
    }
  }
}

//------------------------------------------------------------------------------
- (NSDictionary *)arguments {
  return [factory_ arguments];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject (NSKeyValueCoding) ||
//------------------------------------------------------------------------------
- (void)setValue:(id)value forKey:(NSString *)key {
  if ([key isEqualToString:@"onmessage_"]) {
    [onmessage_ autorelease];
    onmessage_ = [value retain];
  }
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || NSObject ||
//------------------------------------------------------------------------------
- (void)dealloc {
  [self disconnect];
  [onmessage_ release];
  [identifier_ release];
  [super dealloc];
}

@end
