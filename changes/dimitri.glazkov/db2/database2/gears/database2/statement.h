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

#ifndef GEARS_DATABASE2_STATEMENT_H__
#define GEARS_DATABASE2_STATEMENT_H__

#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h" // for JsCallContext
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// forward declarations
class Database2Error;
class Database2;
class Database2Transaction;

// represents Database2Statement
class Database2Statement {
 public:
  Database2Statement(const std::string16 &sql_statement, 
                     const JsArray &sql_arguments,
                     JsRootedCallback *callback,
                     JsRootedCallback *error_callback) :
      sql_statement_(sql_statement), 
      sql_arguments_(sql_arguments), callback_(callback), 
      error_callback_(error_callback) {}

  bool HasCallback() const { 
    return callback_.get() != NULL
      && !JsTokenIsNullOrUndefined(callback_->token()); 
  }

  bool HasErrorCallback() const { 
    return error_callback_.get() != NULL
      && !JsTokenIsNullOrUndefined(error_callback_->token());
  }

  void InvokeCallback(Database2Transaction* tx);
  void InvokeErrorCallback(Database2Transaction* tx, Database2Error* error);

 private:
  std::string16 sql_statement_;
  JsArray sql_arguments_;
  scoped_ptr<JsRootedCallback> callback_;
  scoped_ptr<JsRootedCallback> error_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(Database2Statement);
};

#endif // GEARS_DATABASE2_STATEMENT_H__
