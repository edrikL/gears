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

#include "gears/ui/common/html_dialog.h"

#include <assert.h>
#include "gears/base/common/js_types.h"
#include "gears/base/opera/browsing_context_op.h"
#include "gears/base/opera/opera_utils.h"
#include "third_party/opera/opera_callback_api.h"


bool HtmlDialog::DoModalImpl(const char16 *html_filename,
                             int width,
                             int height,
                             const char16 *arguments_string) {
  OperaGearsApiInterface *opera_api = OperaUtils::GetBrowserApiForGears();
  assert(opera_api);

  // Opera uses the JsContext to root the dialog to the current document. So
  // browsing_context_ must be set (via the HtmlDialog constructor) for
  // the permissions dialog, but is not required for the settings dialog.
  JsContextPtr js_context = NULL;
  if (browsing_context_) {
    js_context =
        static_cast<OPBrowsingContext*>(browsing_context_)->GetJsContext();
    assert(js_context);
  }
  const unsigned short *result;
  opera_api->OpenDialog(js_context, html_filename, width, height,
                        arguments_string, &result);

  return SetResult(result);
}

bool HtmlDialog::DoModelessImpl(const char16 *html_filename,
                                int width,
                                int height,
                                const char16 *arguments_string,
                                ModelessCompletionCallback callback,
                                void *closure) {
  // Unused in Opera.
  assert(false);
  return false;
}

bool HtmlDialog::GetLocale(std::string16 *locale) {
#ifdef OS_WINCE
  return GetCurrentSystemLocale(locale);
#else
  // TODO(stighal@opera.com): Get the correct locale.
  assert(false);
  return false;
#endif
}
