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
#include "gears/base/npapi/module_wrapper.h"
#include "gears/database/npapi/result_set.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

DECLARE_GEARS_BRIDGE(GearsResultSet, GearsResultSetWrapper);

// This class serves as the bridge between the GearsResultSet implementation and
// the browser binding layer.
class GearsResultSetWrapper
    : public ModuleWrapper<GearsResultSetWrapper> {
 public:
  GearsResultSetWrapper(NPP instance)
      : ModuleWrapper<GearsResultSetWrapper>(instance) {
  }

  static void InitClass() {
    RegisterMethod("field", &GearsResultSet::Field);
    RegisterMethod("fieldByName", &GearsResultSet::FieldByName);
    RegisterMethod("fieldName", &GearsResultSet::FieldName);
    RegisterMethod("fieldCount", &GearsResultSet::FieldCount);
    RegisterMethod("close", &GearsResultSet::Close);
    RegisterMethod("next", &GearsResultSet::Next);
    RegisterMethod("isValidRow", &GearsResultSet::IsValidRow);
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GearsResultSetWrapper);
};

ModuleWrapperBaseClass *CreateGearsResultSet(GearsBaseClass *sibling) {
  return GearsResultSetWrapper::Create(sibling);
}
