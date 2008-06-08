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
#include "third_party/convert_utf/ConvertUTF.h"

#ifdef ANDROID
// Android is missing wcslen. This is just a wide character strlen.
#include <unicode/ustring.h>
size_t wcslen(const char16 *str) {
  return u_strlen(str);
}
#endif  // ANDROID

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

  if (len <= 0) {
    *out16 = STRING16(L"");
    return true;
  }

  const UTF8 *source_ptr = reinterpret_cast<const UTF8*>(in);
  const UTF8 *source_end_ptr = source_ptr + len;  // should point 'beyond last'

  // UTF16 string has at most as many 'characters' as UTF8 one.
  out16->resize(len);
  UTF16 *target_ptr = reinterpret_cast<UTF16*>(&(*out16)[0]);
  UTF16 *target_ptr_original = target_ptr;
  UTF16 *target_end_ptr = target_ptr + len;
  ConversionResult result = ConvertUTF8toUTF16(&source_ptr, source_end_ptr,
                                               &target_ptr, target_end_ptr,
                                               strictConversion);

  // Resize to be the size of the # of converted characters.
  // Note that stl strings always account for \0 end-of-line character
  // automatically, so no need to do "+1" here.
  out16->resize(result == conversionOK ? target_ptr - target_ptr_original : 0);

  return result == conversionOK;
}

//------------------------------------------------------------------------------
// String16ToUTF8
//------------------------------------------------------------------------------
bool String16ToUTF8(const char16 *in, int len, std::string *out8) {
  assert(in);
  assert(len >= 0);
  assert(out8);

  if (len <= 0) {
    *out8 = "";
    return true;
  }

  const UTF16 *source_ptr = reinterpret_cast<const UTF16*>(in);
  const UTF16 *source_end_ptr = source_ptr + len;  // should point 'beyond last'

  // UTF8 string has at most 4 times as many 'characters' as UTF16 one.
  if (len > INT_MAX / 4) {  // overflow check
    *out8 = "";
    return false;
  }
  int out_len = len * 4;

  out8->resize(out_len);
  UTF8 *target_ptr = reinterpret_cast<UTF8*>(&(*out8)[0]);
  UTF8 *target_ptr_original = target_ptr;
  UTF8 *target_end_ptr = target_ptr + out_len;
  ConversionResult result = ConvertUTF16toUTF8(&source_ptr, source_end_ptr,
                                               &target_ptr, target_end_ptr,
                                               strictConversion);

  // Resize to be the size of the # of converted characters.
  // Note that stl strings always account for \0 end-of-line character
  // automatically, so no need to do "+1" here.
  out8->resize(result == conversionOK ? target_ptr - target_ptr_original : 0);

  return result == conversionOK;
}


