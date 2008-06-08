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

#include <math.h>

#include "gears/base/common/base_class.h"
#include "gears/base/common/basictypes.h"  // for isnan
#include "gears/base/common/js_marshal.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/js_types.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

#if BROWSER_FF
#include "gears/base/common/js_runner_ff_marshaling.h"
#include "gears/base/firefox/dom_utils.h"
#elif BROWSER_IE
#include <dispex.h>
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#include "gears/base/ie/module_wrapper.h"
#include "genfiles/interfaces.h"
#elif BROWSER_NPAPI
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/npapi/module_wrapper.h"
#include "gears/base/npapi/np_utils.h"
#include "gears/base/npapi/scoped_npapi_handles.h"
#elif BROWSER_SAFARI
#include "gears/base/safari/browser_utils.h"
#endif

static inline bool DoubleIsInt64(double val) {
  return val >= kint64min && val <= kint64max && floor(val) == val;
}

static inline bool DoubleIsInt32(double val) {
  return val >= kint32min && val <= kint32max && floor(val) == val;
}


// Special conversion functions for FireFox
#if BROWSER_FF

static bool ComModuleToToken(JsContextPtr context,
                             IScriptable* in,
                             JsToken* out) {
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

static bool JsTokenToComModule(JsContextPtr context, JsToken in,
                               IScriptable** out) {
  // TODO(kevinww): make this work in a worker pool
  nsresult nr;
  nsCOMPtr<nsIXPConnect> xpc;
  xpc = do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr);
  if (NS_FAILED(nr))
    return false;

#if BROWSER_FF3
  nsAXPCNativeCallContext *ncc;
  nr = xpc->GetCurrentNativeCallContext(&ncc);
#else
  nsCOMPtr<nsIXPCNativeCallContext> ncc;
  nr = xpc->GetCurrentNativeCallContext(getter_AddRefs(ncc));
#endif
  if (NS_FAILED(nr))
    return false;

  JSObject *obj = JSVAL_TO_OBJECT(in);
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  nr = xpc->GetWrappedNativeOfJSObject(context, obj, getter_AddRefs(wrapper));
  if (NS_FAILED(nr))
    return false;

  nsCOMPtr<nsISupports> isupports = NULL;
  nr = wrapper->GetNative(getter_AddRefs(isupports));
  if (NS_FAILED(nr))
    return false;

  *out = isupports.get();
  (*out)->AddRef();
  return true;
}

bool JsTokenToDispatcherModule(JsContextWrapperPtr context_wrapper,
                               JsContextPtr context,
                               const JsToken in,
                               ModuleImplBaseClass **out) {
  ModuleImplBaseClass *module = context_wrapper->GetModuleFromJsToken(in);
  if (module) {
    *out = module;
    return true;
  }
  return false;
}

#elif BROWSER_IE
static bool JsTokenToComModule(JsContextPtr context, JsToken in,
                               IScriptable** out) {
  if (in.vt == VT_DISPATCH) {
    *out = in.pdispVal;
    return true;
  }
  return false;
}

bool JsTokenToDispatcherModule(JsContextWrapperPtr context_wrapper,
                               JsContextPtr context,
                               const JsToken in,
                               ModuleImplBaseClass **out) {
  if (in.vt == VT_DISPATCH) {
    CComQIPtr<GearsModuleProviderInterface> module_provider(in.pdispVal);
    if (!module_provider) {
      return false;
    }
    VARIANT var;
    HRESULT hr = module_provider->get_moduleWrapper(&var);
    if (FAILED(hr)) {
      return false;
    }
    ModuleWrapper *module_wrapper =
        reinterpret_cast<ModuleWrapper*>(var.byref);
    *out = module_wrapper->GetModuleImplBaseClass();
    return true;
  }
  return false;
}

#elif BROWSER_NPAPI
static bool JsTokenToComModule(JsContextPtr context, JsToken in,
                               IScriptable** out) {
  // TODO(nigeltao): implement.
  return false;
}

bool JsTokenToDispatcherModule(JsContextWrapperPtr context_wrapper,
                               JsContextPtr context,
                               const JsToken in,
                               ModuleImplBaseClass **out) {
  if (!NPVARIANT_IS_OBJECT(in)) {
    return false;
  }
  NPObject *object = NPVARIANT_TO_OBJECT(in);
  if (object->_class != GetNPClass<ModuleWrapper>()) {
    return false;
  }
  *out = static_cast<ModuleWrapper*>(object)->GetModuleImplBaseClass();
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
  // Check that we're initialized.
  assert(JsTokenIsArray(array_, js_context_));

  jsuint array_length;
  if (JS_GetArrayLength(js_context_, JSVAL_TO_OBJECT(array_), &array_length) !=
      JS_TRUE) {
    return false;
  }
  *length = static_cast<int>(array_length);
  return true;
}

bool JsArray::GetElement(int index, JsScopedToken *out) const {
  // Check that we're initialized.
  assert(JsTokenIsArray(array_, js_context_));

  // JS_GetElement sets the token to JS_VOID if we request a non-existent index.
  // This is the correct jsval for JSPARAM_UNDEFINED.
  return JS_GetElement(js_context_, JSVAL_TO_OBJECT(array_),
                       index, out) == JS_TRUE;
}

bool JsArray::SetElement(int index, const JsToken &value) {
  // Check that we're initialized.
  assert(JsTokenIsArray(array_, js_context_));

  return JS_DefineElement(js_context_,
                          JSVAL_TO_OBJECT(array_),
                          index, value,
                          nsnull, nsnull, // getter, setter
                          JSPROP_ENUMERATE) == JS_TRUE;
}

bool JsArray::SetElementBool(int index, bool value) {
  return SetElement(index, BOOLEAN_TO_JSVAL(value));
}

bool JsArray::SetElementInt(int index, int value) {
  return SetElement(index, INT_TO_JSVAL(value));
}

bool JsArray::SetElementDouble(int index, double value) {
  JsToken jsval;
  if (DoubleToJsToken(js_context_, value, &jsval)) {
    return SetElement(index, jsval);
  } else {
    return false;
  }
}

bool JsArray::SetElementString(int index, const std::string16 &value) {
  JsToken token;
  if (StringToJsToken(js_context_, value.c_str(), &token)) {
    return SetElement(index, token);
  } else {
    return false;
  }
}

bool JsArray::SetElementComModule(int index, IScriptable *value) {
  JsToken token;
  if (ComModuleToToken(js_context_, value, &token)) {
    return SetElement(index, token);
  } else {
    return false;
  }
}

bool JsArray::SetElementDispatcherModule(int index,
                                         ModuleImplBaseClass *value) {
  return SetElement(index, value->GetWrapperToken());
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
  // Check that we're initialized.
  assert(JsTokenIsArray(array_, js_context_));

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
  // Check that we're initialized.
  assert(JsTokenIsArray(array_, js_context_));

  std::string16 name = IntegerToString16(index);

  HRESULT hr = ActiveXUtils::GetDispatchProperty(array_.pdispVal,
                                                 name.c_str(),
                                                 out);
  if (SUCCEEDED(hr)) {
    return true;
  } else if (hr == DISP_E_UNKNOWNNAME) {
    // There's no value at this index, so this element is undefined.
    out->Clear();
    return true;
  }
  return false;
}

