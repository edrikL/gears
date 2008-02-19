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
//
// Scoped objects for the various Win32 handle types.
// Destructor implementation must be provided for each type.
//
// An earlier implementation defined its own SafeHandle class,
// instead of using shared scoped-container code.

#ifndef GEARS_BASE_COMMON_SCOPED_WIN32_HANDLES_H__
#define GEARS_BASE_COMMON_SCOPED_WIN32_HANDLES_H__

#include <wincrypt.h>
#include <windows.h>
#include <wininet.h>
#include "gears/base/common/scoped_token.h"


// SAFE_HANDLE: automatically call CloseHandle()
class CloseHandleFunctor {
 public:
  void operator()(HANDLE x) const {
    if (x != INVALID_HANDLE_VALUE) { ::CloseHandle(x); }
  }
};
typedef scoped_token<HANDLE, CloseHandleFunctor> SAFE_HANDLE;


// SAFE_MAPPEDVIEW: automatically call UnmapViewOfFile()
class UnmapViewOfFileFunctor {
 public:
  void operator()(LPVOID x) const {
    if (x != NULL) { ::UnmapViewOfFile(x); }
  }
};
typedef scoped_token<LPVOID, UnmapViewOfFileFunctor> SAFE_MAPPEDVIEW;


// SAFE_HINTERNET: automatically call InternetCloseHandle()
class InternetCloseHandleFunctor {
 public:
  void operator()(HINTERNET x) const {
    if (x != NULL) { ::InternetCloseHandle(x); }
  }
};
typedef scoped_token<HINTERNET, InternetCloseHandleFunctor> SAFE_HINTERNET;


// SAFE_HCRYPTPROV: automatically call CryptReleaseContext()
class CryptReleaseContextFunctor {
 public:
  void operator()(HCRYPTPROV x) const {
    if (x != NULL) { ::CryptReleaseContext(x, 0); }
  }
};
typedef scoped_token<HCRYPTPROV, CryptReleaseContextFunctor> SAFE_HCRYPTPROV;


// SAFE_HCRYPTKEY: automatically call CryptDestroyKey()
class CryptDestroyKeyFunctor {
 public:
  void operator()(HCRYPTKEY x) const {
    if (x != NULL) { ::CryptDestroyKey(x); }
  }
};
typedef scoped_token<HCRYPTKEY, CryptDestroyKeyFunctor> SAFE_HCRYPTKEY;


// SAFE_HCRYPTHASH: automatically call CryptDestroyHash()
class CryptDestroyHashFunctor {
 public:
  void operator()(HCRYPTHASH x) const {
    if (x != NULL) { ::CryptDestroyHash(x); }
  }
};
typedef scoped_token<HCRYPTHASH, CryptDestroyHashFunctor> SAFE_HCRYPTHASH;


#endif  // GEARS_BASE_COMMON_SCOPED_WIN32_HANDLES_H__
