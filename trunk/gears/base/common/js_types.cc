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

#include "gears/base/common/base_class.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/js_types.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#if BROWSER_FF
#include "gears/base/firefox/dom_utils.h"
#elif BROWSER_IE
#include <dispex.h>
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#elif BROWSER_NPAPI
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/npapi/np_utils.h"
#elif BROWSER_SAFARI
#include "gears/base/safari/browser_utils.h"
#endif

// Special conversion functions for FireFox
#if BROWSER_FF

static bool StringToToken(JsContextPtr context,
                          const std::string16& in,
                          JsToken* out) {
  JSString *jstr = JS_NewUCStringCopyZ(
                       context,
                       reinterpret_cast<const jschar *>(in.c_str()));
  if (jstr) {
    *out = STRING_TO_JSVAL(jstr);
    return true;
  } else {
    return false;
  }
}

static bool ModuleToToken(JsContextPtr context, IScriptable* in, JsToken* out) {
  nsresult nr;
  nsCOMPtr<nsIXPConnect> xpc;
  xpc = do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr);
  if (NS_FAILED(nr))
    return false;

  // Retrieves a scope object for the current script or function.
  // http://developer.mozilla.org/en/docs/JS_GetScopeChain
  JSObject* scope = JS_GetScopeChain(context);

  // convert a nsISupports into a JSObject, so it can be put in a jsval
  nsCOMPtr<nsIXPConnectJSObjectHolder> object_holder;
  nsIID iid = NS_GET_IID(IScriptable);
  nr = xpc->WrapNative(context, scope, in, iid,
                       getter_AddRefs(object_holder));
  if (NS_FAILED(nr))
    return false;

  JSObject* object = NULL;
  nr = object_holder->GetJSObject(&object);
  if (NS_FAILED(nr))
    return false;

  *out = OBJECT_TO_JSVAL(object);
  return true;
}

#endif

// Browser specific JsArray functions.
#if BROWSER_FF

JsArray::JsArray() : js_context_(NULL), array_(0) {
}

JsArray::~JsArray() {
  if (array_ && JSVAL_IS_GCTHING(array_))
    JS_RemoveRoot(js_context_, &array_);
}

bool JsArray::SetArray(JsToken value, JsContextPtr context) {
  // check that it's an array
  if (!JSVAL_IS_OBJECT(value) ||
      !JS_IsArrayObject(context, JSVAL_TO_OBJECT(value))) {
    return false;
  }

  if (array_ && JSVAL_IS_GCTHING(array_))
    JS_RemoveRoot(js_context_, &array_);
  array_ = value;
  js_context_ = context;
  if (JSVAL_IS_GCTHING(array_))
    JS_AddRoot(js_context_, &array_);
  return true;
}

bool JsArray::GetLength(int *length) const {
  jsuint array_length;
  if (JS_GetArrayLength(js_context_,
                        JSVAL_TO_OBJECT(array_), &array_length)) {
    *length = static_cast<int>(array_length);
    return true;
  }

  return false;
}

bool JsArray::GetElement(int index, JsScopedToken *out) const {
  if (!array_) return false;

  return JS_GetElement(js_context_, JSVAL_TO_OBJECT(array_),
                       index, out) == JS_TRUE;
}

bool JsArray::SetElement(int index, const JsToken &value) {
  if (!array_)
    return false;

  JSBool result = JS_DefineElement(js_context_,
                                   JSVAL_TO_OBJECT(array_),
                                   index, value,
                                   nsnull, nsnull, // getter, setter
                                   JSPROP_ENUMERATE);

  // stops warning C4800 from VC (BOOL to bool performance warning)
  return result == JS_TRUE;
}

bool JsArray::SetElementBool(int index, bool value) {
  return SetElement(index, BOOLEAN_TO_JSVAL(value));
}

bool JsArray::SetElementInt(int index, int value) {
  return SetElement(index, INT_TO_JSVAL(value));
}

bool JsArray::SetElementDouble(int index, double value) {
  return SetElement(index, DOUBLE_TO_JSVAL(value));
}

bool JsArray::SetElementString(int index, const std::string16& value) {
  JsToken token;
  if (StringToToken(js_context_, value, &token)) {
    return SetElement(index, token);
  } else {
    return false;
  }
}

bool JsArray::SetElementModule(int index, IScriptable* value) {
  if (!array_)
    return false;

  JsToken token;
  if (ModuleToToken(js_context_, value, &token)) {
    return SetElement(index, token);
  } else {
    return false;
  }
}


#elif BROWSER_IE

JsArray::JsArray() : js_context_(NULL) {
}

JsArray::~JsArray() {
}

bool JsArray::SetArray(JsToken value, JsContextPtr context) {
  if (value.vt != VT_DISPATCH) return false;

  array_ = value;

  return true;
}

bool JsArray::GetLength(int *length) const {
  if (array_.vt != VT_DISPATCH) return false;

  CComVariant out;
  if (FAILED(ActiveXUtils::GetDispatchProperty(array_.pdispVal,
                                               STRING16(L"length"),
                                               &out))) {
    return false;
  }
  if (out.vt != VT_I4) return false;

  *length = out.lVal;

  return true;
}

bool JsArray::GetElement(int index, JsScopedToken *out) const {
  if (array_.vt != VT_DISPATCH) return false;

  std::string16 name = IntegerToString16(index);

  HRESULT hr = ActiveXUtils::GetDispatchProperty(array_.pdispVal,
                                                 name.c_str(),
                                                 out);
  if (SUCCEEDED(hr)) {
    return true;
  } else if (hr == DISP_E_UNKNOWNNAME) {
    // There's no value at this index.
    int length;
    if (GetLength(&length)) {
      if (index < length) {
        // If the index is valid, then this element is undefined.
        out->Clear();
        return true;
      }
    }
  }
  return false;
}

