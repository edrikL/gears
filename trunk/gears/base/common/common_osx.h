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


#ifndef GEARS_BASE_COMMON_COMMON_OSX_H__
#define GEARS_BASE_COMMON_COMMON_OSX_H__

#include "gears/base/common/string16.h"

// TODO: Move all generally applicable OS X code from common_sf.h here.

// Initialize an NSAutoReleasePool.
void *InitAutoReleasePool();

// Destroys an autorelease pool, passing in NULL is legal and is a no-op.
void DestroyAutoReleasePool(void *pool);

#ifdef DEBUG
#define LOG(a) OSXGearsLog a
#define LOG16(a) OSXGearsLog16 a
#else
#define LOG(a) 0
#define LOG16(a) 0
#endif

#if defined(__cplusplus)
extern "C" {
#endif
void OSXGearsLog(const char *fn, ...);
void OSXGearsLog16(const char16 *msg_utf16, ...);
#if defined(__cplusplus)
}
#endif


#endif  // GEARS_BASE_COMMON_COMMON_OSX_H__
