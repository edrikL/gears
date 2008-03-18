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

#include "gears/base/common/url_utils.h"


bool IsRelativeUrl(const char16 *url) {
  // From RFC 2396 (URI Generic Syntax):
  // * "Relative URI references are distinguished from absolute URI in that
  //    they do not begin with a scheme name."
  // * "Scheme names consist of a sequence of characters beginning with a
  //    lower case letter and followed by any combination of lower case
  //    letters, digits, plus ('+'), period ('.'), or hyphen ('-').  For
  //    resiliency, programs interpreting URI should treat upper case letters
  //    as equivalent to lower case in scheme names (e.g., allow 'HTTP' as
  //    well as 'http')."
  // The algorithm below does not support escaped characters.

  bool first_is_alpha = (url[0] >= 'a' && url[0] <= 'z') || 
                        (url[0] >= 'A' && url[0] <= 'Z');
  if (first_is_alpha) {
    int i = 1;
    while ((url[i] >= 'a' && url[i] <= 'z') || 
           (url[i] >= 'A' && url[i] <= 'Z') ||
           (url[i] >= '0' && url[i] <= '9') ||
           (url[i] == '+') || (url[i] == '.') || (url[i] == '-')) {
      ++i;
    }
    if (url[i] == ':') {
      return false;  // absolute URL
    }
  }
  return true;
}
