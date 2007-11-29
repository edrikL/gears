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
// TODO(cprince): change ATLASSERT to DCHECK
#include "gears/base/ie/atl_headers.h"


#ifdef WINCE
// WinCE doesn't allow message-only windows (HWND_MESSAGE). Instead, create a
// pop-up window (doesn't require a parent) and don't make visible (default).
const HWND  kMessageOnlyWindowParent = NULL;
const DWORD kMessageOnlyWindowStyle  = WS_POPUP;
#else
const HWND  kMessageOnlyWindowParent = HWND_MESSAGE;
const DWORD kMessageOnlyWindowStyle  = NULL;
#endif

#if defined(WINCE) && defined(DEBUG)
// TODO(cprince): Remove this class as part of LOG() refactoring.
// Also note that LOG() calls should take string16, so the string conversion
// done below can go away at that time.
class GearsTrace {
 public:
  GearsTrace(const char* file_name, int line_no)
      : file_name_(file_name), line_no_(line_no) {}

  void operator() (const char* format, ...) const {
    va_list ptr; va_start(ptr, format);
    // convert the format string to wchar_t
    int in_len = strlen(format);
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, format, in_len , NULL, 0);
    if (wide_len > 0) {
      wchar_t* format_wide = new wchar_t[wide_len + 1];
      wide_len = MultiByteToWideChar(
         CP_UTF8, 0, format, in_len, format_wide, wide_len);
      if (wide_len > 0) {
        ATL::CTrace::s_trace.TraceV(
            file_name_, line_no_, atlTraceGeneral, 0, format_wide, ptr);
      }
      delete format_wide;
    }
    va_end(ptr);
  }
 private:
  GearsTrace& operator=(const GearsTrace& other);
  const char *const file_name_;
  const int line_no_;
};

#define LOG(args) GearsTrace(__FILE__, __LINE__) args
#else
#define LOG(args) ATLTRACE args
#endif

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
