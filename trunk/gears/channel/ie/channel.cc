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

#include "gears/base/ie/activex_utils.h"
#include "gears/channel/ie/channel.h"

STDMETHODIMP GearsChannel::put_onmessage(const VARIANT *in_handler) {
  if (!ActiveXUtils::VariantIsNullOrUndefined(in_handler)) {
    if (in_handler->vt == VT_DISPATCH) {
      onmessage_handler_.reset(new JsRootedCallback(NULL, *in_handler));
    } else {
      RETURN_EXCEPTION(STRING16(L"The onmessage callback "
                                L"must be a function."));
    }
  } else {
    onmessage_handler_.reset(NULL);
  }

  RETURN_NORMAL();
}

STDMETHODIMP GearsChannel::put_onerror(const VARIANT *in_handler) {
  if (!ActiveXUtils::VariantIsNullOrUndefined(in_handler)) {
    if (in_handler->vt == VT_DISPATCH) {
      onerror_handler_.reset(new JsRootedCallback(NULL, *in_handler));
    } else {
      RETURN_EXCEPTION(STRING16(L"The onerror callback "
                                L"must be a function."));
    }
  } else {
    onerror_handler_.reset(NULL);
  }

  RETURN_NORMAL();
}

STDMETHODIMP GearsChannel::put_ondisconnect(const VARIANT *in_handler) {
  if (!ActiveXUtils::VariantIsNullOrUndefined(in_handler)) {
    if (in_handler->vt == VT_DISPATCH) {
      ondisconnect_handler_.reset(new JsRootedCallback(NULL, *in_handler));
    } else {
      RETURN_EXCEPTION(STRING16(L"The ondisconnect callback "
                                L"must be a function."));
    }
  } else {
    ondisconnect_handler_.reset(NULL);
  }

  RETURN_NORMAL();
}

STDMETHODIMP GearsChannel::send(const BSTR *message_string) {
  // TODO(pankaj): NYI
  RETURN_NORMAL();
}

STDMETHODIMP GearsChannel::connect(const BSTR *channel_name) {
  // TODO(pankaj): NYI
  RETURN_NORMAL();
}

STDMETHODIMP GearsChannel::disconnect() {
  // TODO(pankaj): NYI
  RETURN_NORMAL();
}
