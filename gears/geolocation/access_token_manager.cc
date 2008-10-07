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

#include "gears/geolocation/access_token_manager.h"

#include "gears/geolocation/geolocation_db.h"

// static
AccessTokenManager AccessTokenManager::instance_;

// static
AccessTokenManager *AccessTokenManager::GetInstance() {
  return &instance_;
}

void AccessTokenManager::Register(const std::string16 &url) {
  MutexLock lock(&access_tokens_mutex_);
  user_count_.Ref();
  // If we don't have a token for this URL in the map, try to get one from the
  // database.
  if (access_tokens_.find(url) == access_tokens_.end()) {
    GeolocationDB *db = GeolocationDB::GetDB();
    std::string16 access_token;
    if (db && db->RetrieveAccessToken(url, &access_token)) {
      // Empty tokens should never be stored in the DB.
      assert(!access_token.empty());
      access_tokens_[url] = access_token;
    }
  }
}

void AccessTokenManager::Unregister() {
  MutexLock lock(&access_tokens_mutex_);
  // If this is the last user, write the tokens to the database.
  if (user_count_.Unref()) {
    GeolocationDB *db = GeolocationDB::GetDB();
    if (db) {
      for (AccessTokenMap::const_iterator iter = access_tokens_.begin();
           iter != access_tokens_.end();
           iter++) {
        if (!iter->second.empty()) {
          db->StoreAccessToken(iter->first, iter->second);
        }
      }
    }
  }
}

void AccessTokenManager::GetToken(const std::string16 &url,
                                  std::string16 *access_token) {
  assert(access_token);
  MutexLock lock(&access_tokens_mutex_);
  AccessTokenMap::const_iterator iter = access_tokens_.find(url);
  if (iter == access_tokens_.end()) {
    access_token->clear();
  } else {
    *access_token = iter->second;
  }
}

void AccessTokenManager::SetToken(const std::string16 &url, const std::string16 &access_token) {
  MutexLock lock(&access_tokens_mutex_);
  access_tokens_[url] = access_token;
}
