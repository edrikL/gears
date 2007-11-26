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
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// TODO(mpcomplete): implement these.
#if BROWSER_NPAPI
#define BROWSER_IE 1
#endif

#if BROWSER_IE
#include "gears/base/ie/atl_headers.h"
#elif BROWSER_SAFARI
#include <CoreFoundation/CoreFoundation.h>
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
  CFStringRef inStr = CFStringCreateWithBytes(NULL, (const UInt8 *)in, len, 
                                              kCFStringEncodingUTF8, false);
  const UniChar *outStr = CFStringGetCharactersPtr(inStr);
  CFIndex length = CFStringGetLength(inStr);
  
  if (!length) {
    CFRelease(inStr);
    return false;
  }
  
  // If the outStr is empty, we'll have to convert in a slower way
  if (!outStr) {
    scoped_array<UniChar> buffer(new UniChar[length + 1]);
    CFStringGetCharacters(inStr, CFRangeMake(0, length), buffer.get());
    buffer[length] = 0;
    out16->assign(buffer.get());
  } else {
    out16->assign(outStr);
  }
  
  CFRelease(inStr);
  return true;
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


//------------------------------------------------------------------------------
// UTF8PathToUrl
//------------------------------------------------------------------------------
// This is based on net_GetURLSpecFromFile in Firefox.  See:
// http://lxr.mozilla.org/seamonkey/source/netwerk/base/src/nsURLHelperWin.cpp
std::string UTF8PathToUrl(const std::string &path, bool directory) {
  std::string result("file:///");

  result += path;

  // Normalize all backslashes to forward slashes.
  size_t loc = result.find('\\');
  while (loc != std::string::npos) {
    result.replace(loc, 1, 1, '/');
    loc = result.find('\\', loc + 1);
  }

  result = EscapeUrl(result, ESCAPE_DIRECTORY|ESCAPE_FORCED);

  // EscapeUrl doesn't escape semi-colons, so do that here.
  loc = result.find(';');
  while (loc != std::string::npos) {
    result.replace(loc, 1, "%3B");
    loc = result.find('\\', loc + 1);
  }

  // If this is a directory, we need to make sure a slash is at the end.
  if (directory && result.c_str()[result.length() - 1] != '/') {
    result += "/";
  }

  return result;
}

//------------------------------------------------------------------------------
// EscapeUrl
//------------------------------------------------------------------------------
// This is based on NS_EscapeURL from Firefox.
// See: http://lxr.mozilla.org/seamonkey/source/xpcom/io/nsEscape.cpp

static const unsigned char HEX_ESCAPE = '%';
static const int escape_chars[256] =
/*  0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F */
{
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 0x */
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 1x */
     0,1023,   0, 512,1023,   0,1023,1023,1023,1023,1023,1023,1023,1023, 953, 784,       /* 2x   !"#$%&'()*+,-./ */
  1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1008, 912,   0,1008,   0, 768,       /* 3x  0123456789:;<=>? */
  1008,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,       /* 4x  @ABCDEFGHIJKLMNO  */
  1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023, 896, 896, 896, 896,1023,       /* 5x  PQRSTUVWXYZ[\]^_ */
     0,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,       /* 6x  `abcdefghijklmno */
  1023,1023,1023,1023,1023,1023,1023,1023,1023,1023,1023, 896,1012, 896,1023,   0,       /* 7x  pqrstuvwxyz{|}~ */
     0   /* 8x  DEL               */
};

#define NO_NEED_ESC(C) (escape_chars[((unsigned int) (C))] & (flags))

//  Returns an escaped string.

std::string EscapeUrl(const std::string &source, unsigned int flags) {
  std::string result;
  unsigned int i = 0;
  static const char hex_chars[] = "0123456789ABCDEF";
  bool forced = (flags & ESCAPE_FORCED) != 0;
  bool ignore_non_ascii = (flags & ESCAPE_ONLYASCII) != 0;
  bool ignore_ascii = (flags & ESCAPE_ONLYNONASCII) != 0;
  bool colon = (flags & ESCAPE_COLON) != 0;

  char temp_buffer[100];
  unsigned int temp_buffer_pos = 0;

  bool previous_is_non_ascii = false;
  for (i = 0; i < source.length(); ++i) {
    unsigned char c = source.c_str()[i];

    // if the char has not to be escaped or whatever follows % is
    // a valid escaped string, just copy the char.
    //
    // Also the % will not be escaped until forced
    // See bugzilla bug 61269 for details why we changed this
    //
    // And, we will not escape non-ascii characters if requested.
    // On special request we will also escape the colon even when
    // not covered by the matrix.
    // ignoreAscii is not honored for control characters (C0 and DEL)
    //
    // And, we should escape the '|' character when it occurs after any
    // non-ASCII character as it may be part of a multi-byte character.
    if ((NO_NEED_ESC(c) || (c == HEX_ESCAPE && !forced)
         || (c > 0x7f && ignore_non_ascii)
         || (c > 0x1f && c < 0x7f && ignore_ascii))
        && !(c == ':' && colon)
        && !(previous_is_non_ascii && c == '|' && !ignore_non_ascii)) {
      temp_buffer[temp_buffer_pos] = c;
      ++temp_buffer_pos;
    }
    else {
      // Do the escape magic.
      temp_buffer[temp_buffer_pos] = HEX_ESCAPE;
      temp_buffer[temp_buffer_pos + 1] = hex_chars[c >> 4];  // high nibble
      temp_buffer[temp_buffer_pos + 2] = hex_chars[c & 0x0f];  // low nibble
      temp_buffer_pos += 3;
    }

    if (temp_buffer_pos >= sizeof(temp_buffer) - 4) {
      temp_buffer[temp_buffer_pos] = '\0';
      result += temp_buffer;
      temp_buffer_pos = 0;
    }

    previous_is_non_ascii = (c > 0x7f);
  }

  temp_buffer[temp_buffer_pos] = '\0';
  result += temp_buffer;

  return result;
}
