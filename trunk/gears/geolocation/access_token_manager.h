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
//
// NetworkLocationRequest objects are specific to host, server URL and language.
// The access token should be specific to a server URL only. This class manages
// sharing the access token between multiple NetworkLocationRequest objects, and
// reading and writing the value to the database for storage between sessions.

#ifndef GEARS_GEOLOCATION_ACCESS_TOKEN_MANAGER_H__
#define GEARS_GEOLOCATION_ACCESS_TOKEN_MANAGER_H__

#include "gears/base/common/common.h"
#include <map>
#include "gears/base/common/mutex.h"
#include "gears/base/common/scoped_refptr.h"  // For RefCount

class AccessTokenManager {
 public:
  static AccessTokenManager *GetInstance();
  void Register(const std::string16 &url);
  void Unregister();
  // Returns the empty string if no token exists.
  void GetToken(const std::string16 &url, std::string16 *access_token);
  void SetToken(const std::string16 &url, const std::string16 &access_token);

 private:
  AccessTokenManager() {}
  ~AccessTokenManager() {}

  RefCount user_count_;

  // A map from server URL to access token.
  typedef std::map<std::string16, std::string16> AccessTokenMap;
  AccessTokenMap access_tokens_;
  Mutex access_tokens_mutex_;

  static AccessTokenManager instance_;

  DISALLOW_EVIL_CONSTRUCTORS(AccessTokenManager);
};

#endif  // GEARS_GEOLOCATION_ACCESS_TOKEN_MANAGER_H__
