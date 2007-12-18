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

#include "gears/base/common/string16.h"  // for string16
#include "gears/base/common/common.h"  // for DISALLOW_EVIL_CONSTRUCTORS

#if BROWSER_FF

#include <nsCOMPtr.h>  // for JsParamFetcher
#include "gecko_internal/nsIXPConnect.h" // for JsParamFetcher
#include "gecko_internal/jsapi.h"

// Abstracted types for values used with JavaScript engines.
typedef jsval      JsToken;
typedef jsval      JsScopedToken;  // unneeded in FF, see comment on JsArray
typedef JSContext* JsContextPtr;
typedef nsresult   JsNativeMethodRetval;

#elif BROWSER_IE

#include <windows.h>
// no "base_interface_ie.h" because IE doesn't require a COM base interface

// Abstracted types for values used with JavaScript engines.
typedef VARIANT JsToken;
typedef CComVariant JsScopedToken;
typedef void* JsContextPtr; // unused in IE
typedef HRESULT JsNativeMethodRetval;

#elif BROWSER_NPAPI

#include "npapi.h"
#include "npupp.h"

// Abstracted types for values used with JavaScript engines.
typedef NPVariant JsToken;
typedef NPP JsContextPtr;
typedef NPError JsNativeMethodRetval;

#endif

class JsRootedToken;
typedef JsRootedToken JsRootedCallback;

// Utility functions to convert JsToken to various primitives.
// TODO(aa): Add coercion to these functions (since they include the word "to")
// and add new functions to determine what the token really is.
bool JsTokenToBool(JsToken t, JsContextPtr cx, bool *out);
bool JsTokenToInt(JsToken t, JsContextPtr cx, int *out);
bool JsTokenToString(JsToken t, JsContextPtr cx, std::string16 *out);
bool JsTokenToDouble(JsToken t, JsContextPtr cx, double *out);
bool JsTokenToNewCallback(JsToken t, JsContextPtr cx, JsRootedCallback **out);

// Utility function to check for the JavaScript values null and undefined. We
// usually treat these two identically to prevent confusion.
bool JsTokenIsNullOrUndefined(JsToken t);

#if BROWSER_FF

// A JsToken that won't get GC'd out from under you.
class JsRootedToken {
 public:
  JsRootedToken(JsContextPtr context, JsToken token)
      : context_(context), token_(token) {
    if (JSVAL_IS_GCTHING(token_)) {
      JS_AddRoot(context_, &token_);
    }
  }

  ~JsRootedToken() {
    if (JSVAL_IS_GCTHING(token_)) {
      JS_RemoveRoot(context_, &token_);
    }
  }

  JsToken token() const { return token_; }
  JsContextPtr context() const { return context_; }

  bool GetAsBool(bool *out) const {
    return JsTokenToBool(token_, context_, out);
  }

  bool IsNullOrUndefined() const {
    return JsTokenIsNullOrUndefined(token_);
  }

  // TODO(aa): GetAsString(), etc. But needs restructuring first. See note below
  // above IE implementation.

 private:
  JsContextPtr context_;
  JsToken token_;
  DISALLOW_EVIL_CONSTRUCTORS(JsRootedToken);
};

#elif BROWSER_IE

// A JsToken that won't get GC'd out from under you.
// TODO(aa): This leaks for things like strings and arrays. We need to correctly
// handle lifetime. Also need to think of different requirements for when token
// is an input parameter vs a return value.
class JsRootedToken {
 public:
  JsRootedToken(JsContextPtr context, JsToken token)
       : token_(token) { // IE doesn't use JsContextPtr
    if (token_.vt == VT_DISPATCH) {
      token_.pdispVal->AddRef();
    }
  }

  ~JsRootedToken() {
    if (token_.vt == VT_DISPATCH) {
      token_.pdispVal->Release();
    }
  }

  JsToken token() const { return token_; }
  JsContextPtr context() const { return NULL; }

  bool GetAsBool(bool *out) const {
    return JsTokenToBool(token_, NULL, out);
  }

 private:
  JsToken token_;
  DISALLOW_EVIL_CONSTRUCTORS(JsRootedToken);
};

// Implementations of boilerplate code.
// - We don't currently need GetNativeBaseClass on IE.

#elif BROWSER_NPAPI

// An NPVariant that takes ownership of its value and releases it when it goes
// out of scope.
class ScopedNPVariant : public NPVariant {
 public:
  ScopedNPVariant() { VOID_TO_NPVARIANT(*this); }
  template<class T>
  explicit ScopedNPVariant(T value) { Reset(value); }

