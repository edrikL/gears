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
#include <set>
#include <gecko_sdk/include/nspr.h> // for PR_*
#include <gecko_sdk/include/nsCOMPtr.h>
#include <gecko_internal/jsapi.h>
#include <gecko_internal/nsIJSContextStack.h>
#include <gecko_internal/nsIPrincipal.h>
#include <gecko_internal/nsIScriptContext.h>
#include <gecko_internal/nsIScriptGlobalObject.h>
#include <gecko_internal/nsIScriptObjectPrincipal.h>

#include "gears/base/common/js_runner.h"

#include "ff/genfiles/channel.h"
#include "ff/genfiles/database.h"
#include "ff/genfiles/desktop_ff.h"
#include "ff/genfiles/httprequest.h"
#include "ff/genfiles/localserver.h"
#include "ff/genfiles/timer_ff.h"
#include "ff/genfiles/workerpool.h"
#include "gears/base/common/common.h" // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/exception_handler_win32.h"
#include "gears/base/common/html_event_monitor.h"
#include "gears/base/common/js_runner_ff_marshaling.h"
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/factory/firefox/factory.h"

#ifdef DEBUG
#include "ff/genfiles/test_ff.h"
#endif

// Internal base class used to share some code between DocumentJsRunner and
// JsRunner. Do not override these methods from JsRunner or DocumentJsRunner.
// Either share the code here, or move it to those two classes if it's
// different.
class JsRunnerBase : public JsRunnerInterface {
 public:
  JsRunnerBase() : js_engine_context_(NULL) {}

  JsContextPtr GetContext() {
    return js_engine_context_;
  }

  JsRootedToken *NewObject(const char16 *optional_global_ctor_name,
                           bool dump_on_error = false) {
    if (!js_engine_context_) {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      LOG(("Could not get JavaScript engine context."));
      return NULL;
    }

    JSObject *global_object = JS_GetGlobalObject(js_engine_context_);
    if (!global_object) {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      LOG(("Could not get global object from script engine."));
      return NULL;
    }

    std::string ctor_name_utf8;
    if (optional_global_ctor_name) {
      if (!String16ToUTF8(optional_global_ctor_name, &ctor_name_utf8)) {
        if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
        LOG(("Could not convert constructor name."));
        return NULL;
      }
    } else {
      ctor_name_utf8 = "Object";
    }

    jsval val = INT_TO_JSVAL(0);
    JSBool result = JS_GetProperty(js_engine_context_, global_object,
                                   ctor_name_utf8.c_str(), &val);
    if (!result) {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      LOG(("Could not get constructor property from global object."));
      return NULL;
    }

    JSFunction *ctor = JS_ValueToFunction(js_engine_context_, val);
    if (!ctor) {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      LOG(("Could not convert constructor property to function."));
      return NULL;
    }

    // NOTE: We are calling the specified function here as a regular function,
    // not as a constructor. I could not find a way to call a function as a
    // constructor using JSAPI other than JS_ConstructObject which takes
    // arguments I don't know how to provide. Ideally, there would be something
    // like DISPATCH_CONSTRUCT in IE.
    //
    // This is OK for the built-in constructors that we want to call (such as
    // "Error", "Object", etc) because those objects are specified to behave as
    // constructors even without the 'new' keyword.
    //
    // For more information, see:
    // * ECMAScript spec section 15.2.1, 15.3.1, 15.4.1, etc.
    // * DISPATCH_CONSTRUCT:
    //     http://msdn2.microsoft.com/en-us/library/asd22sd4.aspx
    result = JS_CallFunction(js_engine_context_, global_object, ctor, 0, NULL,
                             &val);
    if (!result) {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      LOG(("Could not call constructor function."));
      return NULL;
    }

    if (JSVAL_IS_OBJECT(val)) {
      return new JsRootedToken(GetContext(), val);
    } else {
      if (dump_on_error) ExceptionManager::CaptureAndSendMinidump();
      LOG(("Constructor did not return an object"));
      return NULL;
    }
  }

  bool SetPropertyString(JsToken object, const char16 *name,
                         const char16 *value) {
    // TODO(aa): Figure out the lifetime of this string.
    JSString *jstr = JS_NewUCStringCopyZ(
                         js_engine_context_,
                         reinterpret_cast<const jschar *>(value));
    if (jstr) {
      return SetProperty(object, name, STRING_TO_JSVAL(jstr));
    } else {
      return false;
    }
  }