bool JsArray::SetElement(int index, const JsScopedToken& in) {
  if (array_.vt != VT_DISPATCH)
    return false;

  std::string16 name(IntegerToString16(index));
  HRESULT hr = ActiveXUtils::AddAndSetDispatchProperty(
    array_.pdispVal, name.c_str(), &in);
  return SUCCEEDED(hr);
}

bool JsArray::SetElementBool(int index, bool value) {
  return SetElement(index, CComVariant(value));
}

bool JsArray::SetElementInt(int index, int value) {
  return SetElement(index, CComVariant(value));
}

bool JsArray::SetElementDouble(int index, double value) {
  return SetElement(index, CComVariant(value));
}

bool JsArray::SetElementString(int index, const std::string16& value) {
  return SetElement(index, CComVariant(value.c_str()));
}

bool JsArray::SetElementModule(int index, IScriptable* value) {
  return SetElement(index, CComVariant(value));
}

#elif BROWSER_NPAPI

JsArray::JsArray() : js_context_(NULL) {
  VOID_TO_NPVARIANT(array_);
}

JsArray::~JsArray() {
}

bool JsArray::SetArray(JsToken value, JsContextPtr context) {
  // check that it's an array (can only test that it has a length property).
  NPIdentifier length_id = NPN_GetStringIdentifier("length");
  NPVariant np_length;
  if (!NPVARIANT_IS_OBJECT(value) ||
      !NPN_GetProperty(context, NPVARIANT_TO_OBJECT(value), length_id,
                       &np_length)) {
    return false;
  }

  array_ = value;
  js_context_ = context;
  return true;
}

bool JsArray::GetLength(int *length) const {
  if (!NPVARIANT_IS_OBJECT(array_)) return false;

  NPObject *array = NPVARIANT_TO_OBJECT(array_);

  NPIdentifier length_id = NPN_GetStringIdentifier("length");

  NPVariant np_length;
  if (!NPN_GetProperty(js_context_, array, length_id, &np_length)) return false;

  return JsTokenToInt(np_length, js_context_, length);
}

bool JsArray::GetElement(int index, JsScopedToken *out) const {
  if (!NPVARIANT_IS_OBJECT(array_)) return false;

  NPObject *array = NPVARIANT_TO_OBJECT(array_);

  NPIdentifier index_id = NPN_GetIntIdentifier(index);

  if (!NPN_GetProperty(js_context_, array, index_id, out)) return false;

  return true;
}

#endif

// Common JsArray functions

bool JsArray::GetElementAsBool(int index, bool *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToBool(token, js_context_, out);
}

bool JsArray::GetElementAsInt(int index, int *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToInt(token, js_context_, out);
}

bool JsArray::GetElementAsDouble(int index, double *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToDouble(token, js_context_, out);
}

bool JsArray::GetElementAsString(int index, std::string16 *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToString(token, js_context_, out);
}

bool JsArray::GetElementAsArray(int index, JsArray *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return out->SetArray(token, js_context_);
}

bool JsArray::GetElementAsObject(int index, JsObject *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return out->SetObject(token, js_context_);
}

bool JsArray::GetElementAsFunction(int index, JsRootedCallback **out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToNewCallback(token, js_context_, out);
}

// SetElement is only available on FF and IE
#if defined(BROWSER_FF) || defined(BROWSER_IE)

bool JsArray::SetElementArray(int index, JsArray* value) {
  return SetElement(index, value->array_);
}

bool JsArray::SetElementObject(int index, JsObject* value) {
  return SetElement(index, value->token());
}

bool JsArray::SetElementFunction(int index, JsRootedCallback* value) {
  return SetElement(index, value->token());
}

#endif

// Browser specific JsObject functions.
#if BROWSER_FF

JsObject::JsObject() : js_context_(NULL), js_object_(0) {
}

JsObject::~JsObject() {
  if (js_object_ && JSVAL_IS_GCTHING(js_object_))
    JS_RemoveRoot(js_context_, &js_object_);
}

bool JsObject::SetObject(JsToken value, JsContextPtr context) {
  if (JSVAL_IS_OBJECT(value)) {
    if (js_object_ && JSVAL_IS_GCTHING(js_object_))
      JS_RemoveRoot(js_context_, &js_object_);
    js_object_ = value;
    js_context_ = context;
    if (JSVAL_IS_GCTHING(js_object_))
      JS_AddRoot(js_context_, &js_object_);
    return true;
  }

  return false;
}

bool JsObject::GetProperty(const std::string16 &name,
                           JsScopedToken *out) const {
  if (!js_object_) return false;

  return JS_GetUCProperty(js_context_, JSVAL_TO_OBJECT(js_object_),
                          reinterpret_cast<const jschar*>(name.c_str()),
                          name.length(), out) == JS_TRUE;
}

bool JsObject::SetProperty(const std::string16 &name, const JsToken &value) {
  if (!js_object_) {
    LOG(("Specified object is not initialized."));
    return false;
  }

  std::string name_utf8;
  if (!String16ToUTF8(name.c_str(), &name_utf8)) {
    LOG(("Could not convert property name to utf8."));
    return false;
  }

  JSBool result = JS_DefineProperty(js_context_,
                                    JSVAL_TO_OBJECT(js_object_),
                                    name_utf8.c_str(), value,
                                    nsnull, nsnull, // getter, setter
                                    JSPROP_ENUMERATE);
  if (!result) {
    LOG(("Could not define property."));
    return false;
  }

  return true;
}

bool JsObject::SetPropertyBool(const std::string16& name, bool value) {
  return SetProperty(name, BOOLEAN_TO_JSVAL(value));
}

bool JsObject::SetPropertyInt(const std::string16 &name, int value) {
  return SetProperty(name, INT_TO_JSVAL(value));
}

bool JsObject::SetPropertyDouble(const std::string16& name, double value) {
  return SetProperty(name, DOUBLE_TO_JSVAL(value));
}

