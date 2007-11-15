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

#include "gears/base/common/security_model.h"
#include "gears/base/common/common.h" // for DISALLOW_EVIL_CONSTRUCTORS
#include "gears/base/common/scoped_token.h"
#include "gears/base/common/string16.h" // for string16

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
  NS_IMETHOD GetNativeBaseClass(GearsBaseClass **retval) { \
    *retval = this; \
    return NS_OK; \
  }

#elif BROWSER_IE
#elif BROWSER_NPAPI
#elif BROWSER_SAFARI
#endif // BROWSER_xyz

class ModuleWrapperBaseClass;
class JsRunnerInterface;

// TODO(mpcomplete): rename to ModuleImplBaseClass
// Exposes the minimal set of information that Scour objects need to work
// consistently across the main-thread and worker-thread JavaScript engines.
class GearsBaseClass {
 public:
  GearsBaseClass() : is_initialized_(false) {}

  // Initialization functions
  //
  // Init from sibling -- should be used for most objects.
  // Init from DOM -- should only be used for main-thread factories.
  bool InitBaseFromSibling(const GearsBaseClass *other);
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
  void JsWorkerSetParams(int argc, JsToken *argv);
  int          JsWorkerGetArgc() const;
  JsToken*     JsWorkerGetArgv() const;
#elif BROWSER_IE
  // These do not exist in IE yet.
#endif

#if BROWSER_NPAPI
  // Methods for dealing with the JavaScript wrapper interface.
  void SetJsWrapper(ModuleWrapperBaseClass *wrapper) { js_wrapper_ = wrapper; }
  void AddRef();
  void Release();
#endif

 private:
  // TODO(cprince): This state should be constant per (thread,page) tuple.
  // Instead of making a copy for every object (GearsBaseClass), we could
  // keep a reference to one shared class per tuple.
  // (Recall idea for PageSharedState + PageThreadSharedState classes.)
  bool is_initialized_;
  bool env_is_worker_;
#if BROWSER_FF || BROWSER_NPAPI
  JsContextPtr  env_page_js_context_;
#elif BROWSER_IE
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
#elif BROWSER_IE
  // These do not exist in IE yet.
#endif

  JsRunnerInterface *js_runner_;

#if BROWSER_NPAPI
  // Weak pointer to our JavaScript wrapper.
  ModuleWrapperBaseClass *js_wrapper_;
#endif

  DISALLOW_EVIL_CONSTRUCTORS(GearsBaseClass);
};

// Interface for the wrapper class that binds the Gears object to the
// JavaScript engine.
class ModuleWrapperBaseClass {
 public:
  // Returns the object that implements the Gears functionality.
  virtual GearsBaseClass *GetImplObject() const = 0;

  // Returns a token for this wrapper class that can be returned via the
  // JsRunnerInterface.
  virtual JsToken GetWrapperToken() const = 0;

  // Adds a reference to the wraper class.
  virtual void AddRef() = 0;

  // Releases a reference to the wraper class.
  virtual void Release() = 0;
 protected:
  // Don't allow direct deletion via this interface.
  virtual ~ModuleWrapperBaseClass() { }
};

// ScopedModuleWrapper: automatically call Release()
class ReleaseWrapperFunctor {
 public:
  void operator()(ModuleWrapperBaseClass *x) const {
    if (x != NULL) { x->Release(); }
  }
};

typedef scoped_token<ModuleWrapperBaseClass*, ReleaseWrapperFunctor>
    ScopedModuleWrapper;


#endif // GEARS_BASE_COMMON_BASE_CLASS_H__
