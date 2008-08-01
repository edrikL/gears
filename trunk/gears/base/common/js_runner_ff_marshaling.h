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
// Handles interaction between JavaScript and C++ in Firefox. You can think
// of this as the equivalent of XPConnect, except that Gears C++ modules are
// not XPCOM objects, but instead have a DispatcherInterface instance.

#ifndef GEARS_BASE_COMMON_JS_RUNNER_FF_MARSHALING_H__
#define GEARS_BASE_COMMON_JS_RUNNER_FF_MARSHALING_H__

#include <map>
#include <set>
#include <vector>

#include <gecko_internal/jsapi.h>
#include "gears/base/common/dispatcher.h"
#include "gears/base/common/leak_counter.h"
#include "third_party/linked_ptr/linked_ptr.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

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
  scoped_ptr<std::string>    alloc_name;

  JsWrapperDataForProto() : JsWrapperData(PROTO_JSOBJECT) {
    LEAK_COUNTER_INCREMENT(JsWrapperDataForProto);
  }
  ~JsWrapperDataForProto() {
    LEAK_COUNTER_DECREMENT(JsWrapperDataForProto);
  }
};

struct JsWrapperDataForInstance : public JsWrapperData {
  JSObject            *jsobject;
  JsContextWrapper    *js_wrapper;
  ModuleImplBaseClass *module;

  JsWrapperDataForInstance() : JsWrapperData(INSTANCE_JSOBJECT) {
    LEAK_COUNTER_INCREMENT(JsWrapperDataForInstance);
  }
  ~JsWrapperDataForInstance() {
    LEAK_COUNTER_DECREMENT(JsWrapperDataForInstance);
  }
};

struct JsWrapperDataForFunction : public JsWrapperData {
  scoped_ptr<JsRootedToken>  function_root;
  DispatchId                 dispatch_id;
  int                        flags;

  JsWrapperDataForFunction() : JsWrapperData(FUNCTION_JSOBJECT) {
    LEAK_COUNTER_INCREMENT(JsWrapperDataForFunction);
  }
  ~JsWrapperDataForFunction() {
    LEAK_COUNTER_DECREMENT(JsWrapperDataForFunction);
  }
};

class JsContextWrapper {
 public:
  JsContextWrapper(JSContext *cx, JSObject *global_obj);
  ~JsContextWrapper();
  void CleanupRoots();

  // Creates a new JavaScript object to represent a Gears module in the JS
  // engine.
  bool CreateJsTokenForModule(ModuleImplBaseClass *module, JsToken *token_out);

  ModuleImplBaseClass *GetModuleFromJsToken(const JsToken token);

 private:
  // Initializes a new class with the given name.
  bool InitClass(const char *class_name, JsWrapperDataForProto *proto_data,
                 JSObject **proto_obj);

  // Populate a JS prototype based on all the methods in a C++ class.
  bool AddAllFunctionsToPrototype(JSObject *proto_obj,
                                  DispatcherInterface *dispatcher);
  bool AddFunctionToPrototype(JSObject *proto_obj, const char *name,
                              bool is_getter, bool is_setter,
                              DispatchId dispatch_id);

  // Initializes the custom wrapper data for an instance.
  bool SetupInstanceObject(JSObject *instance_obj, ModuleImplBaseClass *module);


  static JSBool JsWrapperCaller(JSContext *cx, JSObject *obj,
                                uintN argc, jsval *argv, jsval *retval);

  JSContext *cx_;
  JSObject  *global_obj_;

  friend void FinalizeNative(JSContext *cx, JSObject *obj);

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
};


#endif // GEARS_BASE_COMMON_JS_RUNNER_FF_MARSHALING_H__
