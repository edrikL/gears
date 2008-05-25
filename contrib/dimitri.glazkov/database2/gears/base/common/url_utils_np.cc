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

#include <assert.h>

#include "gears/base/common/url_utils.h"

#include "gears/base/common/string_utils.h"
#include "gears/third_party/googleurl/src/gurl.h"

bool ResolveAndNormalize(const char16 *base, const char16 *url,
                         std::string16 *out) {
  assert(url && out);
  GURL gurl;
  if (base != NULL) {
    GURL base_gurl(base);
    if (!base_gurl.is_valid()) {
      return false;
    }
    gurl = base_gurl.Resolve(url);
  } else {
    gurl = GURL(url);
  }

  if (!gurl.is_valid()) {
    return false;
  }

  // strip the fragment part of the url
  GURL::Replacements strip_fragment;
  strip_fragment.SetRef("", url_parse::Component());
  gurl = gurl.ReplaceComponents(strip_fragment);

  if (!UTF8ToString16(gurl.spec().c_str(), gurl.spec().length(), out)) {
    return false;
  }

  return true;
}
