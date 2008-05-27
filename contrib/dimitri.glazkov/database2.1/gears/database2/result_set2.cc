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

#include "gears/database2/result_set2.h"

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"

DECLARE_GEARS_WRAPPER(Database2ResultSet);

template<>
void Dispatcher<Database2ResultSet>::Init() {
  RegisterProperty("insertId", &Database2ResultSet::GetInsertId, NULL);
  RegisterProperty("rowsAffected", &Database2ResultSet::GetRowsAffected, NULL);
  RegisterProperty("rows", &Database2ResultSet::GetRows, NULL);
}


// static
bool Database2ResultSet::Create(const ModuleImplBaseClass *sibling,
                                scoped_refptr<Database2ResultSet> *instance) {
  assert(instance);
  return CreateModule<Database2ResultSet>(sibling->GetJsRunner(), instance)
      && instance->get()->InitBaseFromSibling(sibling);
}

void Database2ResultSet::GetInsertId(JsCallContext *context) {
  // stub
  context->SetReturnValue(JSPARAM_INT, 0);
}

void Database2ResultSet::GetRowsAffected(JsCallContext *context) {
  // stub
  context->SetReturnValue(JSPARAM_INT, 0);
}

void Database2ResultSet::GetRows(JsCallContext *context) {
  // stub
  // return nothing for now
}
