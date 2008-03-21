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

#include <algorithm>
#include <cctype>
#include <string>
#include "gears/base/common/security_model.h"
#include "gears/base/common/string_utils.h"
#include "gears/localserver/common/http_constants.h"

#if BROWSER_IE
#include <windows.h>
#include <wininet.h>
#elif BROWSER_FF
#include <gecko_sdk/include/nsIURI.h>
#include "gears/base/common/common.h"
#include "gears/base/firefox/dom_utils.h"
#elif BROWSER_SAFARI
#include "gears/base/safari/browser_utils.h"
#include "gears/base/safari/scoped_cf.h"
#include "gears/base/safari/cf_string_utils.h"
#elif BROWSER_NPAPI
#include "gears/third_party/googleurl/src/url_parse.h"
#endif


const char16* kUnknownDomain      = STRING16(L"_null_.localdomain");
const char*   kUnknownDomainAscii =           "_null_.localdomain";

//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
bool SecurityOrigin::Init(const char16 *full_url, const char16 *scheme,
                          const char16 *host, int port) {
  assert(full_url && scheme && host); // file URLs pass 0 for 'port'
  if (!full_url[0] || !scheme[0] || !host[0])
    return false;

  full_url_ = full_url;
  scheme_ = scheme;
  host_ = host;
  port_ = port;

  port_string_ = IntegerToString16(port_);
  LowerString(scheme_);
  LowerString(host_);
  
  url_ = scheme_;
  url_ += STRING16(L"://");
  url_ += host;
  if (!IsDefaultPort(scheme_, port_)) {
    url_ += STRING16(L":");
    url_ += port_string_;
  }

  initialized_ = true;
  return true;
}


