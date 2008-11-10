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

#include "gears/geolocation/reverse_geocoder.h"

#include "gears/base/common/event.h"
#include "gears/base/common/stopwatch.h"  // For GetCurrentTimeMillis
#include "gears/geolocation/access_token_manager.h"


ReverseGeocoder::ReverseGeocoder(const std::string16 &url,
                                 const std::string16 &host_name,
                                 const std::string16 &address_language,
                                 ReverseGeocoderListenerInterface *listener)
    : url_(url),
      host_name_(host_name),
      address_language_(address_language),
      listener_(listener),
      request_(NULL) {
  assert(listener_);

  AccessTokenManager::GetInstance()->Register(url_);
}

ReverseGeocoder::~ReverseGeocoder() {
  if (request_) {
    request_->StopThreadAndDelete();
  }

  AccessTokenManager::GetInstance()->Unregister();
}

bool ReverseGeocoder::MakeRequest(BrowsingContext *browsing_context,
                                  const Position &position) {
  // Lazily create the network request object. We must do this on the same
  // thread from which we'll call MakeRequest().
  if (request_ == NULL) {
    request_ = NetworkLocationRequest::Create(browsing_context, url_,
                                              host_name_, this);
  }
  assert(request_);

  RadioData radio_data;
  WifiData wifi_data;
  std::string16 access_token;
  AccessTokenManager::GetInstance()->GetToken(url_, &access_token);
  return request_->MakeRequest(access_token,
                               radio_data,
                               wifi_data,
                               true,  // request_address
                               address_language_,
                               position.latitude,
                               position.longitude,
                               position.timestamp);
}

// NetworkLocationRequest::ListenerInterface implementation.
void ReverseGeocoder::LocationResponseAvailable(
    const Position &position,
    bool server_error,
    const std::string16 &access_token) {
  // Record access_token if it's set.
  if (!access_token.empty()) {
    AccessTokenManager::GetInstance()->SetToken(url_, access_token);
  }

  listener_->ReverseGeocodeAvailable(position, server_error);
}
