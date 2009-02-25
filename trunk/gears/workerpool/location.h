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

#ifndef GEARS_WORKERPOOL_LOCATION_H__
#define GEARS_WORKERPOOL_LOCATION_H__

#include "gears/base/common/base_class.h"
#include "third_party/googleurl/src/url_parse.h"

// The Gears worker location property is analagous to the one in HTML5's spec:
// http://www.whatwg.org/specs/web-workers/current-work/#worker-locations
class GearsLocation : public ModuleImplBaseClass {
 public:
  static const std::string kModuleName;

  GearsLocation();
  virtual ~GearsLocation();

  // IN: -
  // OUT: string hash
  void GetHash(JsCallContext *context);

  // IN: -
  // OUT: string host
  void GetHost(JsCallContext *context);

  // IN: -
  // OUT: string hostname
  void GetHostname(JsCallContext *context);

  // IN: -
  // OUT: string href
  void GetHref(JsCallContext *context);

  // IN: -
  // OUT: string pathname
  void GetPathname(JsCallContext *context);

  // IN: -
  // OUT: string protocol
  void GetProtocol(JsCallContext *context);

  // IN: -
  // OUT: string port
  void GetPort(JsCallContext *context);

  // IN: -
  // OUT: string search
  void GetSearch(JsCallContext *context);

 private:
  url_parse::Parsed parsed_;
  bool parsed_is_valid_;

  // Implements the above GetXxx methods, after translating the Xxx into an
  // url_parse::Parsed::ComponentType (e.g. GetPort passes PORT).
  // The host_port argument is for implementing GetHost, which does not map
  // directly to a ComponentType -- GetHost should return the concatenation
  // of the host and the port, provided that the port is a non-standard port,
  // and just the host, otherwise.
  void GetComponent(
      JsCallContext *context,
      url_parse::Parsed::ComponentType component_type,
      bool host_port);

  DISALLOW_EVIL_CONSTRUCTORS(GearsLocation);
};

#endif // GEARS_WORKERPOOL_LOCATION_H__
