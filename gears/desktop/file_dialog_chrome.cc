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

#if BROWSER_CHROME

#include "gears/base/chrome/browsing_context_cr.h"
#include "gears/base/common/string_utils.h"
#include "gears/desktop/file_dialog_chrome.h"
#include "gears/ui/common/i18n_strings.h"

bool FileDialogChrome::BeginSelection(NativeWindowPtr parent,
                                      const FileDialog::Options& options,
                                      std::string16* error) {
  std::string16 default_filter;
  for (StringList::const_iterator it = options.filter.begin();
       it != options.filter.end();
       ++it) {
    // Handle extensions of the form ".foo".
    if (L'.' == (*it)[0]) {
      default_filter.append(L";*");
      default_filter.append(*it);
    }
  }

  std::string16 filter;
  filter.append(GetLocalString(RECOMMENDED_FILE_TYPES_STRING));
  filter.push_back('|');
  // Append everything but the first character, which is always ';'.
  filter.insert(filter.end(), default_filter.begin() + 1,
                default_filter.end());
  filter.push_back('|');

  // Always include an unrestricted filter that the user may select.
  // On Win32, *.* matches everything, even files with no extension.
  filter.append(GetLocalString(ALL_FILE_TYPES_STRING));
  filter.push_back('|');
  filter.append(STRING16(L"*.*"));
  filter.push_back('|');

  // Terminate the filter with an extra null.
  filter.push_back('|');

  CRBrowsingContext *cr_context =
      static_cast<CRBrowsingContext*>(browsing_context_);
  CPBrowsingContext cp_context = 0;
  if (cr_context) {
    cp_context = cr_context->context;
  } else {
    return false;
  }

  return OpenDialog(cp_context,
                    MULTIPLE_FILES == options.mode,
                    options.dialog_title.c_str(),
                    filter.c_str());
}

void FileDialogChrome::CancelSelection() {
}

void FileDialogChrome::DoUIAction(UIAction action) {
}

#endif  // BROWSER_CHROME
