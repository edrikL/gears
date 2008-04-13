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

#import "gears/base/common/string_utils.h"
#import "gears/base/safari/cf_string_utils.h"
#import "gears/localserver/common/http_constants.h"
#import "gears/localserver/common/localserver_db.h"
#import "gears/localserver/safari/localserver_db_proxy.h"

@implementation GearsWebCacheDB
//------------------------------------------------------------------------------
+ (BOOL)canService:(NSURL *)url {
  NSString *urlStr = [url absoluteString];
  std::string16 url16;
  
  if ([urlStr string16:&url16]) {
    WebCacheDB *db = WebCacheDB::GetDB();
    
    if (db && db->CanService(url16.c_str()))
      return YES;
  }
  
  return NO;
}

//------------------------------------------------------------------------------
+ (NSData *)service:(NSURL *)url mimeType:(NSString **)mimeType          
         statusCode:(int *)statusCode redirectURL:(NSURL **)redirectURL {
  std::string16 url16;
  
  assert(url);
  assert(mimeType);
  assert(statusCode);
  assert(redirectURL);
  
  if (![[url absoluteString] string16:&url16])
    return nil;
  
  WebCacheDB *db = WebCacheDB::GetDB();
  WebCacheDB::PayloadInfo payload;
  
  if (!db->Service(url16.c_str(), false, &payload))
    return nil;

  // The requested url may redirect to another location
  std::string16 redirect_url;
  if (payload.IsHttpRedirect()) {
    // We collapse a chain of redirects and hop directly to the final 
    // location for which we have a cache entry
    while (payload.IsHttpRedirect()) {
      if (!payload.GetHeader(HttpConstants::kLocationHeader, &redirect_url))
        return nil;

      // Fetch a response for redirect_url from our DB
      if (!db->Service(redirect_url.c_str(), false, &payload))
        return nil;
    }
    
    *redirectURL = [NSURL URLWithString:
      [NSString stringWithString16:redirect_url.c_str()]];
  } else {
    *redirectURL = nil;
  }
  
  std::string16 header16;
  
  if (payload.GetHeader(HttpConstants::kContentTypeHeader, &header16))
    *mimeType = [NSString stringWithString16:header16.c_str()];
  else
    *mimeType = nil;
 
  *statusCode = payload.status_code;
  
  // Copy the data
  std::vector<unsigned char> *dataVec = payload.data.get();
  const unsigned char *bytes = dataVec ? &(*dataVec)[0] : NULL;
  unsigned int length = dataVec ? dataVec->size() : 0;
  
  return [NSData dataWithBytes:bytes length:length];
}

@end