bool JsObject::SetPropertyString(const std::string16 &name,
                                 const std::string16 &value) {
  JsToken token;
  if (StringToToken(js_context_, value, &token)) {
    return SetProperty(name, token);
  } else {
    return false;
  }
}

bool JsObject::SetPropertyModule(const std::string16& name,
                                 IScriptable* value) {
  JsToken token;
  if (ModuleToToken(js_context_, value, &token)) {
    return SetProperty(name, token);
  } else {
    return false;
  }
}

#elif BROWSER_IE

JsObject::JsObject() : js_context_(NULL) {
}

JsObject::~JsObject() {
}

bool JsObject::SetObject(JsToken value, JsContextPtr context) {
  if (value.vt != VT_DISPATCH) return false;

  js_object_ = value;

  return true;
}

bool JsObject::GetProperty(const std::string16 &name,
                           JsScopedToken *out) const {
  if (js_object_.vt != VT_DISPATCH) return false;

  return SUCCEEDED(ActiveXUtils::GetDispatchProperty(js_object_.pdispVal,
                                                     name.c_str(),
                                                     out));
}

bool JsObject::SetProperty(const std::string16 &name, const JsToken &value) {
  if (js_object_.vt != VT_DISPATCH) { return false; }

  HRESULT hr = ActiveXUtils::AddAndSetDispatchProperty(
    js_object_.pdispVal, name.c_str(), &value);
  return SUCCEEDED(hr);
}

bool JsObject::SetPropertyBool(const std::string16& name, bool value) {
  return SetProperty(name, CComVariant(value));
}

bool JsObject::SetPropertyInt(const std::string16 &name, int value) {
  return SetProperty(name, CComVariant(value));
}

bool JsObject::SetPropertyDouble(const std::string16& name, double value) {
  return SetProperty(name, CComVariant(value));
}

bool JsObject::SetPropertyString(const std::string16 &name,
                                 const std::string16 &value) {
  return SetProperty(name, CComVariant(value.c_str()));
}

bool JsObject::SetPropertyModule(const std::string16& name,
                                 IScriptable* value) {
  return SetProperty(name, CComVariant(value));
}

#elif BROWSER_NPAPI

JsObject::JsObject() : js_context_(NULL) {
  VOID_TO_NPVARIANT(js_object_);
}

JsObject::~JsObject() {
}

bool JsObject::SetObject(JsToken value, JsContextPtr context) {
  if (NPVARIANT_IS_OBJECT(value)) {
    js_object_ = value;
    js_context_ = context;
    return true;
  }

  return false;
}

bool JsObject::GetProperty(const std::string16 &name,
                           JsScopedToken *out) const {
  if (!NPVARIANT_IS_OBJECT(js_object_)) return false;

  std::string name_utf8;
  if (!String16ToUTF8(name.c_str(), &name_utf8)) return false;

  NPObject *object = NPVARIANT_TO_OBJECT(js_object_);
  NPIdentifier name_id = NPN_GetStringIdentifier(name_utf8.c_str());

  return NPN_GetProperty(js_context_, object, name_id, out);
}

bool JsObject::SetProperty(const std::string16 &name, const JsToken &value) {
  if (!NPVARIANT_IS_OBJECT(js_object_)) { return false; }

  std::string name_utf8;
  if (!String16ToUTF8(name.c_str(), &name_utf8)) { return false; }

  NPObject *np_object = NPVARIANT_TO_OBJECT(js_object_);
  NPIdentifier np_name = NPN_GetStringIdentifier(name_utf8.c_str());
  return NPN_SetProperty(js_context_, np_object, np_name, &value);
}

bool JsObject::SetPropertyString(const std::string16 &name,
                                 const std::string16 &value) {
  return SetProperty(name, ScopedNPVariant(value.c_str()));
}

bool JsObject::SetPropertyInt(const std::string16 &name, int value) {
  return SetProperty(name, ScopedNPVariant(value));
}

#endif

// Common JsObject functions

bool JsObject::GetPropertyAsBool(const std::string16 &name, bool *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToBool(token, js_context_, out);
}

bool JsObject::GetPropertyAsInt(const std::string16 &name, int *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToInt(token, js_context_, out);
}

bool JsObject::GetPropertyAsDouble(const std::string16 &name,
                                   double *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToDouble(token, js_context_, out);
}

bool JsObject::GetPropertyAsString(const std::string16 &name,
                                   std::string16 *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToString(token, js_context_, out);
}

bool JsObject::GetPropertyAsArray(const std::string16 &name,
                                  JsArray *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return out->SetArray(token, js_context_);
}

bool JsObject::GetPropertyAsObject(const std::string16 &name,
                                   JsObject *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return out->SetObject(token, js_context_);
}

bool JsObject::GetPropertyAsFunction(const std::string16 &name,
                                     JsRootedCallback **out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToNewCallback(token, js_context_, out);
}

// SetProperty is only available on FF and IE
#if defined(BROWSER_FF) || defined(BROWSER_IE)

bool JsObject::SetPropertyArray(const std::string16& name, JsArray* value) {
  return SetProperty(name, value->token());
}

bool JsObject::SetPropertyObject(const std::string16& name, JsObject* value) {
  return SetProperty(name, value->token());
}

bool JsObject::SetPropertyFunction(const std::string16& name,
                                   JsRootedCallback* value) {
  return SetProperty(name, value->token());
}

#endif

#if defined(BROWSER_NPAPI) || defined(BROWSER_IE) || defined(BROWSER_FF)

