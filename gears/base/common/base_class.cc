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

#include <assert.h>

#include "gears/base/common/base_class.h"
#include "gears/base/common/security_model.h" // for kUnknownDomain
#include "gears/base/common/js_runner.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#if BROWSER_FF
#include "gears/base/firefox/dom_utils.h"
#elif BROWSER_IE
#include "gears/base/ie/activex_utils.h"
#elif BROWSER_SAFARI
#include "gears/base/safari/browser_utils.h"
#endif


bool GearsBaseClass::InitBaseFromSibling(const GearsBaseClass *other) {
  assert(other->is_initialized_);
  return InitBaseManually(other->env_is_worker_,
#if BROWSER_FF
                          other->env_page_js_context_,
#elif BROWSER_IE
                          other->env_page_iunknown_site_,
#endif
                          other->env_page_origin_,
                          other->js_runner_);
}


#if BROWSER_FF
bool GearsBaseClass::InitBaseFromDOM() {
#elif BROWSER_IE
bool GearsBaseClass::InitBaseFromDOM(IUnknown *site) {
#elif BROWSER_SAFARI
bool GearsBaseClass::InitBaseFromDOM(const char *url_str) {
#endif
  bool is_worker = false;
  SecurityOrigin security_origin;
#if BROWSER_FF
  JsContextPtr cx;
  bool succeeded = DOMUtils::GetJsContext(&cx) &&
                   DOMUtils::GetPageOrigin(&security_origin);
  return succeeded && InitBaseManually(is_worker, cx, security_origin,
                                       NewDocumentJsRunner(NULL, cx));
#elif BROWSER_IE
  bool succeeded = ActiveXUtils::GetPageOrigin(site, &security_origin);
  return succeeded && InitBaseManually(is_worker, site, security_origin,
                                       NewDocumentJsRunner(site, NULL));
#elif BROWSER_SAFARI
  bool succeeded = SafariURLUtilities::GetPageOrigin(url_str, &security_origin);
  return succeeded && InitBaseManually(is_worker, security_origin, 0);
#endif
}


bool GearsBaseClass::InitBaseManually(bool is_worker,
#if BROWSER_FF
                                      JsContextPtr cx,
#elif BROWSER_IE
                                      IUnknown *site,
#endif
                                      const SecurityOrigin &page_origin,
                                      JsRunnerInterface *js_runner) {
  assert(!is_initialized_);
  // We want to make sure page_origin is initialized, but that isn't exposed
  // directly.  So access any member to trip its internal 'initialized_' assert.
  assert(page_origin.port() != -1);

  env_is_worker_ = is_worker;
#if BROWSER_FF
  env_page_js_context_ = cx;
#elif BROWSER_IE
  env_page_iunknown_site_ = site;
#endif
  env_page_origin_ = page_origin;
  js_runner_ = js_runner;

#if BROWSER_FF
  worker_js_argc_ = 0;
  worker_js_argv_ = NULL;
#elif BROWSER_IE
  // These do not exist in IE yet.
#endif

  is_initialized_ = true;
  return true;
}


bool GearsBaseClass::EnvIsWorker() const {
  assert(is_initialized_);
  return env_is_worker_;
}

const std::string16& GearsBaseClass::EnvPageLocationUrl() const {
  assert(is_initialized_);
  return env_page_origin_.full_url();
}

#if BROWSER_FF
JsContextPtr GearsBaseClass::EnvPageJsContext() const {
  assert(is_initialized_);
  return env_page_js_context_;
}
#elif BROWSER_IE
IUnknown* GearsBaseClass::EnvPageIUnknownSite() const {
  assert(is_initialized_);
  return env_page_iunknown_site_;
}
#endif

const SecurityOrigin& GearsBaseClass::EnvPageSecurityOrigin() const {
  assert(is_initialized_);
  return env_page_origin_;
}

JsRunnerInterface *GearsBaseClass::GetJsRunner() const {
  assert(is_initialized_);
  return js_runner_;
}


//-----------------------------------------------------------------------------
#if BROWSER_FF  // the rest of this file only applies to Firefox, for now


void GearsBaseClass::JsWorkerSetParams(int argc, JsToken *argv) {
  assert(is_initialized_);
  worker_js_argc_ = argc;
  worker_js_argv_ = argv;
}

int GearsBaseClass::JsWorkerGetArgc() const {
  assert(is_initialized_);
  assert(EnvIsWorker());
  return worker_js_argc_;
}

JsToken* GearsBaseClass::JsWorkerGetArgv() const {
  assert(is_initialized_);
  assert(EnvIsWorker());
  return worker_js_argv_;
}



JsParamFetcher::JsParamFetcher(GearsBaseClass *obj) {
  if (obj->EnvIsWorker()) {
    js_context_ = obj->EnvPageJsContext();
    js_argc_ = obj->JsWorkerGetArgc();
    js_argv_ = obj->JsWorkerGetArgv();
  } else {
    nsresult nr;
    // TODO(cprince): Use context from 'obj' instead of 'ncc_'.
    // Also, can we get argc/argv from that JS context, instead of from 'ncc_'?
    xpc_ = do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr);
    if (xpc_ && NS_SUCCEEDED(nr)) {
      nr = xpc_->GetCurrentNativeCallContext(getter_AddRefs(ncc_));
      if (ncc_ && NS_SUCCEEDED(nr)) {
        ncc_->GetJSContext(&js_context_);
        assert(js_context_ == obj->EnvPageJsContext());
        PRUint32 argc;
        ncc_->GetArgc(&argc);
        js_argc_ = static_cast<int>(argc);
        ncc_->GetArgvPtr(&js_argv_);
      }
    }
  }
}


// We need has_string_retval for correct lookups, since sometimes retvals are
// required parameters and count toward js_argc_.
//
// Consider a Firefox function with optional params and a string retval.  XPIDL
// makes the retval the final param.  So params[0] might point at an optional
// param or the required retval. We need has_string_retval to know which it is.
//
// TODO(aa): has_mysterious_retval is named thusly because we have observed that
// sometimes other types than strings cause the behavior described above. This
// needs to be investigated and encapsulated.
bool JsParamFetcher::IsOptionalParamPresent(int i, bool has_mysterious_retval) {
  // Return false if there weren't enough params.
  if (i >= GetCount(has_mysterious_retval)) {
    return false;
  }

  JsToken token;

  // Also return false if the value was <undefined> or <null>.
  GetAsToken(i, &token);
  if (JSVAL_IS_VOID(token) || JSVAL_IS_NULL(token)) {
    return false;
  }

  return true;
}


bool JsParamFetcher::GetAsToken(int i, JsToken *out) {
  *out = js_argv_[i];
  return true;
}

bool JsParamFetcher::GetAsInt(int i, int *out) {
  JsToken t = js_argv_[i];
  return TokenToInt(t, out);
}

bool JsParamFetcher::GetAsString(int i, std::string16 *out) {
  JsToken t = js_argv_[i];
  return TokenToString(t, out);
}

bool JsParamFetcher::GetAsArray(int i, JsToken *out_array, int *out_length) {
  JsToken array;
  if (GetAsToken(i, &array)) {

    // check that it's an array
    if (JSVAL_IS_OBJECT(array) &&
        JS_IsArrayObject(js_context_, JSVAL_TO_OBJECT(array))) {

      jsuint length;
      if (JS_GetArrayLength(js_context_, JSVAL_TO_OBJECT(array), &length)) {
        *out_array = array;
        *out_length = static_cast<int>(length);
        return true; // succeeded
      }
    }
  }
  return false; // failed
}

bool JsParamFetcher::GetAsCallback(int i, JsCallback *out) {
  out->function = js_argv_[i];
  out->context = GetContextPtr();
  return true;
}


bool JsParamFetcher::GetFromArrayAsToken(JsToken array, int i, JsToken *out) {
  if (ArrayIndexToToken(array, i, out)) {
    return true;
  }
  return false;
}

bool JsParamFetcher::GetFromArrayAsInt(JsToken array, int i, int *out) {
  JsToken token;
  if (ArrayIndexToToken(array, i, &token)) {
    if (TokenToInt(token, out)) {
      return true;
    }
  }
  return false;
}

bool JsParamFetcher::GetFromArrayAsString(JsToken array, int i,
                                          std::string16 *out) {
  JsToken token;
  if (ArrayIndexToToken(array, i, &token)) {
    if (TokenToString(token, out)) {
      return true;
    }
  }
  return false;
}


bool JsParamFetcher::TokenToInt(JsToken t, int *out) {
  if (!JSVAL_IS_INT(t)) { return false; }

  int32 i32_value;
  if (!JS_ValueToECMAInt32(js_context_, t, &i32_value)) { return false; }
  *out = i32_value;
  return true;
}

bool JsParamFetcher::TokenToString(JsToken t, std::string16 *out) {
  // for optional args, we want JS null to act like the default _typed_ value
  if (JSVAL_IS_NULL(t)) {
    out->clear();
    return true;
  }

  if (!JSVAL_IS_STRING(t)) { return false; }

  JSString *js_str = JS_ValueToString(js_context_, t);
  if (!js_str) { return false; }

  out->assign(reinterpret_cast<const char16 *>(JS_GetStringChars(js_str)),
              JS_GetStringLength(js_str));
  return true;
}

bool JsParamFetcher::ArrayIndexToToken(JsToken array, int i, JsToken *out) {
  if (!JS_GetElement(js_context_, JSVAL_TO_OBJECT(array), i, out)) {
    return false;
  }
  return true;
}


JsNativeMethodRetval JsSetException(const GearsBaseClass *obj,
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
  JS_SetPendingException(cx, error_object->GetToken());

  bool success = js_runner->SetPropertyString(error_object->GetToken(),
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
  //
  // TODO(cprince): Track the lifetime of these root containers, to avoid the
  // small memory leak.  Idea: maybe we should only return scoped JsToken
  // objects, which transfer ownership on assignment and RemoveRoot on release.
  assert(JSVAL_IS_GCTHING(t));
  jsval *root_container = new jsval(t);
  JSBool js_ok = JS_AddRoot(cx, root_container);
  return js_ok ? true : false;
}


#endif // BROWSER_FF
//-----------------------------------------------------------------------------