  bool SetPropertyInt(JsToken object, const char16 *name, int value) {
    return SetProperty(object, name, INT_TO_JSVAL(value));
  }

  virtual bool InvokeCallbackSpecialized(
                   const JsRootedCallback *callback, int argc, jsval *argv,
                   JsRootedToken **optional_alloc_retval) = 0;

  bool InvokeCallback(const JsRootedCallback *callback,
                      int argc, JsParamToSend *argv,
                      JsRootedToken **optional_alloc_retval) {
    assert(callback && (!argc || argv));

    if (callback->IsNullOrUndefined()) { return false; }

    // Setup argument array.
    scoped_array<jsval> js_engine_argv(new jsval[argc]);
    for (int i = 0; i < argc; ++i)
      ConvertJsParamToToken(argv[i], callback->context(), &js_engine_argv[i]);

    // Invoke the method.
    return InvokeCallbackSpecialized(callback, argc, js_engine_argv.get(),
                                     optional_alloc_retval);
  }

  // Add the provided handler to the notification list for the specified event.
  virtual bool AddEventHandler(JsEventType event_type,
                               JsEventHandlerInterface *handler) {
    assert(event_type >= 0 && event_type < MAX_JSEVENTS);

    event_handlers_[event_type].insert(handler);
    return true;
  }

  // Remove the provided handler from the notification list for the specified
  // event.
  virtual bool RemoveEventHandler(JsEventType event_type,
                                  JsEventHandlerInterface *handler) {
    assert(event_type >= 0 && event_type < MAX_JSEVENTS);

    event_handlers_[event_type].erase(handler);
    return true;
  }

#ifdef DEBUG
  void ForceGC() {
    if (js_engine_context_) {
      JS_GC(js_engine_context_);
    }
  }
#endif

 protected:
  // Alert all monitors that an event has occured.
  void SendEvent(JsEventType event_type) {
    assert(event_type >= 0 && event_type < MAX_JSEVENTS);

    // Make a copy of the list of listeners, in case they change during the
    // alert phase.
    std::vector<JsEventHandlerInterface *> monitors;
    monitors.insert(monitors.end(),
                    event_handlers_[event_type].begin(),
                    event_handlers_[event_type].end());

    std::vector<JsEventHandlerInterface *>::iterator monitor;
    for (monitor = monitors.begin();
         monitor != monitors.end();
         ++monitor) {
      // Check that the listener hasn't been removed.  This can occur if a
      // listener removes another listener from the list.
      if (event_handlers_[event_type].find(*monitor) !=
                                     event_handlers_[event_type].end()) {
        (*monitor)->HandleEvent(event_type);
      }
    }
  }

  JSContext *js_engine_context_;

 private:
  bool SetProperty(JsToken object, const char16 *name, jsval value) {
    if (!JSVAL_IS_OBJECT(object)) {
      LOG(("Specified token is not an object."));
      return false;
    }

    std::string name_utf8;
    if (!String16ToUTF8(name, &name_utf8)) {
      LOG(("Could not convert property name to utf8."));
      return false;
    }

    JSBool result = JS_DefineProperty(js_engine_context_,
                                      JSVAL_TO_OBJECT(object),
                                      name_utf8.c_str(), value,
                                      nsnull, nsnull, // getter, setter
                                      JSPROP_ENUMERATE);
    if (!result) {
      LOG(("Could not define property."));
      return false;
    }

    return true;
  }

  std::set<JsEventHandlerInterface *> event_handlers_[MAX_JSEVENTS];

  DISALLOW_EVIL_CONSTRUCTORS(JsRunnerBase);
};


class JsRunner : public JsRunnerBase {
 public:
  JsRunner() : alloc_js_wrapper_(NULL),
               error_handler_(NULL), global_obj_(NULL), js_runtime_(NULL),
               js_script_(NULL) {
    InitJavaScriptEngine();
  }
  ~JsRunner();

  bool AddGlobal(const std::string16 &name, IGeneric *object, gIID iface_id);
  bool Start(const std::string16 &full_script);
  bool Stop();
  bool Eval(const std::string16 &full_script);
  void SetErrorHandler(JsErrorHandlerInterface *handler) {
    error_handler_ = handler;
  }
  bool InvokeCallbackSpecialized(const JsRootedCallback *callback,
                                 int argc, jsval *argv,
                                 JsRootedCallback **optional_alloc_retval);

