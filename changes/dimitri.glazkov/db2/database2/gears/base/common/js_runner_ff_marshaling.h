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
// Functions for defining XPCOM class prototypes and global variables inside
// a Firefox JavaScript context.
//
// Subsequent interaction between JavaScript and XPCOM C++ objects is
// handled automatically by this wrapper.
//
// TODO(aa): Remove all code marked TODO_REMOVE_NSISUPPORTS when we remove
// support for nsISupports-based modules.

#ifndef GEARS_WORKERPOOL_FIREFOX_JS_WRAPPER_H__
#define GEARS_WORKERPOOL_FIREFOX_JS_WRAPPER_H__

#include <map>
#include <set>
#include <vector>

#include <gecko_sdk/include/nsCOMPtr.h>
#include <gecko_internal/jsapi.h>
#include "gears/base/common/dispatcher.h"
#include "gears/third_party/linked_ptr/linked_ptr.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// TODO_REMOVE_NSISUPPORTS: The next two definitions.
class IIDLessThanFunctor {
 public:
  bool operator()(const nsIID &x, const nsIID &y) const {
    // Mozilla also treats nsIID as 16 contiguous bytes.
    return memcmp(&x, &y, 16) < 0;
  }
};
typedef std::map<nsIID, JSObject*, IIDLessThanFunctor> IIDToProtoMap;
typedef std::map<const std::string, JSObject*> NameToProtoMap;

class JsContextWrapper;

//
// Structs that record different custom data for the different JSObject types.
// The structs have a common header so we can differentiate generic JSObjects.
//

enum JsWrapperDataType {
  PROTO_JSOBJECT,
  INSTANCE_JSOBJECT,
  FUNCTION_JSOBJECT,
  UNKNOWN_JSOBJECT // do not use
};

// These structures are saved as the private data on JSObjects.
// By sharing a common base class, we can cast the raw pointer
// stored on the JSObject to a JsWrapperDataHeader to determine the
// JsWrapperData type.
struct JsWrapperDataHeader {
  JsWrapperDataType type;

  explicit JsWrapperDataHeader(JsWrapperDataType t) : type(t) {}
};

struct JsWrapperData {
  const JsWrapperDataHeader  header;
  JsWrapperData(JsWrapperDataType t) : header(t) {};
};

struct JsWrapperDataForProto : public JsWrapperData {
  JSObject                   *jsobject;
  JsContextWrapper           *js_wrapper;
  scoped_ptr<JsRootedToken>  proto_root;
  scoped_ptr<JSClass>        alloc_jsclass;

  // These fields are used for isupports modules
  // TODO_REMOVE_NSISUPPORTS: All the rest of the fields.
  scoped_ptr<std::string>    alloc_name;
  nsIID                      iface_id;
  nsCOMPtr<nsIInterfaceInfo> iface_info;
  nsCOMPtr<nsIFactory>       factory;

  JsWrapperDataForProto() : JsWrapperData(PROTO_JSOBJECT) {}
};

struct JsWrapperDataForInstance : public JsWrapperData {
  JSObject          *jsobject;
  JsContextWrapper  *js_wrapper;

  // This field is used for dispatcher modules.
  ModuleImplBaseClass *module;

  // This field is used for isupports modules.
  // TODO_REMOVE_NSISUPPORTS: All the rest of the fields.
  nsCOMPtr<nsISupports> isupports;

  JsWrapperDataForInstance() : JsWrapperData(INSTANCE_JSOBJECT) {}
};

struct JsWrapperDataForFunction : public JsWrapperData {
  scoped_ptr<JsRootedToken>  function_root;

  // This field is used for dispatcher modules.
  DispatchId                 dispatch_id;
  int                        flags;

  // These fields is used for isupports modules.
  // TODO_REMOVE_NSISUPPORTS: All the rest of the fields.
  JsWrapperDataForProto      *proto_data; // parent proto (purposely not scoped)
  std::string                name;
  const nsXPTMethodInfo      *function_info; // NOT a refcounted type
  nsCOMPtr<nsIInterfaceInfo> iface_info;
  int                        vtable_index;

  JsWrapperDataForFunction() : JsWrapperData(FUNCTION_JSOBJECT) {}
};

class JsContextWrapper {
 public:
  JsContextWrapper(JSContext *cx, JSObject *global_obj);
  void CleanupRoots();