// Given a JsToken, extract it into a JsArgument.  Object pointers are weak
// references (ref count is not increased).
static bool ConvertTokenToArgument(JsCallContext *context,
                                   const JsToken &variant,
                                   JsArgument *param) {
  switch (param->type) {
    case JSPARAM_BOOL: {
      bool *value = static_cast<bool *>(param->value_ptr);
      if (!JsTokenToBool(variant, context->js_context(), value)) {
        // TODO(aa): These errors should indicate which argument index had the
        // wrong type.
        context->SetException(
            STRING16(L"Invalid argument type: expected bool."));
        return false;
      }
      break;
    }
    case JSPARAM_INT: {
      int *value = static_cast<int *>(param->value_ptr);
      if (!JsTokenToInt(variant, context->js_context(), value)) {
        context->SetException(
            STRING16(L"Invalid argument type: expected int."));
        return false;
      }
      break;
    }
    case JSPARAM_DOUBLE: {
      double *value = static_cast<double *>(param->value_ptr);
      if (!JsTokenToDouble(variant, context->js_context(), value)) {
        context->SetException(
            STRING16(L"Invalid argument type: expected double."));
        return false;
      }
      break;
    }
    case JSPARAM_OBJECT: {
      JsObject *value = static_cast<JsObject *>(param->value_ptr);
      if (!value->SetObject(variant, context->js_context())) {
        context->SetException(
            STRING16(L"Invalid argument type: expected object."));
        return false;
      }
      break;
    }
    case JSPARAM_ARRAY: {
      JsArray *value = static_cast<JsArray *>(param->value_ptr);
      if (!value->SetArray(variant, context->js_context())) {
        context->SetException(
            STRING16(L"Invalid argument type: expected array."));
        return false;
      }
      break;
    }
    case JSPARAM_FUNCTION: {
      JsRootedCallback **value =
          static_cast<JsRootedCallback **>(param->value_ptr);
      if (!JsTokenToNewCallback(variant, context->js_context(), value)) {
        context->SetException(
            STRING16(L"Invalid argument type: expected function."));
        return false;
      }
      break;
    }
    case JSPARAM_STRING16: {
      std::string16 *value = static_cast<std::string16 *>(param->value_ptr);
      if (!JsTokenToString(variant, context->js_context(), value)) {
        context->SetException(
            STRING16(L"Invalid argument type: expected string."));
        return false;
      }
      break;
    }
    default:
      assert(false);
      return false;
  }

  return true;
}

#endif

#if BROWSER_FF

bool JsTokenToBool(JsToken t, JsContextPtr cx, bool *out) {
  if (!JSVAL_IS_BOOLEAN(t)) { return false; }

  JSBool bool_value;
  if (!JS_ValueToBoolean(cx, t, &bool_value)) { return false; }
  *out = (bool_value == JS_TRUE);
  return true;
}

bool JsTokenToInt(JsToken t, JsContextPtr cx, int *out) {
  if (!JSVAL_IS_INT(t)) { return false; }

  int32 i32_value;
  if (!JS_ValueToECMAInt32(cx, t, &i32_value)) { return false; }
  *out = i32_value;
  return true;
}

bool JsTokenToDouble(JsToken t, JsContextPtr cx, double *out) {
  if (!JSVAL_IS_DOUBLE(t)) { return false; }

  jsdouble dbl_value;
  if (!JS_ValueToNumber(cx, t, &dbl_value)) { return false; }
  *out = dbl_value;
  return true;
}

bool JsTokenToString(JsToken t, JsContextPtr cx, std::string16 *out) {
  // for optional args, we want JS null to act like the default _typed_ value
  if (JSVAL_IS_NULL(t)) {
    out->clear();
    return true;
  }

  if (!JSVAL_IS_STRING(t)) { return false; }

  JSString *js_str = JS_ValueToString(cx, t);
  if (!js_str) { return false; }

  out->assign(reinterpret_cast<const char16 *>(JS_GetStringChars(js_str)),
              JS_GetStringLength(js_str));
  return true;
}

bool JsTokenToNewCallback(JsToken t, JsContextPtr cx, JsRootedCallback **out) {
  // We allow null or undefined rooted callbacks but not non-function values.
  if (!JsTokenIsNullOrUndefined(t)) {
    JSObject *obj = JSVAL_TO_OBJECT(t);
    if (!JS_ObjectIsFunction(cx, obj)) { return false; }
  }

  *out = new JsRootedCallback(cx, t);
  return true;
}

bool JsTokenIsNullOrUndefined(JsToken t) {
  return JSVAL_IS_NULL(t) || JSVAL_IS_VOID(t); // null or undefined
}

bool JsTokenIsObject(JsToken t) {
  return JSVAL_IS_OBJECT(t) && !JSVAL_IS_NULL(t); // JSVAL_IS_OBJECT returns
                                                  // true for <null>.
}

#elif BROWSER_IE

bool JsTokenToBool(JsToken t, JsContextPtr cx, bool *out) {
  if (t.vt != VT_BOOL) { return false; }
  *out = (t.boolVal == VARIANT_TRUE);
  return true;
}

bool JsTokenToInt(JsToken t, JsContextPtr cx, int *out) {
  if (t.vt != VT_I4) { return false; }
  *out = t.lVal;
  return true;
}

bool JsTokenToDouble(JsToken t, JsContextPtr cx, double *out) {
  if (t.vt != VT_R8) { return false; }
  *out = t.dblVal;
  return true;
}

bool JsTokenToString(JsToken t, JsContextPtr cx, std::string16 *out) {
  if (t.vt != VT_BSTR) { return false; }
  out->assign(t.bstrVal);
  return true;
}

bool JsTokenToNewCallback(JsToken t, JsContextPtr cx, JsRootedCallback **out) {
  // We allow null or undefined rooted callbacks but not non-function values.
  if (!JsTokenIsNullOrUndefined(t)) {
    if (t.vt != VT_DISPATCH) { return false; }
  }

  *out = new JsRootedCallback(cx, t);
  return true;
}

bool JsTokenIsNullOrUndefined(JsToken t) {
  return t.vt == VT_NULL || t.vt == VT_EMPTY; // null or undefined
}

