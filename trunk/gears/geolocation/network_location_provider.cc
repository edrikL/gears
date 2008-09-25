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

#include "gears/geolocation/network_location_provider.h"

#include "gears/base/common/event.h"
#include "gears/base/common/stopwatch.h"  // For GetCurrentTimeMillis
#include "gears/geolocation/geolocation_db.h"

// The maximum period of time we'll wait for a complete set of device data
// before sending the request.
static const int kDataCompleteWaitPeriod = 1000 * 2;  // 2 seconds
// The baseline minimum period between network requests.
static const int kBaselineMinimumRequestInterval = 1000 * 5;  // 5 seconds
// The upper limit of the minimum period between network requests.
static const int kMinimumRequestIntervalLimit = 1000 * 60 * 60 * 3;  // 3 hours


// NetworkLocationRequest objects are specific to host, server URL and language.
// The access token should be specific to a server URL only. This class manages
// sharing the access token between multiple NetworkLocationRequest objects, and
// reading and writing the value to the database for storage between sessions.
class AccessTokenManager {
 public:
  static AccessTokenManager *GetInstance() {
    return &instance_;
  }

  void Register(const std::string16 &url) {
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

  void Unregister() {
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

  // Returns the empty string if no token exists.
  void GetToken(const std::string16 &url, std::string16 *access_token) {
    assert(access_token);
    MutexLock lock(&access_tokens_mutex_);
    AccessTokenMap::const_iterator iter = access_tokens_.find(url);
    if (iter == access_tokens_.end()) {
      access_token->clear();
    } else {
      *access_token = iter->second;
    }
  }

  void SetToken(const std::string16 &url, const std::string16 &access_token) {
    MutexLock lock(&access_tokens_mutex_);
    access_tokens_[url] = access_token;
  }

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

// static
AccessTokenManager AccessTokenManager::instance_;


// The BackoffManager class is used to implement exponential back-off for
// network requests in case of sever errors. Users report to the BackoffManager
// class when they make a request to or receive a response from a given url. The
// BackoffManager class provides the earliest time at which subsequent requests
// should be made.
class BackoffManager {
 public:
  static void ReportRequest(const std::string16 &url) {
    MutexLock lock(&servers_mutex_);
    ServerMap::iterator iter = servers_.find(url);
    if (iter != servers_.end()) {
      iter->second.first = GetCurrentTimeMillis();
    } else {
      servers_[url] = std::make_pair(GetCurrentTimeMillis(),
                                     kBaselineMinimumRequestInterval);
    }
  }

  static int64 ReportResponse(const std::string16 &url, bool server_error) {
    // Use exponential back-off on server error.
    MutexLock lock(&servers_mutex_);
    ServerMap::iterator iter = servers_.find(url);
    assert(iter != servers_.end());
    int64 *interval = &iter->second.second;
    if (server_error) {
      if (*interval < kMinimumRequestIntervalLimit) {
        // Increase interval by between 90% and 110%.
        srand(static_cast<unsigned int>(GetCurrentTimeMillis()));
        double increment_proportion = 0.9 + 0.2 * rand() / RAND_MAX;
        int64 increment = static_cast<int64>(*interval * increment_proportion);
        if (increment > kMinimumRequestIntervalLimit - *interval) {
          *interval = kMinimumRequestIntervalLimit;
        } else {
          *interval += increment;
        }
      }
    } else {
      *interval = kBaselineMinimumRequestInterval;
    }
    return iter->second.first + *interval;
  }

 private:
  // A map from server URL to a pair of integers representing the last request
  // time and the current minimum interval between requests, both in
  // milliseconds.
  typedef std::map<std::string16, std::pair<int64, int64> > ServerMap;
  static ServerMap servers_;

  // The mutex used to protect the map.
  static Mutex servers_mutex_;

  DISALLOW_EVIL_CONSTRUCTORS(BackoffManager);
};

BackoffManager::ServerMap BackoffManager::servers_;
Mutex BackoffManager::servers_mutex_;


LocationProviderBase *NewNetworkLocationProvider(
    const std::string16 &url,
    const std::string16 &host_name,
    const std::string16 &language) {
  return new NetworkLocationProvider(url, host_name, language);
}


NetworkLocationProvider::NetworkLocationProvider(const std::string16 &url,
                                                 const std::string16 &host_name,
                                                 const std::string16 &language)
    : url_(url),
      host_name_(host_name),
      request_address_(false),
      request_address_from_last_request_(false),
      address_language_(language),
      is_shutting_down_(false),
      is_new_data_available_(false),
      is_new_listener_waiting_(false) {
  // TODO(steveblock): Consider allowing multiple values for "address_language"
  // in the network protocol to allow better sharing of network location
  // providers.

  // Get the device data providers. The first call to Register will create the
  // provider and it will be deleted by ref counting.
  radio_data_provider_ = RadioDataProvider::Register(this);
  wifi_data_provider_ = WifiDataProvider::Register(this);

  AccessTokenManager::GetInstance()->Register(url_);

  // Start the worker thread
  Start();
}

NetworkLocationProvider::~NetworkLocationProvider() {
  if (request_) {
    request_->StopThreadAndDelete();
  }

  // Shut down the worker thread
  is_shutting_down_ = true;
  thread_notification_event_.Signal();
  Join();

  radio_data_provider_->Unregister(this);
  wifi_data_provider_->Unregister(this);

  AccessTokenManager::GetInstance()->Unregister();
}

void NetworkLocationProvider::RegisterListener(
    LocationProviderBase::ListenerInterface *listener,
    bool request_address) {
  // Determine whether this listener requires an address when the last request
  // does not.
  bool new_listener_requires_address =
      !request_address_from_last_request_ && request_address;

  // Update whether or not we need to request an address.
  request_address_ |= request_address;

  // If we now need to request an address when we did not before, we don't add
  // the listener. This is because if a request is currently in progress, we
  // don't want the new listener to be called back with a position without an
  // address. We add the listener when we make the next request.
  if (new_listener_requires_address) {
    MutexLock lock(&new_listeners_requiring_address_mutex_);
    new_listeners_requiring_address_.insert(listener);
  } else {
    LocationProviderBase::RegisterListener(listener, request_address);
  }

  // Signal to the worker thread that there is a new listener.
  is_new_listener_waiting_ = true;
  thread_notification_event_.Signal();
}

void NetworkLocationProvider::UnregisterListener(
    LocationProviderBase::ListenerInterface *listener) {
  assert(listener);

  // First try removing the listener from the set of new listeners waiting for
  // an address. Otherwise, try the regular listeners.
  MutexLock new_listeners_lock(&new_listeners_requiring_address_mutex_);
  ListenerSet::iterator iter = new_listeners_requiring_address_.find(listener);
  if (iter != new_listeners_requiring_address_.end()) {
    new_listeners_requiring_address_.erase(iter);
  } else {
    LocationProviderBase::UnregisterListener(listener);
  }

  // Update whether or not we need to request an address.
  if (request_address_) {
    if (!new_listeners_requiring_address_.empty()) {
      return;
    }
    MutexLock listeners_lock(GetListenersMutex());
    ListenerMap *listeners = GetListeners();
    for (ListenerMap::const_iterator iter = listeners->begin();
         iter != listeners->end();
         iter++) {
      if (iter->second.first == true) {
        return;
      }
    }
    request_address_ = false;
  }
}

// LocationProviderBase implementation
void NetworkLocationProvider::GetPosition(Position *position) {
  assert(position);
  MutexLock lock(&position_mutex_);
  *position = position_;
}

// DeviceDataProviderInterface::ListenerInterface implementation.
void NetworkLocationProvider::DeviceDataUpdateAvailable(
    RadioDataProvider *provider) {
  MutexLock lock(&data_mutex_);
  assert(provider == radio_data_provider_);
  is_radio_data_complete_ = radio_data_provider_->GetData(&radio_data_);

  DeviceDataUpdateAvailableImpl();
}

void NetworkLocationProvider::DeviceDataUpdateAvailable(
    WifiDataProvider *provider) {
  assert(provider == wifi_data_provider_);
  MutexLock lock(&data_mutex_);
  is_wifi_data_complete_ = wifi_data_provider_->GetData(&wifi_data_);

  DeviceDataUpdateAvailableImpl();
}

// NetworkLocationRequest::ListenerInterface implementation.
void NetworkLocationProvider::LocationResponseAvailable(
    const Position &position,
    bool server_error,
    const std::string16 &access_token) {
  // Cache the position
  position_mutex_.Lock();
  position_ = position;
  position_mutex_.Unlock();

  // Record access_token if it's set.
  if (!access_token.empty()) {
    AccessTokenManager::GetInstance()->SetToken(url_, access_token);
  }

  // Get earliest time for next request.
  earliest_next_request_time_ = BackoffManager::ReportResponse(url_,
                                                               server_error);

  // Signal to the worker thread that this request has completed.
  is_last_request_complete_ = true;
  thread_notification_event_.Signal();

  // Let listeners know that we now have a position available.
  UpdateListeners();
}

// Thread implementation
void NetworkLocationProvider::Run() {
  // Create the network request object. We must do this on the same thread from
  // which we'll call Start().
  request_ = NetworkLocationRequest::Create(url_, host_name_, this);
  if (!request_) {
    LOG(("Failed to create NetworkLocationRequest object.\n"));
    assert(false);
    return;
  }

  // Get the device data.
  data_mutex_.Lock();
  is_radio_data_complete_ = radio_data_provider_->GetData(&radio_data_);
  is_wifi_data_complete_ = wifi_data_provider_->GetData(&wifi_data_);
  timestamp_ = GetCurrentTimeMillis();

  // For the first request, wait for a certain maximum time period to get as
  // much device data as possible.
  int64 start_time = timestamp_;
  while (true) {
    if (is_radio_data_complete_ && is_wifi_data_complete_) {
      data_mutex_.Unlock();
      break;
    }
    data_mutex_.Unlock();

    int64 elapsed_time = GetCurrentTimeMillis() - start_time;
    int timeout = kDataCompleteWaitPeriod - static_cast<int>(elapsed_time);
    if (timeout <= 0) {
      break;
    }
    if (!thread_notification_event_.WaitWithTimeout(timeout)) {
      // Quit waiting if we time out.
      break;
    }
    // Terminate the thread if requested.
    if (is_shutting_down_) {
      return;
    }
    // The event should be due to new device data or a new listener.
    assert(is_new_data_available_ || is_new_listener_waiting_);
    // Lock the data mutex to test is_radio_data_complete_ and
    // is_wifi_data_complete_ on the next loop.
    data_mutex_.Lock();
  }

  earliest_next_request_time_ = 0;
  MakeRequest();

  // Loop continually, making requests whenever new data becomes available,
  // subject to the minimum interval.
  //
  // This loop is structured such that we don't require mutex locks to
  // synchronise changes to is_new_data_available_ etc with signals on
  // thread_notification_event_. Note that if we get a signal before we wait,
  // the wait will proceed immediately, so we don't miss signals.
  int64 remaining_time = 1;
  while (!is_shutting_down_) {
    if (remaining_time > 0) {
      remaining_time = earliest_next_request_time_ - GetCurrentTimeMillis();
    }

    // If the minimum time period has not yet elapsed, set the timeout such
    // that the wait expires when the period has elapsed.
    if (remaining_time > 0) {
      thread_notification_event_.WaitWithTimeout(
          static_cast<int>(remaining_time));
    } else {
      thread_notification_event_.Wait();
    }

    // Update remaining time now we've woken up. Note that it can never
    // transition from <= 0 to > 0.
    if (remaining_time > 0) {
      remaining_time = earliest_next_request_time_ - GetCurrentTimeMillis();
    }

    bool make_request = false;
    if (is_new_listener_waiting_) {
      // A new listener has just registered with this provider. If new data is
      // available, force a new request now, unless a request is already in
      // progress. If not, update listeners with the last known position,
      // provided we have one.
      if (is_new_data_available_) {
        if (is_last_request_complete_) {
          make_request = true;
        }
      } else {
        // Before the first network request completes, position_ may not be
        // valid, so we do not update the listeners. They will be updated once
        // the network request completes.
        if (position_.IsInitialized()) {
          // Update listeners with the last known position.
          UpdateListeners();
        }
      }
      is_new_listener_waiting_ = false;
    }

    // If a new listener has now registered such that we now require an address,
    // we make a new request once the current request completes.
    new_listeners_requiring_address_mutex_.Lock();
    if (!new_listeners_requiring_address_.empty()) {
      if (is_last_request_complete_) {
        make_request = true;
      }
    }
    new_listeners_requiring_address_mutex_.Unlock();

    // If the thread is not shutting down, we have new data, the last request
    // has completed, and the minimum time has elapsed, make the next request.
    if (!is_shutting_down_ &&
        is_new_data_available_ &&
        is_last_request_complete_ &&
        remaining_time <= 0) {
      make_request = true;
    }

    // TODO(steveblock): If the request does not complete within some maximum
    // time period, we should kill it and start a new request.
    if (make_request) {
      MakeRequest();
      remaining_time = 1;
    }
  }
}

// Other methods

bool NetworkLocationProvider::MakeRequest() {
  // Reset flags
  is_new_data_available_ = false;
  is_last_request_complete_ = false;
  is_new_listener_waiting_ = false;

  // If we have new listeners waiting for an address, request_address_
  // must be true.
  assert(new_listeners_requiring_address_.empty() || request_address_);

  // Move the new listeners waiting for an address to the list of listeners.
  MutexLock lock(&new_listeners_requiring_address_mutex_);
  for (ListenerSet::const_iterator iter =
       new_listeners_requiring_address_.begin();
       iter != new_listeners_requiring_address_.end();
       iter++) {
    LocationProviderBase::RegisterListener(*iter, true);
  }
  new_listeners_requiring_address_.clear();

  request_address_from_last_request_ = request_address_;

  BackoffManager::ReportRequest(url_);

  std::string16 access_token;
  AccessTokenManager::GetInstance()->GetToken(url_, &access_token);
  return request_->MakeRequest(access_token,
                               radio_data_,
                               wifi_data_,
                               request_address_,
                               address_language_,
                               kBadLatLng,  // We don't have a position to pass
                               kBadLatLng,  // to the server.
                               timestamp_);
}

void NetworkLocationProvider::DeviceDataUpdateAvailableImpl() {
  timestamp_ = GetCurrentTimeMillis();

  // Signal to the worker thread that new data is available.
  is_new_data_available_ = true;
  thread_notification_event_.Signal();
}
