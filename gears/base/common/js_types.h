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

#ifndef GEARS_BASE_COMMON_JS_TYPES_H__
#define GEARS_BASE_COMMON_JS_TYPES_H__

#include <assert.h>
#include <functional>
#include <vector>
#include "gears/base/common/basictypes.h"  // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"  // for string16

class JsArray;
class JsObject;
class JsRootedToken;
class JsRunnerInterface;
class MarshaledJsToken;
class ModuleImplBaseClass;
struct ModuleEnvironment;
typedef JsRootedToken JsRootedCallback;

// TODO(michaeln): Split into multiple browser specific files.

//------------------------------------------------------------------------------
// JsToken and friends
//------------------------------------------------------------------------------

#if BROWSER_FF

#include <gecko_internal/jsapi.h>

// Abstracted types for values used with JavaScript engines.
typedef jsval JsToken;
typedef jsval JsScopedToken;  // unneeded in FF, see comment on JsArray
typedef JSContext* JsContextPtr;

#elif BROWSER_IE || BROWSER_IEMOBILE

#include <windows.h>
// no "base_interface_ie.h" because IE doesn't require a COM base interface

// Abstracted types for values used with JavaScript engines.
typedef VARIANT JsToken;
typedef CComVariant JsScopedToken;
typedef void* JsContextPtr;  // unused in IE

#elif BROWSER_WEBKIT

#include <WebKit/npapi.h>
#include <WebKit/npfunctions.h>
#include <WebKit/npruntime.h>

#elif BROWSER_NPAPI

#include "third_party/npapi/nphostapi.h"

#endif

#if BROWSER_NPAPI || BROWSER_WEBKIT
// An NPVariant that takes ownership of its value and releases it when it goes
// out of scope.
class ScopedNPVariant : public NPVariant {
 public:
  ScopedNPVariant() { VOID_TO_NPVARIANT(*this); }
  // Must not be 'explicit' so we can use it as a JsScopedToken transparently.
  template<class T>
  ScopedNPVariant(T value) { VOID_TO_NPVARIANT(*this); Reset(value); }

  ~ScopedNPVariant() { Reset(); }

  // This is necessary so we can use it as a JsScopedToken transparently.
  ScopedNPVariant& operator=(const NPVariant &value) {
    Reset(value);
    return *this;
  }
  ScopedNPVariant& operator=(const ScopedNPVariant &value) {
    Reset(value);
    return *this;
  }

  // Frees the old value and replaces it with the new value.  Strings are
  // copied, and objects are retained.
  void Reset();
  void ResetToNull();
  void Reset(bool value);
  void Reset(int value);
  void Reset(double value);
  void Reset(const char *value);
  void Reset(const char16 *value);
  void Reset(NPObject *value);
  void Reset(const NPVariant &value);

  // Gives up ownership of this variant, without freeing or releasing the
  // underyling object.  The variant will be VOID after this call.
  void Release();
};

// Abstracted types for values used with JavaScript engines.
typedef NPVariant JsToken;
typedef ScopedNPVariant JsScopedToken;
typedef NPP JsContextPtr;

struct JsTokenEqualTo : public std::binary_function<JsToken, JsToken, bool>  {
  JsTokenEqualTo(NPP npp);
  JsTokenEqualTo(const JsTokenEqualTo& that) { *this = that; }
  ~JsTokenEqualTo();

  JsTokenEqualTo& operator=(const JsTokenEqualTo &that);

  bool operator()(const JsToken &x, const JsToken &y) const  {
    // All we are looking for in this comparator is that different NPVariants
    // will compare differently, but that the same NPObject* (wrapped as a
    // NPVariant) will compare the same.  A non-goal is that the NPVariant
    // representing the integer 3 is "equal to" one representing 3.0.
    if (x.type != y.type) {
      return false;
    }
    switch (x.type) {
      case NPVariantType_Void:
        return true;
        break;
      case NPVariantType_Null:
        return true;
        break;
      case NPVariantType_Bool:
        return x.value.boolValue == y.value.boolValue;
        break;
      case NPVariantType_Int32:
        return x.value.intValue == y.value.intValue;
        break;
      case NPVariantType_Double:
        return x.value.doubleValue == y.value.doubleValue;
        break;
      case NPVariantType_String:
        // TODO(michaeln): compare string values rather than pointers?
        return x.value.stringValue.UTF8Characters ==
               y.value.stringValue.UTF8Characters;
        break;
      case NPVariantType_Object:
        return CompareObjects(x, y);
        break;
      default:
        // do nothing
        break;
    }
    return false;
  }
 private:
  bool CompareObjects(const JsToken &x, const JsToken &y) const;

