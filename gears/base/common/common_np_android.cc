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

// Out-of-line helper function to log including a file and line number.
// This avoids polluting the namespace with Android defines.

#include "gears/base/common/common_np.h"
// LOG unfortunately namespace clashes with the Android logging system.
#undef LOG_INNER
#undef LOG
#include "cutils/logd.h"

#include <stdarg.h>
#include <string.h>

#define kTempStringLength       1024

void android_log_helper(const char *tag,
                        const char *file,
                        unsigned line,
                        const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (line > 0) {
    // Prepend with file:line. This means printing the varargs into a
    // temporary string.
    char *s = new char[kTempStringLength];
    vsnprintf(s, kTempStringLength, fmt, ap);
    // No need to print the complete path. Find the last component.
    for (;;) {
      const char *slash = strchr(file, '/');
      if (!slash)
        break;
      file = slash + 1;
    }
    // Print file name, line number, thread ID, and message.
    __android_log_print(ANDROID_LOG_INFO,
                        tag,
                        "%s:%u (%d): %s",
                        file,
                        line,
                        gettid(),
                        s);
    delete[] s;
  } else {
    // Just print without the file:line prefix
    __android_log_vprint(ANDROID_LOG_INFO, tag, fmt, ap);
  }
  va_end(ap);
}
