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

// Scoped objects for the various CoreFoundation types.

#ifndef GEARS_BASE_SAFARI_SCOPED_CF_H__
#define GEARS_BASE_SAFARI_SCOPED_CF_H__

#include <CoreFoundation/CoreFoundation.h>
#define kInvalidID 0
#include <CoreServices/CoreServices.h>
#undef kInvalidID

#include "gears/base/common/scoped_token.h"

class ReleaseCFTypeFunctor {
 public:
  void operator()(CFTypeRef x) const {
    if (x != NULL) { CFRelease(x); }
  }
};

template<typename T>
class scoped_cftype : public scoped_token<T, ReleaseCFTypeFunctor> {
 public:
  explicit scoped_cftype(T value) :
      scoped_token<T, ReleaseCFTypeFunctor>(value) {
  }
};


class ReleaseHandleFunctor {
 public:
  void operator()(Handle x) const {
    if (x != NULL) { DisposeHandle(x); }
  }
};

typedef scoped_token<Handle, ReleaseHandleFunctor> scoped_Handle;


// TODO(aa): Replace usages of the below with scoped_cftype above?

// CFDictionaryRef
typedef scoped_token<CFDictionaryRef, ReleaseCFTypeFunctor> scoped_CFDictionary;

// CFHTTPMessageRef
typedef scoped_token<CFHTTPMessageRef, ReleaseCFTypeFunctor>
  scoped_CFHTTPMessage;

// CFMachPortRef
typedef scoped_token<CFMachPortRef, ReleaseCFTypeFunctor> scoped_CFMachPort;

// CFMessagePortRef
typedef scoped_token<CFMessagePortRef, ReleaseCFTypeFunctor>
  scoped_CFMessagePort;

// CFMutableDataRef
typedef scoped_token<CFMutableDataRef, ReleaseCFTypeFunctor>
  scoped_CFMutableData;

// CFMutableStringRef
typedef scoped_token<CFMutableStringRef, ReleaseCFTypeFunctor> 
  scoped_CFMutableString;

// CFReadStreamRef
typedef scoped_token<CFReadStreamRef, ReleaseCFTypeFunctor> scoped_CFReadStream;

// CFRunLoopSourceRef
typedef scoped_token<CFRunLoopSourceRef, ReleaseCFTypeFunctor> 
  scoped_CFRunLoopSource;

// CFStringRef
typedef scoped_token<CFStringRef, ReleaseCFTypeFunctor> scoped_CFString;

// CFURLRef
typedef scoped_token<CFURLRef, ReleaseCFTypeFunctor> scoped_CFURL;

// CFUUIDRef
typedef scoped_token<CFUUIDRef, ReleaseCFTypeFunctor> scoped_CFUUID;

#endif  // GEARS_BASE_SAFARI_SCOPED_CF_H__
