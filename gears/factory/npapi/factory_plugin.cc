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

#include "gears/factory/npapi/factory_plugin.h"

#include "gears/base/npapi/np_utils.h"
#include "gears/factory/npapi/factory.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// Properties and Methods
static NamedIdentifier kVersionId("version");
static NamedIdentifier kCreateId("create");
static NamedIdentifier kGetBuildInfoId("getBuildInfo");

// This class serves as the bridge between the GearsFactory implementation and
// the browser binding layer.
class GearsFactoryPlugin : public PluginBase {
 public:
  static NPObject* ClassAllocate(NPP npp, NPClass *npclass) {
    return new GearsFactoryPlugin(npp);
  }

  GearsFactoryPlugin(NPP instance);
  virtual ~GearsFactoryPlugin() {}

  virtual bool Invoke(NPIdentifier name, const NPVariant *args,
                      uint32_t num_args, NPVariant *result);
  virtual bool GetProperty(NPIdentifier name, NPVariant *result);

  virtual IDList& GetPropertyList() {
    static IDList properties;
    return properties;
  }
  virtual IDList& GetMethodList() {
    static IDList methods;
    return methods;
  }

 private:
  void InitPluginIds();

  scoped_ptr<GearsFactory> impl_;
};

static NPClass* GetNPClass() {
  return PluginBase::GetClass<GearsFactoryPlugin>();
}

PluginBase* CreateGearsFactoryPlugin(JsContextPtr context) {
  return static_cast<PluginBase*>(NPN_CreateObject(context, GetNPClass()));
}

GearsFactoryPlugin::GearsFactoryPlugin(NPP instance) :
    PluginBase(instance),
    impl_(new GearsFactory) {
  InitPluginIds();
  impl_->InitBaseFromDOM(instance);
}

void GearsFactoryPlugin::InitPluginIds() {
  static bool did_init = false;
  if (did_init)
    return;
  did_init = true;

  RegisterProperty(&kVersionId);
  RegisterMethod(&kCreateId);
  RegisterMethod(&kGetBuildInfoId);
}

bool GearsFactoryPlugin::Invoke(NPIdentifier name, const NPVariant *args,
                                uint32_t num_args, NPVariant *result) {
  if (name == kCreateId.id) {
    // TODO(mpcomplete): implement me
    char val[] = "create invoked";
    NPString np_val = NPN_StringDup(val, ARRAYSIZE(val)+1);
    STRINGN_TO_NPVARIANT(np_val.utf8characters, np_val.utf8length, *result);
    impl_->Create();
    return true;
  } else if (name == kGetBuildInfoId.id) {
    // TODO(mpcomplete): implement me
    return true;
  }
 
  return false;
}

bool GearsFactoryPlugin::GetProperty(NPIdentifier name, NPVariant *result) {
  if (name == kVersionId.id) {
    // TODO(mpcomplete): implement me
    char16 val[] = PRODUCT_VERSION_STRING;
    NPString np_val = NPN_StringDup(val, ARRAYSIZE(val)+1);
    STRINGN_TO_NPVARIANT(np_val.utf8characters, np_val.utf8length, *result);
    return true;
  }

  return false;
}
