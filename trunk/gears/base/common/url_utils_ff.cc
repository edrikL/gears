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

#include <assert.h>
#include <nsIIOService.h>
#include <nsIURI.h>

#include "gears/base/common/url_utils.h"

#include "gears/base/firefox/dom_utils.h"


bool ResolveAndNormalize(const char16 *base, const char16 *url,
                         std::string16 *out) {
  assert(url && out);

  // manufacture a full URL
  nsCOMPtr<nsIURI> full_url;
  if (base) {
    if (!DOMUtils::NewResolvedURI(base, url, getter_AddRefs(full_url))) {
      return false;
    }
  } else {
    if (!DOMUtils::NewAbsoluteURI(url, getter_AddRefs(full_url))) {
      return false;
    }
  }

  // get the full url string
  nsCString full_utf8;
  nsresult nr = full_url->GetSpec(full_utf8);
  NS_ENSURE_SUCCESS(nr, false);

  // strip the fragment part of the url
  const char *start = full_utf8.get();
  const char *hash = strchr(start, '#');
  if (hash) {
    full_utf8.SetLength(hash - start);
  }

  // convert to string16
  nsString full_utf16;
  nr = NS_CStringToUTF16(full_utf8, NS_CSTRING_ENCODING_UTF8, full_utf16);
  NS_ENSURE_SUCCESS(nr, false);
  out->assign(full_utf16.get());
  return true;
}
