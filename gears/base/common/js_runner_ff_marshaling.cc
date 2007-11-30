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
// Handles interaction between JavaScript and C++ in Firefox.
// Responsibilities include declaring methods and attributes for each class,
// as well as marshaling (converting) arguments and return values.
//
// In Firefox, the XPConnect layer (/js/src/xpconnect/src) does this work.
// But XPConnect is not thread-safe, relies on the DOM, etc.  So for Scour
// worker threads we need to implement our own layer.
//
// Architecture Summary:
// * To add a class, we create a 'prototype' JSObject, then attach a 'function'
//   JSObject for each class function (methods, and property getters/setters).
//   To enumerate the class functions, we use xptinfo.
// * We use the same code (JsWrapperCaller) for all functions.  This code knows
//   how to convert params and return values between JavaScript and XPCOM C++.
//   It invokes the underlying C++ function using xptcall.
// * The devil is in the details.  Similar-looking alternatives often do not
//   work.  When all else fails, look CAREFULLY at what XPConnect does.
//
// Limitations:
// Any C++ class used with this wrapper cannot call into XPConnect -- because
// this wrapper replaces it!  This has no effect on most primitive types (like
// strings, ints, and pointers).  But because optional params and varargs are
// normally handled using XPConnect, they must be handled differently in C++
// objects used with this wrapper.
//
// More specifically, optional params and varargs must be extracted from the
// internal jsval argument array.  We pass this array to C++ objects using the
// ModuleImplBaseClass interface.  To simplify life, use the JsParamFetcher
// class (from base_class.h) for optional params and varargs, since it hides the
// differences between XPConnect and the internal jsval array.
//
// (Note: it may be possible to remove the above limitations, but this wrapper
// has already become too complex and taken too long to get working.)
//
// Additional info:
// * 'jsval' is the wrapper the Firefox JS engine uses for all args and retvals.
// * A jsval can contain a JSObject*, an integer, or a number of other types.
// * A JSObject can represent an instance, a function, a class prototype, etc.
//
// Final note:
// Figuring all this out was REALLY HARD.  XPConnect is a vast, uncharted sea
// of 35,000+ lines of code.  If you want to go crazy, start reading through it
// and try to make sense of it all.


// TODO(cprince): figure out how to cleanup allocated JsWrapperDataFor* structs
// when the JSContext goes away.  Maybe can use things like Finalize properties?
//
// Similarly, where do we destroy:
// * New objects returned by Scour classes (For example, the ResultSet objects
//   returned by Database::Execute.)
// * JS_NewObject (and similar) return values, on Define*() failure
//   (Or maybe rely on JSContext cleanup -- at a higher level -- to handle it.)


#include "nsComponentManagerUtils.h"
#include "nsMemory.h" // for use in JSData2Native
#include "gears/third_party/gecko_internal/nsIInterfaceInfoManager.h"
#include "gears/third_party/gecko_internal/nsITimerInternal.h"
#include "gears/third_party/gecko_internal/nsIVariant.h" // for use in JSData2Native
// TODO(cprince): see if we can remove nsIVariant.h/nsMemory.h after cleanup.
#include "gears/third_party/gecko_internal/xptinfo.h"

#include "ff/genfiles/base_interface_ff.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/firefox/xpcom_dynamic_load.h"
#include "gears/base/common/js_runner_ff_marshaling.h"


// The "reserved slot" in which we store our custom data for functions.
// (Non-function custom data can be stored using JS_SetPrivate.)
static const int kFunctionDataReservedSlotIndex = 0;

// Magic constants the JavaScript engine uses with the argv array.
static const int kArgvFunctionIndex = -2; // negative indices [sic]
static const int kArgvInstanceIndex = -1;

static const int kGarbageCollectionIntervalMsec = 2000;

//
// Forward declarations of local functions defined below.
//

// Functions for converting data between JavaScript and XPCOM C++.
static JSBool NativeSimpleData2JS(JSContext *cx, jsval *dest, const void *src,
                                  const nsXPTType &type_info);

static JSBool VariantDataToJS(JSContext *cx, JSObject *scope_obj,
                              nsIVariant *variant,
                              JsContextWrapper *js_wrapper, // only for IID map
                              nsresult *error_out, jsval *jsval_out);
JSBool NativeData2JS(JSContext *cx, JSObject *scope_obj,
                     jsval *d, const void *s,
                     const nsXPTType &type_info, const nsIID *iid,
                     JsContextWrapper *js_wrapper, // only for IID map
                     nsresult *error_out);
static JSBool NativeStringWithSize2JS(JSContext *cx,
                                      jsval *dest, const void *src,
                                      const nsXPTType &type_info,
                                      JSUint32 count, nsresult *pErr);
static JSBool JSData2Native(JSContext *cx, void *d, jsval s,
                            const nsXPTType &type_info,
                            JSBool use_allocator, const nsIID *iid,
                            nsresult* error_out);
static JSBool GetInterfaceTypeFromParam(nsIInterfaceInfo *iface_info,
                                        const nsXPTMethodInfo *function_info,
                                        const nsXPTParamInfo &param_info,
                                        uint16 vtable_index,
                                        uint8 param_index,
                                        uint8 type_tag,
                                        nsXPTCVariant *param_array,
                                        nsIID *result);


// Called when a JS object based on native code is cleaned up.  We need to
// remove any reference counters it held here.
void FinalizeNative(JSContext *cx, JSObject *obj) {
  JsWrapperData *p = reinterpret_cast<JsWrapperData *>(JS_GetPrivate(cx, obj));
  if (!p)
    return;
  switch(p->header.type) {
    case PROTO_JSOBJECT:
      {
        JsWrapperDataForProto *proto =
            static_cast<JsWrapperDataForProto *>(p);
        proto->js_wrapper->iid_to_proto_map_.erase(proto->iface_id);
      }
      break;
    case INSTANCE_JSOBJECT:
      {
        JsWrapperDataForInstance *instance =
            static_cast<JsWrapperDataForInstance *>(p);
        instance->js_wrapper->instance_wrappers_.erase(instance);
      }
      break;
    default:
      assert(false);  // Should never reach this line.
      break;
  }
}


JsContextWrapper::JsContextWrapper(JSContext *cx, JSObject *global_obj)
    : cx_(cx), global_obj_(global_obj) {
  // This creates a timer to run the garbage collector on a repeating interval,
  // which is what Firefox does in source/dom/src/base/nsJSEnvironment.cpp.
  nsresult result;
  gc_timer_ = do_CreateInstance("@mozilla.org/timer;1", &result);

  if (NS_SUCCEEDED(result)) {
    // Turning off idle causes the callback to be invoked in this thread,
    // instead of in the Timer idle thread.
    nsCOMPtr<nsITimerInternal> timer_internal(do_QueryInterface(gc_timer_));
    timer_internal->SetIdle(false);

    // Start the timer
    gc_timer_->InitWithFuncCallback(GarbageCollectionCallback,
                                    cx_,
                                    kGarbageCollectionIntervalMsec,
                                    nsITimer::TYPE_REPEATING_SLACK);
  }
}

void JsContextWrapper::GarbageCollectionCallback(nsITimer *timer,
                                                 void *context) {
  JSContext *cx = reinterpret_cast<JSContext *>(context);
  JS_GC(cx);
}

void JsContextWrapper::CleanupRoots() {
  // Stop garbage collection now.
  gc_timer_ = NULL;

  // Remove the roots associated with the protos we've created.  We need to
  // keep the rest of the structure around until the JS engine has been shut
  // down.
  std::vector<linked_ptr<JsWrapperDataForProto> >::iterator proto;
  for (proto = proto_wrappers_.begin();
       proto != proto_wrappers_.end();
       ++proto) {
    (*proto)->proto_root.reset();
  }

  // Clean up the rest of the roots.
  function_wrappers_.clear();
  global_roots_.clear();
}

bool JsContextWrapper::DefineClass(const nsIID *iface_id,
                                   const nsCID *class_id,
                                   const char *class_name,
                                   JSObject **proto_obj_out) {
  nsresult nr;

  // For class_name and class_id, define both or neither.
  // These values should be defined iff "new CLASSNAME" should work.
  assert((class_name && class_id) || (!class_name && !class_id));

  // allocate heap memory for variable that will be held by JS
  const char *name_for_alloc = "";                  // this is to avoid
  if (class_name) { name_for_alloc = class_name; }  // heap alloc/free/alloc
  scoped_ptr<std::string> class_name_copy(new std::string(name_for_alloc));

  // always use the same JSClass, except for the name field
  JSClass js_wrapper_class = {
    class_name_copy->c_str(), JSCLASS_NEW_RESOLVE | // TODO(cprince): need flag?
                              JSCLASS_HAS_PRIVATE, // name, flags
    JS_PropertyStub, JS_PropertyStub,  // defineProperty, deleteProperty
    JS_PropertyStub, JS_PropertyStub, // getProperty, setProperty
    JS_EnumerateStub, JS_ResolveStub, // enum, resolve
    JS_ConvertStub, FinalizeNative, // convert, finalize
    JSCLASS_NO_OPTIONAL_MEMBERS
  };
  scoped_ptr<JSClass> alloc_jsclass(new JSClass(js_wrapper_class));

  // add the class to the JSContext
  JSObject *proto_obj = JS_InitClass(cx_, global_obj_,
                                     NULL, // parent_proto
                                     alloc_jsclass.get(), // JSClass *
                                     JsContextWrapper::JsWrapperConstructor,
                                     0, // ctor_num_args
                                     NULL, NULL,  //   prototype   props/funcs
                                     NULL, NULL); // "constructor" props/funcs
  if (!proto_obj) { return false; }

  // setup the JsWrapperDataForProto struct
  scoped_ptr<JsWrapperDataForProto> proto_data(new JsWrapperDataForProto);

  proto_data->jsobject = proto_obj;
  proto_data->iface_id = *iface_id;
  proto_data->js_wrapper = this;
  proto_data->alloc_name.swap(class_name_copy); // take ownership
  proto_data->alloc_jsclass.swap(alloc_jsclass); // take ownership
  proto_data->proto_root.reset(new JsRootedToken(cx_,
                                                 OBJECT_TO_JSVAL(proto_obj)));

  // nsIFactory for this class lets us create instances later
  if (!class_id) {
    proto_data->factory = NULL;
  } else {
    proto_data->factory = do_GetClassObject(*class_id);
    if (!proto_data->factory) { return false; }
  }

  // nsIInterfaceInfo for this IID lets us lookup properties later
  nsCOMPtr<nsIInterfaceInfoManager> iface_info_manager;
  iface_info_manager = do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID,
                                     &nr);
  if (NS_FAILED(nr)) { return false; }
  nr = iface_info_manager->GetInfoForIID(iface_id,
                                         getter_AddRefs(proto_data->iface_info));
  if (NS_FAILED(nr)) { return false; }

  bool succeeded = AddFunctionsToPrototype(proto_obj, proto_data.get());
  if (!succeeded) { return false; }

  // save values
  iid_to_proto_map_[*iface_id] = proto_obj;

  if (proto_obj_out) {
    *proto_obj_out = proto_data->jsobject;
  }

  proto_wrappers_.push_back(
                      linked_ptr<JsWrapperDataForProto>(proto_data.get()));

  // succeeded; prevent scoped cleanup of allocations, and save the pointer
  JS_SetPrivate(cx_, proto_obj, proto_data.release());
  return true;
}


