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

// Common definitions and functions for Gears Safari.

#ifndef GEARS_BASE_COMMON_COMMON_SF_H__
#define GEARS_BASE_COMMON_COMMON_SF_H__

#include <CoreFoundation/CoreFoundation.h>

#include "genfiles/product_constants.h"
#include "gears/base/common/basictypes.h"
#include "gears/base/safari/cf_string_utils.h"

struct NPObject;

// Stupid MacTypes defines this as 0
#undef kInvalidID

#if defined(__cplusplus)
extern "C" {
#endif
void SafariGearsLog(const char *fn, ...);
  
#ifdef __OBJC__

// NSInteger is a new type defined in Leopard, add these typedefs so code
// using it will work with pre-10.5 SDKs.
#if __LP64__ || NS_BUILD_32_LIKE_64
typedef long NSInteger;
typedef unsigned long NSUInteger;
#else
typedef int NSInteger;
typedef unsigned int NSUInteger;
#endif

NSString *StringWithLocalizedKey(NSString *key, ...);
void ThrowExceptionKey(NSString *key, ...);
#endif

#if defined(__cplusplus)
}
#endif

#ifdef DEBUG
#define LOG(a) SafariGearsLog a
#else
#define LOG(a) 0
#endif

// Wrappers for throwing localized vararg exceptions
#ifdef __OBJC__
#define ThrowExceptionKeyAndReturn(key, ...) \
  do { \
    ThrowExceptionKey(key, ##__VA_ARGS__); \
    return; \
  } while(0)

#define ThrowExceptionKeyAndReturnNil(key, ...) \
  do { \
    ThrowExceptionKey(key, ##__VA_ARGS__); \
    return nil; \
  } while(0)

#define ThrowExceptionKeyAndReturnNo(key, ...) \
  do { \
    ThrowExceptionKey(key, ##__VA_ARGS__); \
    return NO; \
  } while(0)

#endif  // __OBJC__

// Throw exception via WebKit's WebScriptObject interface.
// We need this to work around http://bugs.webkit.org/show_bug.cgi?id=16829
void WebKitNPN_SetException(NPObject* obj, const char *message);

// Initialize an NSAutoReleasePool.
void *InitAutoReleasePool();

// Destroys an autoreleas pool, passing in NULL is legal and is a no-op.
void DestroyAutoReleasePool(void *pool);

// Debug only code to help us assert that class methods are restricted to a
// single thread.  To use, add a DECL_SINGLE_THREAD to your class declaration.
// Then, add ASSERT_SINGLE_THREAD() calls to the top of each class method.
#if defined(__cplusplus)
#ifdef DEBUG

class CurrentThreadID {
 public:
  CurrentThreadID() {
    id_ = pthread_self();
  }
  pthread_t get() {
    return id_;
  }
 private:
  pthread_t id_;
};

#define DECL_SINGLE_THREAD \
  CurrentThreadID current_thread_id_;

#define ASSERT_SINGLE_THREAD() \
assert(pthread_equal(current_thread_id_.get(), pthread_self()))
       
#else
#define DECL_SINGLE_THREAD
#define ASSERT_SINGLE_THREAD()
#endif  // DEBUG
#endif  // C++       

#endif  // GEARS_BASE_COMMON_COMMON_SF_H__
