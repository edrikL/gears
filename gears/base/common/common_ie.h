// Copyright 2005, Google Inc.
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

#ifndef GEARS_BASE_COMMON_COMMON_IE_H__
#define GEARS_BASE_COMMON_COMMON_IE_H__

#include <windows.h>  // for DWORD
#include <assert.h>
#include <stdio.h>
#ifdef OS_WINCE
#include "gears/base/common/wince_compatibility.h"  // For GearsTrace
#endif
#include "gears/base/ie/atl_browser_headers.h"
// TODO(cprince): change ATLASSERT to DCHECK


#ifdef OS_WINCE
// Use of ATLTRACE (which is used by LOG and LOG16) may cause a stack fault on
// WinCE. See http://code.google.com/p/google-gears/issues/detail?id=342 for
// details. So we disable logging by default on WinCE.
#else
#define ENABLE_LOGGING
#endif

#if defined(DEBUG) && defined(ENABLE_LOGGING)

#if defined(OS_WINCE)
#define LOG(args) GearsTrace(__FILE__, __LINE__) args
#define LOG16(args) ATLTRACE args
#else  // defined(OS_WINCE)
// ATLTRACE for Win32 can take either a wide or narrow string.
#define LOG(args) ATLTRACE args
#define LOG16(args) ATLTRACE args
#endif  // defined(OS_WINCE)

#else  // defined(DEBUG) && defined(ENABLE_LOGGING)

#define LOG(args) __noop
#define LOG16(args) __noop

#endif  // defined(DEBUG) && defined(ENABLE_LOGGING)

// Debug only code to help us assert that class methods are restricted to a
// single thread.  To use, add a DECL_SINGLE_THREAD to your class declaration.
// Then, add ASSERT_SINGLE_THREAD() calls to the top of each class method.
#ifdef DEBUG

class CurrentThreadID {
 public:
  CurrentThreadID() {
    id_ = GetCurrentThreadId();
  }
  DWORD get() {
    return id_;
  }
 private:
  DWORD id_;
};

#define DECL_SINGLE_THREAD \
    CurrentThreadID thread_id_;

#define ASSERT_SINGLE_THREAD() \
    ATLASSERT(thread_id_.get() == GetCurrentThreadId())

#else
#define DECL_SINGLE_THREAD
#define ASSERT_SINGLE_THREAD()
#endif

#endif  // GEARS_BASE_COMMON_COMMON_IE_H__