bool JsContextWrapper::DefineGlobal(JSObject *proto_obj,
                                    nsISupports *instance_isupports,
                                    const std::string16 &instance_name) {

  JsWrapperDataForProto *proto_data =
      static_cast<JsWrapperDataForProto*>(JS_GetPrivate(cx_, proto_obj));
  assert(proto_data);
  assert(proto_data->header.type == PROTO_JSOBJECT);

  // Note: JS_NewObject by itself does not call the object constructor.
  // JS_ConstructObject is a superset that does.
  JSObject *instance_obj = JS_NewObject(cx_,
                                        proto_data->alloc_jsclass.get(),
                                        proto_obj,
                                        global_obj_); // parent
  if (!instance_obj) { return false; }

  global_roots_.push_back(linked_ptr<JsRootedToken>(
                            new JsRootedToken(cx_,
                                              OBJECT_TO_JSVAL(instance_obj))));

  bool succeeded = SetupInstanceObject(cx_, instance_obj, instance_isupports);
  if (!succeeded) { return false; }

  // To define a global instance, add the name as a property of the
  // global namespace.
  JSBool js_ok;
  js_ok = JS_DefineUCProperty(
              cx_, global_obj_,
              reinterpret_cast<const jschar *>(instance_name.c_str()),
              instance_name.length(),
              OBJECT_TO_JSVAL(instance_obj),
              NULL, NULL, // getter, setter
              0 |     // these flags are optional
              //JSPROP_PERMANENT |
              //JSPROP_READONLY |
              JSPROP_ENUMERATE);
  if (!js_ok) { return false; }

  return true; // succeeded
}


// Helper function for enumerating methods and property getters/setters of a
// XPCOM object, and exposing those functions to the JavaScript engine.
//
// [Reference: this is inspired by Mozilla's DefinePropertyIfFound() in
// /xpconnect/src/xpcwrappednativejsops.cpp.]
bool JsContextWrapper::AddFunctionsToPrototype(JSObject *proto_obj,
                                               JsWrapperDataForProto *proto_data) {
  nsresult nr;
  JSBool js_ok;

  nsIInterfaceInfo *iface_info = proto_data->iface_info;

  PRUint16 num_methods;
  nr = iface_info->GetMethodCount(&num_methods);
  if (NS_FAILED(nr)) { return false; }

  // Add all methods to the prototype JSObject.
  // FYI: The typelib spec ensures attributes will appear as a getter (if
  //      read-only), or as a getter *directly* followed by a setter.

  const int kFirstReflectedMethod = 3; // don't expose QI/AddRef/Release
  // TODO(cprince): could assert [0..2] are QueryInterface, AddRef, Release

  for (int i = kFirstReflectedMethod; i < num_methods; ++i) { // vtable_index

    const nsXPTMethodInfo *function_info;
    nr = iface_info->GetMethodInfo(i, &function_info);
    if (NS_FAILED(nr)) { return false; }

    if (function_info->IsHidden() || function_info->IsNotXPCOM()) {
      // we typically get here because the IDL set [noscript] and/or [notxpcom]
      continue; // don't expose to JS engine
    }

    const char *function_name = function_info->GetName(); // does NOT alloc

    // Create a JSFunction object for the property getter/setter or method.
    int newfunction_flags = (function_info->IsGetter() ? JSFUN_GETTER : 0) |
                            (function_info->IsSetter() ? JSFUN_SETTER : 0);
    JSFunction *function = JS_NewFunction(cx_,
                                          JsContextWrapper::JsWrapperCaller,
                                          function_info->GetParamCount(),
                                          newfunction_flags,
                                          proto_obj, // parent
                                          function_name);
    // TODO(cprince): do we need to manually cleanup all JS_NewFunction values
    // if this loop fails at some point? (Probably handled as JSContext cleanup
    // outside this function.)
    JSObject *function_obj = JS_GetFunctionObject(function);

    // Save info about the function.
    scoped_ptr<JsWrapperDataForFunction> function_data(
                                             new JsWrapperDataForFunction);
    function_data->name = function_name;
    function_data->function_info = function_info;
    function_data->iface_info = iface_info;
    function_data->vtable_index = i;
    function_data->proto_data = proto_data;
    function_data->function_root.reset(new JsRootedToken(
                                               cx_,
                                               OBJECT_TO_JSVAL(function_obj)));

    // Assume function is a method, and revise if it's a getter or setter.
    // FYI: xptinfo reports getters/setters separately (but with same name).
    JSPropertyOp getter = NULL;
    JSPropertyOp setter = NULL;
    jsval        method = OBJECT_TO_JSVAL(function_obj);
    uintN        function_flags = 0;

    if (function_info->IsGetter()) {
      getter = (JSPropertyOp) function_obj;
      method = OBJECT_TO_JSVAL(NULL);
      function_flags |= (JSPROP_GETTER | JSPROP_SHARED);
      // TODO(cprince): need JSPROP_READONLY for no-setter attributes?
    }
    if (function_info->IsSetter()) {
      setter = (JSPropertyOp) function_obj;
      method = OBJECT_TO_JSVAL(NULL);
      function_flags |= (JSPROP_SETTER | JSPROP_SHARED);
    }

    // Note: JS_DefineProperty is written to handle adding a setter to a
    // previously defined getter with the same name.
    js_ok = JS_DefineProperty(cx_, proto_obj, function_name,
                              method, // method
                              getter, setter, // getter, setter
                              function_flags);
    if (!js_ok) { return false; }

    // succeeded; prevent scoped cleanup of allocations, and save the pointer
    //
    // We cannot use JS_SetPrivate() here because a function JSObject stores
    // its JSFunction pointer there (see js_LinkFunctionObject in jsfun.c).
    //
    // Instead, use reserved slots, which we DO have.  From js_FunctionClass in
    // jsfun.c: "Reserve two slots in all function objects for XPConnect."
    //
    // We must use PRIVATE_TO_JSVAL (only works on pointers!) to prevent the
    // garbage collector from touching any private data stored in JS 'slots'.
    assert(0 == (0x01 & reinterpret_cast<int>(function_data.get())));
    function_wrappers_.push_back(
        linked_ptr<JsWrapperDataForFunction>(function_data.get()));
    jsval pointer_jsval = PRIVATE_TO_JSVAL((jsval)function_data.release());
    assert(!JSVAL_IS_GCTHING(pointer_jsval));
    JS_SetReservedSlot(cx_, function_obj, kFunctionDataReservedSlotIndex,
                       pointer_jsval);
  }

  return true;
}


bool JsContextWrapper::SetupInstanceObject(JSContext *cx,
                                           JSObject *instance_obj,
                                           nsISupports *isupports) {
  assert(isupports);

  // setup the JsWrapperDataForInstance struct
  scoped_ptr<JsWrapperDataForInstance> instance_data(
      new JsWrapperDataForInstance);

  // Keep a pointer to the context wrapper so that we can access it in the
  // static finalize function.
  instance_data->js_wrapper = this;
  instance_data->jsobject = instance_obj;
  instance_data->isupports = isupports;

  instance_wrappers_[instance_data.get()] =
      linked_ptr<JsWrapperDataForInstance>(instance_data.get());

  // succeeded; prevent scoped cleanup of allocations, and save the pointer
  JS_SetPrivate(cx, instance_obj, instance_data.release());
  return true;
}


// Helper class that uses scope to ensure cleanup of JsWorkerSetParams()
// in ModuleImplBaseClass.
class ScopedJsArgSetter {
 public:
  // sets argc/argv/context on construction
  ScopedJsArgSetter(nsISupports *isupports, JSContext *cx,
                    int argc, jsval *argv) {
    scour_native_ = NULL; // dtor only checks this
    nsresult nr;
    scour_idl_ = do_QueryInterface(isupports, &nr);
    if (NS_SUCCEEDED(nr) && scour_idl_) {
      scour_native_ = NULL;
      scour_idl_->GetNativeBaseClass(&scour_native_);
      if (scour_native_) {
        // TODO(cprince): Remove the 'cx' argument after some bake time.
        assert(cx == scour_native_->EnvPageJsContext());
        prev_argc_    = scour_native_->JsWorkerGetArgc();
        prev_argv_    = scour_native_->JsWorkerGetArgv();
        scour_native_->JsWorkerSetParams(argc, argv);
      }
    }
  }
  // restores argc/argv/context on destruction
  ~ScopedJsArgSetter() {
    if (scour_native_) {
      scour_native_->JsWorkerSetParams(prev_argc_, prev_argv_);
    }
  }
 private:
  nsCOMPtr<GearsBaseClassInterface> scour_idl_;
  ModuleImplBaseClass *scour_native_;
  // must save/restore any previous values to handle re-entrancy
  // (example: JS code calls foo.abort(), C++ abort() invokes a JS handler
  // for 'onabort', and the JS handler calls any C++ function)
  int        prev_argc_;
  jsval     *prev_argv_;
};


