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

#include "gears/database2/transaction.h"

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/database2/database2.h"
#include "gears/database2/statement.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

DECLARE_GEARS_WRAPPER(Database2Transaction);

template<>
void Dispatcher<Database2Transaction>::Init() {
  RegisterMethod("executeSql", &Database2Transaction::ExecuteSql);
}

void Database2Transaction::ExecuteSql(JsCallContext *context) {
  std::string16 sql_Database2Statement;
  JsArray sql_arguments;
  JsRootedCallback *callback = NULL;
  JsRootedCallback *error_callback = NULL;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &sql_Database2Statement },
    { JSPARAM_OPTIONAL, JSPARAM_ARRAY, &sql_arguments},
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &callback },
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &error_callback }
  };

  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  // if (!is_open_) {
  // throw exception saying the transaction is closed
  // }
  // create Database2Statement with sql, arguments, and callbacks
  // add to statement queue
  // if first item in the queue, invoke ExecuteNextStatement

  // ideally, if the queue is empty prior to this call, this should be done
  // without pushing/popping the statement
}

void Database2Transaction::Start() {
  // queue operation to begin transaction
  // interpreter()->Run(new Database2BeginCommand(this, database()));
}

void Database2Transaction::ExecuteNextStatement(JsCallContext *context) {
  // pop statement from the end of the queue
  // if no more statements,
  // interpreter()->RunCommand(new Database2CommitCommand(this));
  // otherwise
  // if (async_) {
  //   interpreter()->RunCommand(new Database2AsyncExecuteCommand(this, conn,
  //     stmt));
  // }
  // else {
  //   interperter()->RunCommand(new Database2SyncExecuteCommand(context, conn));
  // }
}

void Database2Transaction::InvokeCallback() {
  // prepare to return transaction
  JsParamToSend send_argv[] = {
    { JSPARAM_MODULE, static_cast<ModuleImplBaseClass *>(this) }
  };

  GetJsRunner()->InvokeCallback(callback_.get(), ARRAYSIZE(send_argv),
                                 send_argv, NULL);
}

void Database2Transaction::InvokeErrorCallback() {
}

void Database2Transaction::InvokeSuccessCallback() {
}

bool Database2Transaction::Create(const Database2* database,
                                  const bool async,
                                  JsRootedCallback* callback,
                                  JsRootedCallback* error_callback,
                                  JsRootedCallback* success_callback,
                                  Database2Transaction** instance) {
  Database2Transaction *tx = 
     CreateModule<Database2Transaction>(database->GetJsRunner());
  if (tx && tx->InitBaseFromSibling(database)) {
    tx->async_ = async;
    // set callbacks
    tx->callback_.reset(callback);
    tx->error_callback_.reset(error_callback);
    tx->success_callback_.reset(success_callback);
    *instance = tx;
    return true;
  }
  return false;
}