  // Defines a class prototype in the JS context based on an XPCOM interface.
  // This must be called for every XPCOM-based Gears class that will be defined
  // as a global (see below) _OR_ returned by another class.
  //
  // * contract_id and class_name can be NULL, in which case you can still 
  //   return objects of this type, but JS code cannot use "new CLASSNAME".
  // * proto_obj_out can be NULL; if it's not, value is undefined on failure.
  //
  // TODO_REMOVE_NSISUPPORTS: Remove this method.
  bool DefineClass(const nsIID *iface_id,
                   const nsCID *class_id,
                   const char *class_name,
                   JSObject **proto_obj_out);

  // Defines a named object of the given type in the JS global namespace.
  // TODO_REMOVE_NSISUPPORTS: This can be implemented with InitObjectForModule()
  // and JsObject::SetPropertyObject().
  bool DefineGlobal(JSObject *proto_obj,
                    nsISupports *instance_isupports,
                    const std::string16 &instance_name);

  // Creates a new JavaScript object to represent a Gears module in the JS
  // engine.
  bool CreateModuleJsObject(ModuleImplBaseClass *module, JsToken *object_out);

  ModuleImplBaseClass *GetModuleFromJsToken(const JsToken token);

 private:
  // Defines a class prototype in the JS context based on a ModuleImplBaseClass.
  bool DefineClass(ModuleImplBaseClass *module, JSObject **proto_obj_out);

  // Initializes a new class with the given name.
  // TODO_REMOVE_NSISUPPORTS: Remove the iface_id and class_id fields, which are
  // only needed for XPCOM support. Also probably makes sense to roll this into
  // DefineClass() at that time.
  bool InitClass(const char *class_name, JsWrapperDataForProto *proto_data,
                 const nsIID *iface_id, const nsIID *class_id,
                 JSObject **proto_obj);

  // Populate a JS prototype based on all the methods in a C++ class. The C++
  // class can be either dispatcher-based or isupports-based.
  bool AddAllFunctionsToPrototype(JSObject *proto_obj,
                                  DispatcherInterface *dispatcher);
  bool AddAllFunctionsToPrototype(JSObject *proto_obj,
                                  JsWrapperDataForProto *proto_data);
  // Helper function used by both methods above.
  bool AddFunctionToPrototype(JSObject *proto_obj, const char *name,
                              bool is_getter, bool is_setter,
                              DispatchId dispatch_id, // dispatcher modules only
                              // The remainder of the params are only used for
                              // isupports modules.
                              // TODO_REMOVE_NSISUPPORTS: remaining params
                              JsWrapperDataForProto *proto_data,
                              const nsXPTMethodInfo *function_info,
                              nsIInterfaceInfo *iface_info,
                              int vtable_index,
                              int param_count);

  // Shared code for initializing the custom wrapper data for an instance.
  // Pass *module for dispatcher modules and *isupports for isupports modules.
  // TODO_REMOVE_NSISUPPORTS: remove isupports.
  bool SetupInstanceObject(JSObject *instance_obj, ModuleImplBaseClass *module,
                           nsISupports *isupports);


  static JSBool JsWrapperCaller(JSContext *cx, JSObject *obj,
                                uintN argc, jsval *argv, jsval *retval);
  static JSBool JsWrapperConstructor(JSContext *cx, JSObject *obj,
                                     uintN argc, jsval *argv, jsval *retval);

  JSContext *cx_;
  JSObject  *global_obj_;

  friend JSBool NativeData2JS(JSContext *cx, JSObject *scope_obj,
                            jsval *d, const void *s,
                            const nsXPTType &type_info, const nsIID *iid,
                            JsContextWrapper *js_wrapper,
                            nsresult *error_out);
  friend void FinalizeNative(JSContext *cx, JSObject *obj);

  // A centralized mapping from IID to proto (per JS context), which we need
  // for marshaling pointer return values when Object1 returns an Object2.
  // We also have a mapping from class name to proto for new-style modules.
  // TODO_REMOVE_NSISUPPORTS: this field.
  IIDToProtoMap iid_to_proto_map_;

  // Map from module name to prototype. We use this so that we create only one
  // prototype for each module type.
  NameToProtoMap name_to_proto_map_;

  // The set of JSClass objects associated with each Dispatcher-based module
  // seen in CreateModuleJsObject.
  std::set<JSClass*> dispatcher_classes_;

  // This is stored as a map because we need to be able to selectively
  // remove elements when the associated JS object is finalized.
  std::map<JsWrapperDataForInstance *, linked_ptr<JsWrapperDataForInstance> >
      instance_wrappers_;
  std::vector<linked_ptr<JsWrapperDataForProto> > proto_wrappers_;
  std::vector<linked_ptr<JsWrapperDataForFunction> > function_wrappers_;
  std::vector<linked_ptr<JsRootedToken> > global_roots_;
};


#endif // GEARS_WORKERPOOL_FIREFOX_JS_WRAPPER_H__