// General-purpose wrapper to invoke any class function (method, or
// property getter/setter).
//
// All calls to Scour C++ objects from a worker thread will go through
// this function.
//
// [Reference: this is inspired by Mozilla's XPCWrappedNative::CallMethod() in
// /xpconnect/src/xpcwrappednative.cpp. Also: XPC_WN_(CallMethod|GetterSetter).]
JSBool JsContextWrapper::JsWrapperCaller(JSContext *cx, JSObject *obj,
                                         uintN argc, jsval *argv,
                                         jsval *js_retval) {
  nsresult nr;
  JSBool retval = JS_FALSE;  // JS_FALSE tells JS engine an exception occurred
  JSBool js_ok;

  // Gather data regarding the function and instance being called.
  JSObject *function_obj = JSVAL_TO_OBJECT(argv[kArgvFunctionIndex]);
  assert(function_obj);

  JSObject *instance_obj = JSVAL_TO_OBJECT(argv[kArgvInstanceIndex]);
  assert(instance_obj);


  JsWrapperDataForFunction *function_data;
  jsval function_data_jsval;
  js_ok = JS_GetReservedSlot(cx, function_obj,
                             kFunctionDataReservedSlotIndex,
                             &function_data_jsval);
  function_data = static_cast<JsWrapperDataForFunction *>(
                      JSVAL_TO_PRIVATE(function_data_jsval));
  assert(function_data);
  assert(function_data->header.type == FUNCTION_JSOBJECT);

  JsWrapperDataForInstance *instance_data =
      static_cast<JsWrapperDataForInstance*>(JS_GetPrivate(cx, instance_obj));
  assert(instance_data);
  assert(instance_data->header.type == INSTANCE_JSOBJECT);

  // Give the ModuleImplBaseClass direct access to JS params
  ScopedJsArgSetter arg_setter(instance_data->isupports, cx, argc, argv);

  // Get interface information about this function.
  const nsXPTMethodInfo *function_info = function_data->function_info;
  int num_params = function_info->GetParamCount();

  // Check the param count.
  int expected_argc = num_params;
  if (num_params >= 1 && function_info->GetParam(num_params - 1).IsRetval()) {
    --expected_argc;
  }
  if (static_cast<int>(argc) < expected_argc) {
    // TODO(cprince): consider calling JS_ReportError, JS_SetPendingException,
    // JS_ThrowReportedError, or similar to set the exception text.
    //JS_ReportError(cx, "Exception!");
    return retval;
  }

  // Allocate param buffer.
  scoped_array<nsXPTCVariant> param_array(new nsXPTCVariant[num_params]);

  // Clear param flags (for safe cleanup later).
  for (int i = 0; i < num_params; ++i) {
    nsXPTCVariant *param = &param_array[i];
    param->ClearFlags();
    param->val.p = nsnull;
  }

  // Convert params.
  // FYI: The xpidl compiler ensures retvals are the last arg.
  for (int i = 0; i < num_params; ++i) {
    nsXPTCVariant *param = &param_array[i];
    const nsXPTParamInfo &param_info = function_info->GetParam(i);
    const nsXPTType &type_info = param_info.GetType();
    uint8 type_tag = type_info.TagPart();

    JSBool use_allocator = JS_FALSE;
    jsval src = 0;

    if (type_info.IsDependent()) {
      assert(false); // we omitted "dependent" param support (what are those?)
      return retval;
    }

    param->type = type_info;

//=============================================================================
// TODO(cprince): cleanup all code in the FILE below this line. Much of the code
// will go away after the webcache APIs get converted, but for now I want to
// keep the original XPConnect layout so we can quickly catch missing cases,
// and so we can easily see how they did things we find we need.
//=============================================================================

    if (type_tag == nsXPTType::T_INTERFACE) {
      param->SetValIsInterface();
    }

    // set 'src' to be the object from which we get the value and
    // prepare for out param

    if (param_info.IsOut()) {
      param->SetPtrIsData();
      param->ptr = &param->val;

      if (!param_info.IsRetval()) {
        assert(false); // NOT YET IMPLEMENTED!
/***
        if (JSVAL_IS_PRIMITIVE(argv[i]) ||
            !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(argv[i]),
                              rt->GetStringID(XPCJSRuntime::IDX_VALUE),
                              &src)) {
          assert(false); // NOT YET IMPLEMENTED!
          //ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, i, cx);
          //goto done;
        }
***/
      }

      if (type_info.IsPointer() &&
          type_tag != nsXPTType::T_INTERFACE &&
          !param_info.IsShared()) {
        assert(false); // NOT YET IMPLEMENTED!
        //use_allocator = JS_TRUE;
        //param->SetValIsAllocated();
      }

      if (!param_info.IsIn()) {
        continue;
      }

    } else { // if (param_info.IsOut()) ... else ...

      if (type_info.IsPointer()) {

        switch (type_tag) {
          case nsXPTType::T_IID:
            assert(false); // NOT YET IMPLEMENTED!
            //param->SetValIsAllocated();
            //use_allocator = JS_TRUE;
            break;

          case nsXPTType::T_ASTRING:
            // Fall through to the T_DOMSTRING case

          case nsXPTType::T_DOMSTRING:
            if (param_info.IsDipper()) {
              // Is an 'out' DOMString. Make a new nsAString
              // now and then continue in order to skip the call to
              // JSData2Native

//#if PARAM_AUTOSTRING_COUNT
//                        // If autoStrings array support is enabled, then use
//                        // one of them if they are not already used up.
//                        if(autoStringIndex < PARAM_AUTOSTRING_COUNT)
//                        {
//                            // Don't call SetValIsDOMString because we don't
//                            // want to delete this pointer.
//                            param->val.p = &autoStrings[autoStringIndex++];
//                            continue;
//                        }
//#endif
/***
              param->SetValIsDOMString();
              if (!(param->val.p = new nsVoidableString())) {
                JS_ReportOutOfMemory(cx);
                goto done;
              }
***/
              // xpidl generates a param for nsAString return values;
              // we need to allocate a buffer for XPTC to pass in.
              //
              // nsString is same as nsVoidableString/nsAutoString, except
              // without a pre-allocated buffer for small strings.
              param->SetValIsDOMString();
              if (!(param->val.p = new nsString())) {
                JS_ReportOutOfMemory(cx);
                goto done;
              }
              continue;
            }
            // else...

            // Is an 'in' DOMString. Set 'use_allocator' to indicate
            // that JSData2Native should allocate a new
            // nsAString.
            param->SetValIsDOMString();
            use_allocator = JS_TRUE;
            break;

          case nsXPTType::T_UTF8STRING:
            // Fall through to the C string case for now...
          case nsXPTType::T_CSTRING:
            assert(false); // NOT YET IMPLEMENTED!
            //param->SetValIsCString();
            //if(param_info.IsDipper()) {
            //  // Is an 'out' CString.
            //  if(!(param->val.p = new nsCString())) {
            //    JS_ReportOutOfMemory(cx);
            //    goto done;
            //  }
            //  continue;
            //}
            //// else ...
            //// Is an 'in' CString.
            //use_allocator = JS_TRUE;
            break;
        } // END: switch (tag_type)
      } // END: if (type_info.IsPointer())

      // Do this *after* the above because in the case where we have a
      // "T_DOMSTRING && IsDipper()" then argv might be null since this
      // is really an 'out' param masquerading as an 'in' param.
      src = argv[i];

    } // END: if (param_info.IsOut()) ... else ...

    nsIID param_iid;

    if (type_tag == nsXPTType::T_INTERFACE) {
      nr = function_data->iface_info->GetIIDForParamNoAlloc(function_data->vtable_index,
                                                            &param_info,
                                                            &param_iid);
      if (NS_FAILED(nr)) {
        assert(false); // NOT YET IMPLEMENTED!
        //ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, i, cx);
        //goto done;
      }
    }

    nsresult err;
    js_ok = JSData2Native(cx, &param->val, src, type_info,
                          use_allocator, &param_iid, &err);

    if (!js_ok) {
      assert(false); // NOT YET IMPLEMENTED!
      return js_ok;
      //ThrowBadParam(err, i, cx);
      //goto done;
    }

  } // END convert params: for (int i = 0; i < num_params; ++i)


/***
    // if any params were dependent, then we must iterate again to convert them.
    if(foundDependentParam)
    {
        for(i = 0; i < num_params; i++)
        {
            const nsXPTParamInfo& param_info = function_info->GetParam(i);
            const nsXPTType& type = param_info.GetType();

            if(!type_info.IsDependent())
                continue;

            nsXPTType datum_type;
            JSUint32 array_count;
            JSUint32 array_capacity;
            JSBool use_allocator = JS_FALSE;
            PRBool isArray = type_info.IsArray();

            PRBool isSizedString = isArray ?
                    JS_FALSE :
                    type_info.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                    type_info.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;

            nsXPTCVariant* param = &param_array[i];
            param->type = type;

            if(isArray)
            {
                param->SetValIsArray();

                if(NS_FAILED(function_data->iface_info->GetTypeForParam(function_data->vtable_index, &param_info, 1,
                                                    &datum_type)))
                {
                    Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, cx);
                    goto done;
                }
            }
            else
                datum_type = type;

            if(datum_type.IsInterfacePointer())
            {
                param->SetValIsInterface();
            }

            // set 'src' to be the object from which we get the value and
            // prepare for out param

            if(param_info.IsOut())
            {
                param->SetPtrIsData();
                param->ptr = &param->val;

                if(!param_info.IsRetval() &&
                   (JSVAL_IS_PRIMITIVE(argv[i]) ||
                    !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(argv[i]),
                        rt->GetStringID(XPCJSRuntime::IDX_VALUE), &src)))
                {
                    ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, i, cx);
                    goto done;
                }

                if(datum_type.IsPointer() &&
                   !datum_type.IsInterfacePointer() &&
                   (isArray || !param_info.IsShared()))
                {
                    use_allocator = JS_TRUE;
                    param->SetValIsAllocated();
                }

                if(!param_info.IsIn())
                    continue;
            }
            else
            {
                src = argv[i];

                if(datum_type.IsPointer() &&
                   datum_type.TagPart() == nsXPTType::T_IID)
                {
                    use_allocator = JS_TRUE;
                    param->SetValIsAllocated();
                }
            }

            if(datum_type.IsInterfacePointer() &&
               !GetInterfaceTypeFromParam(//cx,
                                          function_data->iface_info, function_info, param_info,
                                          function_data->vtable_index, i, datum_type.TagPart(),
                                          param_array, &param_iid))
                goto done;

            if(isArray || isSizedString)
            {
                if(!GetArraySizeFromParam(cx, function_data->iface_info, function_info, param_info,
                                          function_data->vtable_index, i, GET_SIZE,
                                          param_array, &array_capacity)||
                   !GetArraySizeFromParam(cx, function_data->iface_info, function_info, param_info,
                                          function_data->vtable_index, i, GET_LENGTH,
                                          param_array, &array_count))
                    goto done;

                if(isArray)
                {
                    if(array_count &&
                       !XPCConvert::JSArray2Native(cx, (void**)&param->val, src,
                                                   array_count, array_capacity,
                                                   datum_type,
                                                   use_allocator,
                                                   &param_iid, &err))
                    {
                        // XXX need exception scheme for arrays to indicate bad element
                        ThrowBadParam(err, i, cx);
                        goto done;
                    }
                }
                else // if(isSizedString)
                {
                    if(!XPCConvert::JSStringWithSize2Native(cx,
                                                   (void*)&param->val,
                                                   src,
                                                   array_count, array_capacity,
                                                   datum_type, use_allocator,
                                                   &err))
                    {
                        ThrowBadParam(err, i, cx);
                        goto done;
                    }
                }
            }
            else
            {
                if(!XPCConvert::JSData2Native(cx, &param->val, src, type,
                                              use_allocator, &param_iid,
                                              &err))
                {
                    ThrowBadParam(err, i, cx);
                    goto done;
                }
            }
        }
    }
***/


    nr = NS_ERROR_FAILURE;
/***
    {
        // avoid deadlock in case the native method blocks somehow
        AutoJSSuspendRequest req(cx);  // scoped suspend of request
***/

        nr = XPTC_InvokeByIndex_DynLoad(instance_data->isupports,
                                        function_data->vtable_index,
                                        num_params, param_array.get());
/***
        // resume non-blocking JS operations now
    }


    xpcc->SetLastResult(nr);
***/

    if (NS_FAILED(nr)) {
      //ThrowBadResult(nr, cx); // eventually calls JS_SetPendingException
      goto done;
    }
/***
      else if (cx.GetExceptionWasThrown()) {
      // the native callee claims to have already set a JSException
      goto done;
    }
***/


    // convert results
    for (int i = 0; i < num_params; ++i) {
      nsXPTCVariant *param = &param_array[i];
      const nsXPTParamInfo &param_info = function_info->GetParam(i);
      const nsXPTType &type_info = param_info.GetType();

      if (!param_info.IsOut() && !param_info.IsDipper()) {
        continue;
      }

      jsval v = JSVAL_NULL;
/***
      AUTO_MARK_JSVAL(cx, &v);
***/
      nsXPTType datum_type;
      PRBool isArray = type_info.IsArray();
      PRBool isSizedString = isArray ?
                JS_FALSE :
                type_info.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                type_info.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;
      nsIID param_iid;
/***
      JSUint32 array_count;
***/

      if (isArray) {
        if(NS_FAILED(function_data->iface_info->GetTypeForParam(function_data->vtable_index,
                                                &param_info, 1,
                                                &datum_type))) {
          assert(false); // NOT YET IMPLEMENTED!
          //Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, cx);
          goto done;
        }
      } else {
        datum_type = type_info;
      }

      if(isArray || isSizedString) {
          assert(false); // NOT YET IMPLEMENTED!
/***
            if(!GetArraySizeFromParam(cx, function_data->iface_info, function_info, param_info,
                                      function_data->vtable_index, i, GET_LENGTH, param_array,
                                      &array_count))
                goto done;
***/
      }

      if(datum_type.IsInterfacePointer()) {
        if (!GetInterfaceTypeFromParam(//cx,
                                       function_data->iface_info, function_info, param_info,
                                       function_data->vtable_index, i, datum_type.TagPart(),
                                       param_array.get(), &param_iid)) {
          goto done;
        }
      }

      if(isArray) {
        assert(false); // NOT YET IMPLEMENTED!
/***
            if(!XPCConvert::NativeArray2JS(cx, &v, (const void**)&param->val,
                                           datum_type, &param_iid,
                                           array_count, cx.GetCurrentJSObject(),
                                           &err))
            {
                // XXX need exception scheme for arrays to indicate bad element
                assert(false); // NOT YET IMPLEMENTED!
                //ThrowBadParam(err, i, cx);
                goto done;
            }
***/
      } else if(isSizedString) {
        assert(false); // NOT YET IMPLEMENTED!
/***
        if(!XPCConvert::NativeStringWithSize2JS(cx, &v,
                                                (const void*)&param->val,
                                                datum_type,
                                                array_count, &err)) {
          assert(false); // NOT YET IMPLEMENTED!
          //ThrowBadParam(err, i, cx);
          goto done;
        }
***/
      } else {

        nsresult err;
/***
        if (!NativeData2JS(cx,
                           cx.GetCurrentJSObject(),
                           &v, &param->val,
                           datum_type, &param_iid,
                           &err)) {
***/
        js_ok = NativeData2JS(cx, obj, &v, &param->val, type_info, &param_iid,
                              function_data->proto_data->js_wrapper, &err);
        if (!js_ok) {
          assert(false); // NOT YET IMPLEMENTED!
//          //ThrowBadParam(err, i, cx);
//          goto done;
        }

      }

      if (param_info.IsRetval()) {
        //if (!cx.GetReturnValueWasSet()) { // I'M NOT SIMULATING THIS; I DON'T THINK I NEED TO BE
        //  cx.SetRetVal(v);
        //}
        *js_retval = v; // CPRINCE REPLACEMENT
      } else {
        assert(false); // NOT YET IMPLEMENTED!
/***
        // we actually assured this before doing the invoke
        NS_ASSERTION(JSVAL_IS_OBJECT(argv[i]), "out var is not object");
        if(!OBJ_SET_PROPERTY(cx, JSVAL_TO_OBJECT(argv[i]),
                             rt->GetStringID(XPCJSRuntime::IDX_VALUE), &v)) {
          assert(false); // NOT YET IMPLEMENTED!
          //ThrowBadParam(NS_ERROR_XPC_CANT_SET_OUT_VAL, i, cx);
          goto done;
        }
***/
      }
    } // END convert results: for (int i = 0; i < num_params; ++i)


    retval = JS_TRUE;
done:
    // iterate through the params (again!) and clean up
    // any alloc'd stuff and release wrappers of params
/***
    if (param_array.get()) {
***/
        // cleanup params
        for (int i = 0; i < num_params; ++i) {
            nsXPTCVariant *param = &param_array[i];
            void* val_p = param->val.p;
//  continue; // FIXME: HUGE HACK! REMOVE THIS AFTER GETTING INVOKE TO WORK
            if(!val_p)
                continue;

            if(param->IsValArray()) {
                assert(false); // NOT YET IMPLEMENTED!
/***
                // going to have to cleanup the array and perhaps its contents
                if(param->IsValAllocated() || param->IsValInterface())
                {
                    // we need to figure out how many elements are present.
                    JSUint32 array_count;

                    const nsXPTParamInfo& param_info = function_info->GetParam(i);
                    if(!GetArraySizeFromParam(cx, function_data->iface_info, function_info,
                                              param_info, function_data->vtable_index,
                                              i, GET_LENGTH, param_array,
                                              &array_count))
                    {
                        NS_ASSERTION(0,"failed to get array length, we'll leak here");
                        continue;
                    }
                    if(param->IsValAllocated())
                    {
                        void** a = (void**)val_p;
                        for(JSUint32 k = 0; k < array_count; k++)
                        {
                            void* o = a[k];
                            if(o) nsMemory::Free(o);
                        }
                    }
                    else // if(param->IsValInterface())
                    {
                        nsISupports** a = (nsISupports**)val_p;
                        for(JSUint32 k = 0; k < array_count; k++)
                        {
                            nsISupports* o = a[k];
                            NS_IF_RELEASE(o);
                        }
                    }
                }
                // always free the array itself
                nsMemory::Free(val_p);
***/
            } else if(param->IsValAllocated()) {
                nsMemory::Free(val_p);
            } else if(param->IsValInterface()) {
// FIXME: MAKE SURE REMOVING THIS IS RIGHT (I can believe it; recall double-addref NativeWrapper weirdness.)
              //((nsISupports*)val_p)->Release();
            } else if(param->IsValDOMString()) {
              //delete (nsAString*) val_p; // we allocated a wrapper nsAString
              delete (nsString*) val_p; // we allocated a wrapper nsString, or nsDependentString (which inherits from nsString)
            } else if(param->IsValUTF8String()) {
                delete (nsCString*) val_p;
            } else if(param->IsValCString()) {
                delete (nsCString*) val_p;
            }
        } // END cleanup params: for (int i = 0; i < num_params; ++i)
/***
    }
***/

/*** DON'T NEED THIS -- WE USE SCOPED_ARRAY
    if (param_array) {
      delete [] param_array;
    }
***/

    return retval;
}


