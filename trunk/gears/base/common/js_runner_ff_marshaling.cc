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
// But XPConnect is not thread-safe, relies on the DOM, etc.  So for worker
// threads we need to implement our own layer.
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
// * New objects returned by Gears classes (For example, the ResultSet objects
//   returned by Database::Execute.)
// * JS_NewObject (and similar) return values, on Define*() failure
//   (Or maybe rely on JSContext cleanup -- at a higher level -- to handle it.)


#include <gecko_sdk/include/nsComponentManagerUtils.h>
#include <gecko_sdk/include/nsMemory.h> // for use in JSData2Native
#include <gecko_internal/nsIInterfaceInfoManager.h>
#include <gecko_internal/nsIVariant.h> // for use in JSData2Native
// TODO(cprince): see if we can remove nsIVariant.h/nsMemory.h after cleanup.
#include <gecko_internal/xptinfo.h>

#include "genfiles/base_interface_ff.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/js_runner_ff_marshaling.h"
#include "gears/base/firefox/module_wrapper.h"
#include "gears/base/firefox/xpcom_dynamic_load.h"


// The "reserved slot" in which we store our custom data for functions.
// (Non-function custom data can be stored using JS_SetPrivate.)
static const int kFunctionDataReservedSlotIndex = 0;

// Magic constants the JavaScript engine uses with the argv array.
static const int kArgvFunctionIndex = -2; // negative indices [sic]
static const int kArgvInstanceIndex = -1;

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
            
        if (instance->module) {
          ModuleWrapper *module_wrapper =
              static_cast<ModuleWrapper *>(instance->module->GetWrapper());
          module_wrapper->Destroy();
        }

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
}