 private:
  bool InitJavaScriptEngine();

  bool GetProtoFromIID(const nsIID iface_id, JSObject **proto);
  static void JS_DLL_CALLBACK JsErrorHandler(JSContext *cx, const char *message,
                                             JSErrorReport *report);

  JsContextWrapper *alloc_js_wrapper_; // (created in thread)
  JsErrorHandlerInterface *error_handler_;
  JSObject *global_obj_;
  IIDToProtoMap proto_interfaces_;
  std::vector<IGeneric *> globals_;
  JSRuntime *js_runtime_;
  JSScript *js_script_;
  scoped_ptr<JsRootedToken> js_script_root_;

  DISALLOW_EVIL_CONSTRUCTORS(JsRunner);
};

void JS_DLL_CALLBACK JsRunner::JsErrorHandler(JSContext *cx,
                                              const char *message,
                                              JSErrorReport *report) {
  JsRunner *js_runner = static_cast<JsRunner*>(JS_GetContextPrivate(cx));
  if (js_runner && js_runner->error_handler_ && report) {
    JsErrorInfo error_info;
    error_info.line = report->lineno + 1; // Reported lines start at zero.

    // The error message can either be in the separate *message param or in
    // *report->ucmessage. For example, running the following JS in a worker
    // causes the separate message param to get used:
    //   throw new Error("foo")
    // Other errors cause the report->ucmessage property to get used.
    //
    // Mozilla also does this, see:
    // http://lxr.mozilla.org/mozilla1.8.0/source/dom/src/base/nsJSEnvironment.cpp#163
    if (report->ucmessage) {
      error_info.message = reinterpret_cast<const char16 *>(report->ucmessage);
    } else if (message) {
      std::string16 message_str;
      if (UTF8ToString16(message, &message_str)) {
        error_info.message = message_str;
      }
    }

    js_runner->error_handler_->HandleError(error_info);
  }
}

