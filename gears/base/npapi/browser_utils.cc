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

#include "gears/base/npapi/browser_utils.h"

#include <stack>

#include "gears/base/common/string_utils.h"
#include "gears/base/common/js_types.h"
#include "gears/base/npapi/scoped_npapi_handles.h"

// TODO(mpcomplete): when we have workerpool support, these will probably need
// to be thread-local variables.
static std::stack<JsCallContext*> call_stack;

void BrowserUtils::EnterScope(JsCallContext *context) {
  call_stack.push(context);
}

void BrowserUtils::ExitScope() {
  assert(!call_stack.empty());
  call_stack.pop();
}

void BrowserUtils::SetJsException(const std::string16& message) {
  assert(!call_stack.empty());
  call_stack.top()->SetException(message);
}

bool BrowserUtils::GetPageLocationUrl(JsContextPtr context,
                                      std::string16 *location_url) {
  assert(location_url);

  // Retrieve window.location.href.
  NPObject* window;
  if (NPN_GetValue(context, NPNVWindowNPObject, &window) != NPERR_NO_ERROR)
    return false;
  ScopedNPObject window_scoped(window);

  static NPIdentifier location_id = NPN_GetStringIdentifier("location");
  ScopedNPVariant np_location;
  if (!NPN_GetProperty(context, window, location_id, &np_location))
    return false;

  assert(NPVARIANT_IS_OBJECT(np_location));
  static NPIdentifier href_id = NPN_GetStringIdentifier("href");
  ScopedNPVariant np_href;
  if (!NPN_GetProperty(context, NPVARIANT_TO_OBJECT(np_location),
                       href_id, &np_href)) {
    return false;
  }

  assert(NPVARIANT_IS_STRING(np_href));
  NPString np_str = NPVARIANT_TO_STRING(np_href);

  return (UTF8ToString16(np_str.utf8characters,
                         np_str.utf8length,
                         location_url));
}

bool BrowserUtils::GetPageSecurityOrigin(JsContextPtr context,
                                         SecurityOrigin *security_origin) {
  std::string16 location;
  if (!GetPageLocationUrl(context, &location))
    return false;
  return security_origin->InitFromUrl(location.c_str());
}