// General-purpose wrapper to construct any class object.
// Gets called when an object is created in JavaScript using "new CLASSNAME".
JSBool JsContextWrapper::JsWrapperConstructor(JSContext *cx, JSObject *obj,
                                              uintN argc, jsval *argv,
                                              jsval *retval) {
  assert(false);
  // To finish this function, we would need to call InitBase* on the
  // object returned by do_CreateInstance().  But it's not immediately clear
  // where that information would come from (maybe thread-local storage?),
  // and we currently don't need "new CLASSNAME" in JS threads anyway.
  // So punt for now.  We can revive the code below if/when necessary.
  return JS_FALSE;
/*
  assert(argc == 0); // we don't handle arguments here

  JSObject *proto_obj = JS_GetPrototype(cx, obj);
  if (!proto_obj) { return JS_FALSE; }

  JsWrapperDataForProto *proto_data =
      static_cast<JsWrapperDataForProto*>(JS_GetPrivate(cx, proto_obj));
  assert(proto_data);
  assert(proto_data->header.type == PROTO_JSOBJECT);
  //JsWrapperJSObjectData *proto_objdata =
  //    static_cast<JsWrapperJSObjectData*>(JS_GetPrivate(cx, proto_obj));
  //assert(proto_objdata);
  //assert(proto_objdata->IsProto());

  nsresult nr;
  nsCOMPtr<nsISupports> isupports = do_CreateInstance(proto_data->factory, &nr); // FIXME: IS NSISUPPORTS WHAT I WANT HERE??
  if (NS_FAILED(nr)) { return false; }
  // NOTE: could pass a class_id instead of a factory here (though maybe
  // factory is more efficient for frequent creation?)

  bool succeeded = SetupInstanceObject(cx, obj, isupports);
  if (!succeeded) { return JS_FALSE; }

  *retval = OBJECT_TO_JSVAL(obj);
  return JS_TRUE;
*/
}