void JsContextWrapper::CleanupRoots() {
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

bool JsContextWrapper::CreateModuleJsObject(ModuleImplBaseClass *module,
                                            JsToken *object_out) {
  // We require the name property to be set since we use it as the key for
  // caching created prototype objects.
  const std::string &module_name = module->get_module_name();

  JSObject *proto = NULL;
  JSClass *js_class = NULL;
  JsWrapperDataForProto *proto_data = NULL;

  // Get the JSClass for this type of Module, or else create one if we've
  // never seen this class before.
  NameToProtoMap::iterator iter = name_to_proto_map_.find(module_name);
  if (iter != name_to_proto_map_.end()) {
    proto = iter->second;
    proto_data = static_cast<JsWrapperDataForProto*>(JS_GetPrivate(cx_, proto));
    js_class = proto_data->alloc_jsclass.get();
  } else {
    scoped_ptr<JsWrapperDataForProto> proto_data_alloc(
        new JsWrapperDataForProto);

    if (!InitClass(module_name.c_str(), proto_data_alloc.get(),
                   NULL, NULL, &proto))
      return false;

    proto_data = proto_data_alloc.release();
    js_class = proto_data->alloc_jsclass.get();

    if (!AddAllFunctionsToPrototype(proto,
                                    module->GetWrapper()->GetDispatcher())) {
      return false;
    }

    // save values
    name_to_proto_map_[module_name] = proto;
    proto_wrappers_.push_back(linked_ptr<JsWrapperDataForProto>(proto_data));
    dispatcher_classes_.insert(js_class);

    // succeeded; save the pointer
    JS_SetPrivate(cx_, proto, proto_data);
  }

  JS_BeginRequest(cx_);
  JSObject *instance_obj = JS_NewObject(cx_, js_class, proto, global_obj_);
  JS_EndRequest(cx_);
  if (!instance_obj) return false;

  if (!SetupInstanceObject(instance_obj, module, NULL))
    return false;

  *object_out = OBJECT_TO_JSVAL(instance_obj);
  return true;
}


bool JsContextWrapper::DefineClass(const nsIID *iface_id,
                                   const nsCID *class_id,
                                   const char *class_name,
                                   JSObject **proto_obj_out) {
  nsresult nr;

  // For class_name and class_id, define both or neither.
  // These values should be defined iff "new CLASSNAME" should work.
  assert((class_name && class_id) || (!class_name && !class_id));

  scoped_ptr<JsWrapperDataForProto> proto_data(new JsWrapperDataForProto);
  JSObject *proto_obj;

  if (!InitClass(class_name ? class_name : "", proto_data.get(), iface_id,
                 class_id, &proto_obj)) {
      return false;
  }

  // nsIInterfaceInfo for this IID lets us lookup properties later
  nsCOMPtr<nsIInterfaceInfoManager> iface_info_manager;
  iface_info_manager = do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID,
                                     &nr);
  if (NS_FAILED(nr)) { return false; }
  nr = iface_info_manager->GetInfoForIID(iface_id,
                                         getter_AddRefs(proto_data->iface_info));
  if (NS_FAILED(nr)) { return false; }

  bool succeeded = AddAllFunctionsToPrototype(proto_obj, proto_data.get());
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
  JsRequest request(cx_);
  JSObject *instance_obj = JS_NewObject(cx_,
                                        proto_data->alloc_jsclass.get(),
                                        proto_obj,
                                        global_obj_); // parent
  if (!instance_obj) { return false; }

  global_roots_.push_back(linked_ptr<JsRootedToken>(
                            new JsRootedToken(cx_,
                                              OBJECT_TO_JSVAL(instance_obj))));

  bool succeeded = SetupInstanceObject(instance_obj, NULL, instance_isupports);
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


bool JsContextWrapper::InitClass(const char *class_name,
                                 JsWrapperDataForProto *proto_data,
                                 const nsIID *iface_id, const nsIID *class_id,
                                 JSObject **proto_obj) {
  assert(class_name);
  scoped_ptr<std::string> class_name_copy(new std::string(class_name));

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
  JS_BeginRequest(cx_);
  *proto_obj = JS_InitClass(cx_, global_obj_,
                            NULL, // parent_proto
                            alloc_jsclass.get(), // JSClass *
                            NULL, // constructor
                            0, // ctor_num_args
                            NULL, NULL,  //   prototype   props/funcs
                            NULL, NULL); // "constructor" props/funcs
  JS_EndRequest(cx_);
  if (!*proto_obj) { return false; }

  // setup the JsWrapperDataForProto struct
  proto_data->jsobject = *proto_obj;
  proto_data->js_wrapper = this;
  proto_data->alloc_name.swap(class_name_copy); // take ownership
  proto_data->alloc_jsclass.swap(alloc_jsclass); // take ownership
  proto_data->proto_root.reset(new JsRootedToken(cx_,
                                                 OBJECT_TO_JSVAL(*proto_obj)));

  if (iface_id) {
    proto_data->iface_id = *iface_id;
  }

  // nsIFactory for this class lets us create instances later
  if (!class_id) {
    proto_data->factory = NULL;
  } else {
    proto_data->factory = do_GetClassObject(*class_id);
    if (!proto_data->factory) { return false; }
  }

  return true;
}


bool JsContextWrapper::AddAllFunctionsToPrototype(
                           JSObject *proto_obj,
                           DispatcherInterface *dispatcher) {
  DispatcherNameList::const_iterator members =
      dispatcher->GetMemberNames().begin();
  for (; members != dispatcher->GetMemberNames().end(); ++members) {
    DispatchId dispatch_id = members->second;
    bool has_getter = dispatcher->HasPropertyGetter(dispatch_id);
    bool has_setter = dispatcher->HasPropertySetter(dispatch_id);

    if (has_getter) {
      if (!AddFunctionToPrototype(proto_obj,
                                  members->first.c_str(), // member name
                                  true, false, // is_getter, is_setter
                                  dispatch_id,
                                  // remaining params not used
                                  NULL, NULL, NULL, 0, 0))
        return false;
    }

    if (has_setter) {
      if (!AddFunctionToPrototype(proto_obj,
                                  members->first.c_str(), // member name
                                  false, true, // is_getter, is_setter
                                  dispatch_id,
                                  // remaining params not used
                                  NULL, NULL, NULL, 0, 0))
        return false;
    }

    // If there is no getter or setter, this member must be a method.
    if (!has_getter && !has_setter) {
      if (!AddFunctionToPrototype(proto_obj,
                                  members->first.c_str(), // member name
                                  false, false, // is_getter, is_setter
                                  dispatch_id,
                                  // remaining params not used
                                  NULL, NULL, NULL, 0, 0))
        return false;
    }
  }

  return true;
}


// Helper function for enumerating methods and property getters/setters of a
// XPCOM object, and exposing those functions to the JavaScript engine.
//
// [Reference: this is inspired by Mozilla's DefinePropertyIfFound() in
// /xpconnect/src/xpcwrappednativejsops.cpp.]
bool JsContextWrapper::AddAllFunctionsToPrototype(
                           JSObject *proto_obj,
                           JsWrapperDataForProto *proto_data) {
  nsresult nr;
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

    if (!AddFunctionToPrototype(proto_obj, function_info->GetName(),
                                function_info->IsGetter() == PR_TRUE,
                                function_info->IsSetter() == PR_TRUE,
                                NULL, // dispatch_id, not used for isupports
                                proto_data,  function_info, iface_info,
                                i, // vtable_index
                                function_info->GetParamCount())) {
      return false;
    }
  }

  return true;
}


