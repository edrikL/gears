// Copyright 2006, Google Inc.
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
// Common definitions for shared code between the Firefox and IE implementations
// of Gears.

#ifndef GEARS_BASE_COMMON_COMMON_H__
#define GEARS_BASE_COMMON_COMMON_H__

#include "common/genfiles/product_constants.h"  // from OUTDIR
#include "gears/base/common/int_types.h"
#include "gears/base/common/string16.h"

// TODO(michaeln): would be nice to be able to include base/basictypes.h
// here rather than cherry pick and duplicate code, resolve issues with
// doing so.

// ARRAYSIZE performs essentially the same calculation as arraysize,
// but can be used on anonymous types or types defined inside
// functions.  It's less safe than arraysize as it accepts some
// (although not all) pointers.  Therefore, you should use arraysize
// whenever possible.
//
// The expression ARRAYSIZE(a) is a compile-time constant of type
// size_t.
//
// ARRAYSIZE catches a few type errors.  If you see a compiler error
//
//   "warning: division by zero in ..."
//
// when using ARRAYSIZE, you are (wrongfully) giving it a pointer.
// You should only use ARRAYSIZE on statically allocated arrays.
//
// The following comments are on the implementation details, and can
// be ignored by the users.
//
// ARRAYSIZE(arr) works by inspecting sizeof(arr) (the # of bytes in
// the array) and sizeof(*(arr)) (the # of bytes in one array
// element).  If the former is divisible by the latter, perhaps arr is
// indeed an array, in which case the division result is the # of
// elements in the array.  Otherwise, arr cannot possibly be an array,
// and we generate a compiler error to prevent the code from
// compiling.
//
// Since the size of bool is implementation-defined, we need to cast
// !(sizeof(a) & sizeof(*(a))) to size_t in order to ensure the final
// result has type size_t.
//
// This macro is not perfect as it wrongfully accepts certain
// pointers, namely where the pointer size is divisible by the pointee
// size.  Since all our code has to go through a 32-bit compiler,
// where a pointer is 4 bytes, this means all pointers to a type whose
// size is 3 or greater than 4 will be (righteously) rejected.
//
// Kudos to Jorg Brown for this simple and elegant implementation.
//
// - wan 2005-11-16
// 
// Starting with Visual C++ 2005, ARRAYSIZE is defined in WinNT.h
#if defined(_MSC_VER) && _MSC_VER >= 1400
#include <windows.h>
#else
#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
   static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
#endif

// Defines private prototypes for copy constructor and assigment operator. Do
// not implement these methods.
#define DISALLOW_EVIL_CONSTRUCTORS(CLASS) \
 private:                                 \
  CLASS(const CLASS&);                    \
  CLASS& operator=(const CLASS&)


// Macros for returning success/failure from scripted objects.
//
// TODO(cprince): Move these to base_class.h when IE has such a file.
// This generic "common.h" should eventually go away.  Each file should
// explicitly #include what it really needs, instead of using this catch-all.
#if BROWSER_IE

#include "gears/base/common/common_ie.h"

#define RETURN_NORMAL()  return S_OK
#define RETURN_EXCEPTION(msg)  \
  {  \
    /* AtlReportError has char* and WCHAR* versions */ \
    return AtlReportError(CLSID_NULL, msg); \
  }

#elif BROWSER_FF

#include "gears/base/common/common_ff.h"

#define RETURN_NORMAL()  return NS_OK
#define RETURN_EXCEPTION(msg) \
  { \
    LOG(("Exception: %s", msg)); \
    return JsSetException(this, msg); \
  }

#elif BROWSER_NPAPI

#include "gears/base/common/common_np.h"

// TODO(mpcomplete): implement these
#define RETURN_NORMAL()  return true
#define RETURN_EXCEPTION(msg) \
{ \
    return false; \
}

#elif BROWSER_SAFARI

#include "gears/base/common/common_sf.h"

#define RETURN_NORMAL()  return 0
#define RETURN_EXCEPTION(msg) \
{ \
    /* TODO(waylonis): Return e.message to try/catch blocks in Safari. */ \
    NSLog(@"Exception: %s", msg); \
    return 1; \
}

#else
#error "common.h: BROWSER_?? not defined."
#endif


#endif // GEARS_BASE_COMMON_COMMON_H__