  NPP npp_;
  ScopedNPVariant compare_func_;
};

#endif  // BROWSER_NPAPI || BROWSER_WEBKIT

// The JsParam* types define values for sending and receiving JS parameters.
enum JsParamType {
  JSPARAM_BOOL,
  JSPARAM_INT,
  JSPARAM_INT64,
  JSPARAM_DOUBLE,
  JSPARAM_STRING16,
  JSPARAM_OBJECT,
  JSPARAM_ARRAY,
  JSPARAM_FUNCTION,
  JSPARAM_MODULE,
  // JSPARAM_DOM_ELEMENT should only be used from the main JavaScript thread,
  // not from worker threads.
  JSPARAM_DOM_ELEMENT,
  JSPARAM_NULL,
  JSPARAM_UNDEFINED,
  JSPARAM_UNKNOWN,
  JSPARAM_ABSTRACT_TOKEN,
  // TODO(nigeltao): Can we eliminate JSPARAM_TOKEN? Is it used anymore?
  JSPARAM_TOKEN
};

enum JsParamRequirement {
  JSPARAM_OPTIONAL,
  JSPARAM_REQUIRED,
};

struct JsParamToSend {
  JsParamType type;
  const void *value_ptr;
};

struct JsArgument {
  JsParamRequirement requirement;
  JsParamType type;
  void* value_ptr;
  bool was_specified;
};

// A JsToken is a script-engine specific concept of a C++ data structure that
// represents a JavaScript value (e.g. an integer, or a JS object). For
// example, for Mozilla's Spidermonkey, a JsToken is a jsval, and for IE's
// ActiveScript, a JsToken is a VARIANT.
//
// An AbstractJsToken is a cross-platform abstraction of the various JsToken
// implementations, so that code that needs to manipulate such things (i.e.
// the js_marshal code) can do so without being script-engine specfic.
//
// An AbstractJsToken can be copied using regular operator=, and the copy will
// refer to the same underlying JsToken. Typically, an AbstractJsToken will be
// a pointer to something, and quite possibly a pointer to something on the
// stack (e.g. in a stack frame belonging to a JsCallContext call). Thus, code
// that manipulates an AbstractJsToken should not keep a copy of one that will
// live for longer than the current stack frame, such as storing a copy in a
// member variable.
// 
// However, the way in which an AbstractJsToken is mapped to and from the
// script-engine specific JsToken is a private implementation detail, and
// should not be relied upon.
typedef void* AbstractJsToken;
typedef std::vector<AbstractJsToken> AbstractJsTokenVector;

// TODO(nigeltao): Eliminate this function. Only code inside js_types_xx.cc
// should manipulate or refer to JsToken instances. Currently, the only other
// use of JsToken is in js_marshal.cc, and we should be able to fix up
// JsScopedToken so that the js_marshal code need not need to know what a
// (script-engine specific) JsToken is.
AbstractJsToken JsTokenPtrToAbstractJsToken(JsToken *token);

//------------------------------------------------------------------------------
// JsTokenToXxx, XxxToJsToken
//------------------------------------------------------------------------------
// TODO(nigeltao): Take this section out of js_types.h and into a separate
// js_types_xx.h file, for xx in {activescript,npapi,spidermonkey}. Other code
// shouldn't manipulate JsTokens directly, they should use the public methods
// of JsRunnerInterface and JsCallContext.

// Utility functions to convert JsToken to various C++ types without coercion.
// If the type of the underlying JavaScript variable does not exactly match the
// requested type, these functions return false.
bool JsTokenToBool_NoCoerce(JsToken t, JsContextPtr cx, bool *out);
bool JsTokenToInt_NoCoerce(JsToken t, JsContextPtr cx, int *out);
bool JsTokenToInt64_NoCoerce(JsToken t, JsContextPtr cx, int64 *out);
bool JsTokenToDouble_NoCoerce(JsToken t, JsContextPtr cx, double *out);
bool JsTokenToString_NoCoerce(JsToken t, JsContextPtr cx, std::string16 *out);
bool JsTokenToObject_NoCoerce(JsToken t, JsContextPtr cx, JsObject **out);
bool JsTokenToArray_NoCoerce(JsToken t, JsContextPtr cx, JsArray **out);
bool JsTokenToNewCallback_NoCoerce(JsToken t, JsContextPtr cx,
                                   JsRootedCallback **out);