// [Reference: this is inspired by Mozilla's XPCVariant::VariantDataToJS() in
// /xpconnect/src/xpcvariant.cpp]
static JSBool VariantDataToJS(JSContext *cx, JSObject *scope_obj,
                              nsIVariant *variant,
                              JsContextWrapper *js_wrapper,
                              nsresult *error_out, jsval *jsval_out)
{
    // Get the type early because we might need to spoof it below.
    PRUint16 type;
    if(NS_FAILED(variant->GetDataType(&type)))
        return JS_FALSE;

// UH OH, HOW WILL WE KNOW IF WE RECEIVED AN XPCVARIANT? (Maybe we won't, so we don't care??)
/***
    nsCOMPtr<XPCVariant> xpcvariant = do_QueryInterface(variant);
    if(xpcvariant)
    {
        jsval realVal = xpcvariant->GetJSVal();
        if(JSVAL_IS_PRIMITIVE(realVal) ||
           type == nsIDataType::VTYPE_ARRAY ||
           type == nsIDataType::VTYPE_ID)
        {
            // Not a JSObject (or is a JSArray or is a JSObject representing
            // an nsID),.
            // So, just pass through the underlying data.
            *jsval_out = realVal;
            return JS_TRUE;
        }

        // else, it's an object and we really need to double wrap it if we've
        // already decided that its 'natural' type is as some sort of interface.

        // We just fall through to the code below and let it do what it does.
    }
***/

    // The nsIVariant is not a XPCVariant (or we act like it isn't).
    // So we extract the data and do the Right Thing.

    // We ASSUME that the variant implementation can do these conversions...

    JSBool success;
    nsIID iid;
    PRUint32 string_num_chars;
    /*
    nsAutoString astring;
    nsCAutoString cString;
    nsUTF8String utf8String;
    */
    nsXPTCVariant xpctvar;
    xpctvar.flags = 0;

    switch(type)
    {
        case nsIDataType::VTYPE_INT8:
        case nsIDataType::VTYPE_INT16:
        case nsIDataType::VTYPE_INT32:
        case nsIDataType::VTYPE_INT64:
        case nsIDataType::VTYPE_UINT8:
        case nsIDataType::VTYPE_UINT16:
        case nsIDataType::VTYPE_UINT32:
        case nsIDataType::VTYPE_UINT64:
        case nsIDataType::VTYPE_FLOAT:
        case nsIDataType::VTYPE_DOUBLE:
        {
            // Easy. Handle inline.
            if(NS_FAILED(variant->GetAsDouble(&xpctvar.val.d)))
                return JS_FALSE;
            return JS_NewNumberValue(cx, xpctvar.val.d, jsval_out);
        }
        case nsIDataType::VTYPE_BOOL:
        {
            // Easy. Handle inline.
            if(NS_FAILED(variant->GetAsBool(&xpctvar.val.b)))
                return JS_FALSE;
            *jsval_out = BOOLEAN_TO_JSVAL(xpctvar.val.b);
            return JS_TRUE;
        }
        case nsIDataType::VTYPE_CHAR:
            if(NS_FAILED(variant->GetAsChar(&xpctvar.val.c)))
                return JS_FALSE;
            xpctvar.type = (uint8)TD_CHAR;
            break;
        case nsIDataType::VTYPE_WCHAR:
            if(NS_FAILED(variant->GetAsWChar(&xpctvar.val.wc)))
                return JS_FALSE;
            xpctvar.type = (uint8)TD_WCHAR;
            break;
        case nsIDataType::VTYPE_ID:
            if(NS_FAILED(variant->GetAsID(&iid)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PNSIID | XPT_TDP_POINTER);
            xpctvar.val.p = &iid;
            break;
        case nsIDataType::VTYPE_ASTRING:
            assert(false); // NOT YET IMPLEMENTED!
/***
            if(NS_FAILED(variant->GetAsAString(astring)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_ASTRING | XPT_TDP_POINTER);
            xpctvar.val.p = &astring;
***/
            break;
        case nsIDataType::VTYPE_DOMSTRING:
            assert(false); // NOT YET IMPLEMENTED!
/***
            if(NS_FAILED(variant->GetAsAString(astring)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_DOMSTRING | XPT_TDP_POINTER);
            xpctvar.val.p = &astring;
***/
            break;
        case nsIDataType::VTYPE_CSTRING:
            assert(false); // NOT YET IMPLEMENTED!
/***
            if(NS_FAILED(variant->GetAsACString(cString)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_CSTRING | XPT_TDP_POINTER);
            xpctvar.val.p = &cString;
***/
            break;
        case nsIDataType::VTYPE_UTF8STRING:
            assert(false); // NOT YET IMPLEMENTED!
/***
            if(NS_FAILED(variant->GetAsAUTF8String(utf8String)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_UTF8STRING | XPT_TDP_POINTER);
            xpctvar.val.p = &utf8String;
***/
            break;
        case nsIDataType::VTYPE_CHAR_STR:
            assert(false); // NOT YET IMPLEMENTED!
/***
            if(NS_FAILED(variant->GetAsString((char**)&xpctvar.val.p)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PSTRING | XPT_TDP_POINTER);
            xpctvar.SetValIsAllocated();
***/
            break;
        case nsIDataType::VTYPE_STRING_SIZE_IS:
            assert(false); // NOT YET IMPLEMENTED!
/***
            if(NS_FAILED(variant->GetAsStringWithSize(&string_num_chars,
                                                      (char**)&xpctvar.val.p)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PSTRING_SIZE_IS | XPT_TDP_POINTER);
***/
            break;
        case nsIDataType::VTYPE_WCHAR_STR:
            assert(false); // NOT YET IMPLEMENTED!
/***
            if(NS_FAILED(variant->GetAsWString((PRUnichar**)&xpctvar.val.p)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PWSTRING | XPT_TDP_POINTER);
            xpctvar.SetValIsAllocated();
***/
            break;
        case nsIDataType::VTYPE_WSTRING_SIZE_IS:
            if(NS_FAILED(variant->GetAsWStringWithSize(&string_num_chars,
                                                      (PRUnichar**)&xpctvar.val.p)))
                return JS_FALSE;
            xpctvar.type = (uint8)(TD_PWSTRING_SIZE_IS | XPT_TDP_POINTER);
            break;
        case nsIDataType::VTYPE_INTERFACE:
        case nsIDataType::VTYPE_INTERFACE_IS:
        {
            nsID* piid;
            if(NS_FAILED(variant->GetAsInterface(&piid, &xpctvar.val.p)))
                return JS_FALSE;

            iid = *piid;
            nsMemory::Free((char*)piid);

            xpctvar.type = (uint8)(TD_INTERFACE_IS_TYPE | XPT_TDP_POINTER);
            if(xpctvar.val.p)
                xpctvar.SetValIsInterface();
            break;
        }
        case nsIDataType::VTYPE_ARRAY:
        {
            assert(false); // NOT YET IMPLEMENTED!
/***
            nsDiscriminatedUnion du;
            nsVariant::Initialize(&du);
            nsresult rv;

            rv = variant->GetAsArray(&du.u.array.mArrayType,
                                     &du.u.array.mArrayInterfaceID,
                                     &du.u.array.mArrayCount,
                                     &du.u.array.mArrayValue);
            if(NS_FAILED(rv))
                return JS_FALSE;

            // must exit via VARIANT_DONE from here on...
            du.mType = nsIDataType::VTYPE_ARRAY;
            success = JS_FALSE;

            nsXPTType conversionType;
            PRUint16 elementType = du.u.array.mArrayType;
            const nsID* pid = nsnull;

            switch(elementType)
            {
                case nsIDataType::VTYPE_INT8:
                case nsIDataType::VTYPE_INT16:
                case nsIDataType::VTYPE_INT32:
                case nsIDataType::VTYPE_INT64:
                case nsIDataType::VTYPE_UINT8:
                case nsIDataType::VTYPE_UINT16:
                case nsIDataType::VTYPE_UINT32:
                case nsIDataType::VTYPE_UINT64:
                case nsIDataType::VTYPE_FLOAT:
                case nsIDataType::VTYPE_DOUBLE:
                case nsIDataType::VTYPE_BOOL:
                case nsIDataType::VTYPE_CHAR:
                case nsIDataType::VTYPE_WCHAR:
                    conversionType = nsXPTType((uint8)elementType);
                    break;

                case nsIDataType::VTYPE_ID:
                case nsIDataType::VTYPE_CHAR_STR:
                case nsIDataType::VTYPE_WCHAR_STR:
                    conversionType = nsXPTType((uint8)elementType | XPT_TDP_POINTER);
                    break;

                case nsIDataType::VTYPE_INTERFACE:
                    pid = &NS_GET_IID(nsISupports);
                    conversionType = nsXPTType((uint8)elementType | XPT_TDP_POINTER);
                    break;

                case nsIDataType::VTYPE_INTERFACE_IS:
                    pid = &du.u.array.mArrayInterfaceID;
                    conversionType = nsXPTType((uint8)elementType | XPT_TDP_POINTER);
                    break;

                // The rest are illegal.
                case nsIDataType::VTYPE_VOID:
                case nsIDataType::VTYPE_ASTRING:
                case nsIDataType::VTYPE_DOMSTRING:
                case nsIDataType::VTYPE_CSTRING:
                case nsIDataType::VTYPE_UTF8STRING:
                case nsIDataType::VTYPE_WSTRING_SIZE_IS:
                case nsIDataType::VTYPE_STRING_SIZE_IS:
                case nsIDataType::VTYPE_ARRAY:
                case nsIDataType::VTYPE_EMPTY_ARRAY:
                case nsIDataType::VTYPE_EMPTY:
                default:
                    NS_ERROR("bad type in array!");
                    goto VARIANT_DONE;
            }

            success =
                XPCConvert::NativeArray2JS(cx, jsval_out,
                                           (const void**)&du.u.array.mArrayValue,
                                           conversionType, pid,
                                           du.u.array.mArrayCount,
                                           scope_obj, error_out);

VARIANT_DONE:
            nsVariant::Cleanup(&du);
            return success;
***/ return JS_FALSE; // replaces "return success;"
        }
        case nsIDataType::VTYPE_EMPTY_ARRAY:
        {
            JSObject* array = JS_NewArrayObject(cx, 0, nsnull);
            if(!array)
                return JS_FALSE;
            *jsval_out = OBJECT_TO_JSVAL(array);
            return JS_TRUE;
        }
        case nsIDataType::VTYPE_VOID:
        case nsIDataType::VTYPE_EMPTY:
            *jsval_out = JSVAL_VOID;
            return JS_TRUE;
        default:
            NS_ERROR("bad type in variant!");
            return JS_FALSE;
    }

    // If we are here then we need to convert the data in the xpctvar.

    if(xpctvar.type.TagPart() == TD_PSTRING_SIZE_IS ||
       xpctvar.type.TagPart() == TD_PWSTRING_SIZE_IS)
    {
/***
        success = XPCConvert::NativeStringWithSize2JS(cx, jsval_out,
                                                      (const void*)&xpctvar.val,
                                                      xpctvar.type,
                                                      string_num_chars, error_out);
***/
        success = NativeStringWithSize2JS(cx, jsval_out,
                                          static_cast<const void*>(&xpctvar.val),
                                          xpctvar.type,
                                          string_num_chars, error_out);
    }
    else
    {
        success = NativeData2JS(cx, scope_obj,
                                jsval_out, (const void*)&xpctvar.val,
                                xpctvar.type, &iid,
                                js_wrapper,
                                error_out);
    }

    if(xpctvar.IsValAllocated())
        nsMemory::Free((char*)xpctvar.val.p);
    else if(xpctvar.IsValInterface())
        ((nsISupports*)xpctvar.val.p)->Release();

    return success;
}







// ASSUMES 'HAVE_LONG_LONG' WAS DEFINED
#define JAM_DOUBLE(cx,v,d) (d = JS_NewDouble(cx, (jsdouble)v) , \
                            d ? DOUBLE_TO_JSVAL(d) : JSVAL_ZERO)
// Win32 can't handle uint64 to double conversion
#define JAM_DOUBLE_U64(cx,v,d) JAM_DOUBLE(cx,((int64)v),d)

#define FIT_32(cx,i,d) (INT_FITS_IN_JSVAL(i) ? \
                        INT_TO_JSVAL(i) : JAM_DOUBLE(cx,i,d))

// XXX will this break backwards compatability???
#define FIT_U32(cx,i,d) ((i) <= JSVAL_INT_MAX ? \
                         INT_TO_JSVAL(i) : JAM_DOUBLE(cx,i,d))