//------------------------------------------------------------------------------
// InitFromUrl
//------------------------------------------------------------------------------
bool SecurityOrigin::InitFromUrl(const char16 *full_url) {
  initialized_ = false;

#if BROWSER_IE
  URL_COMPONENTSW components = {0};
  components.dwStructSize = sizeof(URL_COMPONENTSW);
  components.dwHostNameLength = 1;
  components.dwSchemeLength = 1;
  if (!InternetCrackUrlW(full_url, 0, 0, &components)) {
    return false;
  }

  // Disallow urls with embedded username:passwords. These
  // are disabled by default in IE such that InternetCrackUrl fails.
  // To have consistent behavior, do the same for all browsers.
  if (components.lpszUserName && components.dwUserNameLength) {
    return false;
  }
  switch (components.nScheme) {
    case INTERNET_SCHEME_HTTP:
    case INTERNET_SCHEME_HTTPS: {
      if (!components.lpszScheme || !components.lpszHostName) {
        return false;
      }
      std::string16 scheme(components.lpszScheme, components.dwSchemeLength);
      std::string16 host(components.lpszHostName, components.dwHostNameLength);
      return Init(full_url, scheme.c_str(), host.c_str(), components.nPort);
    }
    case INTERNET_SCHEME_FILE:
      return Init(full_url, HttpConstants::kFileScheme, kUnknownDomain, 0);
    default:
      return false;
  }

#elif BROWSER_FF
  nsCOMPtr<nsIURI> url_obj;
  if (!DOMUtils::NewAbsoluteURI(full_url, getter_AddRefs(url_obj)))
    return false;

  // Disallow urls with embedded username:passwords. These
  // are disabled by default in IE such that InternetCrackUrl fails.
  // To have consistent behavior, do the same for all browsers.
  nsCString user_name;
  url_obj->GetUsername(user_name);
  if (user_name.Length() > 0)
    return false;

  enum SchemeType { kSchemeTypeHttp, kSchemeTypeHttps, kSchemeTypeFile };
  const struct {
    SchemeType scheme_type;
    const char16 *scheme;
    const char *schemeAscii;
    int default_port;
  } kSchemes [] = {
    { kSchemeTypeHttp, HttpConstants::kHttpScheme,
      HttpConstants::kHttpSchemeAscii, HttpConstants::kHttpDefaultPort },
    { kSchemeTypeHttps, HttpConstants::kHttpsScheme,
      HttpConstants::kHttpsSchemeAscii, HttpConstants::kHttpsDefaultPort },
    { kSchemeTypeFile, HttpConstants::kFileScheme,
      HttpConstants::kFileSchemeAscii, HttpConstants::kFileDefaultPort }
  };
  nsresult nr = NS_OK;
  PRBool match = PR_FALSE;
  size_t i = 0;
  for (; i < ARRAYSIZE(kSchemes); ++i) {
    nr = url_obj->SchemeIs(kSchemes[i].schemeAscii, &match);
    if (NS_FAILED(nr)) { return false; }
    if (match)
      break;
  }
  if (!match) { return false; }
  switch (kSchemes[i].scheme_type) {
    case kSchemeTypeHttp:
    case kSchemeTypeHttps: {
      nsCString utf8;
      nr = url_obj->GetHost(utf8); // or GetAsciiHost to convert using IDNA spec
      if (NS_FAILED(nr)) { return false; }
      std::string16 host(NS_ConvertUTF8toUTF16(utf8).get());
      int port = -1;
      nr = url_obj->GetPort(&port);
      if (NS_FAILED(nr)) { return false; }
      if (port == -1) {
        // -1 implies the default port for the scheme
        port = kSchemes[i].default_port;
      }
      return Init(full_url, kSchemes[i].scheme, host.c_str(), port);
    }
    case kSchemeTypeFile:
      return Init(full_url, HttpConstants::kFileScheme, kUnknownDomain, 0);
    default:
      return false;
  }

#elif BROWSER_SAFARI
  scoped_CFString url_str(CFStringCreateWithString16(full_url));
  scoped_CFURL url(CFURLCreateWithString(kCFAllocatorDefault,
                                         url_str.get(), NULL));
  if (!url.get())
    return false;

  // Disallow urls with embedded username:passwords.
  // To have consistent behavior, do the same for all browsers.
  // CFURLCopyUserName() will return NULL if there is no user name.
  scoped_CFString user_name(CFURLCopyUserName(url.get()));
  if (user_name.get())
    return false;
  
  scoped_CFString scheme_str(CFURLCopyScheme(url.get()));
  if (!scheme_str.get())
    return false;

  std::string16 scheme;
  CFStringRefToString16(scheme_str.get(), &scheme);
  LowerString(scheme);

  if (scheme == HttpConstants::kHttpScheme ||
      scheme == HttpConstants::kHttpsScheme) {
    scoped_CFString host_str(CFURLCopyHostName(url.get()));
    std::string16 host;
    CFStringRefToString16(host_str.get(), &host);
    
    // CF's implementation is that if no port is specified, it returns -1
    // The expectation is that http defaults to 80 and https defaults to 443.
    int port = CFURLGetPortNumber(url.get());
    
    if (port < 0) {
      if (scheme == HttpConstants::kHttpsScheme)
        port = HttpConstants::kHttpsDefaultPort;
      else
        port = HttpConstants::kHttpDefaultPort;
    }

    return Init(full_url, scheme.c_str(), host.c_str(), port);
  } else if (scheme == HttpConstants::kFileScheme) {
    return Init(full_url, HttpConstants::kFileScheme, kUnknownDomain, 0);
  }
#elif BROWSER_NPAPI
  int url_len = wcslen(full_url);

  url_parse::Component scheme_comp;
  if (!url_parse::ExtractScheme(full_url, url_len, &scheme_comp)) {
    return false;
  }

  std::wstring scheme(full_url + scheme_comp.begin, scheme_comp.len);
  LowerString(scheme);
  if (scheme == STRING16(L"http") || scheme == STRING16(L"https")) {
    url_parse::Parsed parsed;
    url_parse::ParseStandardURL(full_url, url_len, &parsed);

    // Disallow urls with embedded username:passwords. These are disabled by
    // default in IE such that InternetCrackUrl fails. To have consistent
    // behavior, do the same for all browsers.
    if (parsed.username.len != -1 || parsed.password.len != -1) {
      return false;
    }

    int port = 80;
    if (parsed.port.len > 0) {
      std::string16 port_str(full_url + parsed.port.begin, parsed.port.len);
      port = ParseLeadingInteger(port_str.c_str(), NULL);
    }
    std::string16 host(full_url + parsed.host.begin, parsed.host.len);
    return Init(full_url, scheme.c_str(), host.c_str(), port);
  } else if (scheme == STRING16("file")) {
    return Init(full_url, HttpConstants::kFileScheme, kUnknownDomain, 0);
  }
#endif
  
  return false;
}
