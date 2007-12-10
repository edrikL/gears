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
#include "gears/base/ie/activex_utils.h"
#elif BROWSER_NPAPI
#include "gears/base/npapi/browser_utils.h"
#include "gears/base/npapi/np_utils.h"
#elif BROWSER_SAFARI
#include "gears/base/safari/browser_utils.h"
#endif

// Browser specific JsArray functions.
#if BROWSER_FF
JsArray::JsArray() : js_context_(NULL), array_(0) {
}

bool JsArray::SetArray(JsToken value, JsContextPtr context) {
  // check that it's an array
  if (!JSVAL_IS_OBJECT(value) ||
      !JS_IsArrayObject(context, JSVAL_TO_OBJECT(value))) {
    return false;
  }

  array_ = value;
  js_context_ = context;
  return true;
}

bool JsArray::GetLength(int *length) {
  jsuint array_length;
  if (JS_GetArrayLength(js_context_,
                        JSVAL_TO_OBJECT(array_), &array_length)) {
    *length = static_cast<int>(array_length);
    return true;
  }

  return false;
}

bool JsArray::GetElement(int index, JsScopedToken *out) {
  if (!array_) return false;

  return JS_GetElement(js_context_, JSVAL_TO_OBJECT(array_),
                       index, out) == JS_TRUE;
}

#elif BROWSER_IE

JsArray::JsArray() : js_context_(NULL) {
}

bool JsArray::SetArray(JsToken value, JsContextPtr context) {
  if (value.vt != VT_DISPATCH) return false;

  array_ = value;

  return true;
}

bool JsArray::GetLength(int *length) {
  if (array_.vt != VT_DISPATCH) return false;

  VARIANT out;
  if (FAILED(ActiveXUtils::GetDispatchProperty(array_.pdispVal, L"length",
                                               &out))) {
    return false;
  }
  if (out.vt != VT_I4) return false;

  *length = out.lVal;

  return true;
}

