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

#ifndef GEARS_GEOLOCATION_DEVICE_DATA_PROVIDER_INL_H__
#define GEARS_GEOLOCATION_DEVICE_DATA_PROVIDER_INL_H__

#include <algorithm>

// static
template <typename DataType>
Mutex DeviceDataProviderBase<DataType>::instance_mutex;

// static
template <typename DataType>
DeviceDataProviderBase<DataType>* DeviceDataProviderBase<DataType>::instance =
    NULL;

// static
template<typename DataType>
DeviceDataProviderBase<DataType>* DeviceDataProviderBase<DataType>::Register(
    ListenerInterface *listener) {
  // We protect against Register and Unregister being called asynchronously from
  // different threads. This is the case when a device data provider is used by
  // a NetworkLocationProvider object. Register is always called from the
  // JavaScript thread, but NetworkLocationProvider objects are destructed, and
  // unregister from the device data provider, asynchronously, once their HTTP
  // request has completed.

  MutexLock mutex(&instance_mutex);
  if (!instance) {
    instance = Create();
  }
  assert(instance);
  instance->Ref();
  instance->AddListener(listener);
  return instance;
}

// static
template<typename DataType>
bool DeviceDataProviderBase<DataType>::Unregister(ListenerInterface *listener) {
  MutexLock mutex(&instance_mutex);
  if (!instance->RemoveListener(listener)) {
    return false;
  }
  if (instance->Unref()) {
    delete instance;
    instance = NULL;
  }
  return true;
}

template<typename DataType>
void DeviceDataProviderBase<DataType>::Ref() {
  count_.Ref();
}

template<typename DataType>
bool DeviceDataProviderBase<DataType>::Unref() {
  return count_.Unref();
}

template<typename DataType>
void DeviceDataProviderBase<DataType>::AddListener(
    ListenerInterface *listener) {
  MutexLock mutex(&listeners_mutex_);
  listeners_.insert(listener);
}

template<typename DataType>
bool DeviceDataProviderBase<DataType>::RemoveListener(
    ListenerInterface *listener) {
  MutexLock mutex(&listeners_mutex_);
  typename ListenersSet::iterator iter =
    find(listeners_.begin(), listeners_.end(), listener);
  if (iter == listeners_.end()) {
    return false;
  }
  listeners_.erase(iter);
  return true;
}

#endif  // GEARS_GEOLOCATION_DEVICE_DATA_PROVIDER_INL_H__
