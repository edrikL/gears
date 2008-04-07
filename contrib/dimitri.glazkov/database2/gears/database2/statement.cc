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

#include "gears/database2/statement.h"

#include "gears/database2/transaction.h"

void Database2Statement::InvokeCallback(Database2Transaction *tx) {
  // for now, just return the Database2Statement
  JsParamToSend send_argv[] = {
    { JSPARAM_STRING16, &sql_statement_ }
  };

  if (HasCallback()) {
    tx->GetJsRunner()->InvokeCallback(callback_.get(), ARRAYSIZE(send_argv),
                                      send_argv, NULL);
  }
}

void Database2Statement::InvokeErrorCallback(Database2Transaction *tx,
                                             JsObject *error) {
}

bool Database2Statement::Create(const std::string16 &sql_statement, 
                                const JsArray &sql_arguments, 
                                JsRootedCallback *callback, 
                                JsRootedCallback *error_callback, 
                                Database2Statement **instance) {
  Database2Statement *statement = new Database2Statement();
  statement->sql_statement_.assign(sql_statement);
  statement->callback_.reset(callback);
  statement->error_callback_.reset(error_callback);
  // TODO(dimitri.glazkov): convert sql_arguments to JsParamToSend[]. Accepted
  // types are: double, int32, int64, string. All other types, are considered
  // invalid
  *instance = statement;
  // TODO(dimitri.glazkov): decide whether ever return false (so that creator
  // could throw an exception, for instance) or mark statement as bogus
  return true;
}
