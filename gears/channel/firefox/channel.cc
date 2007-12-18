// Copyright 2007, Google Inc.
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


#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h> // for NS_IMPL_* and NS_INTERFACE_*
#include "gecko_internal/nsIDOMClassInfo.h"

#include "gears/channel/firefox/channel.h"

NS_IMPL_THREADSAFE_ADDREF(GearsChannel)
NS_IMPL_THREADSAFE_RELEASE(GearsChannel)
NS_INTERFACE_MAP_BEGIN(GearsChannel)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsChannelInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsChannelInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsChannel)
NS_INTERFACE_MAP_END

const char *kGearsChannelClassName = "GearsChannel";
const nsCID kGearsChannelClassId = {0x577033F4, 0xF38B, 0x4815, {0xBB, 0x9C,
                                    0xD6, 0x55, 0xA4, 0x35, 0x22, 0x5D}};
                                   // 577033F4-F38B-4815-BB9C-D655A435225D

GearsChannel::GearsChannel() {
}

GearsChannel::~GearsChannel() {
}

NS_IMETHODIMP GearsChannel::SetOnmessage(nsIVariant *in_handler) {
  JsRootedCallback *onerror_handler;
  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"Value is required."));
  }

  // Once we aquire the callback, we're responsible for its lifetime
  if (!js_params.GetAsNewRootedCallback(0, &onerror_handler)) {
    RETURN_EXCEPTION(STRING16(L"Invalid value for onmessage property."));
  }

  onerror_handler_.reset(onerror_handler);

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsChannel::GetOnmessage(nsIVariant **out_value) {
  // TODO(pankaj): NYI
  *out_value = NULL;
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsChannel::SetOnerror(nsIVariant *in_handler) {
  JsRootedCallback *onerror_handler;
  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"Value is required."));
  }

  // Once we aquire the callback, we're responsible for its lifetime
  if (!js_params.GetAsNewRootedCallback(0, &onerror_handler)) {
    RETURN_EXCEPTION(STRING16(L"Invalid value for onerror property."));
  }

  onerror_handler_.reset(onerror_handler);

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsChannel::GetOnerror(nsIVariant **out_value) {
  // TODO(pankaj): NYI
  *out_value = NULL;
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsChannel::SetOndisconnect(nsIVariant *in_handler) {
  JsRootedCallback *onerror_handler;
  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"Value is required."));
  }

  // Once we aquire the callback, we're responsible for its lifetime
  if (!js_params.GetAsNewRootedCallback(0, &onerror_handler)) {
    RETURN_EXCEPTION(STRING16(L"Invalid value for ondisconnect property."));
  }

  onerror_handler_.reset(onerror_handler);

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsChannel::GetOndisconnect(nsIVariant **out_value) {
  // TODO(pankaj): NYI
  *out_value = NULL;
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsChannel::Send() {
  // TODO(pankaj): NYI
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsChannel::Connect() {
  // TODO(pankaj): NYI
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsChannel::Disconnect() {
  // TODO(pankaj): NYI
  RETURN_NORMAL();
}
