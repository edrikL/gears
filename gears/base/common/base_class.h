// Copyright 2006, Google Inc.
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
//
// The C++ base class that all Gears objects should derive from.

#ifndef GEARS_BASE_COMMON_BASE_CLASS_H__
#define GEARS_BASE_COMMON_BASE_CLASS_H__

#include "gears/base/common/security_model.h"
#include "gears/base/common/common.h" // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/string16.h" // for string16


#if BROWSER_FF

#include <nsCOMPtr.h> // for JsParamFetcher
#include "gears/third_party/gecko_internal/nsIXPConnect.h" // for JsParamFetcher
#include "gears/third_party/gecko_internal/jsapi.h"
#include "ff/genfiles/base_interface_ff.h"

// Abstracted types for values used with JavaScript engines.
typedef jsval      JsToken;
typedef JSContext* JsContextPtr;
typedef nsresult   JsNativeMethodRetval;

#elif BROWSER_IE

#include <windows.h>
// no "base_interface_ie.h" because IE doesn't require a COM base interface

// Abstracted types for values used with JavaScript engines.
typedef VARIANT JsToken;
typedef void* JsContextPtr; // unused in IE
typedef HRESULT JsNativeMethodRetval;

#endif

// Utility functions to convert JsToken to various primitives.
// TODO(aa): Add coercion to these functions (since they include the word "to")
// and add new functions to determine what the token really is.
bool JsTokenToBool(JsToken t, JsContextPtr cx, bool *out);
bool JsTokenToInt(JsToken t, JsContextPtr cx, int *out);
bool JsTokenToString(JsToken t, JsContextPtr cx, std::string16 *out);

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

  // TODO(aa): GetAsString(), etc. But needs restructuring first. See note below
  // above IE implementation.

 private:
  JsContextPtr context_;
  JsToken token_;
  DISALLOW_EVIL_CONSTRUCTORS(JsRootedToken);
};

// Implementations of boilerplate code.
#define GEARS_IMPL_BASECLASS \
  NS_IMETHOD GetNativeBaseClass(GearsBaseClass **retval) { \
    *retval = this; \
    return NS_OK; \
  }

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

#elif BROWSER_SAFARI

// Just placeholder values for Safari since the created workers use a separate
// process for JS execution.
typedef void  JsToken;
typedef void* JsContextPtr;
typedef void  JsNativeMethodRetval;

#endif // BROWSER_xyz


typedef JsRootedToken JsRootedCallback;

class JsRunnerInterface;

// Exposes the minimal set of information that Scour objects need to work
// consistently across the main-thread and worker-thread JavaScript engines.
class GearsBaseClass {
 public:
  GearsBaseClass() : is_initialized_(false) {}

  // Initialization functions
  //
  // Init from sibling -- should be used for most objects.
  // Init from DOM -- should only be used for main-thread factories.
  bool InitBaseFromSibling(const GearsBaseClass *other);
#if BROWSER_FF
  bool InitBaseFromDOM();
#elif BROWSER_IE
  bool InitBaseFromDOM(IUnknown *site);
#elif BROWSER_SAFARI
  bool InitBaseFromDOM(const char *url_str);
#endif

  // Host environment information
  bool EnvIsWorker() const;
  const std::string16& EnvPageLocationUrl() const;
#if BROWSER_FF
  JsContextPtr EnvPageJsContext() const;
#elif BROWSER_IE
  IUnknown* EnvPageIUnknownSite() const;
#endif
  const SecurityOrigin& EnvPageSecurityOrigin() const;

  JsRunnerInterface *GetJsRunner() const;

#if BROWSER_FF
  // JavaScript worker-thread parameter information
  void JsWorkerSetParams(int argc, JsToken *argv);
  int          JsWorkerGetArgc() const;
  JsToken*     JsWorkerGetArgv() const;
#elif BROWSER_IE
  // These do not exist in IE yet.
#endif

 private:
  // TODO(cprince): This state should be constant per (thread,page) tuple.
  // Instead of making a copy for every object (GearsBaseClass), we could
  // keep a reference to one shared class per tuple.
  // (Recall idea for PageSharedState + PageThreadSharedState classes.)
  bool is_initialized_;
  bool env_is_worker_;
#if BROWSER_FF
  JsContextPtr  env_page_js_context_;
#elif BROWSER_IE
  CComPtr<IUnknown> env_page_iunknown_site_;
#endif
  SecurityOrigin env_page_origin_;

  // Init manually -- should only be used for worker-thread factories.
  friend class PoolThreadsManager;
  bool InitBaseManually(bool is_worker,
#if BROWSER_FF
                        JsContextPtr cx,
#elif BROWSER_IE
                        IUnknown *site,
#endif
                        const SecurityOrigin &page_origin,
                        JsRunnerInterface *js_runner);

#if BROWSER_FF
  int           worker_js_argc_;
  JsToken      *worker_js_argv_;
#elif BROWSER_IE
  // These do not exist in IE yet.
#endif

  JsRunnerInterface *js_runner_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsBaseClass);
};


//-----------------------------------------------------------------------------
#if BROWSER_FF  // the rest of this file only applies to Firefox, for now

// Helper class to extract JavaScript parameters (including optional params
// and varargs), in a way that hides differences between main-thread and
// worker-thread environments.
class JsParamFetcher {
 public:
  explicit JsParamFetcher(GearsBaseClass *obj);

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
  bool GetAsArray(int i, JsToken *out_array, int *out_length);
  bool GetAsNewRootedCallback(int i, JsRootedCallback **out);

  // TODO(aa): This should probably go away because the tokens you get can
  // become invalid if you store them.
  bool GetFromArrayAsToken(JsToken array, int i, JsToken *out);
  bool GetFromArrayAsInt(JsToken array, int i, int *out);
  bool GetFromArrayAsString(JsToken array, int i, std::string16 *out);

 private:
  bool ArrayIndexToToken(JsToken array, int i, JsToken *out);

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
JsNativeMethodRetval JsSetException(const GearsBaseClass *obj,
                                    const char16 *message);

// Garbage collection helper functions
bool RootJsToken(JsContextPtr cx, JsToken t);


#endif // BROWSER_FF
//-----------------------------------------------------------------------------


#endif // GEARS_BASE_COMMON_BASE_CLASS_H__
