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

#ifndef GEARS_WORKERPOOL_FIREFOX_JS_WRAPPER_H__
#define GEARS_WORKERPOOL_FIREFOX_JS_WRAPPER_H__

#include <map>
#include <nsCOMPtr.h>
#include "gears/third_party/gecko_internal/jsapi.h"

class IIDLessThanFunctor {
 public:
  bool operator()(const nsIID &x, const nsIID &y) const {
    // Mozilla also treats nsIID as 16 contiguous bytes.
    return memcmp(&x, &y, 16) < 0;
  }
};
typedef std::map<nsIID, JSObject*, IIDLessThanFunctor> IIDToProtoMap;

struct JsWrapperDataForProto;
class JsContextWrapper {
 public:
  JsContextWrapper(JSContext *cx, JSObject *global_obj)
      : cx_(cx),
        global_obj_(global_obj) {
  }

  // Defines a class prototype in the JS context.  This must be called for
  // every class that will be defined as a global (see below) _OR_ returned
  // by another class.
  //
  // * contract_id and class_name can be NULL, in which case you can still 
  //   return objects of this type, but JS code cannot use "new CLASSNAME".
  // * proto_obj_out can be NULL; if it's not, value is undefined on failure.
  bool DefineClass(const nsIID *iface_id,
                   const nsCID *class_id,
                   const char *class_name,
                   JSObject **proto_obj_out);

  // Defines a named object of the given type in the JS global namespace.
  bool DefineGlobal(JSObject *proto_obj,
                    nsISupports *instance_isupports,
                    const char16 *instance_name);

 private:
  bool AddFunctionsToPrototype(JSObject *proto_obj,
                               JsWrapperDataForProto *proto_data);

  static JSBool JsWrapperCaller(JSContext *cx, JSObject *obj,
                                uintN argc, jsval *argv, jsval *retval);
  static JSBool JsWrapperConstructor(JSContext *cx, JSObject *obj,
                                     uintN argc, jsval *argv, jsval *retval);

  JSContext *cx_;
  JSObject  *global_obj_;

  // A centralized mapping from IID to proto (per JS context), which we need
  // for marshaling pointer return values when Object1 returns an Object2.
  friend JSBool NativeData2JS(JSContext *cx, JSObject *scope_obj,
                            jsval *d, const void *s,
                            const nsXPTType &type_info, const nsIID *iid,
                            JsContextWrapper *js_wrapper,
                            nsresult *error_out);
  IIDToProtoMap iid_to_proto_map_;
};


#endif // GEARS_WORKERPOOL_FIREFOX_JS_WRAPPER_H__
