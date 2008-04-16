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

#ifndef GEARS_DATABASE2_RESULT_SET2_H__
#define GEARS_DATABASE2_RESULT_SET2_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/scoped_refptr.h"

// interface SQLResultSet
class Database2ResultSet : public ModuleImplBaseClassVirtual {
 public:
  Database2ResultSet() : ModuleImplBaseClassVirtual("Database2ResultSet") {}

  // OUT: int
  void GetInsertId(JsCallContext *context);
  // OUT: int
  void GetRowsAffected(JsCallContext *context);
  // we return a JS array full of result objects
  // OUT: SQLResultSetRowList
  void GetRows(JsCallContext *context);

  // creates an instance, returns true if successful
  // TODO(dimitri.glazkov): Add more parameters
  static bool Create(const ModuleImplBaseClass *sibling,
                     scoped_refptr<Database2ResultSet> *instance);

  DISALLOW_EVIL_CONSTRUCTORS(Database2ResultSet);
};

#endif // GEARS_DATABASE2_RESULT_SET2_H__