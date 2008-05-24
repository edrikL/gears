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

#ifndef GEARS_DATABASE2_CONNECTION_H__
#define GEARS_DATABASE2_CONNECTION_H__

#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/scoped_refptr.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/third_party/sqlite_google/preprocessed/sqlite3.h"

class Database2Values;

class Database2RowHandlerInterface {
 public:
  Database2RowHandlerInterface() {};
  ~Database2RowHandlerInterface() {};

  virtual void Begin() = 0;
  // TODO(dimitri.glazkov): Add row data parameter(s)
  virtual void HandleRow() = 0;
  virtual void End() = 0;

  DISALLOW_EVIL_CONSTRUCTORS(Database2RowHandlerInterface);
};

// Encapsulates database operations, opens and closes database connection
class Database2Connection : public RefCounted {
 public:
  // lazily initialized
   Database2Connection(const std::string16 &name, 
                       const SecurityOrigin &origin) :
     name_(name), origin_(origin) {}
  ~Database2Connection() {
    // close connection
  }

  bool OpenAndVerifyVersion(const std::string16 &database_version);
  bool Execute(const std::string16 &statement,
               Database2Values *arguments,
               Database2RowHandlerInterface *row_handler);
  bool Begin();
  void Rollback();
  bool Commit();

  int error_code() const { return error_code_; }
  std::string16 error_message() const { return error_message_; }

 private:
  bool bogus_version_;
  int expected_version_;

  std::string16 error_message_;
  int error_code_;

  sqlite3 *handle_;
  std::string16 name_;
  SecurityOrigin origin_;

  DISALLOW_EVIL_CONSTRUCTORS(Database2Connection);
};

#endif // GEARS_DATABASE2_CONNECTION_H__
