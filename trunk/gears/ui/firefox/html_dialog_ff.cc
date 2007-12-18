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

#include <nsCOMPtr.h>
#include <nsISupportsPrimitives.h>
#include <nsIProperties.h>
#include "gecko_internal/nsIDOMWindowInternal.h"

#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/firefox/dom_utils.h"


// Helper function used by DoModalImpl(). Finds the active nsIDOMWindow and
// uses it to launch a modal dialog to a given html file, passing it the
// parameters given in the params argument.
static bool LaunchDialog(nsIProperties *params, const char16 *html_filename,
                         int width, int height) {
  // Build the URL to open in the dialog
  std::string16 html_url(STRING16(L"chrome://"
                                  PRODUCT_SHORT_NAME L"/content/"));
  html_url += html_filename;

  // Build the options string which tells Firefox various bits about how we
  // would like the dialog to display.
  std::string16 options(STRING16(
      L"chrome,centerscreen,modal,dialog,titlebar,resizable,"));
  options += STRING16(L"width=");
  options += IntegerToString16(width);
  options += STRING16(L",height=");
  options += IntegerToString16(height);

  // Get the browser window corresponding to the calling JS
  // NOTE: We assume that there is a js window somewhere on the stack.
  nsCOMPtr<nsIDOMWindowInternal> calling_window;
  DOMUtils::GetWindow(getter_AddRefs(calling_window));
  if (!calling_window) {
    return false;
  }

  // Use Firefox's built-in openDialog() method to actually open the modal
  // dialog. Args are:
  // - The URL to open
  // - An arbitrary name for the window
  // - Window options string
  // - An optional nsISupports instance to pass to the dialog as an argument
  nsCOMPtr<nsIDOMWindow> dialog_window;
  nsresult nr = calling_window->OpenDialog(
                    nsString(html_url.c_str()),
                    NS_LITERAL_STRING("html_dialog"),
                    nsString(options.c_str()),
                    params,
                    getter_AddRefs(dialog_window));
  if (NS_FAILED(nr)) {
    return false;
  }

  return true;
}


bool HtmlDialog::DoModalImpl(const char16 *html_filename, int width, int height,
                             const char16 *arguments_string) {
  // Build the params object to send into the dialog
  nsCOMPtr<nsISupportsString> input_supports(
    do_CreateInstance("@mozilla.org/supports-string;1"));
  nsCOMPtr<nsIProperties> params(
    do_CreateInstance("@mozilla.org/properties;1"));
  if (!input_supports || !params) {
    return false;
  }

  input_supports->SetData(nsString(arguments_string));
  params->Set("dialogArguments", input_supports);

  // Launch the dialog
  if (!LaunchDialog(params, html_filename, width, height)) {
    return false;
  }

  // Retrieve the dialog result
  nsString output_nsstring;
  nsCOMPtr<nsISupportsString> output_supports;
  nsresult nr = params->Get("dialogResult", NS_GET_IID(nsISupportsString),
    getter_AddRefs(output_supports));
  if (NS_SUCCEEDED(nr)) {
    nr = output_supports->GetData(output_nsstring);
    if (NS_FAILED(nr)) {
      return false;
    }
  }

  // Set up the result property
  return SetResult(output_nsstring.BeginReading());
}