bool JsTokenIsObject(JsToken t) {
  return t.vt == VT_DISPATCH;
}


#elif BROWSER_NPAPI

bool JsTokenToBool(JsToken t, JsContextPtr cx, bool *out) {
  if (!NPVARIANT_IS_BOOLEAN(t)) { return false; }
  *out = NPVARIANT_TO_BOOLEAN(t);
  return true;
}

bool JsTokenToInt(JsToken t, JsContextPtr cx, int *out) {
  if (NPVARIANT_IS_INT32(t)) {
    *out = NPVARIANT_TO_INT32(t);
    return true;
  } else if (NPVARIANT_IS_DOUBLE(t)) {
    double d = NPVARIANT_TO_DOUBLE(t);
    if (d >= INT_MIN && d <= INT_MAX) {
      *out = static_cast<int>(d);
      return true;
    }
  }
  return false;
}

bool JsTokenToDouble(JsToken t, JsContextPtr cx, double *out) {
  if (NPVARIANT_IS_DOUBLE(t)) {
    *out = NPVARIANT_TO_DOUBLE(t);
    return true;
  } else if (NPVARIANT_IS_INT32(t)) {
    *out = static_cast<double>(NPVARIANT_TO_INT32(t));
    return true;
  }

  return false;
}

bool JsTokenToString(JsToken t, JsContextPtr cx, std::string16 *out) {
  if (!NPVARIANT_IS_STRING(t)) { return false; }
  NPString str = NPVARIANT_TO_STRING(t);
  if (GetNPStringUTF8Characters(str) == 0) {
    // TODO(mpcomplete): find out if UTF8ToString16 can be changed to return
    // true in this case.
    out->clear();
    return true;
  }
  return UTF8ToString16(GetNPStringUTF8Characters(str),
                        GetNPStringUTF8Length(str), out);
}

bool JsTokenToNewCallback(JsToken t, JsContextPtr cx, JsRootedCallback **out) {
  // We allow null or undefined rooted callbacks but not non-function values.
  if (!JsTokenIsNullOrUndefined(t)) {
    // TODO(mpcomplete): is there any way to check if an object is a function?
    if (!NPVARIANT_IS_OBJECT(t)) { return false; }
  }

  *out = new JsRootedCallback(cx, t);
  return true;
}

bool JsTokenIsNullOrUndefined(JsToken t) {
  return NPVARIANT_IS_NULL(t) || NPVARIANT_IS_VOID(t);
}

bool JsTokenIsObject(JsToken t) {
  return NPVARIANT_IS_OBJECT(t);
}


// ScopedNPVariant functions.
void ScopedNPVariant::Reset() {
  NPN_ReleaseVariantValue(this);
  VOID_TO_NPVARIANT(*this);
}

void ScopedNPVariant::Reset(int value) {
  Reset();
  INT32_TO_NPVARIANT(value, *this);
}

void ScopedNPVariant::Reset(double value) {
  Reset();
  DOUBLE_TO_NPVARIANT(value, *this);
}

void ScopedNPVariant::Reset(const char *value) {
  Reset();
  NPString npstr = NPN_StringDup(value, strlen(value));
  NPSTRING_TO_NPVARIANT(npstr, *this);
}

void ScopedNPVariant::Reset(const char16 *value) {
  Reset();
  NPString npstr = NPN_StringDup(value, std::char_traits<char16>::length(value));
  NPSTRING_TO_NPVARIANT(npstr, *this);
}

void ScopedNPVariant::Reset(NPObject *value) {
  Reset();
  OBJECT_TO_NPVARIANT(value, *this);
  NPN_RetainObject(value);
}

void ScopedNPVariant::Reset(const NPVariant &value) {
  if (static_cast<NPVariant*>(this) == &value) return;

  Reset();
  memcpy(this, &value, sizeof(value));
  if (NPVARIANT_IS_OBJECT(value)) {
    NPN_RetainObject(NPVARIANT_TO_OBJECT(*this));
  } else if (NPVARIANT_IS_STRING(value)) {
    NPString npstr = NPN_StringDup(NPVARIANT_TO_STRING(*this));
    NPSTRING_TO_NPVARIANT(npstr, *this);
  }
}

void ScopedNPVariant::Release() {
  VOID_TO_NPVARIANT(*this);
}

#endif

// Browser-specific JsCallContext functions.
#if BROWSER_FF

void ConvertJsParamToToken(const JsParamToSend &param,
                           JsContextPtr context, JsScopedToken *token) {
  switch (param.type) {
    case JSPARAM_BOOL: {
      const bool *value = static_cast<const bool *>(param.value_ptr);
      *token = *value ? JSVAL_TRUE : JSVAL_FALSE;
      break;
    }
    case JSPARAM_INT: {
      const int *value = static_cast<const int *>(param.value_ptr);
      *token = INT_TO_JSVAL(*value);
      break;
    }
    case JSPARAM_DOUBLE: {
      const double *value = static_cast<const double *>(param.value_ptr);
      *token = DOUBLE_TO_JSVAL(*value);
      break;
    }
    case JSPARAM_STRING16: {
      const std::string16 *value = static_cast<const std::string16 *>(
                                                   param.value_ptr);
      // TODO(cprince): Does this string copy get freed?
      JSString *js_string = JS_NewUCStringCopyZ(
          context,
          reinterpret_cast<const jschar *>(value->c_str()));
      *token = STRING_TO_JSVAL(js_string);
      break;
    }
    case JSPARAM_OBJECT: {
      const JsObject *value = static_cast<const JsObject *>(param.value_ptr);
      *token = value->token();
      break;
    }
    case JSPARAM_ARRAY: {
      const JsArray *value = static_cast<const JsArray *>(param.value_ptr);
      *token = value->token();
      break;
    }
    case JSPARAM_FUNCTION: {
      const JsRootedCallback *value = static_cast<const JsRootedCallback *>(
                                                   param.value_ptr);
      *token = value->token();
      break;
    }
    case JSPARAM_MODULE: {
#if 0
      const ModuleImplBaseClass *value =
          static_cast<const ModuleImplBaseClass *>(param.value_ptr);
      *token = value->GetWrapperToken();
#endif
      assert(false);  // TODO(mpcomplete): implement GetWrapperToken().
      break;
    }
    case JSPARAM_NULL:
      *token = JSVAL_NULL;
      break;
    default:
      assert(false);
  }
}

