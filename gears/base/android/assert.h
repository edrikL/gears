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

#ifndef GEARS_BASE_ANDROID_ASSERT_H__
#define GEARS_BASE_ANDROID_ASSERT_H__

// This header exists to prevent bringing in the Android system
// assert.h, which uses fprintf() and abort(), neither of which help
// with debugging because stderr goes nowhere and abort() prevents
// tracebacks.
//
// Note that this has to compile in both C and C++ mode.

#if defined(DEBUG) && !defined(NDEBUG)

#ifdef assert
#error "Somebody already defined assert. Header include order is broken."
#endif

#if defined(__cplusplus)
extern "C"
#endif
void android_log_helper(const char *tag,
                        const char *file,
                        unsigned line,
                        const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

#define assert(cond) \
    do { \
      if(!( cond )) { \
        android_log_helper("Gears", \
                           __FILE__, \
                           __LINE__, \
                           "Assertion failed: %s\n", \
                           #cond ); \
        *(volatile int *) 0 = 0; \
      } \
    } while(0)

#else // !defined(DEBUG)

#define assert(cond) do { } while(0)

#endif // !defined(DEBUG)

#endif // GEARS_BASE_ANDROID_ASSERT_H__
