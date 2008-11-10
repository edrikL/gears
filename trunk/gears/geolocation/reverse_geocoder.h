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

#ifndef GEARS_GEOLOCATION_REVERSE_GEOCODER_H__
#define GEARS_GEOLOCATION_REVERSE_GEOCODER_H__

#include "gears/geolocation/network_location_request.h"

class ReverseGeocoder : public NetworkLocationRequest::ListenerInterface {
 public:
  // Calling this ListenerInterface causes inheritance scoping problems with
  // NetworkLocationRequest::ListenerInterface.
  class ReverseGeocoderListenerInterface {
   public:
    virtual void ReverseGeocodeAvailable(const Position &position,
                                         bool server_error) = 0;
    virtual ~ReverseGeocoderListenerInterface() {}
  };

  ReverseGeocoder(const std::string16 &url,
                  const std::string16 &host_name,
                  const std::string16 &address_language,
                  ReverseGeocoderListenerInterface *listener);
  virtual ~ReverseGeocoder();

  bool MakeRequest(BrowsingContext *browsing_context, const Position &position);

 private:
  // NetworkLocationRequest::ListenerInterface implementation.
  virtual void LocationResponseAvailable(const Position &position,
                                         bool server_error,
                                         const std::string16 &access_token);

  std::string16 url_;
  std::string16 host_name_;
  std::string16 address_language_;
  ReverseGeocoderListenerInterface *listener_;
  NetworkLocationRequest *request_;

  DISALLOW_EVIL_CONSTRUCTORS(ReverseGeocoder);
};

#endif  // GEARS_GEOLOCATION_REVERSE_GEOCODER_H__
