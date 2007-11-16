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

#ifndef GEARS_CHANNEL_FIREFOX_CHANNEL_H__
#define GEARS_CHANNEL_FIREFOX_CHANNEL_H__

#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#include "ff/genfiles/channel.h"
#include "gears/base/common/base_class.h"

// Object identifiers
extern const char *kGearsChannelClassName;
extern const nsCID kGearsChannelClassId;

class GearsChannel
    : public ModuleImplBaseClass,
      public GearsChannelInterface {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  GearsChannel(); 

  NS_IMETHOD SetOnmessage(nsIVariant *in_handler);
  NS_IMETHOD GetOnmessage(nsIVariant **out_handler);
  NS_IMETHOD SetOnerror(nsIVariant *in_handler);
  NS_IMETHOD GetOnerror(nsIVariant **out_handler);
  NS_IMETHOD SetOndisconnect(nsIVariant *in_handler);
  NS_IMETHOD GetOndisconnect(nsIVariant **out_handler);

  NS_IMETHOD Send(//const nsAString &message_string
                 );
  NS_IMETHOD Connect(//const nsAString &channel_name
                    );
  NS_IMETHOD Disconnect();

private:
  ~GearsChannel();

  scoped_ptr<JsRootedCallback> onmessage_handler_;
  scoped_ptr<JsRootedCallback> onerror_handler_;
  scoped_ptr<JsRootedCallback> ondisconnect_handler_;
};

#endif  // GEARS_CHANNEL_FIREFOX_CHANNEL_H__
