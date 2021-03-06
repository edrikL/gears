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

#include "v8_np_utils.h"

#include "DOMWindow.h"
#include "Frame.h"
#include "PlatformString.h"
#undef LOG

#include "npruntime_priv.h"
#include "np_v8object.h"
#include "v8_npobject.h"
#include "v8_proxy.h"

void ConvertV8ObjectToNPVariant(v8::Local<v8::Value> object, NPObject *owner,
                                NPVariant* result) {
  VOID_TO_NPVARIANT(*result);

  // It is really the caller's responsibility to deal with the empty handle
  // case because there could be different actions to take in different
  // contexts.
  ASSERT(!object.IsEmpty());

  if (object.IsEmpty()) return;

  if (object->IsInt32()) {
    INT32_TO_NPVARIANT(object->NumberValue(), *result);  

  } else if (object->IsNumber()) {
    DOUBLE_TO_NPVARIANT(object->NumberValue(), *result);

  } else if (object->IsBoolean()) {
    BOOLEAN_TO_NPVARIANT(object->BooleanValue(), *result);

  } else if (object->IsNull()) {
    NULL_TO_NPVARIANT(*result);

  } else if (object->IsUndefined()) {
    VOID_TO_NPVARIANT(*result);

  } else if (object->IsString()) {
    v8::String::Utf8Value utf8(object);
    char* utf8_chars = strdup(*utf8);
    STRINGN_TO_NPVARIANT(utf8_chars, utf8.length(), *result);
  } else if (object->IsObject()) {
    WebCore::DOMWindow* window = WebCore::V8Proxy::retrieveWindow();
    NPObject* npobject = NPN_CreateScriptObject(
        0, v8::Handle<v8::Object>::Cast(object), window);
    if (npobject) {
      _NPN_RegisterObject(npobject, owner);
    }
    OBJECT_TO_NPVARIANT(npobject, *result);
  }
}


v8::Handle<v8::Value> ConvertNPVariantToV8Object(const NPVariant* variant,
                                                 NPObject* npobject) {
  NPVariantType type = variant->type;

  if (type == NPVariantType_Int32) {
    return v8::Integer::New(NPVARIANT_TO_INT32(*variant));
  }
  if (type == NPVariantType_Double) {
    return v8::Number::New(NPVARIANT_TO_DOUBLE(*variant));
  }
  if (type == NPVariantType_Bool) {
    return NPVARIANT_TO_BOOLEAN(*variant) ? v8::True() : v8::False();
  }
  if (type == NPVariantType_Null) {
    return v8::Null();
  }
  if (type == NPVariantType_Void) {
    return v8::Undefined();
  }
  if (type == NPVariantType_String) {
    NPString src = NPVARIANT_TO_STRING(*variant);
    return v8::String::New(src.UTF8Characters, src.UTF8Length);
  }
  if (type == NPVariantType_Object) {
    NPObject* obj = NPVARIANT_TO_OBJECT(*variant);
    if (obj->_class == NPScriptObjectClass) {
      return reinterpret_cast<V8NPObject*>(obj)->v8_object;
    }
    return CreateV8ObjectForNPObject(obj, npobject);
  }
  return v8::Undefined();
}

// Helper function to create an NPN String Identifier from a v8 string.
NPIdentifier GetStringIdentifier(v8::Handle<v8::String> str) {
  const int kStackBufSize = 100;

  int buf_len = str->Length() + 1;
  if (buf_len <= kStackBufSize) {
    // Use local stack buffer to avoid heap allocations for small strings.
    // Here we should only use the stack space for stack_buf when it's used,
    // not when we use the heap.
    char stack_buf[kStackBufSize];
    str->WriteAscii(stack_buf);
    return NPN_GetStringIdentifier(stack_buf);
  }

  v8::String::AsciiValue ascii(str);
  return NPN_GetStringIdentifier(*ascii);
}
