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

#include "gears/base/common/base_class.h"
#include "gears/base/npapi/plugin.h"
#include "gears/database/npapi/database.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

DECLARE_GEARS_BRIDGE(GearsDatabase, GearsDatabaseWrapper);

// This class serves as the bridge between the GearsDatabase implementation and
// the browser binding layer.
class GearsDatabaseWrapper :
    public PluginBase<GearsDatabaseWrapper>,
    public ModuleWrapperBaseClass {
 public:
  GearsDatabaseWrapper(NPP instance) :
       PluginBase<GearsDatabaseWrapper>(instance),
       impl_(new GearsDatabase) {
  }
  virtual ~GearsDatabaseWrapper() {}

  virtual GearsDatabase *GetGearsObject() const { return impl_.get(); }
  virtual JsToken GetWrapperToken() const {
    NPVariant token;
    OBJECT_TO_NPVARIANT(const_cast<GearsDatabaseWrapper*>(this), token);
    return token;
  }

  virtual void Release() {
    NPN_ReleaseObject(this);
  }

  // TODO(mpcomplete): consolidate with GetGearsObject and remove.
  GearsDatabase *gears_obj() const { return impl_.get(); }

  static void InitClass() {
    RegisterMethod("open", &GearsDatabase::Open);
    RegisterMethod("execute", &GearsDatabase::Execute);
    RegisterMethod("close", &GearsDatabase::Close);
    RegisterMethod("getLastInsertRowId", &GearsDatabase::GetLastInsertRowId);
    RegisterMethod("getExecuteMsec", &GearsDatabase::GetExecuteMsec);
  }

 private:
  scoped_ptr<GearsDatabase> impl_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsDatabaseWrapper);
};

ModuleWrapperBaseClass *CreateGearsDatabase(GearsBaseClass *sibling) {
  JsContextPtr context = sibling->EnvPageJsContext();
  return static_cast<GearsDatabaseWrapper *>(
      NPN_CreateObject(context, GetNPClass<GearsDatabaseWrapper>()));
}
