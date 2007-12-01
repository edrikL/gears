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

#ifndef GEARS_BASE_NPAPI_MODULE_WRAPPER_H__
#define GEARS_BASE_NPAPI_MODULE_WRAPPER_H__

#include "gears/base/common/base_class.h"
#include "gears/base/npapi/plugin.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// Base class that implements common functionality to module wrappers.
template<class T>
class ModuleWrapper
    : public PluginBase<T>,
      public ModuleWrapperBaseClass {
 public:
  static ModuleWrapperBaseClass *Create(ModuleImplBaseClass *sibling) {
    JsContextPtr context = sibling->EnvPageJsContext();
    PluginClass *wrapper = static_cast<PluginClass *>(
        NPN_CreateObject(context, GetNPClass<PluginClass>()));

    if (!wrapper) {
      BrowserUtils::SetJsException(
          STRING16(L"Failed to create requested object."));
      return NULL;
    }

    if (!wrapper->GetImplObject()->InitBaseFromSibling(sibling)) {
      BrowserUtils::SetJsException(STRING16(L"Error initializing base class."));
      wrapper->Release();
      return NULL;
    }

    return wrapper;
  }

  ModuleWrapper(NPP instance) :
       PluginBase<T>(instance),
       impl_(new ImplClass) {
    impl_->SetJsWrapper(this);
  }

  virtual ImplClass *GetImplObject() const { return impl_.get(); }
  virtual JsToken GetWrapperToken() const {
    NPVariant token;
    OBJECT_TO_NPVARIANT(const_cast<ModuleWrapper<T> *>(this), token);
    return token;
  }
  virtual void AddRef() { NPN_RetainObject(this); }
  virtual void Release() { NPN_ReleaseObject(this); }

 protected:
  scoped_ptr<ImplClass> impl_;

  DISALLOW_EVIL_CONSTRUCTORS(ModuleWrapper);
};

#endif // GEARS_BASE_NPAPI_MODULE_WRAPPER_H__