bool JsArray::GetElement(int index, JsScopedToken *out) {
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

#elif BROWSER_NPAPI

JsArray::JsArray() : js_context_(NULL) {
  VOID_TO_NPVARIANT(array_);
}

bool JsArray::SetArray(JsToken value, JsContextPtr context) {
  // check that it's an array (can only test that it has a length property).
  NPIdentifier length_id = NPN_GetStringIdentifier("length");
  if (!NPVARIANT_IS_OBJECT(value) ||
      !NPN_HasProperty(context, NPVARIANT_TO_OBJECT(value), length_id)) {
    return false;
  }

  array_ = value;
  js_context_ = context;
  return true;
}

bool JsArray::GetLength(int *length) {
  if (!NPVARIANT_IS_OBJECT(array_)) return false;

  NPObject *array = NPVARIANT_TO_OBJECT(array_);

  NPIdentifier length_id = NPN_GetStringIdentifier("length");
  if (!NPN_HasProperty(js_context_, array, length_id)) return false;

  NPVariant np_length;
  if (!NPN_GetProperty(js_context_, array, length_id, &np_length)) return false;

  return JsTokenToInt(np_length, js_context_, length);
}

bool JsArray::GetElement(int index, JsScopedToken *out) {
  if (!NPVARIANT_IS_OBJECT(array_)) return false;

  NPObject *array = NPVARIANT_TO_OBJECT(array_);

  std::string index_utf8 = IntegerToString(index);
  NPIdentifier index_id = NPN_GetStringIdentifier(index_utf8.c_str());
  if (!NPN_HasProperty(js_context_, array, index_id)) return false;

  if (!NPN_GetProperty(js_context_, array, index_id, out)) return false;

  return true;
}

#endif

// Common JsArray functions

bool JsArray::GetElementAsBool(int index, bool *out) {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToBool(token, js_context_, out);
}

bool JsArray::GetElementAsInt(int index, int *out) {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToInt(token, js_context_, out);
}

bool JsArray::GetElementAsDouble(int index, double *out) {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToDouble(token, js_context_, out);
}

bool JsArray::GetElementAsString(int index, std::string16 *out) {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return JsTokenToString(token, js_context_, out);
}

bool JsArray::GetElementAsArray(int index, JsArray *out) {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return out->SetArray(token, js_context_);
}

bool JsArray::GetElementAsObject(int index, JsObject *out) {
  JsScopedToken token;
  if (!GetElement(index, &token)) return false;

  return out->SetObject(token, js_context_);
}

// Browser specific JsObject functions.
#if BROWSER_FF

JsObject::JsObject() : js_context_(NULL), js_object_(0) {
}

bool JsObject::SetObject(JsToken value, JsContextPtr context) {
  if (JSVAL_IS_OBJECT(value)) {
    js_object_ = value;
    js_context_ = context;
    return true;
  }

  return false;
}

bool JsObject::GetProperty(const std::string16 &name, JsScopedToken *out) {
  if (!js_object_) return false;

  return JS_GetUCProperty(js_context_, JSVAL_TO_OBJECT(js_object_),
                          reinterpret_cast<const jschar*>(name.c_str()),
                          name.length(), out) == JS_TRUE;
}

#elif BROWSER_IE

JsObject::JsObject() : js_context_(NULL) {
}

bool JsObject::SetObject(JsToken value, JsContextPtr context) {
  if (value.vt != VT_DISPATCH) return false;

  js_object_ = value;

  return true;
}

bool JsObject::GetProperty(const std::string16 &name, JsScopedToken *out) {
  if (js_object_.vt != VT_DISPATCH) return false;

  return SUCCEEDED(ActiveXUtils::GetDispatchProperty(js_object_.pdispVal,
                                                     name.c_str(),
                                                     out));
}

#elif BROWSER_NPAPI

JsObject::JsObject() : js_context_(NULL) {
  VOID_TO_NPVARIANT(js_object_);
}

bool JsObject::SetObject(JsToken value, JsContextPtr context) {
  if (NPVARIANT_IS_OBJECT(value)) {
    js_object_ = value;
    js_context_ = context;
    return true;
  }

  return false;
}

bool JsObject::GetProperty(const std::string16 &name, JsScopedToken *out) {
  if (!NPVARIANT_IS_OBJECT(js_object_)) return false;

  std::string name_utf8;
  if (!String16ToUTF8(name.c_str(), &name_utf8)) return false;

  NPObject *object = NPVARIANT_TO_OBJECT(js_object_);
  NPIdentifier name_id = NPN_GetStringIdentifier(name_utf8.c_str());

  return NPN_GetProperty(js_context_, object, name_id, out);
}

#endif

// Common JsObject functions

bool JsObject::GetPropertyAsBool(const std::string16 &name, bool *out) {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToBool(token, js_context_, out);
}

bool JsObject::GetPropertyAsInt(const std::string16 &name, int *out) {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToInt(token, js_context_, out);
}

bool JsObject::GetPropertyAsDouble(const std::string16 &name, double *out) {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToDouble(token, js_context_, out);
}

bool JsObject::GetPropertyAsString(const std::string16 &name,
                                   std::string16 *out) {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return JsTokenToString(token, js_context_, out);
}

bool JsObject::GetPropertyAsArray(const std::string16 &name, JsArray *out) {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return out->SetArray(token, js_context_);
}

bool JsObject::GetPropertyAsObject(const std::string16 &name, JsObject *out) {
  JsScopedToken token;
  if (!GetProperty(name, &token)) return false;

  return out->SetObject(token, js_context_);
}

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

bool JsTokenIsNullOrUndefined(JsToken t) {
  return JSVAL_IS_NULL(t) || JSVAL_IS_VOID(t); // null or undefined
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

bool JsTokenIsNullOrUndefined(JsToken t) {
  return t.vt == VT_NULL || t.vt == VT_EMPTY; // null or undefined
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
  if (str.utf8length == 0) {
    // TODO(mpcomplete): find out if UTF8ToString16 can be changed to return
    // true in this case.
    out->clear();
    return true;
  }
  return UTF8ToString16(str.utf8characters, str.utf8length, out);
}

bool JsTokenIsNullOrUndefined(JsToken t) {
  return NPVARIANT_IS_NULL(t) || NPVARIANT_IS_VOID(t);
}

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

void ConvertJsParamToToken(const JsParamToSend &param,
                           ScopedNPVariant *variant) {
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
    case JSPARAM_OBJECT_TOKEN: {
      const JsToken *value = static_cast<const JsToken *>(param.value_ptr);
      variant->Reset(NPVARIANT_TO_OBJECT(*value));
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

// Given an NPVariant, extract it into a JsArgument.  Object pointers are
// weak references (ref count is not increased).
static bool ConvertTokenToArgument(JsCallContext *context,
                                   const JsToken &variant,
                                   JsArgument *param) {
  switch (param->type) {
    case JSPARAM_BOOL: {
      bool *value = static_cast<bool *>(param->value_ptr);
      if (!JsTokenToBool(variant, context->js_context(), value)) {
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
    case JSPARAM_OBJECT_TOKEN: {
      JsToken *value = static_cast<JsToken *>(param->value_ptr);
      if (!NPVARIANT_IS_OBJECT(variant)) {
        // TODO(mpcomplete): should we accept null/void here?
        context->SetException(
            STRING16(L"Invalid argument type: expected object."));
        return false;
      }
      *value = variant;
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

      return i;
    }

    if (!ConvertTokenToArgument(this, argv_[i], &output_argv[i]))
      return i;
  }

  return output_argc;
}

void JsCallContext::SetReturnValue(JsParamType type, const void *value_ptr) {
  assert(value_ptr != NULL || type == JSPARAM_NULL);

  JsParamToSend retval = { type, value_ptr };
  ScopedNPVariant np_retval;
  ConvertJsParamToToken(retval, &np_retval);
  *retval_ = np_retval;

  // In NPAPI, return values from callbacks are released by the browser.
  // Therefore, we give up ownership of this variant without releasing it.
  np_retval.Release();
}

void JsCallContext::SetReturnValue(JsParamType type, int) {
  assert(type == JSPARAM_NULL);
  SetReturnValue(type, reinterpret_cast<const void*>(NULL));
}

void JsCallContext::SetReturnValue(JsParamType type,
                                   const ModuleImplBaseClass *value_ptr) {
  assert(type == JSPARAM_MODULE);
  SetReturnValue(type, reinterpret_cast<const void*>(value_ptr));
}

void JsCallContext::SetException(const std::string16 &message) {
  LOG((message.c_str()));

  std::string message_utf8;
  if (!String16ToUTF8(message.data(), message.length(), &message_utf8))
    message_utf8 = "Unknown Gears Error";  // better to throw *something*

  NPN_SetException(object_, message_utf8.c_str());
}

#endif

//-----------------------------------------------------------------------------
#if BROWSER_FF  // the rest of this file only applies to Firefox, for now

JsParamFetcher::JsParamFetcher(ModuleImplBaseClass *obj) {
  if (obj->EnvIsWorker()) {
    js_context_ = obj->EnvPageJsContext();
    js_argc_ = obj->JsWorkerGetArgc();
    js_argv_ = obj->JsWorkerGetArgv();
  } else {
    // In the main thread use the caller's current JS context, NOT the context
    // where 'obj' was created.  These can be different!  Each frame has its
    // own JS context, and code can reference 'top.OtherFrame.FooObject'.
    nsresult nr;
    xpc_ = do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr);
    if (xpc_ && NS_SUCCEEDED(nr)) {
      nr = xpc_->GetCurrentNativeCallContext(getter_AddRefs(ncc_));
      if (ncc_ && NS_SUCCEEDED(nr)) {
        ncc_->GetJSContext(&js_context_);
        PRUint32 argc;
        ncc_->GetArgc(&argc);
        js_argc_ = static_cast<int>(argc);
        ncc_->GetArgvPtr(&js_argv_);
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

JsNativeMethodRetval JsSetException(const ModuleImplBaseClass *obj,
                                    const char16 *message) {
  JsContextPtr cx = NULL;
  JsNativeMethodRetval retval = NS_ERROR_FAILURE;

  if (obj->EnvIsWorker()) {
    cx = obj->EnvPageJsContext();
    if (!cx) { return retval; }
  } else {
    nsresult nr;
    nsCOMPtr<nsIXPConnect> xpc;
    xpc = do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr);
    if (!xpc || NS_FAILED(nr)) { return retval; }

    nsCOMPtr<nsIXPCNativeCallContext> ncc;
    nr = xpc->GetCurrentNativeCallContext(getter_AddRefs(ncc));
    if (!ncc || NS_FAILED(nr)) { return retval; }

    ncc->GetJSContext(&cx);
    if (!cx) { return retval; }

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

  JsRunnerInterface *js_runner = obj->GetJsRunner();
  if (!js_runner) { return retval; }

  scoped_ptr<JsRootedToken> error_object(
                                js_runner->NewObject(STRING16(L"Error")));
  if (!error_object.get()) { return retval; }

  // Note: need JS_SetPendingException to bubble 'catch' in workers.
  JS_SetPendingException(cx, error_object->token());

  bool success = js_runner->SetPropertyString(error_object->token(),
                                              STRING16(L"message"),
                                              message);
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