// This code is shared between JsCallContext and JsParamFetcher for FF.
// It initialises all the parameters which are members of the respective
// classes, execept obj which is an input parameter.
static void GetContextAndArgs(ModuleImplBaseClass *obj,
                              JsContextPtr *js_context,
                              int *js_argc,
                              JsToken **js_argv,
                              nsCOMPtr<nsIXPConnect> *xpc,
                              nsCOMPtr<nsIXPCNativeCallContext> *ncc) {
  if (obj->EnvIsWorker()) {
    *js_context = obj->EnvPageJsContext();
    *js_argc = obj->JsWorkerGetArgc();
    *js_argv = obj->JsWorkerGetArgv();
  } else {
    // In the main thread use the caller's current JS context, NOT the context
    // where 'obj' was created.  These can be different!  Each frame has its
    // own JS context, and code can reference 'top.OtherFrame.FooObject'.
    nsresult nr;
    *xpc = do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr);
    if (*xpc && NS_SUCCEEDED(nr)) {
      nr = (*xpc)->GetCurrentNativeCallContext(getter_AddRefs(*ncc));
      if (*ncc && NS_SUCCEEDED(nr)) {
        (*ncc)->GetJSContext(js_context);
        PRUint32 argc;
        (*ncc)->GetArgc(&argc);
        *js_argc = static_cast<int>(argc);
        (*ncc)->GetArgvPtr(js_argv);
      }
    }
  }
}


JsCallContext::JsCallContext(ModuleImplBaseClass* obj)
    : js_context_(NULL), is_exception_set_(false),
      argc_(0), argv_(NULL), retval_(NULL),
      js_runner_(obj->GetJsRunner()),
      xpc_(NULL), ncc_(NULL) {
  GetContextAndArgs(obj, &js_context_, &argc_, &argv_, &xpc_, &ncc_);
}


JsCallContext::JsCallContext(JsContextPtr cx, JsRunnerInterface *js_runner,
                             int argc, JsToken *argv, JsToken *retval)
    : js_context_(cx), is_exception_set_(false),
      argc_(argc), argv_(argv), retval_(retval),
      js_runner_(js_runner),
      xpc_(NULL), ncc_(NULL) {}


void JsCallContext::SetReturnValue(JsParamType type, const void *value_ptr) {
  // There is only a valid retval_ if the JS caller is expecting a return value.
  if (retval_) {
    JsParamToSend retval = { type, value_ptr };
    ConvertJsParamToToken(retval, js_context(), retval_);
  }
}

void JsCallContext::SetException(const std::string16 &message) {
  assert(!message.empty());
  JsSetException(js_context_, js_runner_, message.c_str(),
                 ncc_ != NULL ? true : false); // notify_native_call_context
}

#elif BROWSER_IE

void ConvertJsParamToToken(const JsParamToSend &param,
                           JsContextPtr context, CComVariant *token) {
  switch (param.type) {
    case JSPARAM_BOOL: {
      const bool *value = static_cast<const bool *>(param.value_ptr);
      *token = *value;  // CComVariant understands 'bool'
      break;
    }
    case JSPARAM_INT: {
      const int *value = static_cast<const int *>(param.value_ptr);
      *token = *value;  // CComVariant understands 'int'
      break;
    }
    case JSPARAM_DOUBLE: {
      const double *value = static_cast<const double *>(param.value_ptr);
      *token = *value;  // CComVariant understands 'double'
      break;
    }
    case JSPARAM_STRING16: {
      const std::string16 *value = static_cast<const std::string16 *>(
                                                   param.value_ptr);
      *token = value->c_str();  // copies 'wchar*' for us
      break;
    }
    case JSPARAM_OBJECT: {
      const JsObject *value = static_cast<const JsObject *>(param.value_ptr);
      *token = value->token();
      break;
    }
    case JSPARAM_ARRAY: {
      const JsArray *value = static_cast<const JsArray *>(param.value_ptr);
      *token = value->token();
      break;
    }
    case JSPARAM_FUNCTION: {
      const JsRootedCallback *value = static_cast<const JsRootedCallback *>(
                                                   param.value_ptr);
      *token = value->token();
      break;
    }
    case JSPARAM_MODULE: {
#if 0
      const ModuleImplBaseClass *value =
          static_cast<const ModuleImplBaseClass *>(param.value_ptr);
      *token = value->GetWrapperToken();
#endif
      assert(false);  // TODO(mpcomplete): implement GetWrapperToken().
      break;
    }
    case JSPARAM_NULL:
      *token = VT_NULL;
      break;
    default:
      assert(false);
  }
}

int JsCallContext::GetArguments(int output_argc, JsArgument *output_argv) {
  bool has_optional = false;

  for (int i = 0; i < output_argc; ++i) {
    has_optional |= output_argv[i].requirement == JSPARAM_OPTIONAL;
    if (output_argv[i].requirement == JSPARAM_REQUIRED)
      assert(!has_optional);  // should not have required arg after optional

    if (i >= static_cast<int>(disp_params_->cArgs)) {
      // Out of arguments
      if (output_argv[i].requirement == JSPARAM_REQUIRED) {
        std::string16 msg;
        msg += STRING16(L"Required argument ");
        msg += IntegerToString16(i + 1);
        msg += STRING16(L" is missing.");
        SetException(msg.c_str());
      }

      // If failed on index [N], then N args succeeded
      return i;
    }

    // args in input array are in reverse order
    int input_arg_index = disp_params_->cArgs - i - 1;
    if (!ConvertTokenToArgument(this, disp_params_->rgvarg[input_arg_index],
                                &output_argv[i]))
      return i;
  }

  return output_argc;
}

