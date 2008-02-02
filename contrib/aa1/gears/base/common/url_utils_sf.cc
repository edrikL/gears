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
#include "gears/base/safari/browser_utils.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/safari/scoped_cf.h"
#include "gears/base/safari/cf_string_utils.h"

bool ResolveAndNormalize(const char16 *base, const char16 *url,
                         std::string16 *out) {
  assert(url && out);
  static const std::string16 kQuestionMark(STRING16(L"?"));
  static const std::string16 kPathSeparator(STRING16(L"/"));
  static const std::string16 kColon(STRING16(L":"));
  static const std::string16 kAtSign(STRING16(L"@"));
  static const std::string16 kSchemeHostSeparator(STRING16(L"://"));
  static const std::string16 kRelativeParentPathSpecifier(STRING16(L"/.."));
  
  scoped_CFURL base_cfurl(NULL);
  if (base) {
    // Escape base URL.
    scoped_CFString base_cf(CFStringCreateWithString16(base));
    scoped_CFString base_unescaped_cf(
                        CFURLCreateStringByReplacingPercentEscapes(
                            kCFAllocatorDefault,
                            base_cf.get(),
                            CFSTR("")));  
    scoped_CFString base_escaped_cf(CFURLCreateStringByAddingPercentEscapes(
                                        kCFAllocatorDefault,
                                        base_unescaped_cf.get(),
                                        NULL,
                                        NULL,
                                        kCFStringEncodingUTF8));
    base_cfurl.reset(CFURLCreateWithString(kCFAllocatorDefault, 
                                           base_escaped_cf.get(), 
                                           NULL));
  }
  
  scoped_CFURL raw_resolved_url_cf(NULL);
  if (std::char_traits<char16>::length(url) == 0) {
    raw_resolved_url_cf.swap(base_cfurl);
  } else {
    std::string url_utf8;
    String16ToUTF8(url, &url_utf8);
    raw_resolved_url_cf.reset(
        CFURLCreateAbsoluteURLWithBytes(
            kCFAllocatorDefault,
            reinterpret_cast<const UInt8*>(url_utf8.c_str()),
            url_utf8.length(),
            kCFStringEncodingUTF8,
            base_cfurl.get(),
            false));
  }
  
  // Save a copy of the raw resolved URL before cracking.
  std::string16 raw_resolved_url;
  CFURLRefToString16(raw_resolved_url_cf.get(), &raw_resolved_url);
  
  std::string16 normalized_url;
  // Make sure we don't need to reallocate on every concatenation below.
  normalized_url.reserve(raw_resolved_url.length());
  
  if (!CFURLCanBeDecomposed(raw_resolved_url_cf.get())) {
    return false;
  }
  
  // Crack URL.
  scoped_CFString scheme_cf(CFURLCopyScheme(raw_resolved_url_cf.get()));
  scoped_CFString username_cf(CFURLCopyUserName(raw_resolved_url_cf.get()));
  scoped_CFString password_cf(CFURLCopyPassword(raw_resolved_url_cf.get()));
  scoped_CFString host_cf(CFURLCopyHostName(raw_resolved_url_cf.get()));
  SInt32 port = CFURLGetPortNumber(raw_resolved_url_cf.get());
  scoped_CFString path_cf(CFURLCopyPath(raw_resolved_url_cf.get()));
  scoped_CFString query_string_cf(
                      CFURLCopyQueryString(raw_resolved_url_cf.get(),
                      NULL));
  // Note that we don't tack the fragment identifier on the end of the URL.
  
  // Convert everything to string16 & build the url back up.
  std::string16 scheme;
  CFStringRefToString16(scheme_cf.get(), &scheme);
  normalized_url += MakeLowerString(scheme);
  
  normalized_url += kSchemeHostSeparator;
  
  std::string16 username;
  CFStringRefToString16(username_cf.get(), &username);
  std::string16 password;
  CFStringRefToString16(password_cf.get(), &password);
  bool has_username = !username.empty();
  bool has_password = !password.empty();
  
  if (has_username) {
    normalized_url += username;
  }
  
  if (has_password) {
    normalized_url += kColon;
    normalized_url += password;
  }
  
  if (has_username || has_password) {
    normalized_url += kAtSign;
  }
  
  std::string16 host;
  CFStringRefToString16(host_cf.get(), &host);
  normalized_url += MakeLowerString(host);
  
  // port is -1 if no port is specified in the input URL.
  // Tack on port only if it's not the default for said scheme.
  if (port != -1 && !IsDefaultPort(scheme, port)) {
    std::string16 port_str = IntegerToString16(port);
    normalized_url += kColon + port_str;
  }
  
  // Now deal with the path portion of the URL.
  // The CF APIs don't truncate '..'s at the beginning of the URL, so we need
  // to handle this ourselves.
  std::string16 path;
  CFStringRefToString16(path_cf.get(), &path);
  ReplaceAll(path, kRelativeParentPathSpecifier, std::string16());

  if (path.empty()) {
    // Always add '/', even if no path specified.
    normalized_url += kPathSeparator;
  } else {
    normalized_url += path;
  }
  
  std::string16 query_string;
  CFStringRefToString16(query_string_cf.get(), &query_string);
  if (!query_string.empty()) {
    normalized_url += kQuestionMark + query_string;
  } else if(EndsWith(raw_resolved_url, kQuestionMark)) {
    // Address corner case where input URL ends with ?, the CF APIs
    // have no way of reporting this.  But the desired behavior is to 
    // preserve an ending question mark on input, so we need to tack it
    // on ourselves here.
    normalized_url += kQuestionMark;
  }
  
  *out = normalized_url;
  return true;
}
