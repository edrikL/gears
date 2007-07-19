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

#include <assert.h>
#include <map>
#include <nsCOMPtr.h>
#include <nspr.h> // for PR_*
#include "gears/third_party/gecko_internal/jsapi.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/common.h" // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/js_runner_ff_marshaling.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/firefox/factory.h"
#include "ff/genfiles/database.h"
#include "ff/genfiles/localserver.h"
#include "ff/genfiles/workerpool.h"

class JsRunner : public JsRunnerInterface {
 public:
  JsRunner() : js_engine_context_(NULL), alloc_js_wrapper_(NULL),
               global_obj_(NULL), js_runtime_(NULL), js_script_(NULL) {
    InitJavaScriptEngine();
  }
  ~JsRunner();

  bool JsRunner::AddGlobal(const char16 *name, IGeneric *object, gIID iface_id);
  bool JsRunner::Start(const char16 *full_script);
  bool JsRunner::Stop();
  bool JsRunner::GetContext(JsContextPtr *context) {
    *context = js_engine_context_;
    return true;
  }
  bool JsRunner::Eval(const char16 *full_script);
  const char16 * JsRunner::GetLastScriptError() {
    return last_script_error_.c_str();
  }

 private:
  bool InitJavaScriptEngine();

  bool GetProtoFromIID(const nsIID iface_id, JSObject **proto);
  static void JS_DLL_CALLBACK JsErrorHandler(JSContext *cx, const char *message,
                                             JSErrorReport *report);

  JSContext *js_engine_context_;
  JsContextWrapper *alloc_js_wrapper_; // (created in thread)
  JSObject *global_obj_;
  IIDToProtoMap proto_interfaces_;
  std::string16 last_script_error_; // struct to cross the OS thread boundary
  std::vector<IGeneric *> globals_;
  JSRuntime *js_runtime_;
  JSScript *js_script_;

  DISALLOW_EVIL_CONSTRUCTORS(JsRunner);
};

class ParentJsRunner : public JsRunnerInterface {
 public:
  ParentJsRunner(IGeneric *base, JsContextPtr context)
      : js_engine_context(context) {
  }

  ~ParentJsRunner() {
  }

  bool ParentJsRunner::AddGlobal(const char16 *name,
                                 IGeneric *object,
                                 gIID iface_id) {
    // TODO(zork): Add this functionality to parent js runners.
    return false;
  }
  bool ParentJsRunner::Start(const char16 *full_script) {
    assert(false); // This should not be called on the parent.
    return false;
  }
  bool ParentJsRunner::Stop() {
    assert(false); // This should not be called on the parent.
    return false;
  }
  bool ParentJsRunner::GetContext(JsContextPtr *context) {
    *context = js_engine_context;
    return true;
  }
  bool ParentJsRunner::Eval(const char16 *full_script);
  const char16 * ParentJsRunner::GetLastScriptError() {
    return NULL;
  }

 private:
  JSContext *js_engine_context;

  DISALLOW_EVIL_CONSTRUCTORS(ParentJsRunner);
};


bool ParentJsRunner::Eval(const char16 *script) {
  /*
  JSObject *object = JS_GetGlobalObject(js_engine_context);

  uintN line_number_start = 0;
  jsval rval;
  JSBool js_ok = JS_EvaluateUCScript(
                       js_engine_context,
                       object,
                       reinterpret_cast<const jschar *>(script),
                       wcslen(script),
                       "script", line_number_start,
                       &rval);
  if (!js_ok) { return false; }
  return true;
  */
  // TODO(zork): This code is currently broken in Firefox.
  return false;
}

void JS_DLL_CALLBACK JsRunner::JsErrorHandler(JSContext *cx,
                                              const char *message,
                                              JSErrorReport *report) {
  JsRunner *js_runner = static_cast<JsRunner*>(JS_GetContextPrivate(cx));
  if (js_runner && report && report->ucmessage) {
    js_runner->last_script_error_ =
        reinterpret_cast<const char16 *>(report->ucmessage);
  }
}