void JsCallContext::SetReturnValue(JsParamType type, const void *value_ptr) {
  // There is only a valid retval_ if the javascript caller is expecting a
  // return value.
  if (retval_) {
    JsParamToSend retval = { type, value_ptr };
    JsScopedToken scoped_retval;
    ConvertJsParamToToken(retval, js_context(), &scoped_retval);

    // In COM, return values are released by the caller.
    scoped_retval.Detach(retval_);
  }
}

void JsCallContext::SetException(const std::string16 &message) {
  assert(!message.empty());
  if (!exception_info_) {
#if DEBUG
    // MSDN says exception_info_ can be null, which seems very unfortunate.
    // Asserting to see if we can find out under what conditions that's true.
    assert(false);
#endif
    return;
  }

  exception_info_->wCode = 1001; // Not used, MSDN says must be > 1000.
  exception_info_->wReserved = 0;
  exception_info_->bstrSource = SysAllocString(PRODUCT_FRIENDLY_NAME);
  exception_info_->bstrDescription = SysAllocString(message.c_str());
  exception_info_->bstrHelpFile = NULL;
  exception_info_->dwHelpContext = 0;
  exception_info_->pvReserved = NULL;
  exception_info_->pfnDeferredFillIn = NULL;
  exception_info_->scode = 0;

  is_exception_set_ = true;
}

#elif BROWSER_NPAPI

void ConvertJsParamToToken(const JsParamToSend &param,
                           JsContextPtr context, JsScopedToken *variant) {
  switch (param.type) {
    case JSPARAM_BOOL: {
      const bool *value = static_cast<const bool *>(param.value_ptr);
      variant->Reset(*value);
      break;
    }
    case JSPARAM_INT: {
      const int *value = static_cast<const int *>(param.value_ptr);
      variant->Reset(*value);
      break;
    }
    case JSPARAM_DOUBLE: {
      const double *value = static_cast<const double *>(param.value_ptr);
      variant->Reset(*value);
      break;
    }
    case JSPARAM_MODULE: {
      const ModuleImplBaseClass *value =
          static_cast<const ModuleImplBaseClass *>(param.value_ptr);
      variant->Reset(NPVARIANT_TO_OBJECT(value->GetWrapperToken()));
      break;
    }
    case JSPARAM_OBJECT: {
      const JsObject *value = static_cast<const JsObject *>(param.value_ptr);
      variant->Reset(NPVARIANT_TO_OBJECT(value->token()));
      break;
    }
    case JSPARAM_ARRAY: {
      const JsArray *value = static_cast<const JsArray *>(param.value_ptr);
      variant->Reset(NPVARIANT_TO_OBJECT(value->token()));
      break;
    }
    case JSPARAM_FUNCTION: {
      const JsRootedCallback *value =
          static_cast<const JsRootedCallback *>(param.value_ptr);
      variant->Reset(NPVARIANT_TO_OBJECT(value->token()));
      break;
    }
    case JSPARAM_STRING16: {
      const std::string16 *value = static_cast<const std::string16 *>(
                                                   param.value_ptr);
      variant->Reset(value->c_str());
      break;
    }
    case JSPARAM_NULL: {
      variant->Reset();  // makes it VOID (undefined).
      NULL_TO_NPVARIANT(*variant);
      break;
    }
    default:
      assert(false);
  }
}
void JsCallContext::SetReturnValue(JsParamType type, const void *value_ptr) {
  assert(value_ptr != NULL || type == JSPARAM_NULL);

  JsParamToSend retval = { type, value_ptr };
  ScopedNPVariant np_retval;
  ConvertJsParamToToken(retval, js_context(), &np_retval);
  *retval_ = np_retval;

  // In NPAPI, return values from callbacks are released by the browser.
  // Therefore, we give up ownership of this variant without releasing it.
  np_retval.Release();
}

void JsCallContext::SetException(const std::string16 &message) {
  assert(!message.empty());
  // TODO(cprince): remove #ifdef and string conversion after refactoring LOG().
#ifdef DEBUG
  std::string message_ascii;
  String16ToUTF8(message.c_str(), message.length(), &message_ascii);
  LOG((message_ascii.c_str()));
#endif

  is_exception_set_ = true;
  
#ifdef BROWSER_WEBKIT
  ThrowWebKitException(message);
#else
  std::string message_utf8;
  if (!String16ToUTF8(message.data(), message.length(), &message_utf8))
    message_utf8 = "Unknown Gears Error";  // better to throw *something*

  NPN_SetException(object_, message_utf8.c_str());
#endif
}

#endif

#if defined(BROWSER_FF) || defined(BROWSER_NPAPI)
int JsCallContext::GetArguments(int output_argc, JsArgument *output_argv) {
  bool has_optional = false;

  for (int i = 0; i < output_argc; ++i) {
    has_optional |= output_argv[i].requirement == JSPARAM_OPTIONAL;
    if (output_argv[i].requirement == JSPARAM_REQUIRED)
      assert(!has_optional);  // should not have required arg after optional

    if (i >= argc_) {
      // Out of arguments
      if (output_argv[i].requirement == JSPARAM_REQUIRED) {
        std::string16 msg;
        msg += STRING16(L"Required argument ");
        msg += IntegerToString16(i + 1);
        msg += STRING16(L" is missing.");
        SetException(msg.c_str());
      }

      // If failed on index [N], then N args succeeded
      return i;
    }

    if (!ConvertTokenToArgument(this, argv_[i], &output_argv[i]))
      return i;
  }

  return output_argc;
}
#endif