// Converts simple types (non-string, non-interface).
// Returns JS_FALSE if the specified type was not a simple type.
static JSBool NativeSimpleData2JS(JSContext *cx, jsval *d, const void *s,
                                  const nsXPTType &type_info) {
  jsdouble *dbl = nsnull;
  switch (type_info.TagPart()) {
    case nsXPTType::T_I8    : *d = INT_TO_JSVAL((int32)*((int8*)s));     break;
    case nsXPTType::T_I16   : *d = INT_TO_JSVAL((int32)*((int16*)s));    break;
    case nsXPTType::T_I32   : *d = FIT_32(cx,*((int32*)s),dbl);          break;
    case nsXPTType::T_I64   : *d = JAM_DOUBLE(cx,*((int64*)s),dbl);      break;
    case nsXPTType::T_U8    : *d = INT_TO_JSVAL((int32)*((uint8*)s));    break;
    case nsXPTType::T_U16   : *d = INT_TO_JSVAL((int32)*((uint16*)s));   break;
    case nsXPTType::T_U32   : *d = FIT_U32(cx,*((uint32*)s),dbl);        break;
    case nsXPTType::T_U64   : *d = JAM_DOUBLE_U64(cx,*((uint64*)s),dbl); break;
    case nsXPTType::T_FLOAT : *d = JAM_DOUBLE(cx,*((float*)s),dbl);      break;
    case nsXPTType::T_DOUBLE: *d = JAM_DOUBLE(cx,*((double*)s),dbl);     break;
    case nsXPTType::T_BOOL  : *d = *((PRBool*)s)?JSVAL_TRUE:JSVAL_FALSE; break;
    default:
      return JS_FALSE;
  }
  return JS_TRUE;
}

#undef JAM_DOUBLE
#undef JAM_DOUBLE_U64
#undef FIT_32
#undef FIT_U32



// [Reference: this is inspired by Mozilla's XPCConvert::NativeData2JS() in
// /xpconnect/src/xpcconvert.cpp]
JSBool NativeData2JS(JSContext *cx, JSObject *scope_obj,
                     jsval *d, const void *s,
                     const nsXPTType &type_info, const nsIID *iid,
                     JsContextWrapper *js_wrapper,
                     nsresult *error_out) {
    NS_PRECONDITION(s, "bad param");
    NS_PRECONDITION(d, "bad param");

    if (error_out) {
      //*error_out = NS_ERROR_XPC_BAD_CONVERT_NATIVE;
      *error_out = NS_ERROR_FAILURE;
    }

    if (NativeSimpleData2JS(cx, d, s, type_info)) {
      return JS_TRUE;
    }

    switch (type_info.TagPart()) {
/**
 moved to NativeSimpleData2JS() above
      case nsXPTType::T_I8    : *d = INT_TO_JSVAL((int32)*((int8*)s));     break;
      case nsXPTType::T_I16   : *d = INT_TO_JSVAL((int32)*((int16*)s));    break;
      case nsXPTType::T_I32   : *d = FIT_32(cx,*((int32*)s),dbl);          break;
      case nsXPTType::T_I64   : *d = JAM_DOUBLE(cx,*((int64*)s),dbl);      break;
      case nsXPTType::T_U8    : *d = INT_TO_JSVAL((int32)*((uint8*)s));    break;
      case nsXPTType::T_U16   : *d = INT_TO_JSVAL((int32)*((uint16*)s));   break;
      case nsXPTType::T_U32   : *d = FIT_U32(cx,*((uint32*)s),dbl);        break;
      case nsXPTType::T_U64   : *d = JAM_DOUBLE_U64(cx,*((uint64*)s),dbl); break;
      case nsXPTType::T_FLOAT : *d = JAM_DOUBLE(cx,*((float*)s),dbl);      break;
      case nsXPTType::T_DOUBLE: *d = JAM_DOUBLE(cx,*((double*)s),dbl);     break;
      case nsXPTType::T_BOOL  : *d = *((PRBool*)s)?JSVAL_TRUE:JSVAL_FALSE; break;
**/
      case nsXPTType::T_CHAR  :
        {
            char* p = (char*)s;
            if(!p)
                return JS_FALSE;

#ifdef STRICT_CHECK_OF_UNICODE
            NS_ASSERTION(! ILLEGAL_CHAR_RANGE(p) , "passing non ASCII data");
#endif // STRICT_CHECK_OF_UNICODE

            JSString* str;
            if(!(str = JS_NewStringCopyN(cx, p, 1)))
                return JS_FALSE;
            *d = STRING_TO_JSVAL(str);
            break;
        }
      case nsXPTType::T_WCHAR :
        {
            jschar* p = (jschar*)s;
            if(!p)
                return JS_FALSE;
            JSString* str;
            if(!(str = JS_NewUCStringCopyN(cx, p, 1)))
                return JS_FALSE;
            *d = STRING_TO_JSVAL(str);
            break;
        }
      default:
        if (!type_info.IsPointer()) {
/***
          XPC_LOG_ERROR(("XPCConvert::NativeData2JS : unsupported type"));
***/
          return JS_FALSE;
        }

        // set the default result
        *d = JSVAL_NULL;

        switch (type_info.TagPart()) {
          case nsXPTType::T_VOID:
/***
            XPC_LOG_ERROR(("XPCConvert::NativeData2JS : void* params not supported"));
***/
            return JS_FALSE;

          case nsXPTType::T_IID:
            {
                assert(false); // NOT YET IMPLEMENTED!
/***
                nsIID* iid2 = *((nsID**)s);
                if(!iid2)
                    break;
                JSObject* obj;
                if(!(obj = xpc_NewIDObject(cx, scope_obj, *iid2)))
                    return JS_FALSE;
                *d = OBJECT_TO_JSVAL(obj);
***/
                break;
            }

          case nsXPTType::T_ASTRING:
            // Fall through to T_DOMSTRING case

          case nsXPTType::T_DOMSTRING: {
/***
                const nsAString* p = *((const nsAString**)s);
                if(!p)
                    break;

                if(!p->IsVoid()) {
                    JSString *str =
                        XPCStringConvert::ReadableToJSString(cx, *p);
                    if(!str)
                        return JS_FALSE;

                    *d = STRING_TO_JSVAL(str);
                }
***/
            const nsString* p = *((const nsString**)s);
            if (!p) {
              break;
            }

            // not supporting IsVoid(), which is Firefox-internal
            const nsAString::char_type *char_ptr = p->get();
            PRUint32 num_chars = p->Length();
            JSBool js_ok = NativeStringWithSize2JS(cx, d, &char_ptr,
                                                   type_info, num_chars,
                                                   error_out);
            if (!js_ok) {
                return JS_FALSE;
            }


            // *d is defaulted to JSVAL_NULL so no need to set it
            // again if p is a "void" string

            break;
          }

          case nsXPTType::T_CHAR_STR:
            {
                assert(false); // NOT YET IMPLEMENTED!
/***
                char* p = *((char**)s);
                if(!p)
                    break;

#ifdef STRICT_CHECK_OF_UNICODE
                PRBool isAscii = PR_TRUE;
                char* t;
                for(t=p; *t && isAscii ; t++) {
                  if(ILLEGAL_CHAR_RANGE(*t))
                      isAscii = PR_FALSE;
                }
                NS_ASSERTION(isAscii, "passing non ASCII data");
#endif // STRICT_CHECK_OF_UNICODE
                JSString* str;
                if(!(str = JS_NewStringCopyZ(cx, p)))
                    return JS_FALSE;
                *d = STRING_TO_JSVAL(str);
***/
                break;
            }

          case nsXPTType::T_WCHAR_STR:
            {
                assert(false); // NOT YET IMPLEMENTED!
/***
                jschar* p = *((jschar**)s);
                if(!p)
                    break;
                JSString* str;
                if(!(str = JS_NewUCStringCopyZ(cx, p)))
                    return JS_FALSE;
                *d = STRING_TO_JSVAL(str);
***/
                break;
            }
          case nsXPTType::T_UTF8STRING:
            {
                assert(false); // NOT YET IMPLEMENTED!
/***
                const nsACString* cString = *((const nsACString**)s);

                if(!cString)
                    break;

                if(!cString->IsVoid())
                {
                    PRUint32 len;
                    jschar *p = (jschar *)UTF8ToNewUnicode(*cString, &len);

                    if(!p)
                        return JS_FALSE;

                    JSString* jsString = JS_NewUCString(cx, p, len);

                    if(!jsString) {
                        nsMemory::Free(p);
                        return JS_FALSE;
                    }

                    *d = STRING_TO_JSVAL(jsString);
                }
***/

                break;

            }
          case nsXPTType::T_CSTRING:
            {
                assert(false); // NOT YET IMPLEMENTED!
/***
                const nsACString* cString = *((const nsACString**)s);

                if(!cString)
                    break;

                if(!cString->IsVoid())
                {
                    PRUnichar* unicodeString = ToNewUnicode(*cString);
                    if(!unicodeString)
                        return JS_FALSE;

                    if(sXPCOMUCStringFinalizerIndex == -1 &&
                       !AddXPCOMUCStringFinalizer())
                        return JS_FALSE;

                    JSString* jsString = JS_NewExternalString(cx,
                                             (jschar*)unicodeString,
                                             cString->Length(),
                                             sXPCOMUCStringFinalizerIndex);

                    if(!jsString)
                    {
                        nsMemory::Free(unicodeString);
                        return JS_FALSE;
                    }

                    *d = STRING_TO_JSVAL(jsString);
                }
***/

                break;
            }

          case nsXPTType::T_INTERFACE:
          case nsXPTType::T_INTERFACE_IS: {
            nsISupports *isupports = *((nsISupports**)s);
            if (!isupports) {
              break; // JavaScript null exits here
            }

            // If interface is nsIVariant, unpack the value
            if(iid->Equals(NS_GET_IID(nsIVariant))) {
              nsCOMPtr<nsIVariant> ivariant = do_QueryInterface(isupports);
              if (!ivariant) {
                return JS_FALSE;
              }

              return VariantDataToJS(cx, scope_obj, ivariant, js_wrapper,
                                     error_out, d);
            } else {
              // Otherwise it's a "real" object interface.
/***
                  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
                  if(!NativeInterface2JSObject(cx, getter_AddRefs(holder),
                                               isupports, iid, scope_obj, PR_TRUE,
                                               error_out))
                      return JS_FALSE;

                  if(holder)
                  {
                      JSObject* jsobj;
                      if(NS_FAILED(holder->GetJSObject(&jsobj)))
                          return JS_FALSE;
                      *d = OBJECT_TO_JSVAL(jsobj);
                  }
***/

              // If we received a generic nsISupports, determine underlying type.
              nsIID param_iid = *iid;
              if (param_iid.Equals(NS_GET_IID(nsISupports))) {
                const IIDToProtoMap *map = &(js_wrapper->iid_to_proto_map_);
                IIDToProtoMap::const_iterator it;
                for (it = map->begin(); it != map->end(); ++it) {
                  nsIID test_iid = it->first;
                  void *new_ptr;
                  if (NS_SUCCEEDED(isupports->QueryInterface(test_iid, &new_ptr))) {
                    isupports->Release(); // just checking IID; restore refcount
                    param_iid = test_iid;
                    break;
                  }
                }
              }

              // Associate interface with matching proto_obj, so JavaScript can
              // know what methods and attributes exist.

              JSObject *out_proto_obj = js_wrapper->iid_to_proto_map_[param_iid];
              assert(out_proto_obj);

              JsWrapperDataForProto *proto_data =
                  static_cast<JsWrapperDataForProto*>(JS_GetPrivate(cx,
                                                                    out_proto_obj));
              assert(proto_data);
              assert(proto_data->header.type == PROTO_JSOBJECT);

              // Note: JS_NewObject by itself does not call the object constructor.
              // JS_ConstructObject is a superset that does.
              JSObject *out_instance_obj = JS_NewObject(cx,
                                                        proto_data->alloc_jsclass.get(),
                                                        out_proto_obj,
                                                        scope_obj); // parent
              if (!out_instance_obj) {
                assert(false); // NOT YET IMPLEMENTED??
                //return <FAILURE>
              }

              // TODO: cleanup JSObject if code BELOW fails?

              bool succeeded = js_wrapper->SetupInstanceObject(
                                               cx, out_instance_obj, isupports);
              if (!succeeded) {
                assert(false); // NOT YET IMPLEMENTED??
                //return <FAILURE>
              }

              // This object has now been AddRef'd in the JS engine, so we can
              // release our pointer.
              isupports->Release();

              *d = OBJECT_TO_JSVAL(out_instance_obj);
            } // END: if (nsIVariant) ... else ...
            break;
          }
          default:
            NS_ASSERTION(0, "bad type");
            return JS_FALSE;
        }

    }
    return JS_TRUE;
}