  ~ScopedNPVariant() { Reset(); }

  // This is necessary for API transparency.
  ScopedNPVariant& operator=(const NPVariant &value) {
    Reset(value);
    return *this;
  }

  // Frees the old value and replaces it with the new value.  Strings are
  // copied, and objects are retained.
  void Reset();
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

typedef ScopedNPVariant JsScopedToken;

// A JsToken that won't get GC'd out from under you.
class JsRootedToken {
 public:
  JsRootedToken(JsContextPtr context, JsToken token) 
       : context_(context), token_(token) { }

  const JsToken& token() const { return token_; }
  JsContextPtr context() const { return context_; }

  bool GetAsBool(bool *out) const {
    return JsTokenToBool(token_, context_, out);
  }

 private:
  JsContextPtr context_;  // TODO(mpcomplete): figure out if this is necessary
  JsScopedToken token_;
  DISALLOW_EVIL_CONSTRUCTORS(JsRootedToken);
};

#elif BROWSER_SAFARI

// Just placeholder values for Safari since the created workers use a separate
// process for JS execution.
typedef void  JsToken;
typedef void  JsScopedToken;
typedef void* JsContextPtr;
typedef void  JsNativeMethodRetval;

#endif // BROWSER_xyz

// JsArray and JsObject make use of a JsScopedToken, which is a token that
// releases any references it has when it goes out of scope.  This is necessary
// in IE and NPAPI, because getting an array element or object property
// increases the reference count, and we need a way to release that reference
// *after* we're done using the object.  In Firefox, JsToken == JsScopedToken
// because Firefox only gives us a weak pointer to the value.

class JsObject;
struct JsParamToSend;

class JsArray {
 public:
  JsArray();
  bool GetElement(int index, JsScopedToken *out) const;
  bool SetArray(JsToken value, JsContextPtr context);
  bool GetElementAsBool(int index, bool *out) const;
  bool GetElementAsInt(int index, int *out) const;
  bool GetElementAsDouble(int index, double *out) const;
  bool GetElementAsString(int index, std::string16 *out) const;
  bool GetElementAsArray(int index, JsArray *out) const;
  bool GetElementAsObject(int index, JsObject *out) const;
  bool GetElementAsFunction(int index, JsRootedCallback **out) const;
  bool GetLength(int *length) const;
 private:
  // Needs access to the raw JsToken.
  friend void ConvertJsParamToToken(const JsParamToSend &param,
                                    JsContextPtr context,
                                    JsScopedToken *token);

  JsContextPtr js_context_;
  JsScopedToken array_;
};

class JsObject {
 public:
  JsObject();
  bool GetProperty(const std::string16 &name, JsScopedToken *value) const;
  bool SetObject(JsToken value, JsContextPtr context);
  bool GetPropertyAsBool(const std::string16 &name, bool *out) const;
  bool GetPropertyAsInt(const std::string16 &name, int *out) const;
  bool GetPropertyAsString(const std::string16 &name, std::string16 *out) const;
  bool GetPropertyAsArray(const std::string16 &name, JsArray *out) const;
  bool GetPropertyAsObject(const std::string16 &name, JsObject *out) const;
  bool GetPropertyAsDouble(const std::string16 &name, double *out) const;
  bool GetPropertyAsFunction(const std::string16 &name,
                             JsRootedCallback **out) const;
 private:
  // Needs access to the raw JsToken.
  friend void ConvertJsParamToToken(const JsParamToSend &param,
                                    JsContextPtr context,
                                    JsScopedToken *token);