//-----------------------------------------------------------------------------
#if BROWSER_FF  // the rest of this file only applies to Firefox, for now

JsParamFetcher::JsParamFetcher(ModuleImplBaseClass *obj) {
  GetContextAndArgs(obj, &js_context_, &js_argc_, &js_argv_, &xpc_, &ncc_);
}


// We need has_mysterious_retval for correct lookups, since sometimes retvals
// are required parameters and count toward js_argc_.
//
// Consider a Firefox function with optional params and a string retval.  XPIDL
// makes the retval the final param.  So params[0] might point at an optional
// param or the required retval.  We need has_mysterious_retval to know which it
// is.
//
// TODO(aa): has_mysterious_retval is named thusly because we have observed that
// sometimes other types than strings cause the behavior described above. This
// needs to be investigated and encapsulated.
bool JsParamFetcher::IsOptionalParamPresent(int i, bool has_mysterious_retval) {
  // Return false if there weren't enough params.
  if (i >= GetCount(has_mysterious_retval)) {
    return false;
  }

  // Also return false if the value was <undefined> or <null>.
  JsToken token = js_argv_[i];
  if (JSVAL_IS_VOID(token) || JSVAL_IS_NULL(token)) {
    return false;
  }

  return true;
}

bool JsParamFetcher::GetAsInt(int i, int *out) {
  // 'has_mysterious_retval' can cause js_argc_ to be one larger than the "real"
  // number of parameters.  So comparing against js_argc_ here doesn't ensure we
  // are reading a real parameter.  However, it does ensure we don't read past
  // the end of the js_argv_ array.
  if (i >= js_argc_) return false;
  JsToken t = js_argv_[i];
  return JsTokenToInt(t, js_context_, out);
}

bool JsParamFetcher::GetAsString(int i, std::string16 *out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()
  JsToken t = js_argv_[i];
  return JsTokenToString(t, js_context_, out);
}

bool JsParamFetcher::GetAsArray(int i, JsArray *out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()

  return out->SetArray(js_argv_[i], js_context_);
}

bool JsParamFetcher::GetAsObject(int i, JsObject *out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()

  return out->SetObject(js_argv_[i], js_context_);
}

bool JsParamFetcher::GetAsNewRootedCallback(int i, JsRootedCallback **out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()

  // We allow null or undefined rooted callbacks but not non-function values.
  if (!JsTokenIsNullOrUndefined(js_argv_[i])) {
    JSObject *obj = JSVAL_TO_OBJECT(js_argv_[i]);
    if (!JS_ObjectIsFunction(GetContextPtr(), obj)) { return false; }
  }

  *out = new JsRootedCallback(GetContextPtr(), js_argv_[i]);
  return true;
}

JsNativeMethodRetval JsSetException(JsContextPtr cx,
                                    JsRunnerInterface *js_runner,
                                    const char16 *message,
                                    bool notify_native_call_context) {
  JsNativeMethodRetval retval = NS_ERROR_FAILURE;

  // When Gears is running under XPConnect, we need to tell it that we threw an
  // exception.
  // TODO_REMOVE_NSISUPPORTS: Remove notify_native_call_context and this branch.
  if (notify_native_call_context) {
    nsresult nr;
    nsCOMPtr<nsIXPConnect> xpc;
    xpc = do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr);
    if (!xpc || NS_FAILED(nr)) { return retval; }

    nsCOMPtr<nsIXPCNativeCallContext> ncc;
    nr = xpc->GetCurrentNativeCallContext(getter_AddRefs(ncc));
    if (!ncc || NS_FAILED(nr)) { return retval; }

    // To make XPConnect display a user-defined exception message, we must
    // call SetExceptionWasThrown AND ALSO return a _success_ error code
    // from our native object.  Otherwise XPConnect will look for an
    // exception object it created, rather than using the one we set below.
    ncc->SetExceptionWasThrown(PR_TRUE);
    retval = NS_OK;
  }

  // First set the exception to any value, in case we fail to create the full
  // exception object below.  Setting any jsval will satisfy the JS engine,
  // we just won't get e.message.  We use INT_TO_JSVAL(1) here for simplicity.
  JS_SetPendingException(cx, INT_TO_JSVAL(1));

  // Create a JS Error object with a '.message' property. The other fields
  // like "lineNumber" and "fileName" are filled in automatically by Firefox
  // based on the top frame of the JS stack. It's important to use an actual
  // Error object so that some tools work correctly. See:
  // http://code.google.com/p/google-gears/issues/detail?id=5
  //
  // Note: JS_ThrowReportedError and JS_ReportError look promising, but they
  // don't quite do what we need.

  scoped_ptr<JsObject> error_object(js_runner->NewObject(STRING16(L"Error")));
  if (!error_object.get()) { return retval; }

  // Note: need JS_SetPendingException to bubble 'catch' in workers.
  JS_SetPendingException(cx, error_object->token());

  bool success = error_object->SetPropertyString(STRING16(L"message"), message);
  if (!success) { return retval; }

  return retval;
}


bool RootJsToken(JsContextPtr cx, JsToken t) {
  // In SpiderMonkey, the garbage collector won't delete items reachable
  // from any "root".  A root is a _pointer_ to a jsval (or to certain other
  // data types).
  //
  // Our code treats jsval like a handle, not an allocated object.  So taking
  // the address of a jsval usually gives a transient value.  To fix this,
  // RootJsToken allocates a jsval copy and uses _that_ pointer for the root.
  assert(JSVAL_IS_GCTHING(t));
  jsval *root_container = new jsval(t);
  JSBool js_ok = JS_AddRoot(cx, root_container);
  return js_ok ? true : false;
}


#endif // BROWSER_FF
//-----------------------------------------------------------------------------
