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

#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/thread_locals.h"
#include "gears/base/ie/module_wrapper.h"

const static std::string kCallContextThreadLocalKey("base:modulewrapper");

ModuleWrapper::ModuleWrapper() : impl_(NULL) {}

ModuleWrapper::~ModuleWrapper() {}

HRESULT ModuleWrapper::GetTypeInfoCount(unsigned int FAR* retval) {
  // JavaScript does not call this
  assert(false);
  return E_NOTIMPL;
}

HRESULT ModuleWrapper::GetTypeInfo(unsigned int index, LCID lcid,
                                   ITypeInfo FAR* FAR* retval) {
  // JavaScript does not call this
  assert(false);
  return E_NOTIMPL;
};

HRESULT ModuleWrapper::GetIDsOfNames(REFIID iid, OLECHAR FAR* FAR* names,
                                     unsigned int num_names, LCID lcid, 
                                     DISPID FAR* retval) {
  // TODO(aa): Get this information from the class itself using something like
  // the NPAPI port's RegisterMethod().
  assert(impl_);
  return impl_->GetIDsOfNames(iid, names, num_names, lcid, retval);
}

HRESULT ModuleWrapper::Invoke(DISPID member_id, REFIID iid, LCID lcid,
                              WORD flags, DISPPARAMS FAR* params,
                              VARIANT FAR* retval, EXCEPINFO FAR* exception,
                              unsigned int FAR* arg_error_index) {
  assert(impl_);

  // Our Gears objects do not ever expect any arguments via IDispatch, so we
  // create a new params object reflecting that. However JScript does use
  // "named args" to indicate when a property is being read or written. So
  // copy those values from what the browser specified.
  CComVariant variant_arg;
  DISPPARAMS wrapped_params;
  wrapped_params.cNamedArgs = params->cNamedArgs;
  wrapped_params.rgdispidNamedArgs = params->rgdispidNamedArgs;
  wrapped_params.rgvarg = &variant_arg;

  if (flags & DISPATCH_PROPERTYPUT) {
    // We have to tell COM we are passing a parameter, even though we don't use
    // it, otherwise the call is rejected.
    wrapped_params.cArgs = 1;
  } else {
    wrapped_params.cArgs = 0;
  }

  JsCallContext js_call_context(params, retval, exception);

  // IDispatch getters do not accept any input params so there is no way to pass
  // this JsCallContext to our implementation methods on the stack. Instead, we
  // stuff them to the side in this thread-local stack.
  // In the future, we will be changing our Gears classes so that they are not
  // COM objects and do not implement IDispatch, but instead have their own
  // dynamic dispatch implementation. Once that is done, we can get rid of this.
  PushJsCallContext(&js_call_context);
  
  HRESULT hr = impl_->Invoke(member_id, iid, lcid, flags, &wrapped_params,
                             NULL, NULL, NULL); // result, exception info, bad
                                                // arg index. We use
                                                // JsCallContext for these
                                                // things instead.
  PopJsCallContext();


  if (js_call_context.is_exception_set()) {
    return DISP_E_EXCEPTION;
  } else if (FAILED(hr)) {
    // Handle generic failures where the Gears module did not specify exception
    // details (bad module!).
    js_call_context.SetException(GET_INTERNAL_ERROR_MESSAGE());
    return DISP_E_EXCEPTION;
  } else {
    return S_OK;
  }
}

void ModuleWrapper::PushJsCallContext(JsCallContext *js_call_context) {
  JsCallContextStack *context_stack = reinterpret_cast<JsCallContextStack *>(
      ThreadLocals::GetValue(kCallContextThreadLocalKey));
  if (!context_stack) {
    context_stack = new JsCallContextStack();
    ThreadLocals::SetValue(kCallContextThreadLocalKey, context_stack,
                           &DestroyJsCallContextStack);

  }
  context_stack->push(js_call_context);
}

void ModuleWrapper::PopJsCallContext() {
  JsCallContextStack *context_stack = reinterpret_cast<JsCallContextStack *>(
    ThreadLocals::GetValue(kCallContextThreadLocalKey));
  assert(context_stack);
  assert(context_stack->size() > 0);
  context_stack->pop();
}

JsCallContext *ModuleWrapper::PeekJsCallContext() {
  JsCallContextStack *context_stack = reinterpret_cast<JsCallContextStack *>(
    ThreadLocals::GetValue(kCallContextThreadLocalKey));
  assert(context_stack);
  assert(context_stack->size() > 0);
  return context_stack->top();
}

void ModuleWrapper::DestroyJsCallContextStack(void *context_stack) {
  if (context_stack) {
    delete reinterpret_cast<JsCallContextStack *>(context_stack);
  }
}