bool JsTokenToModule(JsRunnerInterface *js_runner,
                     JsContextPtr context,
                     const JsToken in,
                     ModuleImplBaseClass **out);

// Utility functions to coerce JsTokens to various C++ types. These functions
// implement coercion as defined by ECMA 262-3 Section 9:
// http://bclary.com/2004/11/07/#a-9
// They return false if the coercion could not be performed.
bool JsTokenToBool_Coerce(JsToken t, JsContextPtr cx, bool *out);
bool JsTokenToInt_Coerce(JsToken t, JsContextPtr cx, int *out);
bool JsTokenToString_Coerce(JsToken t, JsContextPtr cx, std::string16 *out);
bool JsTokenToDouble_Coerce(JsToken t, JsContextPtr cx, double *out);

// Utility function to determine the type of a JsToken.
JsParamType JsTokenGetType(JsToken t, JsContextPtr cx);

// Utility function to tell if a given token is a JavaScript function.
bool JsTokenIsCallback(JsToken t, JsContextPtr cx);

// Utility function to tell if a given token is a JavaScript array.
bool JsTokenIsArray(JsToken t, JsContextPtr cx);

// Utility function to tell if a given token is a JavaScript object. This
// function returns true for all JavaScript objects, including things like
// functions and Date objects. It returns false for all primitive values
// including null and undefined.
bool JsTokenIsObject(JsToken t);

// Utility function to check for the JavaScript values null and undefined. We
// usually treat these two identically to prevent confusion.
bool JsTokenIsNullOrUndefined(JsToken t);

bool BoolToJsToken(JsContextPtr context, bool value, JsScopedToken *out);
bool IntToJsToken(JsContextPtr context, int value, JsScopedToken *out);
bool StringToJsToken(JsContextPtr context, const char16 *value,
                     JsScopedToken *out);
bool DoubleToJsToken(JsContextPtr context, double value, JsScopedToken *out);
bool NullToJsToken(JsContextPtr context, JsScopedToken *out);
bool UndefinedToJsToken(JsContextPtr context, JsScopedToken *out);

//------------------------------------------------------------------------------
// JsRootedToken
//------------------------------------------------------------------------------

// A JsToken that won't get GC'd out from under you.
// TODO(nigeltao): On BROWSER_IE, this might leak for things like strings and
// arrays. We should measure whether or not we are leaking, and if so, better
// handle lifetime. We also need to think of different requirements for when
// token is an input parameter vs a return value.
class JsRootedToken {
 public:
  JsRootedToken(JsContextPtr context, JsToken token);
  ~JsRootedToken();

  // GetAsXxx returns success or failure of the call. Failure could happen if,
  // for example, GetAsString was called on an int token - type coercion is
  // not applied.
  bool GetAsString(std::string16 *out) const;

  // This function does not return the success or failure of the call - the
  // function call always succeeds. Instead, it returns the value of this
  // JsToken, coerced to a boolean value (e.g. the integer 1 coerces to true).
  bool CoerceToBool() const;

  bool IsValidCallback() const;

  JsToken token() const { return token_; }
  JsContextPtr context() const { return context_; }

 private:
  JsContextPtr context_;
  // TODO(nigeltao): this #ifdef just matches the old NPAPI functionality.
  // Ideally we wouldn't need it.
#if BROWSER_NPAPI
  JsScopedToken token_;
#else
  JsToken token_;
#endif
  DISALLOW_EVIL_CONSTRUCTORS(JsRootedToken);
};

//------------------------------------------------------------------------------
// JsArray
//------------------------------------------------------------------------------

// JsArray and JsObject make use of a JsScopedToken, which is a token that
// releases any references it has when it goes out of scope.  This is necessary
// in IE and NPAPI, because getting an array element or object property
// increases the reference count, and we need a way to release that reference
// *after* we're done using the object.  In Firefox, JsToken == JsScopedToken
// because Firefox only gives us a weak pointer to the value.

class JsArray {
 public:
  virtual ~JsArray() {};

  virtual bool GetLength(int *length) const = 0;

  // use the same syntax as JsRootedToken
  virtual const JsScopedToken &token() const = 0;