  JsContextPtr js_context_;
  JsScopedToken js_object_;
};

// The JsParam* types define values for sending and receiving JS parameters.
enum JsParamType {
  JSPARAM_BOOL,
  JSPARAM_INT,  // TODO(mpcomplete): deprecate in favor of double?
  JSPARAM_DOUBLE,
  JSPARAM_STRING16,
  JSPARAM_OBJECT,
  JSPARAM_ARRAY,
  JSPARAM_FUNCTION,
  // TODO(mpcomplete): make JsRunner::NewObject return a JsObject
  JSPARAM_TOKEN,  // avoid if possible
  JSPARAM_MODULE,
  JSPARAM_NULL,
};

enum JsParamRequirement {
  JSPARAM_OPTIONAL,
  JSPARAM_REQUIRED,
};

struct JsParamToSend {
  JsParamType type;
  const void *value_ptr;
};

struct JsParamToRecv {
  JsParamType type;
  void *value_ptr;
};

struct JsArgument {
  JsParamRequirement requirement;
  JsParamType type;
  void* value_ptr;
};

// Temporary: npapi only.
#if BROWSER_NPAPI
class ModuleImplBaseClass;

// This class provides an interface for a property or method access on a native
// object from JavaScript.  It allows consumers to retrieve what arguments were
// passed in, and return a value or exception back to the caller.  Any native
// property or method handler should take an instance of this object as a
// parameter.
class JsCallContext {
 public:
  // Only browser-specific wrapper code should need to instantiate this object.
  JsCallContext(JsContextPtr js_context, NPObject *object,
                int argc, const JsToken *argv, JsToken *retval)
      : js_context_(js_context), object_(object),
        argc_(argc), argv_(argv), retval_(retval),
        is_exception_set_(false) {}

  // Get the arguments a JavaScript caller has passed into a scriptable method
  // of a native object.  Returns the number of arguments successfully read
  // (will bail at the first invalid argument).
  int GetArguments(int argc, JsArgument *argv);

  // Sets the value to be returned to the calling JavaScript.
  //
  // The ModuleImplBaseClass* version should only be used when returning a
  // JSPARAM_MODULE.  It exists because passing a derived class through a void*
  // and then casting to the base class is not safe - the compiler won't be able
  // to correctly adjust the pointer offset.
  //
  // The int version is for use with JSPARAM_NULL, to avoid conflicting with the
  // ModuleImplBaseClass version (works because NULL is defined as 0).
  void SetReturnValue(JsParamType type, const void *value_ptr);
  void SetReturnValue(JsParamType type, const ModuleImplBaseClass *value_ptr);
  void SetReturnValue(JsParamType type, int);

  // Sets an exception to be thrown to the calling JavaScript.  Setting an
  // exception overrides any previous exception and any return values.
  void SetException(const std::string16 &message);

  JsContextPtr js_context() { return js_context_; }
  bool is_exception_set() { return is_exception_set_; }

 private:
   JsContextPtr js_context_;
   NPObject *object_;
   int argc_;
   const JsToken *argv_;
   JsToken *retval_;
   bool is_exception_set_;
};
#endif

// Given a JsParamToSend, extract it into a JsScopedToken.  Resulting token
// increases the reference count for objects.
void ConvertJsParamToToken(const JsParamToSend &param,
                           JsContextPtr context, JsScopedToken *token);

//-----------------------------------------------------------------------------
#if BROWSER_FF  // the rest of this file only applies to Firefox, for now

class ModuleImplBaseClass;

// Helper class to extract JavaScript parameters (including optional params
// and varargs), in a way that hides differences between main-thread and
// worker-thread environments.
class JsParamFetcher {
 public:
  explicit JsParamFetcher(ModuleImplBaseClass *obj);

  JsContextPtr GetContextPtr() { return js_context_; }
  int GetCount(bool has_mysterious_retval) {
    return has_mysterious_retval ? js_argc_ - 1 : js_argc_;
  }
  // In Firefox, set has_string_retval iff method has a string return value.
  bool IsOptionalParamPresent(int i, bool has_string_retval);

  // All functions check the type of the requested value, and they will return
  // false on a mismatch.
  bool GetAsInt(int i, int *out);
  bool GetAsString(int i, std::string16 *out);
  bool GetAsArray(int i, JsArray *out);
  bool GetAsObject(int i, JsObject *out);
  bool GetAsNewRootedCallback(int i, JsRootedCallback **out);

 private:
  JsContextPtr  js_context_;
  int           js_argc_;
  JsToken      *js_argv_;

#if BROWSER_FF
  nsCOMPtr<nsIXPConnect> xpc_;
  nsCOMPtr<nsIXPCNativeCallContext> ncc_;
#elif BROWSER_IE
#endif
};


// Sets JavaScript exceptions, in a way that hides differences
// between main-thread and worker-thread environments.
// Returns the value the caller should return to the JavaScript host engine.
JsNativeMethodRetval JsSetException(const ModuleImplBaseClass *obj,
                                    const char16 *message);

// Garbage collection helper functions
bool RootJsToken(JsContextPtr cx, JsToken t);


#endif // BROWSER_FF
//-----------------------------------------------------------------------------


#endif  // GEARS_BASE_COMMON_JS_TYPES_H__
