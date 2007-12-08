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

#include "gears/factory/npapi/factory_wrapper.h"

#include "gears/base/npapi/module_wrapper.h"
#include "gears/factory/npapi/factory.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

DECLARE_GEARS_WRAPPER(GearsFactory, GearsFactoryWrapper);

// TODO(mpcomplete): The naming is temporary.  Right now we have:
// - GearsFactoryWrapper: serves as the bridge between implementation and the
// JavaScript engine.
// - GearsFactory: the actual implementation of the factory module.
//
// We want to eventually rename GearsFactory -> GearsFactoryImpl, and
// GearsFactoryWrapper -> GearsFactory.  But until we have this abstraction layer
// for all browsers, we need to preserve the old naming scheme.

// This class serves as the bridge between the GearsFactory implementation and
// the browser binding layer.
class GearsFactoryWrapper : public ModuleWrapper<GearsFactoryWrapper> {
 public:
  GearsFactoryWrapper(NPP instance)
      : ModuleWrapper<GearsFactoryWrapper>(instance) {
  }

  static void InitClass() {
    RegisterProperty("version", &GearsFactory::GetVersion, NULL);
    RegisterMethod("create", &GearsFactory::Create);
    RegisterMethod("getBuildInfo", &GearsFactory::GetBuildInfo);
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GearsFactoryWrapper);
};

NPObject* CreateGearsFactoryWrapper(JsContextPtr context) {
  GearsFactoryWrapper *factory_wrapper = static_cast<GearsFactoryWrapper*>(
        NPN_CreateObject(context, GetNPClass<GearsFactoryWrapper>()));
  if (factory_wrapper) {
    GearsFactory *factory = static_cast<GearsFactory*>(
          factory_wrapper->GetImplObject());
    factory->InitBaseFromDOM(context);
  }

  return factory_wrapper;
}
