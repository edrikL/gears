// Copyright 2009, Google Inc.
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

#include "gears/workerpool/location.h"

DECLARE_DISPATCHER(GearsLocation);

template <>
void Dispatcher<GearsLocation>::Init() {
  RegisterProperty("hash",     &GearsLocation::GetHash,     NULL);
  RegisterProperty("host",     &GearsLocation::GetHost,     NULL);
  RegisterProperty("hostname", &GearsLocation::GetHostname, NULL);
  RegisterProperty("href",     &GearsLocation::GetHref,     NULL);
  RegisterProperty("pathname", &GearsLocation::GetPathname, NULL);
  RegisterProperty("port",     &GearsLocation::GetPort,     NULL);
  RegisterProperty("protocol", &GearsLocation::GetProtocol, NULL);
  RegisterProperty("search",   &GearsLocation::GetSearch,   NULL);
}

const std::string GearsLocation::kModuleName("GearsLocation");

GearsLocation::GearsLocation()
    : ModuleImplBaseClass(kModuleName),
      parsed_is_valid_(false) {
}

GearsLocation::~GearsLocation() {
}

void GearsLocation::GetComponent(
    JsCallContext *context,
    url_parse::Parsed::ComponentType component_type,
    bool host_port) {
  const std::string16 &url = module_environment_->security_origin_.full_url();
  if (!parsed_is_valid_) {
    // ParseStandardURL returns void -- it cannot fail.
    url_parse::ParseStandardURL(url.c_str(), url.length(), &parsed_);
    parsed_is_valid_ = true;
  }

  url_parse::Component component;
  switch (component_type) {
    case url_parse::Parsed::SCHEME:
      component = parsed_.scheme;
      break;
    case url_parse::Parsed::HOST:
      component = parsed_.host;
      break;
    case url_parse::Parsed::PORT:
      component = parsed_.port;
      break;
    case url_parse::Parsed::PATH:
      component = parsed_.path;
      break;
    case url_parse::Parsed::QUERY:
      component = parsed_.query;
      break;
    case url_parse::Parsed::REF:
      component = parsed_.ref;
      break;
    default:
      assert(false);
      return;
  }
  if (!component.is_valid()) {
    std::string16 empty;
    context->SetReturnValue(JSPARAM_STRING16, &empty);
    return;
  }
  int begin = component.begin;
  int len = component.len;
  // The HTML5 spec (*) says that we should return "http:" instead of "http",
  // and similarly the search and hash terms should contain "?" and "#".
  // Furthermore, host_port shouldn't add the port part if the port is the
  // default port (i.e. 80 for HTTP, 443 for HTTPS).
  // (*) http://www.w3.org/TR/html5/infrastructure.html and also
  // RFC 3986 "Uniform Resource Identifier (URI): Generic Syntax".
  if (host_port) {
    if (parsed_.scheme.is_valid() && parsed_.port.is_valid()) {
      bool is_default_port = false;
      std::string16 scheme(url.substr(
          parsed_.scheme.begin, parsed_.scheme.len));
      std::string16 port(url.substr(parsed_.port.begin, parsed_.port.len));
      LowerString(scheme);
      if (scheme == STRING16(L"http") && port == STRING16(L"80")) {
        is_default_port = true;
      } else if (scheme == STRING16(L"https") && port == STRING16(L"443")) {
        is_default_port = true;
      }
      if (!is_default_port) {
        len = parsed_.port.end() - begin;
      }
    }
  } else if (component_type == url_parse::Parsed::SCHEME) {
    len++;
  } else if (component_type == url_parse::Parsed::QUERY ||
             component_type == url_parse::Parsed::REF) {
    begin--;
    len++;
  }
  std::string16 result(url.substr(begin, len));
  context->SetReturnValue(JSPARAM_STRING16, &result);
}

void GearsLocation::GetHash(JsCallContext *context) {
  GetComponent(context, url_parse::Parsed::REF, false);
}

void GearsLocation::GetHost(JsCallContext *context) {
  GetComponent(context, url_parse::Parsed::HOST, true);
}

void GearsLocation::GetHostname(JsCallContext *context) {
  GetComponent(context, url_parse::Parsed::HOST, false);
}

void GearsLocation::GetHref(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16,
      &module_environment_->security_origin_.full_url());
}

void GearsLocation::GetPathname(JsCallContext *context) {
  GetComponent(context, url_parse::Parsed::PATH, false);
}

void GearsLocation::GetPort(JsCallContext *context) {
  GetComponent(context, url_parse::Parsed::PORT, false);
}

void GearsLocation::GetProtocol(JsCallContext *context) {
  GetComponent(context, url_parse::Parsed::SCHEME, false);
}

void GearsLocation::GetSearch(JsCallContext *context) {
  GetComponent(context, url_parse::Parsed::QUERY, false);
}
