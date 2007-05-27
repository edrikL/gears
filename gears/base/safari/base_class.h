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

// Components that will be loaded by the Gears Factory will conform to the
// GearsComponent protocol. There is also a base class GearsComponent that
// can be subclassed to provide convenient interaction 

// TODO(waylonis): Rename to GearsBaseClass to match with IE/FF implementations.

#import <Cocoa/Cocoa.h>

@class SafariGearsFactory;
@class WebScriptObject;
class SecurityOrigin;

//------------------------------------------------------------------------------
@protocol GearsComponent
// The dictionary of exposed selector strings and their web kit names
+ (NSDictionary *)webScriptSelectorStrings;

// The dictionary of exposed key names and their web kit names
+ (NSDictionary *)webScriptKeys;
@end

@interface GearsComponent : NSObject <GearsComponent> {
 @protected
  SafariGearsFactory *factory_;     // The creator (STRONG)
  SecurityOrigin *envPageOrigin_;   // The security origin (STRONG)
  BOOL isWorker_;                   // YES, if this is a worker pool instance
}

//------------------------------------------------------------------------------
// Class
//------------------------------------------------------------------------------
// Convert |array| into an NSArray.  If |array| is nil or WebUndefined, return
// nil.  If |array| is not a WebScriptObject (e.g., string or number), return
// [NSNull null] to indicate there was a conversion error.
+ (NSArray *)convertWebScriptArray:(WebScriptObject *)array;

//------------------------------------------------------------------------------
// GearsComponent (Protocol)
//------------------------------------------------------------------------------
// |factory| is the SafariGearsFactory that created this component instance.
- (id)initWithFactory:(SafariGearsFactory *)factory;

//------------------------------------------------------------------------------
// NSObject (WebScripting)
//------------------------------------------------------------------------------
+ (BOOL)isSelectorExcludedFromWebScript:(SEL)sel;
+ (NSString *)webScriptNameForSelector:(SEL)sel;

+ (BOOL)isKeyExcludedFromWebScript:(const char *)property;
+ (NSString *)webScriptNameForKey:(const char *)key;

@end