bool JsArray::SetElement(int index, const JsScopedToken &in) {
  // Check that we're initialized.
  assert(JsTokenIsArray(array_, js_context_));

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

bool JsArray::SetElementString(int index, const std::string16 &value) {
  return SetElement(index, CComVariant(value.c_str()));
}

bool JsArray::SetElementComModule(int index, IScriptable *value) {
  return SetElement(index, CComVariant(value));
}

bool JsArray::SetElementDispatcherModule(int index,
                                         ModuleImplBaseClass *value) {
  return SetElement(index, value->GetWrapperToken());
}

#elif BROWSER_NPAPI

JsArray::JsArray() : js_context_(NULL) {
  VOID_TO_NPVARIANT(array_);
}

JsArray::~JsArray() {
}

bool JsArray::SetArray(JsToken value, JsContextPtr context) {
  if (!JsTokenIsArray(value, context)) return false;

  array_ = value;
  js_context_ = context;
  return true;
}

bool JsArray::GetLength(int *length) const {
  // Check that we're initialized.
  assert(JsTokenIsArray(array_, js_context_));

  NPObject *array = NPVARIANT_TO_OBJECT(array_);

  NPIdentifier length_id = NPN_GetStringIdentifier("length");

  NPVariant np_length;
  if (!NPN_GetProperty(js_context_, array, length_id, &np_length)) return false;

  return JsTokenToInt_NoCoerce(np_length, js_context_, length);
}

bool JsArray::GetElement(int index, JsScopedToken *out) const {
  // Check that we're initialized.
  assert(JsTokenIsArray(array_, js_context_));

  NPObject *array = NPVARIANT_TO_OBJECT(array_);

  NPIdentifier index_id = NPN_GetIntIdentifier(index);

  if (!NPN_GetProperty(js_context_, array, index_id, out)) return false;

  return true;
}

bool JsArray::SetElement(int index, const JsScopedToken &in) {
  // Check that we're initialized.
  assert(JsTokenIsArray(array_, js_context_));

  NPObject *array = NPVARIANT_TO_OBJECT(array_);
  NPIdentifier index_id = NPN_GetIntIdentifier(index);
  return NPN_SetProperty(js_context_, array, index_id, &in);
}

bool JsArray::SetElementBool(int index, bool value) {
  return SetElement(index, JsScopedToken(value));
}

bool JsArray::SetElementInt(int index, int value) {
  return SetElement(index, JsScopedToken(value));
}

bool JsArray::SetElementDouble(int index, double value) {
  return SetElement(index, JsScopedToken(value));
}

bool JsArray::SetElementString(int index, const std::string16 &value) {
  return SetElement(index, JsScopedToken(value.c_str()));
}

bool JsArray::SetElementComModule(int index, IScriptable *value) {
  // TODO(cdevries): implement this
  assert(false);
  return false;
}

bool JsArray::SetElementDispatcherModule(int index,
                                         ModuleImplBaseClass *value) {
  // TODO(nigeltao): implement this
  assert(false);
  return false;
}

#endif

// Common JsArray functions

bool JsArray::GetElementAsBool(int index, bool *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToBool_NoCoerce(token, js_context_, out);
}

bool JsArray::GetElementAsInt(int index, int *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToInt_NoCoerce(token, js_context_, out);
}

bool JsArray::GetElementAsDouble(int index, double *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToDouble_NoCoerce(token, js_context_, out);
}

bool JsArray::GetElementAsString(int index, std::string16 *out) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToString_NoCoerce(token, js_context_, out);
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

  return JsTokenToNewCallback_NoCoerce(token, js_context_, out);
}

JsParamType JsArray::GetElementType(int index) const {
  JsScopedToken token;
  if (!GetElement(index, &token)) return JSPARAM_UNKNOWN;

  return JsTokenGetType(token, js_context_);
}

bool JsArray::SetElementArray(int index, JsArray *value) {
  return SetElement(index, value->array_);
}

bool JsArray::SetElementObject(int index, JsObject *value) {
  return SetElement(index, value->token());
}

bool JsArray::SetElementFunction(int index, JsRootedCallback *value) {
  return SetElement(index, value->token());
}

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

bool JsObject::GetPropertyNames(std::vector<std::string16> *out) const {
  // Check that we're initialized.
  assert(JsTokenIsObject(js_object_));

  JSIdArray *ids = JS_Enumerate(js_context_, JSVAL_TO_OBJECT(js_object_));
  for (int i = 0; i < ids->length; i++) {
    jsval property_key;
    if (!JS_IdToValue(js_context_, ids->vector[i], &property_key)) {
      // JS_IdToValue is implemented as a typecast, and should never fail.
      assert(false);
    }
    if (!JSVAL_IS_STRING(property_key)) {
      continue;
    }
    std::string16 property_name;
    if (!JsTokenToString_NoCoerce(property_key, js_context_, &property_name)) {
      continue;
    }
    out->push_back(property_name);
  }
  JS_DestroyIdArray(js_context_, ids);
  return true;
}

bool JsObject::GetProperty(const std::string16 &name,
                           JsScopedToken *out) const {
  // Check that we're initialized.
  assert(JsTokenIsObject(js_object_));

  return JS_GetUCProperty(js_context_, JSVAL_TO_OBJECT(js_object_),
                          reinterpret_cast<const jschar*>(name.c_str()),
                          name.length(), out) == JS_TRUE;
}

bool JsObject::SetProperty(const std::string16 &name, const JsToken &value) {
  // Check that we're initialized.
  assert(JsTokenIsObject(js_object_));

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
  JsToken jsval;
  if (DoubleToJsToken(js_context_, value, &jsval)) {
    return SetProperty(name, jsval);
  } else {
    return false;
  }
}

bool JsObject::SetPropertyString(const std::string16 &name,
                                 const std::string16 &value) {
  JsToken token;
  if (StringToJsToken(js_context_, value.c_str(), &token)) {
    return SetProperty(name, token);
  } else {
    return false;
  }
}

bool JsObject::SetPropertyComModule(const std::string16& name,
                                 IScriptable* value) {
  JsToken token;
  if (ComModuleToToken(js_context_, value, &token)) {
    return SetProperty(name, token);
  } else {
    return false;
  }
}

