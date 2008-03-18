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

#import <WebKit/WebKit.h>

#import "gears/factory/safari/factory.h"
#import "gears/workerpool/safari/worker.h"
#import "gears/workerpool/safari/worker_supervisor.h"
#import "gears/workerpool/safari/workerpool.h"

@interface GearsWorker(PrivateMethods)
- (void)sendMessage:(id)message target:(NSNumber *)targetID;
- (void)connectionDidDie:(NSNotification *)note;
- (NSString *)wrapperForSource:(NSString *)source;
- (void)loadSource:(NSString *)source;
- (void)applyPendingMessages;
- (void)forceGC;
@end

@implementation GearsWorker
//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || Public ||
//------------------------------------------------------------------------------
- (id)initWithIdentifier:(NSNumber *)identifier poolName:(NSString *)pool {
  if ((self = [super init])) {
    NSConnection *conn = [NSConnection connectionWithRegisteredName:pool 
                                                               host:nil];

    identifier_ = [identifier retain];
    pool_ = [[conn rootProxy] retain];
    
    if (pool_) {
      Protocol *proto = @protocol(GearsWorkerPoolProtocol);
      [(NSDistantObject *)pool_ setProtocolForProxy:proto];
      
      // Create a factory
      factory_ = [[SafariGearsFactory alloc] initWithArguments:
        [pool_ arguments]];
    }
    
    // Check for notification when the server dies
    [[NSNotificationCenter defaultCenter] addObserver:self 
      selector:@selector(connectionDidDie:) name:NSConnectionDidDieNotification
      object:conn];
  }
  
  return self;
}

//------------------------------------------------------------------------------
- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [onmessage_ release];
  [factory_ release];
  [view_ release];
  [identifier_ release];
  [source_ release];
  [super dealloc];
}

//------------------------------------------------------------------------------
- (BOOL)shouldCancel {
  return shouldCancel_;
}

//------------------------------------------------------------------------------
- (void)registerWithPool {
  [pool_ registerWorker:self identifier:identifier_];
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || GearsWorkerProtocol ||
//------------------------------------------------------------------------------
- (NSNumber *)identifier {
  return identifier_;
}

//------------------------------------------------------------------------------
- (void)setSource:(NSString *)source {
  if (!view_) {
    view_ = [[WebView alloc] initWithFrame:NSZeroRect];
    [view_ setFrameLoadDelegate:self];
    [view_ setUIDelegate:self];    
  }

  [self loadSource:source];
}

//------------------------------------------------------------------------------
- (void)processMessage:(NSDictionary *)message {
  if (shouldCancel_)
    return;
  
  NSString *messageStr = [message objectForKey:kGearsWorkerMessageKey];
  NSNumber *sender = [message objectForKey:kGearsWorkerSenderKey];
 
  // The 0th element seems to be ignored when setting the arguments so
  // insert a placeholder
  [onmessage_ callWebScriptMethod:@"call" withArguments:
    [NSArray arrayWithObjects:@"placeholder", messageStr, sender, nil]];
}

//------------------------------------------------------------------------------
- (void)cancel {
  shouldCancel_ = YES;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || WebScripting (informal protocol)
//------------------------------------------------------------------------------
+ (NSString *)webScriptNameForSelector:(SEL)sel {
  if (sel == @selector(sendMessage:target:))
    return @"sendMessage";
#if DEBUG
  if (sel == @selector(forceGC))
    return @"forceGC";
#endif
  
  return nil;
}

//------------------------------------------------------------------------------
+ (BOOL)isSelectorExcludedFromWebScript:(SEL)sel {
  if (sel == @selector(sendMessage:target:))
    return NO;
  
#if DEBUG
  if (sel == @selector(forceGC))
    return NO;
#endif

  return YES;
}

//------------------------------------------------------------------------------
+ (NSString *)webScriptNameForKey:(const char *)key {
  if (!strcmp(key, "onmessage_"))
    return @"onmessage";
  if (!strcmp(key, "identifier_"))
    return @"identifier";
  
  return nil;
}

//------------------------------------------------------------------------------
+ (BOOL)isKeyExcludedFromWebScript:(const char *)key {
  if (!strcmp(key, "onmessage_"))
    return NO;
  if (!strcmp(key, "identifier_"))
    return NO;
  
  return YES;
}

//------------------------------------------------------------------------------
#pragma mark -
#pragma mark || WebView (delegate) ||
//------------------------------------------------------------------------------
- (void)webView:(WebView *)sender 
  windowScriptObjectAvailable:(WebScriptObject *)obj {

  // Specify the key that JS can access for thread control
  [obj setValue:self forKey:@"gearsWorkerPool"];
  [obj setValue:factory_ forKey:@"gearsFactory"];
  
  // Insert some code that wraps up these values in the google namespace.
  // This is what happens in threads_utils.h but since we don't want to bring
  // in all of Gears for some strings, we'll duplicate them here.
  [obj evaluateWebScript:
    @"var google = {};"
    "google.gears = {};"
    "google.gears.factory = gearsFactory;"
    "google.gears.workerPool = gearsWorkerPool;"];
}

//------------------------------------------------------------------------------
// Not offically documented 
// http://lists.apple.com/archives/Webkitsdk-dev/2007/Jan/msg00015.html
- (void)webView:(WebView *)sender addMessageToConsole:(NSDictionary *)dict {
  NSString *messageStr = [dict objectForKey:@"message"];
  NSNumber *lineNumber = [dict objectForKey:@"lineNumber"];

  NSLog(@"JS Error: %@ (Line: %@)", messageStr, lineNumber);
  NSLog(@"Source: %@", source_);
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
#pragma mark || Private ||
//------------------------------------------------------------------------------
- (void)sendMessage:(id)message target:(NSNumber *)targetID {
  NSString *messageStr = message;
  
  // Ensure that we're sending a string.  This routine can be called from the
  // Javascript environment which may also pass along NSNumbers or other
  // objects.
  if (![message isKindOfClass:[NSString class]]) {
    if ([message respondsToSelector:@selector(stringRepresentation)])
      messageStr = [(WebScriptObject *)message stringRepresentation];
    else if ([message respondsToSelector:@selector(stringValue)])
      messageStr = [(NSNumber *)message stringValue];
    else
      MethodLog("Can't convert Msg: %@", NSStringFromClass([message class]));
  }
  
  NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
    messageStr, kGearsWorkerMessageKey, targetID, kGearsWorkerTargetKey,
    identifier_, kGearsWorkerSenderKey, nil];
  
  [pool_ processMessage:dict];
}

//------------------------------------------------------------------------------
- (void)connectionDidDie:(NSNotification *)note {
  [self cancel];
}

//------------------------------------------------------------------------------
- (NSString *)wrapperForSource:(NSString *)source {
  NSString *fmt = @"<html><body><script type=\"text/javascript\">\n"
  "%@\n</script></body></html>\n";
  
  return [NSString stringWithFormat:fmt, source];
}

//------------------------------------------------------------------------------
- (void)loadSource:(NSString *)source {
  if (source) {
    [[view_ mainFrame] loadHTMLString:[self wrapperForSource:source] 
                              baseURL:nil];
    source_ = [source copy];
  }
}

//------------------------------------------------------------------------------
- (void)forceGC {
  // This required by the API, but does nothing in this plugin
}

@end