bool JsContextWrapper::AddFunctionToPrototype(
                           JSObject *proto_obj, const char *name,
                           bool is_getter, bool is_setter,
                           DispatchId dispatch_id,
                           JsWrapperDataForProto *proto_data,
                           const nsXPTMethodInfo *function_info,
                           nsIInterfaceInfo *iface_info,
                           int vtable_index, int param_count) {
  // Create a JSFunction object for the property getter/setter or method.
  int newfunction_flags = 0;
  if (is_getter) {
    newfunction_flags = JSFUN_GETTER;
  } else if (is_setter) {
    newfunction_flags = JSFUN_SETTER;
  }
  
  JS_BeginRequest(cx_);
  JSFunction *function = JS_NewFunction(cx_,
                                        JsContextWrapper::JsWrapperCaller,
                                        param_count,
                                        newfunction_flags,
                                        proto_obj, // parent
                                        name);
  JS_EndRequest(cx_);
  JSObject *function_obj = JS_GetFunctionObject(function);

  // Save info about the function.
  scoped_ptr<JsWrapperDataForFunction> function_data(
                                           new JsWrapperDataForFunction);
  function_data->dispatch_id = dispatch_id;
  function_data->flags = newfunction_flags;
  function_data->name = name; // copy
  function_data->function_info = function_info;
  function_data->iface_info = iface_info;
  function_data->vtable_index = vtable_index;
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

  if (is_getter) {
    getter = (JSPropertyOp) function_obj;
    method = OBJECT_TO_JSVAL(NULL);
    function_flags = (JSPROP_GETTER | JSPROP_SHARED);
    // TODO(cprince): need JSPROP_READONLY for no-setter attributes?
  } else if (is_setter) {
    setter = (JSPropertyOp) function_obj;
    method = OBJECT_TO_JSVAL(NULL);
    function_flags = (JSPROP_SETTER | JSPROP_SHARED);
  }

  // Note: JS_DefineProperty is written to handle adding a setter to a
  // previously defined getter with the same name.
  JS_BeginRequest(cx_);
  JSBool js_ok = JS_DefineProperty(cx_, proto_obj, name,
                                   method, // method
                                   getter, setter, // getter, setter
                                   function_flags);
  JS_EndRequest(cx_);
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
  JS_BeginRequest(cx_);
  JS_SetReservedSlot(cx_, function_obj, kFunctionDataReservedSlotIndex,
                     pointer_jsval);
  JS_EndRequest(cx_);

  return true;
}



