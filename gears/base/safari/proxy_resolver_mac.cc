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

#include "gears/base/safari/proxy_resolver_mac.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include </usr/include/curl/curl.h>
#include <vector>

#include "third_party/googleurl/src/gurl.h"
#include "gears/base/common/string_utils_osx.h"
#include "gears/base/safari/scoped_cf.h"

namespace {

// Convert the supplied CFString into the specified encoding, and return it as
// an STL string of the template type.  Returns an empty string on failure.
template<typename StringType>
StringType CFStringToSTLStringWithEncodingT(CFStringRef cfstring,
                                                   CFStringEncoding encoding) {
  CFIndex length = CFStringGetLength(cfstring);
  if (length == 0)
    return StringType();

  CFRange whole_string = CFRangeMake(0, length);
  CFIndex out_size;
  CFIndex converted = CFStringGetBytes(cfstring,
                                       whole_string,
                                       encoding,
                                       0,      // lossByte
                                       false,  // isExternalRepresentation
                                       NULL,   // buffer
                                       0,      // maxBufLen
                                       &out_size);
  if (converted == 0 || out_size == 0)
    return StringType();

  // out_size is the number of UInt8-sized units needed in the destination.
  // A buffer allocated as UInt8 units might not be properly aligned to
  // contain elements of StringType::value_type.  Use a container for the
  // proper value_type, and convert out_size by figuring the number of
  // value_type elements per UInt8.  Leave room for a NUL terminator.
  typename StringType::size_type elements =
      out_size * sizeof(UInt8) / sizeof(typename StringType::value_type) + 1;

  std::vector<typename StringType::value_type> out_buffer(elements);
  converted = CFStringGetBytes(cfstring,
                               whole_string,
                               encoding,
                               0,      // lossByte
                               false,  // isExternalRepresentation
                               reinterpret_cast<UInt8*>(&out_buffer[0]),
                               out_size,
                               NULL);  // usedBufLen
  if (converted == 0)
    return StringType();

  out_buffer[elements - 1] = '\0';
  return StringType(&out_buffer[0], elements - 1);
}

std::string SysCFStringRefToUTF8(CFStringRef ref) {
  return CFStringToSTLStringWithEncodingT<std::string>(ref,
                                                       kCFStringEncodingUTF8);
}

// Utility function to pull out a value from a dictionary, check its type, and
// return it.  Returns NULL if the key is not present or of the wrong type.
CFTypeRef GetValueFromDictionary(CFDictionaryRef dict,
                                 CFStringRef key,
                                 CFTypeID expected_type) {
  CFTypeRef value = CFDictionaryGetValue(dict, key);
  if (!value)
    return value;

  if (CFGetTypeID(value) != expected_type) {
    scoped_cftype<CFStringRef> expected_type_ref(
        CFCopyTypeIDDescription(expected_type));
    scoped_cftype<CFStringRef> actual_type_ref(
        CFCopyTypeIDDescription(CFGetTypeID(value)));
//    LOG(WARNING) << "Expected value for key "
//                 << SysCFStringRefToUTF8(key)
//                 << " to be "
//                 << SysCFStringRefToUTF8(expected_type_ref)
//                 << " but it was "
//                 << SysCFStringRefToUTF8(actual_type_ref)
//                 << " instead";
    return NULL;
  }

  return value;
}

// Utility function to pull out a boolean value from a dictionary and return it,
// returning a default value if the key is not present.
bool GetBoolFromDictionary(CFDictionaryRef dict,
                           CFStringRef key,
                           bool default_value) {
  CFNumberRef number = (CFNumberRef)GetValueFromDictionary(dict, key,
                                                           CFNumberGetTypeID());
  if (!number)
    return default_value;

  int int_value;
  if (CFNumberGetValue(number, kCFNumberIntType, &int_value))
    return int_value;
  else
    return default_value;
}

// Utility function to pull out a host/port pair from a dictionary and format
// them into a "<host>[:<port>]" style string. Pass in a dictionary that has a
// value for the host key and optionally a value for the port key. Returns a
// formatted string. In the error condition where the host value is especially
// malformed, returns an empty string. (You may still want to check for that
// result anyway.)
std::string GetHostPortFromDictionary(CFDictionaryRef dict,
                                      CFStringRef host_key,
                                      CFStringRef port_key) {
  std::string result;

  CFStringRef host_ref =
      (CFStringRef)GetValueFromDictionary(dict, host_key,
                                          CFStringGetTypeID());
  if (!host_ref) {
//    LOG(WARNING) << "Could not find expected key "
//                 << SysCFStringRefToUTF8(host_key)
//                 << " in the proxy dictionary";
    return result;
  }
  result = SysCFStringRefToUTF8(host_ref);

  CFNumberRef port_ref =
      (CFNumberRef)GetValueFromDictionary(dict, port_key,
                                          CFNumberGetTypeID());
  if (port_ref) {
    int port;
    CFNumberGetValue(port_ref, kCFNumberIntType, &port);
    result += ":";
    result += IntegerToString(port);
  }

  return result;
}

} // namespace