bool JsObject::SetPropertyDispatcherModule(const std::string16 &name,
                                           ModuleImplBaseClass *value) {
  return SetProperty(name, value->GetWrapperToken());
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

bool JsObject::GetPropertyNames(std::vector<std::string16> *out) const {
  // Check that we're initialized.
  assert(JsTokenIsObject(js_object_));

  return SUCCEEDED(
      ActiveXUtils::GetDispatchPropertyNames(js_object_.pdispVal, out));
}

bool JsObject::GetProperty(const std::string16 &name,
                           JsScopedToken *out) const {
  // Check that we're initialized.
  assert(JsTokenIsObject(js_object_));

  // If the property name is unknown, GetDispatchProperty will return
  // DISP_E_UNKNOWNNAME and out will be unchanged.
  HRESULT hr = ActiveXUtils::GetDispatchProperty(js_object_.pdispVal,
                                                 name.c_str(),
                                                 out);
  if (DISP_E_UNKNOWNNAME == hr) {
    // Set the token to the equivalent of JSPARAM_UNDEFINED.
    out->Clear();
    return true;
  }
  return SUCCEEDED(hr);
}

bool JsObject::SetProperty(const std::string16 &name, const JsToken &value) {
  // Check that we're initialized.
  assert(JsTokenIsObject(js_object_));

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

bool JsObject::SetPropertyComModule(const std::string16& name,
                                    IScriptable* value) {
  return SetProperty(name, CComVariant(value));
}

bool JsObject::SetPropertyDispatcherModule(const std::string16 &name,
                                           ModuleImplBaseClass *value) {
  return SetProperty(name, value->GetWrapperToken());
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

bool JsObject::GetPropertyNames(std::vector<std::string16> *out) const {
  // Check that we're initialized.
  assert(JsTokenIsObject(js_object_));

  // TODO(nigeltao): implement
  return false;
}

bool JsObject::GetProperty(const std::string16 &name,
                           JsScopedToken *out) const {
  // Check that we're initialized.
  assert(JsTokenIsObject(js_object_));

  std::string name_utf8;
  if (!String16ToUTF8(name.c_str(), &name_utf8)) return false;

  NPObject *object = NPVARIANT_TO_OBJECT(js_object_);
  NPIdentifier name_id = NPN_GetStringIdentifier(name_utf8.c_str());

  return NPN_GetProperty(js_context_, object, name_id, out);
}

bool JsObject::SetProperty(const std::string16 &name, const JsToken &value) {
  // Check that we're initialized.
  assert(JsTokenIsObject(js_object_));

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

bool JsObject::SetPropertyBool(const std::string16 &name, bool value) {
  return SetProperty(name, ScopedNPVariant(value));
}

bool JsObject::SetPropertyDouble(const std::string16 &name, double value) {
  return SetProperty(name, ScopedNPVariant(value));
}

bool JsObject::SetPropertyComModule(const std::string16& name,
                                    IScriptable* value) {
  // TODO(nigeltao): implement
  return false;
}

bool JsObject::SetPropertyDispatcherModule(const std::string16 &name,
                                           ModuleImplBaseClass *value) {
  // TODO(nigeltao): implement
  return false;
}

#endif

// Common JsObject functions

bool JsObject::GetPropertyAsBool(const std::string16 &name, bool *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToBool_NoCoerce(token, js_context_, out);
}

bool JsObject::GetPropertyAsInt(const std::string16 &name, int *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToInt_NoCoerce(token, js_context_, out);
}

bool JsObject::GetPropertyAsDouble(const std::string16 &name,
                                   double *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToDouble_NoCoerce(token, js_context_, out);
}

bool JsObject::GetPropertyAsString(const std::string16 &name,
                                   std::string16 *out) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToString_NoCoerce(token, js_context_, out);
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

  return JsTokenToNewCallback_NoCoerce(token, js_context_, out);
}

JsParamType JsObject::GetPropertyType(const std::string16 &name) const {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return JSPARAM_UNKNOWN;

  return JsTokenGetType(token, js_context_);
}

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


// Given a JsToken, extract it into a JsArgument.  Object pointers are weak
// references (ref count is not increased).
static bool ConvertTokenToArgument(JsCallContext *context,
                                   const JsToken &variant,
                                   JsArgument *param) {
  switch (param->type) {
    case JSPARAM_BOOL: {
      bool *value = static_cast<bool *>(param->value_ptr);
      if (!JsTokenToBool_NoCoerce(variant, context->js_context(), value)) {
        return false;
      }
      break;
    }
    case JSPARAM_INT: {
      int *value = static_cast<int *>(param->value_ptr);
      if (!JsTokenToInt_NoCoerce(variant, context->js_context(), value)) {
        return false;
      }
      break;
    }
    case JSPARAM_INT64: {
      int64 *value = static_cast<int64 *>(param->value_ptr);
      if (!JsTokenToInt64_NoCoerce(variant, context->js_context(), value)) {
        return false;
      }
      break;
    }
    case JSPARAM_DOUBLE: {
      double *value = static_cast<double *>(param->value_ptr);
      if (!JsTokenToDouble_NoCoerce(variant, context->js_context(), value)) {
        return false;
      }
      break;
    }
    case JSPARAM_OBJECT: {
      JsObject *value = static_cast<JsObject *>(param->value_ptr);
      if (!value->SetObject(variant, context->js_context())) {
        return false;
      }
      break;
    }
    case JSPARAM_ARRAY: {
      JsArray *value = static_cast<JsArray *>(param->value_ptr);
      if (!value->SetArray(variant, context->js_context())) {
        return false;
      }
      break;
    }
    case JSPARAM_FUNCTION: {
      JsRootedCallback **value =
          static_cast<JsRootedCallback **>(param->value_ptr);
      if (!JsTokenToNewCallback_NoCoerce(variant, context->js_context(),
                                         value)) {
        return false;
      }
      break;
    }
    case JSPARAM_STRING16: {
      std::string16 *value = static_cast<std::string16 *>(param->value_ptr);
      if (!JsTokenToString_NoCoerce(variant, context->js_context(), value)) {
        return false;
      }
      break;
    }
    case JSPARAM_TOKEN: {
      JsToken *value = static_cast<JsToken *>(param->value_ptr);
      *value = variant;
      return true;
    }
    case JSPARAM_COM_MODULE: {
      IScriptable **value = static_cast<IScriptable **>(param->value_ptr);
      if (!JsTokenToComModule(context->js_context(), variant, value)) {
        context->SetException(
            STRING16(L"Invalid argument type: expected module."));
        return false;
      }
      break;
    }
    case JSPARAM_DISPATCHER_MODULE: {
      ModuleImplBaseClass **value =
          static_cast<ModuleImplBaseClass **>(param->value_ptr);
      if (!JsTokenToDispatcherModule(
#if BROWSER_FF
          context->js_runner()->GetContextWrapper(),
#else
          NULL,
#endif
          context->js_context(), variant, value)) {
        context->SetException(
            STRING16(L"Invalid argument type: expected module."));
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


bool JsTokenToNewCallback_NoCoerce(JsToken t, JsContextPtr cx,
                                   JsRootedCallback **out) {
  if (!JsTokenIsCallback(t, cx)) { return false; }
  *out = new JsRootedCallback(cx, t);
  return true;
}

bool JsTokenToInt64_NoCoerce(JsToken t, JsContextPtr cx, int64 *out) {
  double dbl;
  if (!JsTokenToDouble_NoCoerce(t, cx, &dbl)) { return false; }
  if (dbl < JS_INT_MIN || dbl > JS_INT_MAX) { return false; }
  if (!DoubleIsInt64(dbl)) { return false; }
  *out = static_cast<int64>(dbl);
  return true;
}


#if BROWSER_FF

bool JsTokenToBool_NoCoerce(JsToken t, JsContextPtr cx, bool *out) {
  if (!JSVAL_IS_BOOLEAN(t)) { return false; }
  *out = (JSVAL_TO_BOOLEAN(t) == JS_TRUE);
  return true;
}

bool JsTokenToDouble_NoCoerce(JsToken t, JsContextPtr cx, double *out) {
  if (JSVAL_IS_DOUBLE(t)) {
    *out = *(JSVAL_TO_DOUBLE(t));
    return true;
  } else if (JSVAL_IS_INT(t)) {
    *out = JSVAL_TO_INT(t);
    return true;
  }

  return false;
}

bool JsTokenToInt_NoCoerce(JsToken t, JsContextPtr cx, int *out) {
  if (!JSVAL_IS_INT(t)) { return false; }
  *out = JSVAL_TO_INT(t);
  return true;
}

bool JsTokenToString_NoCoerce(JsToken t, JsContextPtr cx, std::string16 *out) {
  if (!JSVAL_IS_STRING(t)) { return false; }
  JSString *js_str = JSVAL_TO_STRING(t);
  out->assign(reinterpret_cast<const char16 *>(JS_GetStringChars(js_str)),
              JS_GetStringLength(js_str));
  return true;
}


bool JsTokenToBool_Coerce(JsToken t, JsContextPtr cx, bool *out) {
  JSBool bool_value;
  if (!JS_ValueToBoolean(cx, t, &bool_value)) { return false; }
  *out = (bool_value == JS_TRUE);
  return true;
}

bool JsTokenToInt_Coerce(JsToken t, JsContextPtr cx, int *out) {
  // JS_ValueToECMAInt32 has edge-cases with non-numeric types, for example
  // non-numeric strings, NaN and {} are coerced to 0 rather than returning
  // failure. Compare this behaviour to JS_ValueToNumber (in
  // JsTokenToDouble_Coerce) which returns failure for these cases. It seems
  // best to be consistent in our handling of numbers. Coercing non-numbers to 0
  // could make Gears applications harder to debug, e.g. a user passes a non-
  // numeric type into a Gears API call that expects an integer and rather than
  // being notified of the problem (as they would be for calls that expect a
  // double) Gears instead treats it as 0 and carries on its way.
  // Because of this we use JS_ValueToNumber to coerce to a double and cast
  // to an int, which bypasses these problems. To avoid code duplication,
  // we just call JsTokenToDouble_Coerce and cast the result.
  // Note also that in IE we have to do a double coercion and an int cast too
  // due to the way VariantChangeType() handles integer coercions (rounding
  // instead of truncating).
  double output;
  if (!JsTokenToDouble_Coerce(t, cx, &output)) { return false; }
  // Make sure the double will fit into an int
  if (output < kint32min || output > kint32max) { return false; }
  *out = static_cast<int>(output);
  return true;
}

bool JsTokenToDouble_Coerce(JsToken t, JsContextPtr cx, double *out) {
  jsdouble dbl_value;
  if (!JS_ValueToNumber(cx, t, &dbl_value)) { return false; }
  // Edge-case: NaN should return failure
  if (isnan(dbl_value)) { return false; }
  *out = dbl_value;
  return true;
}

bool JsTokenToString_Coerce(JsToken t, JsContextPtr cx, std::string16 *out) {
  JSString *js_str = JS_ValueToString(cx, t);
  if (!js_str) { return false; }
  out->assign(reinterpret_cast<const char16 *>(JS_GetStringChars(js_str)),
              JS_GetStringLength(js_str));
  return true;
}


JsParamType JsTokenGetType(JsToken t, JsContextPtr cx) {
  if (JSVAL_IS_BOOLEAN(t)) {
    return JSPARAM_BOOL;
  } else if (JSVAL_IS_INT(t)) {
    return JSPARAM_INT;
  } else if (JSVAL_IS_DOUBLE(t)) {
    return JSPARAM_DOUBLE;
  } else if (JSVAL_IS_STRING(t)) {
    return JSPARAM_STRING16;
  } else if (JSVAL_IS_NULL(t)) {
    return JSPARAM_NULL;
  } else if (JSVAL_IS_VOID(t)) {
    return JSPARAM_UNDEFINED;
  } else if (JsTokenIsArray(t, cx)) {
    return JSPARAM_ARRAY;
  } else if (JsTokenIsCallback(t, cx)) {
    return JSPARAM_FUNCTION;
  } else if (JsTokenIsObject(t)) {
    return JSPARAM_OBJECT;
  } else {
    return JSPARAM_UNKNOWN;  // Unsupported type
  }
}

bool JsTokenIsCallback(JsToken t, JsContextPtr cx) {
  return JsTokenIsObject(t) && JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(t));
}

bool JsTokenIsArray(JsToken t, JsContextPtr cx) {
  return JsTokenIsObject(t) && JS_IsArrayObject(cx, JSVAL_TO_OBJECT(t));
}

bool JsTokenIsObject(JsToken t) {
  return JSVAL_IS_OBJECT(t) && !JSVAL_IS_NULL(t); // JSVAL_IS_OBJECT returns
                                                  // true for <null>.
}

bool JsTokenIsNullOrUndefined(JsToken t) {
  return JSVAL_IS_NULL(t) || JSVAL_IS_VOID(t); // null or undefined
}

bool BoolToJsToken(JsContextPtr context, bool value, JsScopedToken *out) {
  *out = BOOLEAN_TO_JSVAL(value);
  return true;
}

bool IntToJsToken(JsContextPtr context, int value, JsScopedToken *out) {
  *out = INT_TO_JSVAL(value);
  return true;
}

// TODO(nigeltao): We creating an unrooted jsval here (and saving it as
// *out), which is vulnerable to garbage collection.  Strictly speaking,
// callers should immediately root the token to avoid holding a dangling
// pointer.  The TODO is to fix up the API so that the callee does the
// right thing.
// Similarly for FooToJsToken in general, for Foo in {Bool, Int, Double}.
bool StringToJsToken(JsContextPtr context, const char16 *value,
                     JsScopedToken *out) {
  JSString *jstr = JS_NewUCStringCopyZ(context,
                                       reinterpret_cast<const jschar *>(value));
  if (jstr) {
    *out = STRING_TO_JSVAL(jstr);
    return true;
  } else {
    return false;
  }
}

bool DoubleToJsToken(JsContextPtr context, double value, JsScopedToken *out) {
  jsdouble* dp = JS_NewDouble(context, value);
  if (!dp)
    return false;

  *out = DOUBLE_TO_JSVAL(dp);
  return true;
}

bool NullToJsToken(JsContextPtr context, JsScopedToken *out) {
  *out = JSVAL_NULL;
  return true;
}
#elif BROWSER_IE

bool JsTokenToBool_NoCoerce(JsToken t, JsContextPtr cx, bool *out) {
  if (t.vt != VT_BOOL) { return false; }
  *out = (t.boolVal == VARIANT_TRUE);
  return true;
}

bool JsTokenToInt_NoCoerce(JsToken t, JsContextPtr cx, int *out) {
  if (t.vt != VT_I4) { return false; }
  *out = t.lVal;
  return true;
}

bool JsTokenToString_NoCoerce(JsToken t, JsContextPtr cx, std::string16 *out) {
  if (t.vt != VT_BSTR) { return false; }
  // NOTE: BSTR semantics demand that null BSTRs be interpreted as empty
  // string.
  if (!t.bstrVal) {
    out->clear();
  } else {
    out->assign(t.bstrVal);
  }
  return true;
}

bool JsTokenToDouble_NoCoerce(JsToken t, JsContextPtr cx, double *out) {
  if (t.vt == VT_R8) {
    *out = t.dblVal;
    return true;
  } else if (t.vt == VT_I4) {
    *out = static_cast<double>(t.lVal);
    return true;
  }

  return false;
}


bool JsTokenToBool_Coerce(JsToken t, JsContextPtr cx, bool *out) {
  // JsToken is a bool
  if (t.vt == VT_BOOL) {
    *out = (t.boolVal == VARIANT_TRUE);
  // Try to coerce the JsToken to a bool
  // There are many edge-cases with to-boolean coercions in IE using
  // VariantChangeType() (NaN is true, {} is false, "test" is false, etc.)
  // so we may as well do this coercion manually. The only values that should
  // be treated as false in JavaScript are false, null, NaN, 0, undefined and
  // an empty string. false is already covered by the case above.
  } else if (JsTokenIsNullOrUndefined(t) ||       // null, undefined
             t.vt == VT_R8 && isnan(t.dblVal) ||  // NaN
             t.vt == VT_I4 && t.lVal == 0 ||      // 0
             t.vt == VT_BSTR && !t.bstrVal[0]) {  // empty string
    *out = false;
  } else {
    *out = true;  // Anything else is true
  }
  return true;
}

bool JsTokenToInt_Coerce(JsToken t, JsContextPtr cx, int *out) {
  if (t.vt == VT_I4) {
    *out = t.lVal;
  // Try to coerce the JsToken to an int.
  } else {
    // Note that doubles are rounded to ints by VariantChangeType() when they
    // should be truncated, so we must force it to truncate by converting to
    // a double and then casting to an int manually.
    double output;
    if (!JsTokenToDouble_Coerce(t, cx, &output)) { return false; }
    // Make sure the double will fit into an int
    if (output < kint32min || output > kint32max) {
      return false;
    }
    *out = static_cast<int>(output);
  }
  return true;
}

bool JsTokenToDouble_Coerce(JsToken t, JsContextPtr cx, double *out) {
  // This coercion is very tricky to get just-right. See test cases in
  // test/testcases/internal_tests.js before making any changes. The expected
  // outputs are based on the result of doing a Number(testval) in JavaScript.
  // JsToken is a double
  if (t.vt == VT_R8) {
    // Edge-case: NaN should return failure
    if (isnan(t.dblVal)) { return false; }
    *out = t.dblVal;
  // Edge-case: boolean true should coerce to 1, not -1.
  } else if (t.vt == VT_BOOL) {
    *out = (t.boolVal == VARIANT_TRUE) ? 1 : 0;
  // Edge-case: null should coerce to 0
  } else if (t.vt == VT_NULL) {
    *out = 0;
  // Edge-case: undefined should return failure
  // Note: this case must come _after_ the previous case because VT_NULL is
  // also VT_EMPTY, but the converse is not true.
  } else if (t.vt == VT_EMPTY) {
    return false;
  // All other coercions
  } else {
    CComVariant variant;
    HRESULT hr;
    hr = VariantChangeType(&variant, &t, 0, VT_R8);
    if (hr != S_OK) {
      // Edge-case: empty strings should coerce to 0
      if (t.vt == VT_BSTR && !t.bstrVal[0]) {
        *out = 0;
        return true;
      } else {
        return false;
      }
    }
    // Edge-case: NaN should return failure
    if (isnan(variant.dblVal)) { return false; }
    *out = variant.dblVal;
  }
  return true;
}

bool JsTokenToString_Coerce(JsToken t, JsContextPtr cx, std::string16 *out) {
  // JsToken is a string
  if (t.vt == VT_BSTR) {
    // A BSTR must have identical semantics for NULL and for "".
    out->assign(t.bstrVal ? t.bstrVal : STRING16(L""));
  // Edge-case: booleans should coerce to "true" and "false".
  // Note that VariantChangeType() converts booleans to "1" and "0",
  // or "True" and "False" (if VARIANT_ALPHABOOL is specified).
  } else if (t.vt == VT_BOOL) {
    if (t.boolVal == VARIANT_TRUE) {
      out->assign(STRING16(L"true"));
    } else {
      out->assign(STRING16(L"false"));
    }
  // Edge-case: null should coerce to "null"
  } else if (t.vt == VT_NULL) {
    out->assign(STRING16(L"null"));
  // Edge-case: undefined should coerce to "undefined"
  // Note: see comment in JsTokenToDouble_Coerce.
  } else if (t.vt == VT_EMPTY) {
    out->assign(STRING16(L"undefined"));
  // Edge-case: NaN should coerce to "NaN"
  } else if (t.vt == VT_R8 && isnan(t.dblVal)) {
    out->assign(STRING16(L"NaN"));
  // All other coercions
  } else {
    CComVariant variant;
    HRESULT hr;
    hr = VariantChangeType(&variant, &t, 0, VT_BSTR);
    if (hr != S_OK) { return false; }
    out->assign(variant.bstrVal);
    // We are responsible for calling VariantClear() on the variant we
    // created if it contains a BSTR
    VariantClear(&variant);
  }
  return true;
}

JsParamType JsTokenGetType(JsToken t, JsContextPtr cx) {
  if (t.vt == VT_BOOL) {
    return JSPARAM_BOOL;
  } else if (t.vt == VT_I4) {
    return JSPARAM_INT;
  } else if (t.vt == VT_R8) {
    return JSPARAM_DOUBLE;
  } else if (t.vt == VT_BSTR) {
    return JSPARAM_STRING16;
  } else if (t.vt == VT_NULL) {
    return JSPARAM_NULL;
  } else if (t.vt == VT_EMPTY) {  // Must come after the previous test
    return JSPARAM_UNDEFINED;
  } else if (JsTokenIsArray(t, cx)) { 
    return JSPARAM_ARRAY;
  } else if (JsTokenIsCallback(t, cx)) {
    return JSPARAM_FUNCTION;
  } else if (JsTokenIsObject(t)) {
    return JSPARAM_OBJECT;
  } else {
    return JSPARAM_UNKNOWN;  // Unsupported type
  }
}

bool JsTokenIsCallback(JsToken t, JsContextPtr cx) {
  if (t.vt != VT_DISPATCH || !t.pdispVal) return false; 
  // Check for call() method which all Function objects have.
  // Note: User defined objects that have a call() method will be incorrectly
  // flagged as callbacks but this check is better than nothing and probably
  // the best we are going to get.
  CComVariant out;
  if (FAILED(ActiveXUtils::GetDispatchProperty(t.pdispVal,
                                               STRING16(L"call"),
                                               &out))) {
    return false;
  }
  return true;
}

bool JsTokenIsArray(JsToken t, JsContextPtr cx) {
  if (t.vt != VT_DISPATCH || !t.pdispVal) return false;
  // Check for join() method which all Array objects have.
  // Note: Function objects also have a length property so it's not safe
  // to assume that an object with a length property is an Array!
  CComVariant out;
  if (FAILED(ActiveXUtils::GetDispatchProperty(t.pdispVal,
                                               STRING16(L"join"),
                                               &out))) {
    return false;
  }
  return true;
}

bool JsTokenIsObject(JsToken t) {
  return t.vt == VT_DISPATCH && t.pdispVal;
}

bool JsTokenIsNullOrUndefined(JsToken t) {
  return t.vt == VT_NULL || t.vt == VT_EMPTY;  // null or undefined
}

bool BoolToJsToken(JsContextPtr context, bool value, JsScopedToken *out) {
  *out = value;
  return true;
}

bool IntToJsToken(JsContextPtr context, int value, JsScopedToken *out) {
  *out = value;
  return true;
}

bool StringToJsToken(JsContextPtr context, const char16 *value,
                     JsScopedToken *out) {
  *out = value;
  return true;
}

bool DoubleToJsToken(JsContextPtr context, double value, JsScopedToken *out) {
  *out = value;
  return true;
}

bool NullToJsToken(JsContextPtr context, JsScopedToken *out) {
  VARIANT null_variant;
  null_variant.vt = VT_NULL;
  *out = null_variant;
  return true;
}

#elif BROWSER_NPAPI

bool JsTokenToBool_NoCoerce(JsToken t, JsContextPtr cx, bool *out) {
  if (!NPVARIANT_IS_BOOLEAN(t)) { return false; }
  *out = NPVARIANT_TO_BOOLEAN(t);
  return true;
}

bool JsTokenToInt_NoCoerce(JsToken t, JsContextPtr cx, int *out) {
  // NOTE: WebKit's implementation of NPVARIANT always returns JavaScript
  // numbers as doubles, even if they are integral and small enough to fit in
  // int32. Therefore this first branch never gets hit in WebKit today. However,
  // it should get hit in other NPAPI implementations.
  if (NPVARIANT_IS_INT32(t)) {
    *out = NPVARIANT_TO_INT32(t);
    return true;
  } else if (NPVARIANT_IS_DOUBLE(t)) {
    double d = NPVARIANT_TO_DOUBLE(t);
    if (DoubleIsInt32(d)) {
      *out = static_cast<int>(d);
      return true;
    }
  }
  return false;
}

bool JsTokenToDouble_NoCoerce(JsToken t, JsContextPtr cx, double *out) {
  if (NPVARIANT_IS_DOUBLE(t)) {
    *out = NPVARIANT_TO_DOUBLE(t);
    return true;
  } else if (NPVARIANT_IS_INT32(t)) {
    *out = static_cast<double>(NPVARIANT_TO_INT32(t));
    return true;
  }

  return false;
}

bool JsTokenToString_NoCoerce(JsToken t, JsContextPtr cx, std::string16 *out) {
  if (!NPVARIANT_IS_STRING(t)) { return false; }
  NPString str = NPVARIANT_TO_STRING(t);
  if (GetNPStringUTF8Length(str) == 0) {
    // TODO(mpcomplete): find out if UTF8ToString16 can be changed to return
    // true in this case.
    out->clear();
    return true;
  }
  return UTF8ToString16(GetNPStringUTF8Characters(str),
                        GetNPStringUTF8Length(str), out);
}


// TODO: Implement coercion for NPAPI. Since NPAPI does not have built-in
// coercion this will have to be done manually in terms of C++ types. It might
// be worth considering reusing that code across all browsers, for consistency.
bool JsTokenToBool_Coerce(JsToken t, JsContextPtr cx, bool *out) {
  // JsToken is a bool
  if (JsTokenGetType(t, cx) == JSPARAM_BOOL) {
    return JsTokenToBool_NoCoerce(t, cx, out);
  // Try to coerce the JsToken to a bool
  // The only values that should be treated as false in JavaScript are false,
  // null, NaN, 0, undefined and an empty string. false is already covered by
  // the case above.
  } else if (JsTokenIsNullOrUndefined(t) ||  // null, undefined
             NPVARIANT_IS_DOUBLE(t) && // 0 or Nan
                 (NPVARIANT_TO_DOUBLE(t) == 0 ||
                  isnan(NPVARIANT_TO_DOUBLE(t))) || 
             NPVARIANT_IS_INT32(t) && NPVARIANT_TO_INT32(t) == 0 ||  // 0
             NPVARIANT_IS_STRING(t) &&  // empty string
                 GetNPStringUTF8Length(NPVARIANT_TO_STRING(t)) == 0) {
    *out = false;
  } else {
    *out = true;  // Anything else is true
  }
  return true;
}

bool JsTokenToInt_Coerce(JsToken t, JsContextPtr cx, int *out) {
  if (JsTokenGetType(t, cx) == JSPARAM_INT) {
    return JsTokenToInt_NoCoerce(t, cx, out);
  // Try to coerce the JsToken to an int.
  } else {
    double output;
    if (!JsTokenToDouble_Coerce(t, cx, &output)) { return false; }
    // Make sure the double will fit into an int
    if (output < kint32min || output > kint32max) {
      return false;
    }
    *out = static_cast<int>(output);
  }
  return true;
}

bool JsTokenToDouble_Coerce(JsToken t, JsContextPtr cx, double *out) {
  // This coercion is very tricky to get just-right. See test cases in
  // test/testcases/internal_tests.js before making any changes. The expected
  // outputs are based on the result of doing a Number(testval) in JavaScript.
  JsParamType type = JsTokenGetType(t, cx);
  // JsToken is a double
  if (type == JSPARAM_DOUBLE) {
    // Edge-case: NaN should return failure
    if (isnan(NPVARIANT_TO_DOUBLE(t))) { return false; }
    *out = NPVARIANT_TO_DOUBLE(t);
  // JsToken is an integer (or a double that can be converted to an integer).
  } else if (type == JSPARAM_INT) {
    int out_int;
    if (!JsTokenToInt_NoCoerce(t, cx, &out_int))
      return false;
    *out = out_int;
    return true;
  // Edge-case: boolean true should coerce to 1, not -1.
  } else if (type == JSPARAM_BOOL) {
    *out = NPVARIANT_TO_BOOLEAN(t) ? 1 : 0;
  // Edge-case: null should coerce to 0
  } else if (NPVARIANT_IS_NULL(t)) {
    *out = 0;
  // Edge-case: undefined should return failure
  } else if (NPVARIANT_IS_VOID(t)) {
    return false;
  // Strings
  } else if (NPVARIANT_IS_STRING(t)) {
    // TODO(mpcomplete): I'm not sure if NPStrings are guaranteed to be null
    // terminated.  Do we need the str temporary?
    NPString s = NPVARIANT_TO_STRING(t);
    std::string str(GetNPStringUTF8Characters(s), GetNPStringUTF8Length(s));

    // Edge-case: NaN should return failure
    if (StringCompareIgnoreCase(str.c_str(), "nan") == 0) {
      return false;
    }

    *out = atof(str.c_str());
    return true;
  // All other coercions
  } else {
    // TODO(mpcomplete): implement me
    assert(false);
    return false;
  }
  return true;
}

bool JsTokenToString_Coerce(JsToken t, JsContextPtr cx, std::string16 *out) {
  JsParamType param_type = JsTokenGetType(t, cx);
  bool converted_succesfully = false;
  
  switch(param_type) {
    case JSPARAM_UNDEFINED: {
      *out = STRING16(L"undefined");
      converted_succesfully = true;
      break;
    }
    case JSPARAM_NULL: {
      *out = STRING16(L"null");
      converted_succesfully = true;
      break;
    }
    case JSPARAM_BOOL: {
      bool bool_val;
      converted_succesfully = JsTokenToBool_NoCoerce(t, cx, &bool_val);
      if (converted_succesfully) {
        *out = bool_val ? STRING16(L"true") : STRING16(L"false");
      }
      break;
    }
    case JSPARAM_STRING16: {
      converted_succesfully = JsTokenToString_NoCoerce(t, cx, out);
      break;
    }
    default: {
      // TODO(mpcomplete): implement me
      break;
    }
  }
  return converted_succesfully;
}


JsParamType JsTokenGetType(JsToken t, JsContextPtr cx) {
  if (NPVARIANT_IS_BOOLEAN(t)) {
    return JSPARAM_BOOL;
  } else if (NPVARIANT_IS_INT32(t)) {
    return JSPARAM_INT;
  } else if (NPVARIANT_IS_DOUBLE(t)) {
    // Patch for WebKit, which reports both ints and doubles as both being of
    // type JSPARAM_DOUBLE.
    double double_val = NPVARIANT_TO_DOUBLE(t);
    if (DoubleIsInt32(double_val)) {
      return JSPARAM_INT;
    }
    return JSPARAM_DOUBLE;
  } else if (NPVARIANT_IS_STRING(t)) {
    return JSPARAM_STRING16;
  } else if (NPVARIANT_IS_NULL(t)) {
    return JSPARAM_NULL;
  } else if (NPVARIANT_IS_VOID(t)) {
    return JSPARAM_UNDEFINED;
  } else if (JsTokenIsArray(t, cx)) {
    return JSPARAM_ARRAY;
  } else if (JsTokenIsCallback(t, cx)) {
    return JSPARAM_FUNCTION;
  } else if (JsTokenIsObject(t)) {
    return JSPARAM_OBJECT;
  } else {
    return JSPARAM_UNKNOWN;  // Unsupported type
  }
}

bool JsTokenIsCallback(JsToken t, JsContextPtr cx) {
  if (!NPVARIANT_IS_OBJECT(t))
    return false;

  // Check for call() method which all Function objects have.
  // Note: User defined objects that have a call() method will be incorrectly
  // flagged as callbacks but this check is better than nothing and probably
  // the best we are going to get.
  NPIdentifier call_id = NPN_GetStringIdentifier("call");
  ScopedNPVariant out;
  if (!NPN_GetProperty(cx, NPVARIANT_TO_OBJECT(t), call_id, &out) ||
      NPVARIANT_IS_VOID(out)) {
    return false;
  }
  return true;
}

bool JsTokenIsArray(JsToken t, JsContextPtr cx) {
  if (!NPVARIANT_IS_OBJECT(t))
    return false;

  // Check for join() method which all Array objects have.
  // Note: Function objects also have a length property so it's not safe
  // to assume that an object with a length property is an Array!
  NPIdentifier join_id = NPN_GetStringIdentifier("join");
  ScopedNPVariant out;
  if (!NPN_GetProperty(cx, NPVARIANT_TO_OBJECT(t), join_id, &out) ||
      NPVARIANT_IS_VOID(out)) {
    return false;
  }
  return true;
}

bool JsTokenIsObject(JsToken t) {
  return NPVARIANT_IS_OBJECT(t);
}

bool JsTokenIsNullOrUndefined(JsToken t) {
  return NPVARIANT_IS_NULL(t) || NPVARIANT_IS_VOID(t);
}

bool BoolToJsToken(JsContextPtr context, bool value, JsScopedToken *out) {
  out->Reset(value);
  return true;
}

bool IntToJsToken(JsContextPtr context, int value, JsScopedToken *out) {
  out->Reset(value);
  return true;
}

bool StringToJsToken(JsContextPtr context, const char16 *value,
                     JsScopedToken *out) {
  out->Reset(value);
  return true;
}

bool DoubleToJsToken(JsContextPtr context, double value, JsScopedToken *out) {
  out->Reset(value);
  return true;
}

bool NullToJsToken(JsContextPtr context, JsScopedToken *out) {
  out->ResetToNull();
  return true;
}

// ScopedNPVariant functions.
void ScopedNPVariant::Reset() {
  NPN_ReleaseVariantValue(this);
  VOID_TO_NPVARIANT(*this);
}

void ScopedNPVariant::ResetToNull() {
  Reset();
  NULL_TO_NPVARIANT(*this);
}

void ScopedNPVariant::Reset(int value) {
  Reset();
  INT32_TO_NPVARIANT(value, *this);
}

void ScopedNPVariant::Reset(bool value) {
  Reset();
  BOOLEAN_TO_NPVARIANT(value, *this);
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
    case JSPARAM_INT64: {
      const int64 *value = static_cast<const int64 *>(param.value_ptr);
      const double dvalue = static_cast<double>(*value);
      jsdouble *js_double = JS_NewDouble(context, dvalue);
      *token = DOUBLE_TO_JSVAL(js_double);
      break;
    }
    case JSPARAM_DOUBLE: {
      const double *value = static_cast<const double *>(param.value_ptr);
      jsdouble *js_double = JS_NewDouble(context, *value);
      *token = DOUBLE_TO_JSVAL(js_double);
      break;
    }
    case JSPARAM_STRING16: {
      const std::string16 *value = static_cast<const std::string16 *>(
                                                   param.value_ptr);
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
    case JSPARAM_DISPATCHER_MODULE:
    case JSPARAM_COM_MODULE: {
      const ModuleImplBaseClass *value =
          static_cast<const ModuleImplBaseClass *>(param.value_ptr);
      *token = value->GetWrapperToken();
      break;
    }
    case JSPARAM_NULL:
      *token = JSVAL_NULL;
      break;
    case JSPARAM_UNDEFINED:
      *token = JSVAL_VOID;
      break;
    default:
      assert(false);
  }
}

JsCallContext::JsCallContext(JsContextPtr cx, JsRunnerInterface *js_runner,
                             int argc, JsToken *argv, JsToken *retval)
    : js_context_(cx), is_exception_set_(false),
      argc_(argc), argv_(argv), retval_(retval),
      js_runner_(js_runner), xpc_(NULL) {}


void JsCallContext::SetReturnValue(JsParamType type, const void *value_ptr) {
  // There is only a valid retval_ if the JS caller is expecting a return value.
  if (retval_) {
    JsParamToSend retval = { type, value_ptr };
    ConvertJsParamToToken(retval, js_context(), retval_);
  }
}

void JsCallContext::SetException(const std::string16 &message) {
  assert(!message.empty());
  is_exception_set_ = true;

  nsresult nr = NS_OK;
#if BROWSER_FF3
  nsAXPCNativeCallContext *ncc = NULL;
  if (xpc_) {
    nr = xpc_->GetCurrentNativeCallContext(&ncc);
  }
#else
  nsCOMPtr<nsIXPCNativeCallContext> ncc;
  if (xpc_) {
    nr = xpc_->GetCurrentNativeCallContext(getter_AddRefs(ncc));
  }
#endif

  JsSetException(js_context_, js_runner_, message.c_str(),
                 ncc && NS_SUCCEEDED(nr) ?
                     true : false); // notify_native_call_context
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
    case JSPARAM_INT64: {
      const int64 *value = static_cast<const int64 *>(param.value_ptr);
      const double dvalue = static_cast<double>(*value);
      *token = dvalue;  // CComVariant understands 'double'
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
    case JSPARAM_DISPATCHER_MODULE:
    case JSPARAM_COM_MODULE: {
      const ModuleImplBaseClass *value =
          static_cast<const ModuleImplBaseClass *>(param.value_ptr);
      *token = value->GetWrapperToken();
      break;
    }
    case JSPARAM_NULL:
      VARIANT null_variant;
      null_variant.vt = VT_NULL;
      *token = null_variant;
      break;
    case JSPARAM_UNDEFINED:
      // Setting *token = VT_EMPTY doesn't seem to work.
      token->Clear();
      break;
    default:
      assert(false);
  }
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
    case JSPARAM_INT64: {
      const int64 *value = static_cast<const int64 *>(param.value_ptr);
      const double dvalue = static_cast<double>(*value);
      variant->Reset(dvalue);
      break;
    }
    case JSPARAM_DOUBLE: {
      const double *value = static_cast<const double *>(param.value_ptr);
      variant->Reset(*value);
      break;
    }
    case JSPARAM_DISPATCHER_MODULE:
    case JSPARAM_COM_MODULE: {
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
    case JSPARAM_UNDEFINED: {
      variant->Reset();  // makes it VOID (undefined).
      VOID_TO_NPVARIANT(*variant);  // TODO(steveblock): Is this needed?
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
  LOG(("SetException: %s\n", message_ascii.c_str()));
#endif

  is_exception_set_ = true;
  
  std::string message_utf8;
  if (!String16ToUTF8(message.data(), message.length(), &message_utf8))
    message_utf8 = "Unknown Gears Error";  // better to throw *something*

  NPObject *window;
  if (NPN_GetValue(js_context(), NPNVWindowNPObject, &window) != NPERR_NO_ERROR)
    return;
  ScopedNPObject window_scoped(window);
  NPN_SetException(window, message_utf8.c_str());
}

#endif  // BROWSER_NPAPI

#if defined(BROWSER_FF) || defined(BROWSER_NPAPI)

int JsCallContext::GetArgumentCount() {
  return argc_;
}

const JsToken &JsCallContext::GetArgument(int index) {
  return argv_[index];
}

#elif BROWSER_IE

int JsCallContext::GetArgumentCount() {
  return disp_params_->cArgs;
}

const JsToken &JsCallContext::GetArgument(int index) {
  int arg_index = disp_params_->cArgs - index - 1;
  return disp_params_->rgvarg[arg_index];
}

#endif

int JsCallContext::GetArguments(int output_argc, JsArgument *output_argv) {
  bool has_optional = false;

  if (GetArgumentCount() > output_argc) {
    SetException(STRING16(L"Too many parameters."));
    return 0;
  }

  for (int i = 0; i < output_argc; ++i) {
    assert(output_argv[i].value_ptr);

    has_optional |= output_argv[i].requirement == JSPARAM_OPTIONAL;
    if (output_argv[i].requirement == JSPARAM_REQUIRED)
      assert(!has_optional);  // should not have required arg after optional

    // TODO(mpcomplete): We need to handle this more generally.  We should
    // continue processing arguments for the case where a developer does
    // something like 'method(null, foo)' if the first argument is optional.
    if (i >= GetArgumentCount() ||
        GetArgumentType(i) == JSPARAM_NULL || 
        GetArgumentType(i) == JSPARAM_UNDEFINED) {
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

    if (!ConvertTokenToArgument(this, GetArgument(i), &output_argv[i])) {
      std::string16 msg(STRING16(L"Argument "));
      msg += IntegerToString16(i + 1);
      msg += STRING16(L" has invalid type or is outside allowed range.");
      SetException(msg);
      return i;
    }
  }

  return output_argc;
}

JsParamType JsCallContext::GetArgumentType(int i) {
  if (i >= GetArgumentCount()) return JSPARAM_UNKNOWN;
  return JsTokenGetType(GetArgument(i), js_context());
}


//-----------------------------------------------------------------------------
#if BROWSER_FF  // the rest of this file only applies to Firefox, for now

JsParamFetcher::JsParamFetcher(ModuleImplBaseClass *obj) {
  if (obj->EnvIsWorker()) {
    js_context_ = obj->EnvPageJsContext();
    js_argc_ = obj->JsWorkerGetArgc();
    js_argv_ = obj->JsWorkerGetArgv();
    js_retval_ = obj->JsWorkerGetRetVal();
  } else {
    // In the main thread use the caller's current JS context, NOT the context
    // where 'obj' was created.  These can be different!  Each frame has its
    // own JS context, and code can reference 'top.OtherFrame.FooObject'.
    nsresult nr;
    xpc_ = do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr);
    if (xpc_ && NS_SUCCEEDED(nr)) {
#if BROWSER_FF3
      nsAXPCNativeCallContext *ncc = NULL;
      nr = xpc_->GetCurrentNativeCallContext(&ncc);
#else
      nsCOMPtr<nsIXPCNativeCallContext> ncc;
      nr = xpc_->GetCurrentNativeCallContext(getter_AddRefs(ncc));
#endif
      if (ncc && NS_SUCCEEDED(nr)) {
        ncc->GetJSContext(&js_context_);
        PRUint32 argc;
        ncc->GetArgc(&argc);
        js_argc_ = static_cast<int>(argc);
        ncc->GetArgvPtr(&js_argv_);
        ncc->GetRetValPtr(&js_retval_);
      }
    }
  }
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
  return JsTokenToInt_NoCoerce(t, js_context_, out);
}

bool JsParamFetcher::GetAsString(int i, std::string16 *out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()
  JsToken t = js_argv_[i];
  return JsTokenToString_NoCoerce(t, js_context_, out);
}

bool JsParamFetcher::GetAsBool(int i, bool *out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()
  JsToken t = js_argv_[i];
  return JsTokenToBool_NoCoerce(t, js_context_, out);
}

bool JsParamFetcher::GetAsDouble(int i, double *out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()
  JsToken t = js_argv_[i];
  return JsTokenToDouble_NoCoerce(t, js_context_, out);
}

bool JsParamFetcher::GetAsArray(int i, JsArray *out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()

  return out->SetArray(js_argv_[i], js_context_);
}

bool JsParamFetcher::GetAsObject(int i, JsObject *out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()

  return out->SetObject(js_argv_[i], js_context_);
}

bool JsParamFetcher::GetAsDispatcherModule(int i, JsRunnerInterface *js_runner,
                                           ModuleImplBaseClass **out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()
  if (!JSVAL_IS_OBJECT(js_argv_[i])) return false;

  if (!JsTokenToDispatcherModule(js_runner->GetContextWrapper(),
                                 js_context_, js_argv_[i], out))
    return false;

  return true;
}

bool JsParamFetcher::GetAsNewRootedCallback(int i, JsRootedCallback **out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()

  // Special case null and undefined callbacks. Some old Gears modules expect
  // this method to work this way and we left it as is to avoid any
  // destabilization. New code should use JsCallContext which is more consistent
  // by failing this case.
  if (JsTokenIsNullOrUndefined(js_argv_[i])) {
    *out = new JsRootedCallback(js_context_, js_argv_[i]);
    return true;
  }

  return JsTokenToNewCallback_NoCoerce(js_argv_[i], js_context_, out);
}

JsParamType JsParamFetcher::GetType(int i) {
  if (i >= js_argc_) return JSPARAM_UNKNOWN;  // see comment above, in GetAsInt()
  return JsTokenGetType(js_argv_[i], js_context_);
}

void JsParamFetcher::SetReturnValue(JsToken retval) {
  if (js_retval_) {
    *js_retval_ = retval;

    if (xpc_) {
      nsresult nr;
#if BROWSER_FF3
      nsAXPCNativeCallContext *ncc = NULL;
      nr = xpc_->GetCurrentNativeCallContext(&ncc);
#else
      nsCOMPtr<nsIXPCNativeCallContext> ncc;
      nr = xpc_->GetCurrentNativeCallContext(getter_AddRefs(ncc));
#endif
      if (ncc &&  NS_SUCCEEDED(nr)) {
        ncc->SetReturnValueWasSet(PR_TRUE);
      }
    }
  }
}

bool JsParamFetcher::GetAsMarshaledJsToken(int i, JsRunnerInterface *js_runner,
                                           MarshaledJsToken **out,
                                           std::string16 *error_message_out) {
  if (i >= js_argc_) return false;  // see comment above, in GetAsInt()
  *out = MarshaledJsToken::Marshal(js_argv_[i], js_runner, js_context_,
                                   error_message_out);
  return *out != NULL;
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

#if BROWSER_FF3
    nsAXPCNativeCallContext *ncc = NULL;
    nr = xpc->GetCurrentNativeCallContext(&ncc);
#else
    nsCOMPtr<nsIXPCNativeCallContext> ncc;
    nr = xpc->GetCurrentNativeCallContext(getter_AddRefs(ncc));
#endif
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

  scoped_ptr<JsObject> error_object(js_runner->NewError(message));
  if (!error_object.get()) { return retval; }

  // Note: need JS_SetPendingException to bubble 'catch' in workers.
  JS_SetPendingException(cx, error_object->token());

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