bool JsContextWrapper::SetupInstanceObject(JSObject *instance_obj,
                                           ModuleImplBaseClass *module,
                                           nsISupports *isupports) {
  assert(isupports || module);
  assert(!(isupports && module)); // where art thou xor?

  // setup the JsWrapperDataForInstance struct
  scoped_ptr<JsWrapperDataForInstance> instance_data(
      new JsWrapperDataForInstance);

  // Keep a pointer to the context wrapper so that we can access it in the
  // static finalize function.
  instance_data->js_wrapper = this;
  instance_data->jsobject = instance_obj;
  instance_data->module = module;
  instance_data->isupports = isupports;

  instance_wrappers_[instance_data.get()] =
      linked_ptr<JsWrapperDataForInstance>(instance_data.get());

  // succeeded; prevent scoped cleanup of allocations, and save the pointer
  JS_SetPrivate(cx_, instance_obj, instance_data.release());
  return true;
}


ModuleImplBaseClass *JsContextWrapper::GetModuleFromJsToken(
    const JsToken token) {
  // First, check that the JsToken represents a (JavaScript) object, and not
  // a JS int, for example.
  if (!JSVAL_IS_OBJECT(token)) {
    return NULL;
  }
  JSObject *obj = JSVAL_TO_OBJECT(token);

  // Next, check that the JSObject is for a Dispatcher-based module, and not
  // any other type of JSObject.  To do that, we get its JSClass, and check
  // that against those JSClasses we have previously seen (in this
  // JsContextWrapper) for Dispatcher-based modules.
  JSClass *js_class = JS_GET_CLASS(cx_, obj);
  if (dispatcher_classes_.find(js_class) == dispatcher_classes_.end()) {
    return NULL;
  }

  // Now that we know that we have a Dispatcher-based module, we know that
  // the JS private data is in fact a JsWrapperDataForInstance*, which we can
  // crack open for its ModuleImplBaseClass*.
  JsWrapperDataForInstance *instance_data =
      reinterpret_cast<JsWrapperDataForInstance*>(JS_GetPrivate(cx_, obj));
  return instance_data->module;
}