  // GetElementXxx returns false on failure, including if the requested element
  // does not exist.
  virtual bool GetElementAsBool(int index, bool *out) const = 0;
  virtual bool GetElementAsInt(int index, int *out) const = 0;
  virtual bool GetElementAsDouble(int index, double *out) const = 0;
  virtual bool GetElementAsString(int index, std::string16 *out) const = 0;
  virtual bool GetElementAsArray(int index, JsArray **out) const = 0;
  virtual bool GetElementAsObject(int index, JsObject **out) const = 0;
  virtual bool GetElementAsFunction(int index,
                                    JsRootedCallback **out) const = 0;

  // This method will type-coerce the element to a string, even if it wasn't
  // already one. For example, the integer 17 would become the string "17".
  virtual bool GetElementAsStringWithCoercion(int index,
                                              std::string16 *out) const = 0;

  // Returns JSPARAM_UNDEFINED if the requested element does not exist.
  virtual JsParamType GetElementType(int index) const = 0;

  virtual bool SetElementBool(int index, bool value) = 0;
  virtual bool SetElementInt(int index, int value) = 0;
  virtual bool SetElementDouble(int index, double value) = 0;
  virtual bool SetElementString(int index, const std::string16 &value) = 0;
  virtual bool SetElementArray(int index, JsArray *value) = 0;
  virtual bool SetElementObject(int index, JsObject *value) = 0;
  virtual bool SetElementFunction(int index, JsRootedCallback *value) = 0;
  virtual bool SetElementModule(int index, ModuleImplBaseClass *value) = 0;
  virtual bool SetElementNull(int index) = 0;
  virtual bool SetElementUndefined(int index) = 0;

  // TODO(nigeltao): These two methods should really be private.
  virtual bool GetElement(int index, JsScopedToken *out) const = 0;
  virtual bool SetElement(int index, const JsScopedToken &value) = 0;
};

//------------------------------------------------------------------------------
// JsObject
//------------------------------------------------------------------------------

class JsObject {
 public:
  virtual ~JsObject() {}

  // GetPropertyXxx returns false on failure, including if the requested element
  // does not exist.
  virtual bool GetPropertyAsBool(const std::string16 &name,
                                 bool *out) const = 0;
  virtual bool GetPropertyAsInt(const std::string16 &name,
                                int *out) const = 0;
  virtual bool GetPropertyAsDouble(const std::string16 &name,
                                   double *out) const = 0;
  virtual bool GetPropertyAsString(const std::string16 &name,
                                   std::string16 *out) const = 0;
  virtual bool GetPropertyAsArray(const std::string16 &name,
                                  JsArray **out) const = 0;
  virtual bool GetPropertyAsObject(const std::string16 &name,
                                   JsObject **out) const = 0;
  virtual bool GetPropertyAsFunction(const std::string16 &name,
                                     JsRootedCallback **out) const = 0;

  // Returns JSPARAM_UNDEFINED if the requested property does not exist.
  virtual JsParamType GetPropertyType(const std::string16 &name) const = 0;
  
  // GetPropertyNames fills the given vector with the (string) names of this
  // JsObject's properties.  There is no guarantee that it retrieves (or does
  // not retrieve) property names from the object's prototype, nor does it
  // rule anything in or out about property keys that aren't strings.  Also,
  // this only *appends* property names to the vector out.  In particular, it
  // does not reset the vector to be empty.
  virtual bool GetPropertyNames(std::vector<std::string16> *out) const = 0;

  // SetPropertyXxx() overwrites the existing named property or adds a new one
  // if none exists.
  virtual bool SetPropertyBool(const std::string16& name,
                               bool value) = 0;
  virtual bool SetPropertyInt(const std::string16 &name,
                              int value) = 0;
  virtual bool SetPropertyDouble(const std::string16& name,
                                 double value) = 0;
  virtual bool SetPropertyString(const std::string16 &name,
                                 const std::string16 &value) = 0;
  virtual bool SetPropertyArray(const std::string16& name,
                                JsArray* value) = 0;
  virtual bool SetPropertyObject(const std::string16& name,
                                 JsObject* value) = 0;
  virtual bool SetPropertyFunction(const std::string16& name,
                                   JsRootedCallback* value) = 0;
  virtual bool SetPropertyModule(const std::string16& name,
                                 ModuleImplBaseClass* value) = 0;
  virtual bool SetPropertyNull(const std::string16 &name) = 0;
  virtual bool SetPropertyUndefined(const std::string16 &name) = 0;
  virtual bool SetPropertyMarshaledJsToken(
      const std::string16& name,
      ModuleEnvironment* module_environment,
      MarshaledJsToken* value) = 0;

