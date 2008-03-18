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
// The C++ base class that all Gears objects should derive from.

#ifndef GEARS_BASE_COMMON_BASE_CLASS_H__
#define GEARS_BASE_COMMON_BASE_CLASS_H__

#include "gears/base/common/common.h"  // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"  // for string16

#include "gears/base/common/js_types.h"

#if BROWSER_FF

#include "ff/genfiles/base_interface_ff.h"

#elif BROWSER_IE

// no "base_interface_ie.h" because IE doesn't require a COM base interface

#elif BROWSER_NPAPI

// no "base_interface_npapi.h" because NPAPI doesn't use COM.

#endif

#if BROWSER_FF

// Implementations of boilerplate code.
#define GEARS_IMPL_BASECLASS \
  NS_IMETHOD GetNativeBaseClass(ModuleImplBaseClass **retval) { \
    *retval = this; \
    return NS_OK; \
  }

#elif BROWSER_IE
#elif BROWSER_NPAPI
#elif BROWSER_SAFARI
#endif  // BROWSER_xyz

class ModuleWrapperBaseClass;
class JsRunnerInterface;

// Exposes the minimal set of information that Gears objects need to work
// consistently across the main-thread and worker-thread JavaScript engines.
class ModuleImplBaseClass {
 public:
  // TODO_REMOVE_NSISUPPORTS: this constructor only used for isupports-based
  // modules.
  ModuleImplBaseClass()
      : module_name_(NULL), is_initialized_(false) {}
  explicit ModuleImplBaseClass(const char *name)
      : module_name_(name), is_initialized_(false) {}

  const char *get_module_name() const {
    return module_name_;
  }

  // Initialization functions
  //
  // Init from sibling -- should be used for most objects.
  // Init from DOM -- should only be used for main-thread factories.
  bool InitBaseFromSibling(const ModuleImplBaseClass *other);
#if BROWSER_FF
  bool InitBaseFromDOM();
#elif BROWSER_IE
  bool InitBaseFromDOM(IUnknown *site);
#elif BROWSER_NPAPI
  bool InitBaseFromDOM(JsContextPtr instance);
#elif BROWSER_SAFARI
  bool InitBaseFromDOM(const char *url_str);
#endif

  // Host environment information
  bool EnvIsWorker() const;
  const std::string16& EnvPageLocationUrl() const;
#if BROWSER_FF || BROWSER_NPAPI
  JsContextPtr EnvPageJsContext() const;
#elif BROWSER_IE
  IUnknown* EnvPageIUnknownSite() const;
#endif
  const SecurityOrigin& EnvPageSecurityOrigin() const;

  JsRunnerInterface *GetJsRunner() const;

#if BROWSER_FF
  // JavaScript worker-thread parameter information
  void JsWorkerSetParams(int argc, JsToken *argv, JsToken *retval);
  int          JsWorkerGetArgc() const;
  JsToken*     JsWorkerGetArgv() const;
  JsToken*     JsWorkerGetRetVal() const;
#elif BROWSER_IE
  // These do not exist in IE yet.
#endif

  // Methods for dealing with the JavaScript wrapper interface.
  void SetJsWrapper(ModuleWrapperBaseClass *wrapper) { js_wrapper_ = wrapper; }
  ModuleWrapperBaseClass *GetWrapper() const { 
    assert(js_wrapper_);
    return js_wrapper_;
  }
  void AddReference();
  void RemoveReference();

  // TODO(aa): Remove and replace call sites with GetWrapper()->GetToken().
  JsToken GetWrapperToken() const;

 private:
  // TODO(cprince): This state should be constant per (thread,page) tuple.
  // Instead of making a copy for every object (ModuleImplBaseClass), we could
  // keep a reference to one shared class per tuple.
  // (Recall idea for PageSharedState + PageThreadSharedState classes.)
  const char *module_name_;
  bool is_initialized_;
  bool env_is_worker_;
#if BROWSER_FF || BROWSER_NPAPI
  // TODO_REMOVE_NSISUPPORTS: Remove this member once all modules are based on
  // Dispatcher. env_page_js_context_ is only really used to initialize
  // JsParamFetcher, which isn't needed with Dispatcher.
  JsContextPtr  env_page_js_context_;
#elif BROWSER_IE
  // Pointer to the object that hosts this object. On Win32, this is the pointer
  // passed to SetSite. On WinCE this is the JS IDispatch pointer.
  CComPtr<IUnknown> env_page_iunknown_site_;
#endif
  SecurityOrigin env_page_origin_;

