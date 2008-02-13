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

#ifndef GEARS_BASE_IE_MODULE_WRAPPER_H__
#define GEARS_BASE_IE_MODULE_WRAPPER_H__

#include <assert.h>
#include <stack>
#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h"

class ATL_NO_VTABLE ModuleWrapper 
    : public IDispatch,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<ModuleWrapper> {      
 public:
  BEGIN_COM_MAP(ModuleWrapper)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(ModuleWrapper)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  ModuleWrapper();
  ~ModuleWrapper();

  static JsCallContext *PeekJsCallContext();

  // IDispatch
  STDMETHOD(GetTypeInfoCount)(unsigned int FAR* retval);
  STDMETHOD(GetTypeInfo)(unsigned int index, LCID lcid,
                         ITypeInfo FAR* FAR* retval);
  STDMETHOD(GetIDsOfNames)(REFIID iid, OLECHAR FAR* FAR* names,
                           unsigned int num_names, LCID lcid, 
                           DISPID FAR* retval);
  STDMETHOD(Invoke)(DISPID member_id, REFIID iid, LCID lcid, WORD flags,
                    DISPPARAMS FAR* params, VARIANT FAR* retval,
                    EXCEPINFO FAR* exception,
                    unsigned int FAR* arg_error_index);

  void Init(IDispatch *impl) {
    assert(!impl_);
    assert(impl);
    impl_ = impl;
  }

 private:
  typedef std::stack<JsCallContext *> JsCallContextStack;

  static void PushJsCallContext(JsCallContext *call_context);
  static void PopJsCallContext();
  static void DestroyJsCallContextStack(void *call_context);

  CComPtr<IDispatch> impl_;

  DISALLOW_EVIL_CONSTRUCTORS(ModuleWrapper);
};

#endif  //  GEARS_BASE_IE_MODULE_WRAPPER_H__
