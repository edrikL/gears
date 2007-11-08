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
//
// The methods of the File class with a Linux-specific implementation.

// TODO(cprince): remove platform-specific #ifdef guards when OS-specific
// sources (e.g. LINUX_CPPSRCS) are implemented
#if defined(LINUX) && !defined(OS_MACOSX)
#include "gears/base/common/file.h"
#include "gears/base/common/int_types.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/localserver/common/http_constants.h"

bool File::CreateDesktopShortcut(BrowserType /*browser_type*/,
                                 const char16 *link_name,
                                 const SecurityOrigin *security_origin,
                                 const char16 *launch_url,
                                 const char16 *icon_url) {
  std::string link_name_utf8;

  if (security_origin == NULL || link_name == NULL) {
    return false;
  }
  if (launch_url == NULL || !IsRelativeUrl(launch_url)) {
    return false;
  }
  if (icon_url == NULL || !IsRelativeUrl(icon_url)) {
    return false;
  }
  if (!IsStringValidPathComponent(link_name)) {
    return false;
  }

  std::string16 full_launch_url(security_origin->url());
  if (launch_url[0] != L'/') {
    full_launch_url += STRING16(L"/");
  }
  full_launch_url += launch_url;

  std::string16 link_name_str16(link_name);

  String16ToUTF8(link_name_str16.c_str(), link_name_str16.size(),
                 &link_name_utf8);
  if (link_name_utf8.size() == 0) {
    return false;
  }

  std::string shortcut_path(getenv("HOME"));
  shortcut_path += "/Desktop/";
  shortcut_path += link_name_utf8;
  shortcut_path += ".desktop";

  FILE *f = fopen(shortcut_path.c_str(), "w");
  if (f != NULL) {
    // See http://standards.freedesktop.org/desktop-entry-spec/latest/ for
    // format documentation.
    //
    // TODO: this whole approach needs to change to create a browser-specific
    // shortcut instead of the standard URL desktop entry.
    std::string16 shortcut_contents(
        STRING16(L"[Desktop Entry]\nType=Link\nVersion=1.0"));
    shortcut_contents += STRING16(L"\nName=");
    shortcut_contents += link_name;
    shortcut_contents += STRING16(L"\nURL=");
    shortcut_contents += full_launch_url;
    shortcut_contents += STRING16(L"\n");

    std::string shortcut_contents_utf8;
    String16ToUTF8(shortcut_contents.c_str(), shortcut_contents.size(),
                   &shortcut_contents_utf8);
    if (shortcut_contents_utf8.size() > 0) {
      fputs(shortcut_contents_utf8.c_str(), f);
    }
    fclose(f);
    return true;
  } else {
    return false;
  }
}
#endif  // #if defined(LINUX) && !defined(OS_MACOSX)