  // Init manually -- should only be used for worker-thread factories.
  friend class PoolThreadsManager;
  bool InitBaseManually(bool is_worker,
#if BROWSER_FF || BROWSER_NPAPI
                        JsContextPtr cx,
#elif BROWSER_IE
                        IUnknown *site,
#endif
                        const SecurityOrigin &page_origin,
                        JsRunnerInterface *js_runner);

#if BROWSER_FF
  int           worker_js_argc_;
  JsToken      *worker_js_argv_;
  JsToken      *worker_js_retval_;
#elif BROWSER_IE
  // These do not exist in IE yet.
#endif

  JsRunnerInterface *js_runner_;
  // Weak pointer to our JavaScript wrapper.
  ModuleWrapperBaseClass *js_wrapper_;

  DISALLOW_EVIL_CONSTRUCTORS(ModuleImplBaseClass);
};


// ModuleWrapper has a member scoped_ptr<ModuleImplBaseClass>, so this
// destructor needs to be virtual. However, adding a virtual destructor
// causes a crash in Firefox because nsCOMPtr expects (nsISupports *)ptr ==
// (ModuleImplBaseClass *)ptr. Therefore, until we convert all the old XPCOM-
// based firefox modules to be Dispatcher-based, we need this separate base
// class.
// TODO_REMOVE_NSISUPPORTS: Remove this class and make ~ModuleImplBaseClass
// virtual when this is the only ModuleImplBaseClass.
class ModuleImplBaseClassVirtual : public ModuleImplBaseClass {
 public:
  ModuleImplBaseClassVirtual() : ModuleImplBaseClass() {}
  ModuleImplBaseClassVirtual(const char *name) : ModuleImplBaseClass(name) {}
  virtual ~ModuleImplBaseClassVirtual(){}

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ModuleImplBaseClassVirtual);
};

class DispatcherInterface;

// Interface for the wrapper class that binds the Gears object to the
// JavaScript engine.
class ModuleWrapperBaseClass {
 public:
  // Returns a token for this wrapper class that can be returned via the
  // JsRunnerInterface.
  virtual JsToken GetWrapperToken() const = 0;

  // Gets the Dispatcher for this module.
  virtual DispatcherInterface *GetDispatcher() const = 0;

  // Adds a reference to the wrapper class.
  virtual void AddReference() = 0;

  // Removes a reference to the wrapper class.
  virtual void RemoveReference() = 0;

 protected:
  // Don't allow direct deletion via this interface.
  virtual ~ModuleWrapperBaseClass() { }
};

// GComPtr: automatically call Release()
class ReleaseWrapperFunctor {
 public:
  void operator()(ModuleImplBaseClass *x) const {
    if (x != NULL) { x->RemoveReference(); }
  }
};

template<class Module>
class GComPtr : public scoped_token<Module*, ReleaseWrapperFunctor> {
 public:
  explicit GComPtr(Module *v)
      : scoped_token<Module*, ReleaseWrapperFunctor>(v) {}

  Module *operator->() const { return this->get(); }

  // IE expects that when you return an object to script, it has already been
  // AddRef()'d. NPAPI and Firefox do not do this. Gears modules should call
  // this method after returning a newly created object to script to do the
  // right thing.
  void ReleaseNewObjectToScript() {
#ifdef BROWSER_IE
    // Leave the object AddRef()'d for IE
#else
    this->get()->RemoveReference();
#endif
    this->release();
  }
};

// Creates new Module of the given type.  Returns NULL on failure. The new
// module's ref count is initialized to 1. Callers should use GComPtr and
// ReleaseNewObjectToScript() with the result of this function to ensure
// consistent behavior across platforms.
template<class Module>
Module *CreateModule(JsRunnerInterface *js_runner);

#endif  // GEARS_BASE_COMMON_BASE_CLASS_H__
