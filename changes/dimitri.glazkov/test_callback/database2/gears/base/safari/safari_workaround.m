// Copyright 2008, Google Inc.
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

// This file contains workarounds and documentation for issues with WebKit and
// OSX that affect Gears.

// Hopefully these issues will be resolved before launch.

// In the meantime, lets mark this whole file as temporary:
// SAFARI-TEMP

// OS X Bugs affecting Gears:

// -----------------------------------------------------------------------------
// rdar://5830581 - NSURLProtocol not called on redirect.
//  Our NSURLProtocol handler isn't called if a URL redirects to another URL
// in the Gears Cache.
// -----------------------------------------------------------------------------
// rdar://5817126 - NSURLProtocol can't return NSHTTPURLResponse under Leopard.
//  This is a regression in Leopard (works fine in Tiger) - If a NSURLProtocol
// returns a class other than NSURLResponse, the system casts it down to a plain
// NSURLResponse.  This means that we can't feed custom HTTP data to Safari.
//
// Workaround: Add allHeaderFields & statusCode selectors to all instances of
// NSURLResponse.

// WebKit Bugs affecting Gears:

// -----------------------------------------------------------------------------
// Bug 16829 - NPN_SetException() sets exception on the Global ExecState 
//             instead of local.
//   This means that we can't use NPN_SetException() to throw exceptions from
// our plugin.
//
// Workaround: use WebScriptObject's throwException: selector instead - see
// common_sf.mm.
// -----------------------------------------------------------------------------
// Bug 18234: JS exception thrown from NPN_InvokeDefault not shown in 
//            error console.
//   If we use NPN_InvokeDefault() to call a JS callback from Gears, and that
// callback throws an exception, the exception isn't bubbled up into the
// the originating JS code - it effectively gets lost.
// -----------------------------------------------------------------------------
// Bug 8519: WebCore doesn't fire window.onerror event.
//  WebKit doesn't support window.onerror(), this affects our unit tests which
// rely on window.onerror to catch failing unit tests.
//   This effectively means that a unit tests that fails with an exception must
// timeout before we continue.
// -----------------------------------------------------------------------------
// Bug 18333: NPAPI: No way of telling the difference between 'ints' and 
//           'doubles'.
//  NPVARIANT_IS_INT32() never returns True in WebKit, this means that we have
// no way of telling the difference between JS calling us with a numeric
// parameter of 42 or 42.0.
//  This bug affects our ability to stringify numbers.
//
// Workaround: We manually check if a double fits in the bounds of an int, and
// if so we manually report it's type as an int, see JsTokenGetType() in
// js_types.cc.

#import <Cocoa/Cocoa.h>

// Workaround for rdar://5817126
// In the meantime we return an empty set of headers and requests that always
// succeed.
@implementation NSURLResponse(GearsNSURLResponse)

- (NSDictionary *)allHeaderFields {
  return [[NSDictionary alloc] init];
}

- (int)statusCode {
  return 200;
}

@end