// Helper function for converting strings.
// If *src is a NULL pointer, returns success without touching *dest.
//
// [Reference: copied from Mozilla's XPCConvert::NativeStringWithSize2JS()
// in /xpconnect/src/xpcconvert.cpp]
static JSBool NativeStringWithSize2JS(JSContext *cx,
                                      jsval *dest, const void *src,
                                      const nsXPTType &type_info,
                                      JSUint32 count,
                                      nsresult *pErr) {
    NS_PRECONDITION(src, "bad param");
    NS_PRECONDITION(dest, "bad param");

    if(pErr)
        *pErr = NS_ERROR_XPC_BAD_CONVERT_NATIVE;

    if(!type_info.IsPointer())
    {
/***
        XPC_LOG_ERROR(("XPCConvert::NativeStringWithSize2JS : unsupported type"));
***/
        return JS_FALSE;
    }
    switch(type_info.TagPart())
    {
        case nsXPTType::T_PSTRING_SIZE_IS:
        {
            char* p = *((char**)src);
            if(!p)
                break;
            JSString* str;
            if(!(str = JS_NewStringCopyN(cx, p, count)))
                return JS_FALSE;
            *dest = STRING_TO_JSVAL(str);
            break;
        }
        case nsXPTType::T_ASTRING: // added by Scour
        case nsXPTType::T_DOMSTRING: // added by Scour
        case nsXPTType::T_PWSTRING_SIZE_IS:
        {
            jschar* p = *((jschar**)src);
            if(!p)
                break;
            JSString* str;
            if(!(str = JS_NewUCStringCopyN(cx, p, count)))
                return JS_FALSE;
            *dest = STRING_TO_JSVAL(str);
            break;
        }
        default:
/***
            XPC_LOG_ERROR(("XPCConvert::NativeStringWithSize2JS : unsupported type"));
***/
            return JS_FALSE;
    }
    return JS_TRUE;
}