JsRunner::~JsRunner() {
  std::vector<IGeneric *>::iterator global;
  for (global = globals_.begin(); global != globals_.end(); global++) {
    NS_RELEASE(*global);
    delete *global;
  }

  // TODO(zork): The script object should be cleaned up here,
  // but it currently causes JS_DestroyContext to occasionally crash.

  if (js_engine_context_) {
    JS_DestroyContext(js_engine_context_);
  }
  if (js_runtime_) {
    JS_DestroyRuntime(js_runtime_);
  }
  delete alloc_js_wrapper_;
}

bool JsRunner::GetProtoFromIID(const nsIID iface_id, JSObject **proto) {
  IIDToProtoMap::iterator proto_interface =
      proto_interfaces_.find(iface_id);
  if (proto_interface != proto_interfaces_.end()) {
    *proto = proto_interface->second;
    return true;
  }

  proto_interfaces_[iface_id] = NULL;
  proto_interface = proto_interfaces_.find(iface_id);

  // passing NULL for class_id and class_name prevents child workers
  // from using "new CLASSNAME"
  if (alloc_js_wrapper_->DefineClass(&iface_id,
                                     NULL, // class_id
                                     NULL, // class_name
                                     &(proto_interface->second))) {
    *proto = proto_interface->second;
    return true;
  } else {
    proto_interfaces_.erase(iface_id);
    return false;
  }
}

class JS_DestroyContextFunctor {
 public:
  inline void operator()(JSContext* x) const {
    if (x != NULL) { JS_DestroyContext(x); }
  }
};
typedef scoped_token<JSContext*, JS_DestroyContextFunctor> scoped_jscontext_ptr;

bool JsRunner::InitJavaScriptEngine() {
  JSBool js_ok;
  bool succeeded;

  // To cleanup after failures we use scoped objects to manage everything that
  // should be destroyed.  On success we take ownership to avoid cleanup.

  // These structs are static because they must live for duration of JS engine.
  // SpiderMonkey README also suggests using static for one-off objects.
  static JSClass global_class = {
      "Global", 0, // name, flags
      JS_PropertyStub, JS_PropertyStub,  // defineProperty, deleteProperty
      JS_PropertyStub, JS_PropertyStub, // getProperty, setProperty
      JS_EnumerateStub, JS_ResolveStub, // enum, resolve
      JS_ConvertStub, JS_FinalizeStub // convert, finalize
  };


  //
  // Instantiate a JavaScript engine
  //

  // Create a new runtime.  If we instead use xpc/RuntimeService to get a
  // runtime, strange things break (like eval).
  const int kRuntimeMaxBytes = 64 * 1024 * 1024; // mozilla/.../js.c uses 64 MB
  js_runtime_ = JS_NewRuntime(kRuntimeMaxBytes);
  if (!js_runtime_) { return false; }

  const int kContextStackChunkSize = 1024; // Firefox often uses 1024;
                                           // also see js/src/readme.html

  scoped_jscontext_ptr cx(JS_NewContext(js_runtime_, kContextStackChunkSize));
  if (!cx.get()) { return false; }
  // VAROBJFIX is recommended in /mozilla/js/src/jsapi.h
  JS_SetOptions(cx.get(), JS_GetOptions(cx.get()) | JSOPTION_VAROBJFIX);

  // JS_SetErrorReporter takes a static callback, so we need
  // JS_SetContextPrivate to later save the error in a per-worker location
  JS_SetErrorReporter(cx.get(), JsErrorHandler);
  JS_SetContextPrivate(cx.get(), static_cast<void*>(this));
#ifdef DEBUG
  // must set this here to allow workerPool.forceGC() during child init
  js_engine_context_ = cx.get();
#endif

  global_obj_ = JS_NewObject(cx.get(), &global_class, 0, 0);

  if (!global_obj_) { return false; }
  js_ok = JS_InitStandardClasses(cx.get(), global_obj_);
  if (!js_ok) { return false; }
  // Note: an alternative is to lazily define the "standard classes" (which
  // include things like eval).  To do that, change JS_InitStandardClasses
  // to JS_SetGlobalObject, and add handlers for Enumerate and Resolve in
  // global_class.  See /mozilla/js/src/js.c for sample code.

  //
  // Define classes in the JSContext
  //

  // first need to create a JsWrapperManager for this thread
  scoped_ptr<JsContextWrapper> js_wrapper(new JsContextWrapper(cx.get(),
                                                               global_obj_));

  js_engine_context_ = cx.release();
  alloc_js_wrapper_ = js_wrapper.release();

  struct {
    const nsIID iface_id;
    JSObject *proto_obj; // gets set by code below
  } classes[] = {
    // TODO(cprince): Unify the interface lists here and in GearsFactory.
    // Could share code, or could query GearsFactory.
    {GEARSFACTORYINTERFACE_IID, NULL},
    // workerpool
    {GEARSWORKERPOOLINTERFACE_IID, NULL},
    // database
    {GEARSDATABASEINTERFACE_IID, NULL},
    {GEARSRESULTSETINTERFACE_IID, NULL},
    // localserver
    {GEARSLOCALSERVERINTERFACE_IID, NULL},
    {GEARSMANAGEDRESOURCESTOREINTERFACE_IID, NULL},
    {GEARSRESOURCESTOREINTERFACE_IID, NULL},
    // GEARSFILESUBMITTERINTERFACE_IID can never be created in a child worker
  };
  const int num_classes = sizeof(classes) / sizeof(classes[0]);

  for (int i = 0; i < num_classes; ++i) {
    // passing NULL for class_id and class_name prevents child workers
    // from using "new CLASSNAME"
    succeeded = GetProtoFromIID(classes[i].iface_id, &classes[i].proto_obj);
    if (!succeeded) { return false; }
  }

#ifdef DEBUG
  // Do it here to trigger potential GC bugs in our code.
  JS_GC(js_engine_context_);
#endif

  return true; // succeeded
}

