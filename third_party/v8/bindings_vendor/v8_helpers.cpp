// Copyright (c) 2008, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "config.h"

#define max max
#define min min
#include "v8_helpers.h"
#include "v8_proxy.h"
#include "v8_index.h"
#include "np_v8object.h"

#include "DOMWindow.h"

void WrapNPObject(v8::Handle<v8::Object> obj, NPObject* npobj) {
  ASSERT(obj->InternalFieldCount() >= 3);

  WebCore::V8Proxy::SetDOMWrapper(obj, WebCore::V8ClassIndex::NPOBJECT, npobj);

  // Create a JS object as a hash map for functions
  obj->SetInternalField(2, v8::Object::New());
}

v8::Local<v8::Context> GetV8Context(NPP npp, NPObject* npobj) {
  V8NPObject* object = reinterpret_cast<V8NPObject*>(npobj);
  return WebCore::V8Proxy::GetContext(object->root_object->frame());
}

WebCore::V8Proxy* GetV8Proxy(NPObject* npobj) {
  V8NPObject* object = reinterpret_cast<V8NPObject*>(npobj);
  WebCore::Frame* frame = object->root_object->frame();
  return WebCore::V8Proxy::retrieve(frame);
}
