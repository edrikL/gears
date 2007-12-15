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

// Implements the GearsFactory class as specified by the Gears API

class GearsFactory;
@class GearsWorkerSupervisor;

@interface SafariGearsFactory : NSObject {
 @private
  NSDictionary *arguments_;           // WebKit page arguments (STRONG)
  GearsFactory *factory_;             // C++ factory (STRONG)  
  GearsWorkerSupervisor *supervisor_; // The supervisor for the factory (STRONG)
}

//------------------------------------------------------------------------------
// Public
//------------------------------------------------------------------------------
// The |arguments| are the ones passed from WebKit to the plugin via the
// plugInViewWithArguments: method.  See base/safari/plugin.h
- (id)initWithArguments:(NSDictionary *)arguments;

- (void)setArguments:(NSDictionary *)arguments;
- (NSDictionary *)arguments;

- (GearsFactory *)gearsFactory;

- (GearsWorkerSupervisor *)supervisor;

- (NSString *)resolveURLString:(NSString *)urlString;

- (void)shutdown;

//------------------------------------------------------------------------------
// NSObject (WebScripting)
//------------------------------------------------------------------------------
+ (BOOL)isKeyExcludedFromWebScript:(const char *)key;
+ (NSString *)webScriptNameForKey:(const char *)key;

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)sel;
+ (NSString *)webScriptNameForSelector:(SEL)sel;

@end