// [Reference: this is inspired by Mozilla's XPCConvert::JSData2Native() in
// /xpconnect/src/xpcconvert.cpp]
static JSBool JSData2Native(JSContext *cx, void* d, jsval s,
                            const nsXPTType& type_info,
                            JSBool use_allocator, const nsIID* iid,
                            nsresult* error_out)
{
    NS_PRECONDITION(d, "bad param");

    int32    ti;
    uint32   tu;
    jsdouble td;
    JSBool isDOMString = JS_TRUE;

    if (error_out)
      //*error_out = NS_ERROR_XPC_BAD_CONVERT_JS;
      *error_out = NS_ERROR_FAILURE;

    switch(type_info.TagPart())
    {
    case nsXPTType::T_I8     :
        if(!JS_ValueToECMAInt32(cx, s, &ti))
            return JS_FALSE;
        *((int8*)d)  = (int8) ti;
        break;
    case nsXPTType::T_I16    :
        if(!JS_ValueToECMAInt32(cx, s, &ti))
            return JS_FALSE;
        *((int16*)d)  = (int16) ti;
        break;
    case nsXPTType::T_I32    :
        if(!JS_ValueToECMAInt32(cx, s, (int32*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_I64    :
        if(JSVAL_IS_INT(s))
        {
            if(!JS_ValueToECMAInt32(cx, s, &ti))
                return JS_FALSE;
            LL_I2L(*((int64*)d),ti);

        }
        else
        {
            if(!JS_ValueToNumber(cx, s, &td))
                return JS_FALSE;
            LL_D2L(*((int64*)d),td);
        }
        break;
    case nsXPTType::T_U8     :
        if(!JS_ValueToECMAUint32(cx, s, &tu))
            return JS_FALSE;
        *((uint8*)d)  = (uint8) tu;
        break;
    case nsXPTType::T_U16    :
        if(!JS_ValueToECMAUint32(cx, s, &tu))
            return JS_FALSE;
        *((uint16*)d)  = (uint16) tu;
        break;
    case nsXPTType::T_U32    :
        if(!JS_ValueToECMAUint32(cx, s, (uint32*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_U64    :
        if(JSVAL_IS_INT(s))
        {
            if(!JS_ValueToECMAUint32(cx, s, &tu))
                return JS_FALSE;
            LL_UI2L(*((int64*)d),tu);
        }
        else
        {
            if(!JS_ValueToNumber(cx, s, &td))
                return JS_FALSE;
#ifdef XP_WIN
            // Note: Win32 can't handle double to uint64 directly
            *((uint64*)d) = (uint64)((int64) td);
#else
            LL_D2L(*((uint64*)d),td);
#endif
        }
        break;
    case nsXPTType::T_FLOAT  :
        if(!JS_ValueToNumber(cx, s, &td))
            return JS_FALSE;
        *((float*)d) = (float) td;
        break;
    case nsXPTType::T_DOUBLE :
        if(!JS_ValueToNumber(cx, s, (double*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_BOOL   :
        if(!JS_ValueToBoolean(cx, s, (JSBool*)d))
            return JS_FALSE;
        break;
    case nsXPTType::T_CHAR   :
        {
            char* bytes=nsnull;
            JSString* str;

            if(!(str = JS_ValueToString(cx, s))||
               !(bytes = JS_GetStringBytes(str)))
            {
                return JS_FALSE;
            }
/***
#ifdef DEBUG
            jschar* chars=nsnull;
            if(nsnull!=(chars = JS_GetStringChars(str)))
            {
                NS_ASSERTION((! ILLEGAL_RANGE(chars[0])),"U+0080/U+0100 - U+FFFF data lost");
            }
#endif // DEBUG
***/
            *((char*)d) = bytes[0];
            break;
        }
    case nsXPTType::T_WCHAR  :
        {
            jschar* chars=nsnull;
            JSString* str;
            if(!(str = JS_ValueToString(cx, s))||
               !(chars = JS_GetStringChars(str)))
            {
                return JS_FALSE;
            }
            *((uint16*)d)  = (uint16) chars[0];
            break;
        }
    default:
        if(!type_info.IsPointer())
        {
            NS_ASSERTION(0,"unsupported type");
            return JS_FALSE;
        }

        switch(type_info.TagPart())
        {
        case nsXPTType::T_VOID:
/***
            XPC_LOG_ERROR(("XPCConvert::JSData2Native : void* params not supported"));
***/
            NS_ASSERTION(0,"void* params not supported");
            return JS_FALSE;
        case nsXPTType::T_IID:
        {
            NS_ASSERTION(use_allocator,"trying to convert a JSID to nsID without allocator : this would leak");

            //JSObject* obj;
            const nsID* pid=nsnull;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type_info.IsReference())
                {
                    if(error_out)
                      //*error_out = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                      *error_out = NS_ERROR_FAILURE;
                    return JS_FALSE;
                }
                // else ...
                *((const nsID**)d) = nsnull;
                return JS_TRUE;
            }

            assert(false); // NOT YET IMPLEMENTED! (missing xpc_JSObjectToID)
            //if(!JSVAL_IS_OBJECT(s) ||
            //   (!(obj = JSVAL_TO_OBJECT(s))) ||
            //   (!(pid = xpc_JSObjectToID(cx, obj))))
            //{
            //    return JS_FALSE;
            //}

            *((const nsID**)d) = pid;
            return JS_TRUE;
        }

        case nsXPTType::T_ASTRING:
        {
            isDOMString = JS_FALSE;
            // Fall through to T_DOMSTRING case.
        }
        case nsXPTType::T_DOMSTRING:
        {
            static const PRUnichar EMPTY_STRING[] = { '\0' };
            static const PRUnichar VOID_STRING[] = { 'u', 'n', 'd', 'e', 'f', 'i', 'n', 'e', 'd', '\0' };

            const PRUnichar* chars = NULL;
            JSString* str = nsnull;
            JSBool isNewString = JS_FALSE;
            PRUint32 length = 0;

            if(JSVAL_IS_VOID(s))
            {
                if(isDOMString)
                {
                    chars  = VOID_STRING;
                    length = NS_ARRAY_LENGTH(VOID_STRING) - 1;
                }
                else
                {
                    chars = EMPTY_STRING;
                    length = 0;
                }
            }
            else if(!JSVAL_IS_NULL(s))
            {
                str = JS_ValueToString(cx, s);
                if(!str)
                    return JS_FALSE;

                length = (PRUint32) JS_GetStringLength(str);
                if(length)
                {
                    chars = (const PRUnichar*) JS_GetStringChars(str);
                    if(!chars)
                        return JS_FALSE;
                    if(STRING_TO_JSVAL(str) != s)
                        isNewString = JS_TRUE;
                }
                else
                {
                    str = nsnull;
                    chars = EMPTY_STRING;
                }
            }

            if(use_allocator)
            {
                // XXX extra string copy when isNewString
                if(str && !isNewString)
                {
                  //XPCReadableJSStringWrapper *wrapper =
                  //    XPCStringConvert::JSStringToReadable(str);
                  // CPRINCE replacement (after reading the source):
                  //*((const nsAString**)d) = wrapper;
                  nsDependentString *wrapper = new nsDependentString(
                      reinterpret_cast<PRUnichar*>(JS_GetStringChars(str)),
                      JS_GetStringLength(str));
                  if(!wrapper)
                    return JS_FALSE;
                  *((const nsAString**)d) = wrapper;
                }
                else if(JSVAL_IS_NULL(s))
                {
                  assert(false); // NOT YET IMPLEMENTED! (missing XPCReadableJSStringWrapper, and more)
                  return JS_FALSE;
/***
                    XPCReadableJSStringWrapper *wrapper =
                        new XPCReadableJSStringWrapper();
// from xpcprivate.h:
//    XPCReadableJSStringWrapper() :
//        nsDependentString(char_traits::sEmptyBuffer, char_traits::sEmptyBuffer)
//    { SetIsVoid(PR_TRUE); }
                    if(!wrapper)
                        return JS_FALSE;

                    *((const nsAString**)d) = wrapper;
***/
                }
                else
                {
                  assert(false); // NOT YET IMPLEMENTED! (commented this out b/c cleanup code casts void* to invoke nsDependentString dtor. Note: cannot access nsAString_external dtor.)
                  return JS_FALSE;
/***
                    // use nsString to encourage sharing
                    const nsAString *rs = new nsString(chars, length);
                    if(!rs)
                        return JS_FALSE;
                    *((const nsAString**)d) = rs;
***/
                }
            }
            else
            {
                nsAString* ws = *((nsAString**)d);

                if(JSVAL_IS_NULL(s) || (!isDOMString && JSVAL_IS_VOID(s)))
                {
                    assert(false); // NOT YET IMPLEMENTED! (missing nsAString::Truncate and ::SetIsVoid)
                    //ws->Truncate();
                    //ws->SetIsVoid(PR_TRUE);
                }
                else
                    ws->Assign(chars, length);
            }
            return JS_TRUE;
        }

        case nsXPTType::T_CHAR_STR:
        {
            char* bytes=nsnull;
            JSString* str;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type_info.IsReference())
                {
                    if(error_out)
                      //*error_out = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                      *error_out = NS_ERROR_FAILURE;
                    return JS_FALSE;
                }
                // else ...
                *((char**)d) = nsnull;
                return JS_TRUE;
            }

            if(!(str = JS_ValueToString(cx, s))||
               !(bytes = JS_GetStringBytes(str)))
            {
                return JS_FALSE;
            }
/***
#ifdef DEBUG
            jschar* chars=nsnull;
            if(nsnull != (chars = JS_GetStringChars(str)))
            {
                PRBool legalRange = PR_TRUE;
                int len = JS_GetStringLength(str);
                jschar* t;
                PRInt32 i=0;
                for(t=chars; (i< len) && legalRange ; i++,t++) {
                  if(ILLEGAL_RANGE(*t))
                      legalRange = PR_FALSE;
                }
                NS_ASSERTION(legalRange,"U+0080/U+0100 - U+FFFF data lost");
            }
#endif // DEBUG
***/
            if(use_allocator)
            {
                int len = (JS_GetStringLength(str) + 1) * sizeof(char);
                if(!(*((void**)d) = nsMemory::Alloc(len)))
                {
                    return JS_FALSE;
                }
                memcpy(*((void**)d), bytes, len);
            }
            else
                *((char**)d) = bytes;

            return JS_TRUE;
        }

        case nsXPTType::T_WCHAR_STR:
        {
            jschar* chars=nsnull;
            JSString* str;

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type_info.IsReference())
                {
                    if(error_out)
                      //*error_out = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                      *error_out = NS_ERROR_FAILURE;
                    return JS_FALSE;
                }
                // else ...
                *((jschar**)d) = nsnull;
                return JS_TRUE;
            }

            if(!(str = JS_ValueToString(cx, s))||
               !(chars = JS_GetStringChars(str)))
            {
                return JS_FALSE;
            }
            if(use_allocator)
            {
                int byte_len = (JS_GetStringLength(str)+1)*sizeof(jschar);
                if(!(*((void**)d) = nsMemory::Alloc(byte_len)))
                {
                    // XXX should report error
                    return JS_FALSE;
                }
                memcpy(*((void**)d), chars, byte_len);
            }
            else
                *((jschar**)d) = chars;

            return JS_TRUE;
        }

        case nsXPTType::T_UTF8STRING:
        {
            jschar* chars;
            PRUint32 length;
            JSString* str;

            if(JSVAL_IS_NULL(s) || JSVAL_IS_VOID(s))
            {
                assert(false); // NOT YET IMPLEMENTED! (missing nsAString::Truncate and ::SetIsVoid)
/***
                if(use_allocator)
                {
                    nsACString *rs = new nsCString();
                    if(!rs)
                        return JS_FALSE;

                    rs->SetIsVoid(PR_TRUE);
                    *((nsACString**)d) = rs;
                }
                else
                {
                    nsCString* rs = *((nsCString**)d);
                    rs->Truncate();
                    rs->SetIsVoid(PR_TRUE);
                }
***/
                return JS_TRUE;
            }

            // The JS val is neither null nor void...

            if(!(str = JS_ValueToString(cx, s))||
               !(chars = JS_GetStringChars(str)))
            {
                return JS_FALSE;
            }

            length = JS_GetStringLength(str);

            nsCString *rs;
            if(use_allocator)
            {
                // Use nsCString to enable sharing
                rs = new nsCString();
                if(!rs)
                    return JS_FALSE;

                *((const nsCString**)d) = rs;
            }
            else
            {
                rs = *((nsCString**)d);
            }
            assert(false); // NOT YET IMPLEMENTED!
            //CopyUTF16toUTF8(nsDependentString((const PRUnichar*)chars, length),
            //                *rs);

            return JS_TRUE;
        }

        case nsXPTType::T_CSTRING:
        {
            const char* chars;
            PRUint32 length;
            JSString* str;

            if(JSVAL_IS_NULL(s) || JSVAL_IS_VOID(s))
            {
                assert(false); // NOT YET IMPLEMENTED! (missing nsAString::Truncate and ::SetIsVoid)
/***
                if(use_allocator)
                {
                    nsACString *rs = new nsCString();
                    if(!rs)
                        return JS_FALSE;

                    rs->SetIsVoid(PR_TRUE);
                    *((nsACString**)d) = rs;
                }
                else
                {
                    nsACString* rs = *((nsACString**)d);
                    rs->Truncate();
                    rs->SetIsVoid(PR_TRUE);
                }
***/
                return JS_TRUE;
            }

            // The JS val is neither null nor void...

            if(!(str = JS_ValueToString(cx, s)) ||
               !(chars = JS_GetStringBytes(str)))
            {
                return JS_FALSE;
            }

            length = JS_GetStringLength(str);

            if(use_allocator)
            {
                const nsACString *rs = new nsCString(chars, length);

                if(!rs)
                    return JS_FALSE;

                *((const nsACString**)d) = rs;
            }
            else
            {
                nsACString* rs = *((nsACString**)d);

                rs->Assign(nsDependentCString(chars, length));
            }
            return JS_TRUE;
        }

        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
        {
            JSObject *obj;
            NS_ASSERTION(iid, "can't do interface conversions without iid");

            if(iid->Equals(NS_GET_IID(nsIVariant)))
            {
              // INFORMATION: newVariant() saves the jsval as a member, calls JS_AddNamedRootRT on the jsval (for garbage collection?), and extracts the jsval into a DiscriminatedUnion (handles the non-IS_OBJECT types, then ID, Array, and finally does a WrapJS to ISupports)
              *((jsval*)d) = s; // (IS THIS A BAD HACK??) pass-on the jsval unchanged
              //assert(false); // NOT YET IMPLEMENTED!
/***
                XPCVariant* variant = XPCVariant::newVariant(cx, s);
                if(!variant)
                    return JS_FALSE;
                *((nsISupports**)d) = NS_STATIC_CAST(nsIVariant*, variant);
***/
                return JS_TRUE;
            }
            //else ...

            if(JSVAL_IS_VOID(s) || JSVAL_IS_NULL(s))
            {
                if(type_info.IsReference())
                {
                    if(error_out)
                      //*error_out = NS_ERROR_XPC_BAD_CONVERT_JS_NULL_REF;
                      *error_out = NS_ERROR_FAILURE;
                    return JS_FALSE;
                }
                // else ...
                *((nsISupports**)d) = nsnull;
                return JS_TRUE;
            }

            // only wrap JSObjects
            if(!JSVAL_IS_OBJECT(s) || !(obj = JSVAL_TO_OBJECT(s)))
            {
                if(error_out && JSVAL_IS_INT(s) && 0 == JSVAL_TO_INT(s))
                    //*error_out = NS_ERROR_XPC_BAD_CONVERT_JS_ZERO_ISNOT_NULL;
                    *error_out = NS_ERROR_FAILURE;
                return JS_FALSE;
            }

            *((jsval*)d) = s; return JS_TRUE; // (IS THIS A BAD HACK??) pass-on the jsval unchanged
            //assert(false); // NOT YET IMPLEMENTED!
/***
            return JSObject2NativeInterface(cx, (void**)d, obj, iid,
                                            nsnull, error_out);
***/
        }
        default:
            NS_ASSERTION(0, "bad type");
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}



static JSBool GetInterfaceTypeFromParam(//JSContext *cx,
                                        nsIInterfaceInfo *iface_info,
                                        const nsXPTMethodInfo *function_info,
                                        const nsXPTParamInfo &param_info,
                                        uint16 vtable_index,
                                        uint8 param_index,
                                        //const nsXPTType &type_info,
                                        uint8 type_tag,
                                        nsXPTCVariant *param_array,
                                        nsIID* result)
{
    nsresult nr;

    // XXX fixup the various exceptions that are thrown

    if(type_tag == nsXPTType::T_INTERFACE)
    {
        nr = iface_info->GetIIDForParamNoAlloc(vtable_index, &param_info, result);
        if(NS_FAILED(nr))
          return JS_FALSE; //ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, paramIndex, cx);
    }
    else if(type_tag == nsXPTType::T_INTERFACE_IS)
    {
      assert(false); // REMOVED FOR SIMPLICITY (OKAY TO UNCOMMENT IF WE FIND IT'S NEEDED)
/***
        uint8 arg_num;
        nr = iface_info->GetInterfaceIsArgNumberForParam(vtable_index, &param_info, &arg_num);
        if(NS_FAILED(nr))
            return JS_FALSE; //Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, cx);

        const nsXPTParamInfo &arg_param = function_info->GetParam(arg_num);
        const nsXPTType &arg_type = arg_param.GetType();

        // The xpidl compiler ensures this. We reaffirm it for safety.
        if(!arg_type.IsPointer() || arg_type.TagPart() != nsXPTType::T_IID)
            return JS_FALSE; //ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, paramIndex, cx);

        nsIID *p = (nsIID*) param_array[arg_num].val.p;
        if(!p)
            return JS_FALSE; //ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, paramIndex, cx);
        *result = *p;
***/
    }
    return JS_TRUE;
}
