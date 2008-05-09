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

#ifndef GEARS_GEOLOCATION_NETWORK_LOCATION_REQUEST_H__
#define GEARS_GEOLOCATION_NETWORK_LOCATION_REQUEST_H__

// NetworkLocationRequest is currently only implemented for Win32.
#ifdef WIN32

#include "gears/base/common/common.h"
#include "gears/base/common/basictypes.h"  // For int64
#include "gears/blob/blob.h"
#include "gears/geolocation/geolocation.h"
#include "gears/geolocation/device_data_provider.h"
#include "gears/localserver/common/async_task.h"
#include <atlsync.h>  // For CEvent.
#include <vector>

// An implementation of an AsyncTask that takes a set of device data and sends
// it to a server to get a position fix. It performs formatting of the request
// and interpretation of the response.
class NetworkLocationRequest : public AsyncTask {
 public:
  // Interface for receiving callbacks from a NetworkLocationRequest object.
  class ListenerInterface {
   public:
    virtual void LocationResponseAvailable(const Position &position) = 0;
  };
  static NetworkLocationRequest* Create(const std::string16 &url,
                                        const std::string16 &host_name,
                                        ListenerInterface *listener);
  bool MakeRequest(const RadioData &radio_data,
                   const WifiData &wifi_data,
                   int64 timestamp);
  void StopThreadAndDelete();
 private:
  // Private constructor and destructor. Callers should use Create() and
  // StopThreadAndDelete.
  NetworkLocationRequest(const std::string16 &url,
                         const std::string16 &host_name,
                         ListenerInterface *listener);
  virtual ~NetworkLocationRequest() {}
  // AsyncTask implementation.
  virtual void Run();
  bool Init();

  int64 timestamp_;  // The timestamp of the data used to make the request.
  scoped_refptr<BlobInterface> post_body_;
  ListenerInterface *listener_;
  std::string16 url_;
  std::string16 host_name_;
  CEvent run_complete_event_;
  DISALLOW_EVIL_CONSTRUCTORS(NetworkLocationRequest);
};

#endif  // WIN32

#endif  // GEARS_GEOLOCATION_NETWORK_LOCATION_REQUEST_H__
