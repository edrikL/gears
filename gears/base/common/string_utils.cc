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

#include "gears/base/common/string_utils.h"

#include "gears/base/common/common.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

// TODO(mpcomplete): implement these.
#if BROWSER_NPAPI && defined(WIN32)
#define BROWSER_IE 1
#endif

#if BROWSER_IE
#include "gears/base/ie/atl_headers.h"
#elif BROWSER_SAFARI
#include "gears/base/safari/cf_string_utils.h"
#endif

//------------------------------------------------------------------------------
// Class templates that provides a case sensitive and insensitive memmatch
// implmentation for both char and char16.
//------------------------------------------------------------------------------

template<class CharT>
class CaseSensitiveFunctor {
public:
  inline CharT operator()(CharT c) const {
    return c;
  }
};

template<class CharT>
class CaseInSensitiveFunctor {
public:
  inline CharT operator()(CharT c) const {
    return std::tolower(c);
  }
};

template<class CharT, class CaseFunctorT>
class MemMatch {
public:
  static inline const CharT *memmatch(const CharT *phaystack, size_t haylen,
                                const CharT *pneedle, size_t neelen) {
    if (0 == neelen) {
      return phaystack; // even if haylen is 0
    }

    if (haylen < neelen) {
      return NULL;
    }

    if (phaystack == pneedle) {
      return phaystack;
    }

    const CharT *haystack = phaystack;
    const CharT *hayend = phaystack + haylen;
    const CharT *needlestart = pneedle;
    const CharT *needle = pneedle;
    const CharT *needleend = pneedle + neelen;

    CaseFunctorT caseFunc;

    for (; haystack < hayend; ++haystack) {
      CharT hay = caseFunc(*haystack);
      CharT nee = caseFunc(*needle);
      if (hay == nee) {
        if (++needle == needleend) {
          return (haystack + 1 - neelen);
        }
      } else if (needle != needlestart) {
        // must back up haystack in case a prefix matched (find "aab" in "aaab")
        haystack -= needle - needlestart; // for loop will advance one more
        needle = needlestart;
      }
    }
    return NULL;
  }
};


//------------------------------------------------------------------------------
// memmatch - char
//------------------------------------------------------------------------------
const char *memmatch(const char *haystack, size_t haylen,
                     const char *needle, size_t neelen,
                     bool case_sensitive) {
  if (case_sensitive)
    return MemMatch< char, CaseSensitiveFunctor<char> >
              ::memmatch(haystack, haylen, needle, neelen);
  else
    return MemMatch< char, CaseInSensitiveFunctor<char> >
              ::memmatch(haystack, haylen, needle, neelen);
}


//------------------------------------------------------------------------------
// memmatch - char16
//------------------------------------------------------------------------------
const char16 *memmatch(const char16 *haystack, size_t haylen,
                       const char16 *needle, size_t neelen,
                       bool case_sensitive) {
  if (case_sensitive)
    return MemMatch< char16, CaseSensitiveFunctor<char16> >
                ::memmatch(haystack, haylen, needle, neelen);
  else
    return MemMatch< char16, CaseInSensitiveFunctor<char16> >
                ::memmatch(haystack, haylen, needle, neelen);
}

//------------------------------------------------------------------------------
// UTF8ToString16
//------------------------------------------------------------------------------
bool UTF8ToString16(const char *in, int len, std::string16 *out16) {
  assert(in);
  assert(len >= 0);
  assert(out16);

  if (len == 0) {
    *out16 = STRING16(L"");
    return true;
  }
#if BROWSER_IE
  int out_len = MultiByteToWideChar(CP_UTF8, 0, in, len, NULL, 0);
  if (out_len <= 0)
    return false;
  scoped_array<char16> tmp(new char16[out_len + 1]);
  out_len = MultiByteToWideChar(CP_UTF8, 0, in, len, tmp.get(), out_len);
  if (out_len <= 0)
    return false;
  tmp[out_len] = 0;
  out16->assign(tmp.get());
  return true;
#elif BROWSER_FF
  nsCString ns_in;
  nsString ns_out;
  ns_in.Assign(in, len);
  nsresult rv = NS_CStringToUTF16(ns_in, NS_CSTRING_ENCODING_UTF8, ns_out);
  NS_ENSURE_SUCCESS(rv, false);
  out16->assign(ns_out.get());
  return true;
#elif BROWSER_SAFARI
  return ConvertToString16UsingEncoding(in, len, kCFStringEncodingUTF8, out16);
#endif
}

//------------------------------------------------------------------------------
// String16ToUTF8
//------------------------------------------------------------------------------
bool String16ToUTF8(const char16 *in, int len, std::string *out8) {
  assert(in);
  assert(len >= 0);
  assert(out8);

  if (len == 0) {
    *out8 = "";
    return true;
  }
#if BROWSER_IE
  int out_len = WideCharToMultiByte(CP_UTF8, 0, in, len, NULL, 0, NULL, NULL);
  if (out_len <= 0)
    return false;
  scoped_array<char> tmp(new char[out_len + 1]);
  out_len = WideCharToMultiByte(CP_UTF8, 0, in, len, tmp.get(), out_len, 
                                NULL, NULL);
  if (out_len <= 0) 
    return false;
  tmp[out_len] = 0;
  out8->assign(tmp.get());
  return true;
#elif BROWSER_FF
  nsString ns_in;
  nsCString ns_out;
  ns_in.Assign(in, len);
  nsresult rv = NS_UTF16ToCString(ns_in, NS_CSTRING_ENCODING_UTF8, ns_out);
  NS_ENSURE_SUCCESS(rv, false);
  out8->assign(ns_out.get());
  return true;
#elif BROWSER_SAFARI
  CFStringRef inStr = CFStringCreateWithBytes(NULL, (const UInt8 *)in,
                                              len * sizeof(UniChar), 
                                              kCFStringEncodingUTF16, false);
  CFIndex length = CFStringGetLength(inStr);
  
  if (!length) {
    CFRelease(inStr);
    return false;
  }

  CFIndex maxUTF8Length = 
    CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
  scoped_array<UInt8> buffer(new UInt8[maxUTF8Length + 1]);
  CFIndex actualUTF8Length;
  CFStringGetBytes(inStr, CFRangeMake(0, length), kCFStringEncodingUTF8, 0, 
                   false, buffer.get(), maxUTF8Length, &actualUTF8Length);
  buffer[actualUTF8Length] = 0;
  out8->assign((const char *)buffer.get());
  CFRelease(inStr);
  return true;
#endif
}