JsRunner::~JsRunner() {
  // Alert modules that the engine is unloading.
  SendEvent(JSEVENT_UNLOAD);

  // We need to remove the roots now, because they will be referencing an
  // invalid context if we wait for the destructor.
  if (alloc_js_wrapper_)
    alloc_js_wrapper_->CleanupRoots();

  std::vector<IGeneric *>::iterator global;
  for (global = globals_.begin(); global != globals_.end(); ++global) {
    NS_RELEASE(*global);
  }

  // Reset the scoped_ptr to unroot the script.  This needs to be done before
  // we destroy the context and runtime, so we can't wait for the destructor.
  js_script_root_.reset(NULL);

  if (js_engine_context_) {
    JS_DestroyContext(js_engine_context_);
  }
  if (js_runtime_) {
    JS_DestroyRuntime(js_runtime_);
  }

  // This has to occur after the context and runtime have been destroyed,
  // because it maintains data structures that the JS engine requires.
  // Specifically, any of the JSObjects stored in the JsWrapperData and the
  // global_boj_ need to exist as long as the object in the JS Engine which
  // they are linked to.
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
    // desktop
    {GEARSDESKTOPINTERFACE_IID, NULL},
    // localserver
    {GEARSLOCALSERVERINTERFACE_IID, NULL},
    {GEARSMANAGEDRESOURCESTOREINTERFACE_IID, NULL},
    {GEARSRESOURCESTOREINTERFACE_IID, NULL},
    // GEARSFILESUBMITTERINTERFACE_IID can never be created in a child worker
#ifdef DEBUG
    // test
    {GEARSTESTINTERFACE_IID, NULL},
#endif
    // timer
    {GEARSTIMERINTERFACE_IID, NULL},
    // httprequest
    {GEARSHTTPREQUESTINTERFACE_IID, NULL},
    // channel
    {GEARSCHANNELINTERFACE_IID, NULL}
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

bool JsRunner::AddGlobal(const std::string16 &name,
                         IGeneric *object,
                         gIID iface_id) {
  JSObject *proto_object;
  if (!GetProtoFromIID(iface_id, &proto_object)) {
    return false;
  }

  if (!alloc_js_wrapper_->DefineGlobal(proto_object, object, name.c_str())) {
    return false;
  }

  globals_.push_back(object);
  NS_ADDREF(globals_.back());

  return true; // succeeded
}

bool JsRunner::Start(const std::string16 &full_script) {
  //
  // Add script code to the engine instance
  //

  uintN line_number_start = 0;
  js_script_ = JS_CompileUCScript(
                       js_engine_context_, global_obj_,
                       reinterpret_cast<const jschar *>(full_script.c_str()),
                       full_script.length(),
                       "script", line_number_start);
  if (!js_script_) { return false; }

  // we must root any script returned by JS_Compile* (see jsapi.h)
  JSObject *compiled_script_obj = JS_NewScriptObject(js_engine_context_,
                                                     js_script_);
  if (!compiled_script_obj) { return false; }
  js_script_root_.reset(new JsRootedToken(
                                js_engine_context_,
                                OBJECT_TO_JSVAL(compiled_script_obj)));


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

bool JsRunner::Eval(const std::string16 &script) {
  JSObject *object = JS_GetGlobalObject(js_engine_context_);

  uintN line_number_start = 0;
  jsval rval;
  JSBool js_ok = JS_EvaluateUCScript(
                       js_engine_context_,
                       object,
                       reinterpret_cast<const jschar *>(script.c_str()),
                       script.length(),
                       "script", line_number_start,
                       &rval);
  if (!js_ok) { return false; }
  return true;
}

bool JsRunner::InvokeCallbackSpecialized(
                   const JsRootedCallback *callback, int argc, jsval *argv,
                   JsRootedToken **optional_alloc_retval) {
  jsval retval;
  JSBool result = JS_CallFunctionValue(
                      callback->context(),
                      JS_GetGlobalObject(callback->context()),
                      callback->token(), argc, argv, &retval);
  if (result == JS_FALSE) { return false; }

  if (optional_alloc_retval) {
    // Note: A valid jsval is returned no matter what the javascript function
    // returns. If the javascript function returns nothing, or explicitly
    // returns <undefined>, the the jsval will be JSVAL_IS_VOID. If the
    // javascript function returns <null>, then the jsval will be JSVAL_IS_NULL.
    // Always returning a JsRootedToken should allow us to coerce these values
    // to other types correctly in the future.
    *optional_alloc_retval = new JsRootedToken(js_engine_context_, retval);
  }

  return true;
}

// Provides the same interface as JsRunner, but for the normal JavaScript engine
// that runs in HTML pages.
class DocumentJsRunner : public JsRunnerBase {
 public:
  DocumentJsRunner(IGeneric *base, JsContextPtr context) {
    js_engine_context_ = context;
  }

  ~DocumentJsRunner() {
  }

  bool AddGlobal(const std::string16 &name, IGeneric *object, gIID iface_id) {
    // TODO(zork): Add this functionality to DocumentJsRunner.
    return false;
  }
  void SetErrorHandler(JsErrorHandlerInterface *handler) {
    assert(false); // This should not be called on DocumentJsRunner.
  }
  bool Start(const std::string16 &full_script) {
    assert(false); // This should not be called on DocumentJsRunner.
    return false;
  }
  bool Stop() {
    assert(false); // This should not be called on DocumentJsRunner.
    return false;
  }
  bool Eval(const std::string16 &full_script);
  bool InvokeCallbackSpecialized(const JsRootedCallback *callback,
                                 int argc, jsval *argv,
                                 JsRootedToken **optional_alloc_retval);
  bool AddEventHandler(JsEventType event_type,
                       JsEventHandlerInterface *handler);

 private:
  static void HandleEventUnload(void *user_param);  // Callback for 'onunload'

  scoped_ptr<HtmlEventMonitor> unload_monitor_;  // For 'onunload' notifications
  DISALLOW_EVIL_CONSTRUCTORS(DocumentJsRunner);
};


bool DocumentJsRunner::Eval(const std::string16 &script) {
  JSObject *object = JS_GetGlobalObject(js_engine_context_);
  if (!object) { return false; }

  // To eval the script, we need the JSPrincipals to be acquired through
  // nsIPrincipal.  nsIPrincipal can be queried through the
  // nsIScriptObjectPrincipal interface on the Script Global Object.  In order
  // to get the Script Global Object, we need to request the private data
  // associated with the global JSObject on the current context.
  nsCOMPtr<nsIScriptGlobalObject> sgo;
  nsISupports *priv = reinterpret_cast<nsISupports *>(JS_GetPrivate(
                                                          js_engine_context_,
                                                          object));
  nsCOMPtr<nsIXPConnectWrappedNative> wrapped_native = do_QueryInterface(priv);

  if (wrapped_native) {
    // The global object is a XPConnect wrapped native, the native in
    // the wrapper might be the nsIScriptGlobalObject.
    sgo = do_QueryWrappedNative(wrapped_native);
  } else {
    sgo = do_QueryInterface(priv);
  }

  JSPrincipals *jsprin;
  nsresult nr;

  nsCOMPtr<nsIScriptObjectPrincipal> obj_prin = do_QueryInterface(sgo, &nr);
  if (NS_FAILED(nr)) { return false; }

  nsIPrincipal *principal = obj_prin->GetPrincipal();
  if (!principal) { return false; }

  principal->GetJSPrincipals(js_engine_context_, &jsprin);

  // Set up the JS stack so that our context is on top.  This is needed to
  // play nicely with plugins that access the context stack, such as Firebug.
  nsCOMPtr<nsIJSContextStack> stack =
      do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  if (!stack) { return false; }

  stack->Push(js_engine_context_);

  uintN line_number_start = 0;
  jsval rval;
  JSBool js_ok = JS_EvaluateUCScriptForPrincipals(
      js_engine_context_, object, jsprin,
      reinterpret_cast<const jschar *>(script.c_str()),
      script.length(), "script", line_number_start, &rval);

  // Restore the context stack.
  JSContext *cx;
  stack->Pop(&cx);

  // Decrements ref count on jsprin (Was added in GetJSPrincipals()).
  (void)JSPRINCIPALS_DROP(js_engine_context_, jsprin);
  if (!js_ok) { return false; }
  return true;
}

bool DocumentJsRunner::AddEventHandler(JsEventType event_type,
                                       JsEventHandlerInterface *handler) {
  if (event_type == JSEVENT_UNLOAD) {
    // Monitor 'onunload' to send the unload event when the page goes away.
    if (unload_monitor_ == NULL) {
      unload_monitor_.reset(new HtmlEventMonitor(kEventUnload,
                                                 HandleEventUnload,
                                                 this));
      nsCOMPtr<nsIDOMEventTarget> event_source;
      if (NS_SUCCEEDED(DOMUtils::GetWindowEventTarget(
                                     getter_AddRefs(event_source))))
      {
        unload_monitor_->Start(event_source);
      } else {
        return false;
      }
    }
  }

  return JsRunnerBase::AddEventHandler(event_type, handler);
}

void DocumentJsRunner::HandleEventUnload(void *user_param) {
  static_cast<DocumentJsRunner*>(user_param)->SendEvent(JSEVENT_UNLOAD);
}

bool DocumentJsRunner::InvokeCallbackSpecialized(
                           const JsRootedCallback *callback,
                           int argc, jsval *argv,
                           JsRootedToken **optional_alloc_retval) {
  // When invoking a callback on the document context, we must go through
  // nsIScriptContext->CallEventHandler because it sets up certain state that
  // the browser error handler expects to find if there is an error. Without
  // this, crashes happen. For more information, see:
  // http://code.google.com/p/google-gears/issues/detail?id=32
  nsCOMPtr<nsIScriptContext> sc;
  sc = GetScriptContextFromJSContext(callback->context());
  if (!sc) { return false; }

  jsval retval;
  nsresult result = sc->CallEventHandler(
                            JS_GetGlobalObject(callback->context()),
                            JSVAL_TO_OBJECT(callback->token()),
                            argc, argv, &retval);
  if (NS_FAILED(result)) { return false; }

  if (optional_alloc_retval) {
    // See note in JsRunner::InvokeCallbackSpecialized about return values of
    // javascript functions.
    *optional_alloc_retval = new JsRootedToken(js_engine_context_, retval);
  }

  return true;
}


JsRunnerInterface* NewJsRunner() {
  return static_cast<JsRunnerInterface*>(new JsRunner());
}

JsRunnerInterface* NewDocumentJsRunner(IGeneric *base, JsContextPtr context) {
  return static_cast<JsRunnerInterface*>(new DocumentJsRunner(base, context));
}
