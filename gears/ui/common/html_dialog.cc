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

#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/ui/common/html_dialog.h"


bool HtmlDialog::DoModal(const char16 *html_filename, int width, int height) {
  // The Json library deals only in UTF-8, so we need to convert :(.
  std::string16 input_string;
  if (!UTF8ToString16(arguments.toStyledString().c_str(), &input_string)) {
    return false;
  }

  return DoModalImpl(html_filename, width, height, input_string.c_str());
}

bool HtmlDialog::DoModeless(const char16 *html_filename, int width, int height,
                            ModelessCompletionCallback callback, 
                            void *closure) {
#ifdef BROWSER_WEBKIT
  // The Json library deals only in UTF-8, so we need to convert :(.
  std::string16 input_string;
  if (!UTF8ToString16(arguments.toStyledString().c_str(), &input_string)) {
    return false;
  }

  return DoModelessImpl(html_filename, width, height, input_string.c_str(),
                        callback, closure);
#else
  assert(false);
  return false;
#endif
}



bool HtmlDialog::SetResult(const char16 *value) {
  // NULL and empty are OK. They just means that the dialog did not set a
  // result.
  if (value == NULL || (*value) == L'\0') {
    result = Json::Value(Json::nullValue);
    return true;
  }

  std::string result_string;
  if (!String16ToUTF8(value, &result_string)) {
    return false;
  }

  Json::Reader reader;
  if (!reader.parse(result_string, result)) {
    LOG(("Error parsing return value from dialog. Error was: %s", 
         reader.getFormatedErrorMessages().c_str()));
    return false;
  }

  return true;
}


// HtmlDialogImpl() is platform-specific
// TODO(aa): Change the platform-specific implementation to follow the pattern
// of base/common/file*.cc.
#ifdef BROWSER_FF
#include "gears/ui/firefox/html_dialog_ff.cc"
#elif BROWSER_IE
#include "gears/ui/ie/html_dialog_ie.cc"
#elif BROWSER_WEBKIT
// In the case of WebKit, DoModalImpl is declared in 
// gears/ui/safari/html_dialog.mm so do nothing here...
#elif OS_ANDROID
// For Android, DoModalImpl is declared in ui/android/html_dialog_android.cc
#elif BROWSER_NPAPI
bool HtmlDialog::DoModalImpl(const char16 *html_filename, int width, int height,
                             const char16 *arguments_string) {
  // TODO(mpcomplete): implement me.
  SetResult(STRING16(L"{\"allow\": true, \"permanently\": true}"));
  return true;
}
#else
bool HtmlDialog::DoModalImpl(const char16 *html_filename, int width, int height,
                             const char16 *arguments_string) {
  // TODO: implement for other browsers.
  assert(false);
  return false;
}
#endif
