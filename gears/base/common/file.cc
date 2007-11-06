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
//
// The methods of the File class with a browser neutral implementation.
// Most methods implementations are browser specific and can be found
// in the file_xx.cc files.

#include <assert.h>
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/thread_locals.h"

const char16 *File::GetFileExtension(const char16 *filename) {
  assert(filename);
  size_t len = std::char_traits<char16>::length(filename);
  const char16* p = filename + (len - 1);
  while (p >= filename) {
    if (*p == kPathSeparator) {
      return filename + len; // Return the address of the trailing NULL
    }
    if (*p == '.') {
      return p;  // Return the address of the "."
    }
    --p;
  }
  return filename + len; // Return the address of the trailing NULL
}


// Support for storing and retrieving the last file error that occurred
// on a given thread. This is used to report better error messages.
// TODO(michaeln): LastError functions can be tricky to use correctly,
// consider something else later

const char16 *File::kCreateFileFailedMessage = STRING16(
                        L"Failed to create file");

static const std::string kLastFileErrorKey("base:LastFileError");

static void DeleteString16(void *thread_local_value) {
  std::string16 *error = reinterpret_cast<std::string16*>(thread_local_value);
  delete error;
}

void File::ClearLastFileError() {
  ThreadLocals::DestroyValue(kLastFileErrorKey);
}

bool File::GetLastFileError(std::string16 *error_out) {
  std::string16 *error = reinterpret_cast<std::string16*>(
                             ThreadLocals::GetValue(kLastFileErrorKey));
  if (error) {
    *error_out = *error;
    return true;
  } else {
    error_out->clear();
    return false;
  }
}

void File::SetLastFileError(const char16 *message,
                            const char16 *filepath,
                            int error_code) {
  std::string16 *value = new std::string16(message);
  (*value) += STRING16(L", '");
  (*value) += filepath;
  (*value) += STRING16(L"', error = ");
  (*value) += IntegerToString16(error_code);

  ThreadLocals::SetValue(kLastFileErrorKey, 
                         value,
                         &DeleteString16);
}