  virtual const JsScopedToken &token() const = 0;

  // TODO(nigeltao): These two methods should really be private.
  virtual bool GetProperty(const std::string16 &name,
                           JsScopedToken *value) const = 0;
  virtual bool SetProperty(const std::string16 &name,
                           const JsToken &value) = 0;
};

//------------------------------------------------------------------------------
// ConvertJsParamToToken, JsCallContext
//------------------------------------------------------------------------------

// Given a JsParamToSend, extract it into a JsScopedToken.  Resulting token
// increases the reference count for objects.
// Returns true if successful. On failure, token is not modified.
// Failure could happen if the caller tried to return a numerical value that
// is outside the range of numbers representable by the particular JavaScript
// engine. For example, if that engine internally only uses int32s or doubles,
// then some int64 values are not representable.
bool ConvertJsParamToToken(const JsParamToSend &param,
                           JsContextPtr context, JsScopedToken *token);

// This class provides an interface for a property or method access on a native
// object from JavaScript.  It allows consumers to retrieve what arguments were
// passed in, and return a value or exception back to the caller.  Any native
// property or method handler should take an instance of this object as a
// parameter.
class JsCallContext {
 public:
  // Only browser-specific wrapper code should need to instantiate this object.
#if BROWSER_NPAPI
  JsCallContext(JsContextPtr js_context, NPObject *object,
                int argc, const JsToken *argv, JsToken *retval);
#elif BROWSER_IE || BROWSER_IEMOBILE
  JsCallContext(DISPPARAMS FAR *disp_params, VARIANT FAR *retval,
                EXCEPINFO FAR *excep_info);
#elif BROWSER_FF
  JsCallContext(JsContextPtr cx, JsRunnerInterface *js_runner,
                int argc, JsToken *argv, JsToken *retval);
#endif

  ~JsCallContext();

  // Get the arguments a JavaScript caller has passed into a scriptable method
  // of a native object. Arguments may be required or optional and these types
  // may be interspersed freely. Required arguments must always be specified,
  // and they cannot be null or undefined.  Optional arguments may be null or
  // undefined, or they can be omitted entirely if not followed by any required
  // arguments. Will fail if a required argument is missing or if an invalid
  // type is passed. In this case, sets an exception on the context and returns
  // false. Returns true otherwise.
  bool GetArguments(int argc, JsArgument *argv);

  // Get the type of an argument that was passed in.
  JsParamType GetArgumentType(int i);
  int GetArgumentCount();

  // TODO(nigeltao): Do we really need the coercion parameter? JS type coercion
  // was introduced as part of the Console code, and was to be introduced
  // generally, but we backed off because changing APIs from non-coercing to
  // coercing is something we can't undo, if we want to maintain backwards
  // compatability in future releases.
  bool GetArgumentAsBool(int i, bool *out, bool coerce=false);
  bool GetArgumentAsInt(int i, int *out, bool coerce=false);
  bool GetArgumentAsDouble(int i, double *out, bool coerce=false);
  bool GetArgumentAsString(int i, std::string16 *out, bool coerce=false);

  // Sets the value to be returned to the calling JavaScript.
  void SetReturnValue(JsParamType type, const void *value_ptr);

  // Sets an exception to be thrown to the calling JavaScript.  Setting an
  // exception overrides any previous exception and any return values.
  void SetException(const std::string16 &message);

  JsContextPtr js_context() { return js_context_; }
  bool is_exception_set() { return is_exception_set_; }
  bool is_return_value_set() { return is_return_value_set_; }
#if BROWSER_FF
  JsRunnerInterface *js_runner() { return js_runner_; }
#endif

 private:
  const JsToken &GetArgument(int index);

  JsContextPtr js_context_;
  bool is_exception_set_;
  bool is_return_value_set_;
#if BROWSER_NPAPI
  NPObject *object_;
  int argc_;
  const JsToken *argv_;
  JsToken *retval_;
#elif BROWSER_IE || BROWSER_IEMOBILE
  DISPPARAMS FAR *disp_params_;
  VARIANT FAR *retval_;
  EXCEPINFO FAR *exception_info_;
#elif BROWSER_FF
  int argc_;
  JsToken *argv_;
  JsToken *retval_;
  JsRunnerInterface *js_runner_;
#endif

  DISALLOW_EVIL_CONSTRUCTORS(JsCallContext);
};

#endif  // GEARS_BASE_COMMON_JS_TYPES_H__