// General-purpose wrapper to invoke any class function (method, or
// property getter/setter).
//
// All calls to Gears C++ objects from a worker thread will go through
// this function.
//
// [Reference: this is inspired by Mozilla's XPCWrappedNative::CallMethod() in
// /xpconnect/src/xpcwrappednative.cpp. Also: XPC_WN_(CallMethod|GetterSetter).]
JSBool JsContextWrapper::JsWrapperCaller(JSContext *cx, JSObject *obj,
                                         uintN argc, jsval *argv,
                                         jsval *js_retval) {
  // Gather data regarding the function and instance being called.
  JSObject *function_obj = JSVAL_TO_OBJECT(argv[kArgvFunctionIndex]);
  assert(function_obj);

  JSObject *instance_obj = JSVAL_TO_OBJECT(argv[kArgvInstanceIndex]);
  assert(instance_obj);


  JsWrapperDataForFunction *function_data;
  jsval function_data_jsval;
  JS_BeginRequest(cx);
  JS_GetReservedSlot(cx, function_obj,
                     kFunctionDataReservedSlotIndex,
                     &function_data_jsval);
  JS_EndRequest(cx);
  function_data = static_cast<JsWrapperDataForFunction *>(
                      JSVAL_TO_PRIVATE(function_data_jsval));
  assert(function_data);
  assert(function_data->header.type == FUNCTION_JSOBJECT);

  JsWrapperDataForInstance *instance_data =
      static_cast<JsWrapperDataForInstance*>(JS_GetPrivate(cx, instance_obj));
  assert(instance_data);
  assert(instance_data->header.type == INSTANCE_JSOBJECT);

  // The presence of dispatch_id indicates this is a method call on a
  // dispatcher-based module.
  if (function_data->dispatch_id) {
    assert(instance_data->module);

    ModuleWrapperBaseClass *module_wrapper =
        instance_data->module->GetWrapper();
    JsCallContext call_context(cx, instance_data->module->GetJsRunner(),
                               argc, argv, js_retval);

    if (function_data->flags == JSFUN_GETTER) {
      if (!module_wrapper->GetDispatcher()->GetProperty(
                                    function_data->dispatch_id,
                                    &call_context)) {
        call_context.SetException(
            STRING16(L"Property not found or not getter."));
        return JS_FALSE;
      }
    } else if (function_data->flags == JSFUN_SETTER) {
      if (!module_wrapper->GetDispatcher()->SetProperty(
                                    function_data->dispatch_id,
                                    &call_context)) {
        call_context.SetException(
            STRING16(L"Property not found or not setter."));
        return JS_FALSE;
      }
    } else {
      if (!module_wrapper->GetDispatcher()->CallMethod(
                                    function_data->dispatch_id,
                                    &call_context)) {
        call_context.SetException(
            STRING16(L"Method not found."));
        return JS_FALSE;
      }
    }
 
    // NOTE: early return for dispatcher-based modules.
    return !call_context.is_exception_set() ? JS_TRUE : JS_FALSE;
  }

  // Otherwise, this is a method call on an ISupports-based module. However,
  // as of July 2008, there are no NS_ISUPPORTS modules any more.
  // TODO(nigeltao): clean up this file.
  assert(false);
  return JS_FALSE;
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
            JS_BeginRequest(cx);
            JSBool success = JS_NewNumberValue(cx, xpctvar.val.d, jsval_out);
            JS_EndRequest(cx);
            return success;
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
            JS_BeginRequest(cx);
            JSObject* array = JS_NewArrayObject(cx, 0, nsnull);
            JS_EndRequest(cx);
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
  JsRequest request(cx);

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

            JS_BeginRequest(cx);
            JSString* str = JS_NewStringCopyN(cx, p, 1);
            JS_EndRequest(cx);
            if(!str)
                return JS_FALSE;
            *d = STRING_TO_JSVAL(str);
            break;
        }
      case nsXPTType::T_WCHAR :
        {
            jschar* p = (jschar*)s;
            if(!p)
                return JS_FALSE;
            JS_BeginRequest(cx);
            JSString* str = JS_NewUCStringCopyN(cx, p, 1);
            JS_EndRequest(cx);
            if(!str)
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
              JS_BeginRequest(cx);
              JSObject *out_instance_obj = JS_NewObject(cx,
                                                        proto_data->alloc_jsclass.get(),
                                                        out_proto_obj,
                                                        scope_obj); // parent
              JS_EndRequest(cx);
              if (!out_instance_obj) {
                assert(false); // NOT YET IMPLEMENTED??
                //return <FAILURE>
              }

              // TODO: cleanup JSObject if code BELOW fails?

              bool succeeded = js_wrapper->SetupInstanceObject(out_instance_obj,
                                                               NULL, isupports);
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
            JS_BeginRequest(cx);
            JSString* str = JS_NewStringCopyN(cx, p, count);
            JS_EndRequest(cx);
            if(!str)
                return JS_FALSE;
            *dest = STRING_TO_JSVAL(str);
            break;
        }
        case nsXPTType::T_ASTRING: // added by Gears
        case nsXPTType::T_DOMSTRING: // added by Gears
        case nsXPTType::T_PWSTRING_SIZE_IS:
        {
            jschar* p = *((jschar**)src);
            if(!p)
                break;
            JS_BeginRequest(cx);
            JSString* str = JS_NewUCStringCopyN(cx, p, count);
            JS_EndRequest(cx);
            if(!str)
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