// static
bool SetupCURLProxies(const std::string16 &request_url, CURL* curl_handle) {
  scoped_cftype<CFDictionaryRef> config_dict(
      SCDynamicStoreCopyProxies(NULL));
  // Parse the url
  GURL request_url_gurl(request_url.c_str());

// TODO(playmobil): respect bypass local names
//  bool bypass_local_names = GetBoolFromDictionary(config_dict.get(),
//                              kSCPropNetProxiesExcludeSimpleHostnames,
//                                                  false);


  // Check if this URL is on the proxy bypass list.
  CFArrayRef bypass_array_ref =
    (CFArrayRef)GetValueFromDictionary(config_dict.get(),
                                       kSCPropNetProxiesExceptionsList,
                                       CFArrayGetTypeID());
  if (bypass_array_ref) {
    CFIndex bypass_array_count = CFArrayGetCount(bypass_array_ref);
    for (CFIndex i = 0; i < bypass_array_count; ++i) {
      CFStringRef bypass_item_ref =
          (CFStringRef)CFArrayGetValueAtIndex(bypass_array_ref, i);
      if (CFGetTypeID(bypass_item_ref) != CFStringGetTypeID()) {
//        LOG(("Expected value for item in the kSCPropNetProxiesExceptionsList"
//                        " to be a CFStringRef but it was not"));
      } else {
        std::string bypass_domain_name = SysCFStringRefToUTF8(bypass_item_ref);

        if (request_url_gurl.DomainIs(bypass_domain_name.c_str())) {
          return true;
        }
      }
    }
  }

  // SOCKS proxy
  if (GetBoolFromDictionary(config_dict.get(),
                            kSCPropNetProxiesSOCKSEnable,
                            false)) {
    std::string host_port =
      GetHostPortFromDictionary(config_dict.get(),
                                kSCPropNetProxiesSOCKSProxy,
                                kSCPropNetProxiesSOCKSPort);

    curl_easy_setopt(curl_handle, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
    curl_easy_setopt(curl_handle, CURLOPT_PROXY, host_port.c_str());

    return true;
  }

  // Otherwise Try HTTP or HTTPS proxies.
  // TODO(playmobil): is the failover to an HTTP proxy OK?

  if (request_url_gurl.SchemeIsSecure() &&
      GetBoolFromDictionary(config_dict.get(),
                            kSCPropNetProxiesHTTPSEnable,
                            false)) {
    std::string host_port =
        GetHostPortFromDictionary(config_dict.get(),
                                  kSCPropNetProxiesHTTPSProxy,
                                  kSCPropNetProxiesHTTPSPort);
    curl_easy_setopt(curl_handle, CURLOPT_PROXY, host_port.c_str());
    return true;
  } else if (GetBoolFromDictionary(config_dict.get(),
                            kSCPropNetProxiesHTTPEnable,
                            false)) {
    std::string host_port =
        GetHostPortFromDictionary(config_dict.get(),
                                  kSCPropNetProxiesHTTPProxy,
                                  kSCPropNetProxiesHTTPPort);
    curl_easy_setopt(curl_handle, CURLOPT_PROXY, host_port.c_str());
    return true;
  }

  return true;
}