bool JsRunner::AddGlobal(const char16 *name, IGeneric *object, gIID iface_id) {
  JSObject *proto_object;
  if (!GetProtoFromIID(iface_id, &proto_object)) {
    return false;
  }

  if (!alloc_js_wrapper_->DefineGlobal(proto_object, object, name)) {
    return false;
  }

  globals_.push_back(object);
  NS_ADDREF(globals_.back());

  return true; // succeeded
}

bool JsRunner::Start(const char16 *full_script) {
  //
  // Add script code to the engine instance
  //

  uintN line_number_start = 0;
  js_script_ = JS_CompileUCScript(
                       js_engine_context_, global_obj_,
                       reinterpret_cast<const jschar *>(full_script),
                       wcslen(full_script),
                       "script", line_number_start);
  if (!js_script_) { return false; }

  // we must root any script returned by JS_Compile* (see jsapi.h)
  JSObject *compiled_script_obj = JS_NewScriptObject(js_engine_context_,
                                                     js_script_);
  if (!compiled_script_obj) { return false; }
  if (!RootJsToken(js_engine_context_,
                   OBJECT_TO_JSVAL(compiled_script_obj))) {
    return false;
  }


  //
  // Start the engine running
  //

  jsval return_string;
  JSBool js_ok = JS_ExecuteScript(js_engine_context_, global_obj_,
                                  js_script_, &return_string);
  if (!js_ok) { return false; }

  return true;
}

bool JsRunner::Stop() {
  // TODO(zork): Implement
  return false;
}

bool JsRunner::Eval(const char16 *script) {
  JSObject *object = JS_GetGlobalObject(js_engine_context_);

  uintN line_number_start = 0;
  jsval rval;
  JSBool js_ok = JS_EvaluateUCScript(
                       js_engine_context_,
                       object,
                       reinterpret_cast<const jschar *>(script),
                       wcslen(script),
                       "script", line_number_start,
                       &rval);
  if (!js_ok) { return false; }
  return true;
}

JsRunnerInterface* NewJsRunner() {
  return static_cast<JsRunnerInterface*>(new JsRunner());
}

JsRunnerInterface* NewParentJsRunner(IGeneric *base, JsContextPtr context) {
  return static_cast<JsRunnerInterface*>(new ParentJsRunner(base, context));
}
