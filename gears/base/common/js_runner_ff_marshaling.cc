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
// Architecture Summary:
// * To add a class, we create a 'prototype' JSObject, then attach a 'function'
//   JSObject for each class function (methods, and property getters/setters).
// * We use the same code (JsWrapperCaller) for all functions.  This code knows
//   how to convert params and return values between JavaScript and the
//   DispatcherInterface instance.
//
// Additional info:
// * 'jsval' is the wrapper the Firefox JS engine uses for all args and retvals.
// * A jsval can contain a JSObject*, an integer, or a number of other types.
// * A JSObject can represent an instance, a function, a class prototype, etc.
//
// TODO(cprince): figure out how to cleanup allocated JsWrapperDataFor* structs
// when the JSContext goes away.  Maybe can use things like Finalize properties?
//
// Similarly, where do we destroy:
// * New objects returned by Gears classes (For example, the ResultSet objects
//   returned by Database::Execute.)
// * JS_NewObject (and similar) return values, on Define*() failure
//   (Or maybe rely on JSContext cleanup -- at a higher level -- to handle it.)

#include "gears/base/common/js_runner_ff_marshaling.h"

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/firefox/module_wrapper.h"


// The "reserved slot" in which we store our custom data for functions.
// (Non-function custom data can be stored using JS_SetPrivate.)
static const int kFunctionDataReservedSlotIndex = 0;

// Magic constants the JavaScript engine uses with the argv array.
static const int kArgvFunctionIndex = -2; // negative indices [sic]
static const int kArgvInstanceIndex = -1;


// Called when a JS object based on native code is cleaned up.  We need to
// remove any reference counters it held here.
void FinalizeNative(JSContext *cx, JSObject *obj) {
  JsWrapperData *p = reinterpret_cast<JsWrapperData *>(JS_GetPrivate(cx, obj));
  if (!p)
    return;
  switch(p->header.type) {
    case PROTO_JSOBJECT:
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

    if (!InitClass(module_name.c_str(), proto_data_alloc.get(), &proto))
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

  if (!SetupInstanceObject(instance_obj, module))
    return false;

  *object_out = OBJECT_TO_JSVAL(instance_obj);
  return true;
}


bool JsContextWrapper::InitClass(const char *class_name,
                                 JsWrapperDataForProto *proto_data,
                                 JSObject **proto_obj) {
  assert(class_name);
  // TODO(nigeltao): Do we really need class_name_copy (and hence alloc_name)?
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
                                  dispatch_id))
        return false;
    }

    if (has_setter) {
      if (!AddFunctionToPrototype(proto_obj,
                                  members->first.c_str(), // member name
                                  false, true, // is_getter, is_setter
                                  dispatch_id))
        return false;
    }

    // If there is no getter or setter, this member must be a method.
    if (!has_getter && !has_setter) {
      if (!AddFunctionToPrototype(proto_obj,
                                  members->first.c_str(), // member name
                                  false, false, // is_getter, is_setter
                                  dispatch_id))
        return false;
    }
  }

  return true;
}


bool JsContextWrapper::AddFunctionToPrototype(
                           JSObject *proto_obj, const char *name,
                           bool is_getter, bool is_setter,
                           DispatchId dispatch_id) {
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
                                        0,
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
                                           ModuleImplBaseClass *module) {
  // setup the JsWrapperDataForInstance struct
  scoped_ptr<JsWrapperDataForInstance> instance_data(
      new JsWrapperDataForInstance);

  // Keep a pointer to the context wrapper so that we can access it in the
  // static finalize function.
  instance_data->js_wrapper = this;
  instance_data->jsobject = instance_obj;
  instance_data->module = module;

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
  assert(instance_data->module);
  assert(function_data->dispatch_id);

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

  return !call_context.is_exception_set() ? JS_TRUE : JS_FALSE;
}
