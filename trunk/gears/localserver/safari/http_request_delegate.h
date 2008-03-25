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

// Wrapper for an NSURLconnection - encapsulates as much of the Objective C
// code as possible, delegating everything else to an "owner" C++ object
// through which the rest of the Gears code interfaces
// This class relies on the HttpRequest "owner"  class to enforce data validity
// and invariants. 

#import <Cocoa/Cocoa.h>
#include "gears/base/common/string16.h"
#include "gears/localserver/safari/http_request_sf.h"

@interface HttpRequestDelegate : NSObject {
 @private
  SFHttpRequest *owner_;
  NSMutableURLRequest *request_;     // (strong)
  NSURLConnection *connection_;      // (strong)
  NSMutableData *received_data_;     // (strong)
  NSInteger status_code_;
  NSDictionary *header_dictionary_;  // (strong)
  CFStringEncoding data_encoding_;
}

#pragma mark Public Instance methods

- (id)initWithOwner:(SFHttpRequest *)owner;

- (bool)open:(const std::string16 &)full_url
      method:(const std::string16 &)method;

- (bool)send:(const std::string &)post_data
   userAgent:(const std::string16 &)user_agent
     headers:(const SFHttpRequest::HttpHeaderVector &)headers
     bypassBrowserCache:(bool)bypass_browser_cache;

// Abort the http request.
- (void)abort;

#pragma mark Public Instance methods -- Access Methods

// These methods should only be called after the connection is closed.
- (void)headers:(SFHttpRequest::HttpHeaderVector *)headers;
- (void)headerByName:(const std::string16 &)name 
               value:(std::string16 *)value;
- (void)statusCode:(int *)status;
- (void)statusText:(std::string16 *)status_line;
- (void)responseBytes:(std::vector<uint8> *)body;
- (bool)responseAsString:(std::string16 *)response;
@end
