// Copyright 2008, Google Inc.
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

#ifndef GEARS_BASE_FIREFOX_MODULE_WRAPPER_H__
#define GEARS_BASE_FIREFOX_MODULE_WRAPPER_H__

#include "gears/base/common/atomic_ops.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/leak_counter.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

// Represents the bridge between the JavaScript engine and a Gears module. A
// ModuleWrapper wraps each Gears module instance and exposes its methods to
// JavaScript. The wrapper has ownership of the module, so that when the
// JavaScript engine decides to destroy the wrapper, the module itself is
// destroyed.
//
// Since the ref count on the actual JS object is not exposed to us, we
// implement our own and "root" (prevent from getting GC'd) the JS object
// until our count reaches zero. Then we release the JS object to get GC'd.
// When that happens, this object is finally deleted.
class ModuleWrapper : public ModuleWrapperBaseClass {
 public:
  ModuleWrapper(JsToken token, JsContextPtr js_context)
      : token_(token), js_context_(js_context) {
    LEAK_COUNTER_INCREMENT(ModuleWrapper);
  }

  virtual ~ModuleWrapper() {
    LEAK_COUNTER_DECREMENT(ModuleWrapper);
  }

  void Init(ModuleImplBaseClass *impl, DispatcherInterface *dispatcher) {
    impl_.reset(impl);
    dispatcher_.reset(dispatcher);
  }

  virtual JsToken GetWrapperToken() const {
    assert(JSVAL_IS_GCTHING(token_));
    return token_;
  }

  virtual DispatcherInterface *GetDispatcher() const {
    assert(dispatcher_.get());
    return dispatcher_.get();
  }

  virtual ModuleImplBaseClass *GetModuleImplBaseClass() const {
    assert(impl_.get());
    return impl_.get();
  }

  virtual void Ref() {
    assert(JSVAL_IS_GCTHING(token_));
    if (ref_count_.Ref()) {
      JsRequest request(js_context_);
      JS_AddRoot(js_context_, &token_);
    }
  }

  virtual void Unref() {
    assert(JSVAL_IS_GCTHING(token_));
    if (ref_count_.Unref()) {
      JsRequest request(js_context_);
      JS_RemoveRoot(js_context_, &token_);
    }
  }

  // When the JavaScript object associated with this wrapper is GC'd, we call
  // this to finally delete the wrapper and module.
  void Destroy() {
    if (ref_count_.Value() == 0) {
      delete this;
    } else {
      token_ = 0;
      assert(false);  // Destroy should not get called when there are still
                      // references.
    }
  }

 private:
  scoped_ptr<ModuleImplBaseClass> impl_;
  scoped_ptr<DispatcherInterface> dispatcher_;

  JsToken token_;
  JsContextPtr js_context_;
  RefCount ref_count_;

  DISALLOW_EVIL_CONSTRUCTORS(ModuleWrapper);
};


// Creates an instance of the class and its wrapper.
template<class GearsClass, class OutType>
bool CreateModule(ModuleEnvironment *module_environment,
                  JsCallContext *context,
                  scoped_refptr<OutType>* module) {
  scoped_ptr<GearsClass> impl(new GearsClass()); 
  impl->InitModuleEnvironment(module_environment);
  scoped_ptr<Dispatcher<GearsClass> > dispatcher(
      new Dispatcher<GearsClass>(impl.get()));

  if (!module_environment->js_runner_->
          InitializeModuleWrapper(impl.get(), dispatcher.get(), context)) {
    return false;
  }

  dispatcher.release();
  module->reset(impl.release());
  return true;
}

#endif // GEARS_BASE_FIREFOX_MODULE_WRAPPER_H__
