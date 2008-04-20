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

#include "gears/database2/database2.h"

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/dispatcher.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/database2/transaction.h"
#include "gears/database2/common.h"

DECLARE_GEARS_WRAPPER(Database2);

template<>
void Dispatcher<Database2>::Init() {
  RegisterMethod("transaction", &Database2::Transaction);
  RegisterMethod("synchronousTransaction", &Database2::SynchronousTransaction);
  RegisterMethod("changeVersion", &Database2::ChangeVersion);
}

void Database2::Transaction(JsCallContext *context) {
  JsRootedCallback *callback = NULL;
  JsRootedCallback *error_callback = NULL;
  JsRootedCallback *success_callback = NULL;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &callback },
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &error_callback },
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &success_callback }
  };

  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  // create transaction
  // populate callbacks
  // populate with threaded interpreter

  // QueueTransaction(tx);
}

void Database2::SynchronousTransaction(JsCallContext *context) {
  JsRootedCallback *callback = NULL;

  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &callback }
  };

  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  static Database2Interpreter interpreter;
  scoped_refptr<Database2Transaction> tx;
  // populate with interpreter (not threaded)
  if (Database2Transaction::Create(this, connection_.get(), &interpreter,
                                   callback, NULL, NULL, &tx)) {
  // sync transaction never enters the queue, there's no need for it
  // but it still checks for pending transactions in the queue
  // to prevent deadlocks
  //if (GetQueue(name, origin)->empty()) {
    // the entire synchronous transaction execution cycle is actually controlled
    // from this method
    tx->Start();
    if (tx->open()) {
      tx->InvokeCallback();
      tx->MarkClosed();
      // implicitly trigger commit
      tx->ExecuteNextStatement(NULL);
    }
    // otherwise, the exception has been already thrown by InvokeErrorCallback,
    // so we just let this go
  // }
  // else {
  //   explicitly disallow nested transactions, thus one can't
  //   start a synchronous transaction while another transaction is still open
  //   set exception INVALID_STATE_ERR
  // }
  } else {
    // raise internal error exception
    context->SetException(kInternalError);
  }
}

void Database2::ChangeVersion(JsCallContext *context) {
  std::string16 oldVersion;
  std::string16 newVersion;
  JsRootedCallback *callback = NULL;
  JsRootedCallback *errorCallback = NULL;
  JsRootedCallback *successCallback = NULL;

  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &oldVersion },
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &newVersion },
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &callback },
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &errorCallback },
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &successCallback }
  };

  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  // populate callbacks
  // populate with threaded interpreter
  // + populate version
  // ...
}

void Database2::QueueTransaction(Database2Transaction *transaction) {
  // Database2TransactionQueue queue = GetQueue(name, origin);
  // returns true, if first iterm in queue
  // bool first = queue->empty();
  // sketch only, consider making a single method to prevent race conditions
  // queue->Push(tx);
  // if (first) {
  // start executing transaction, if first
  // tx->Start();
  // }
}

bool Database2::Open() {
  return connection_->OpenAndVerifyVersion(version_);
}

bool Database2::Create(const ModuleImplBaseClass *sibling, 
                       const std::string16 &name,
                       const std::string16 &version,
                       scoped_refptr<Database2> *instance) {
  assert(instance);
  if (CreateModule<Database2>(sibling->GetJsRunner(), instance)
      && instance->get()->InitBaseFromSibling(sibling)) {
    instance->get()->version_.assign(version);
    instance->get()->connection_.reset(
        new Database2Connection(
            name, instance->get()->EnvPageSecurityOrigin().url()));
    return true;
  }
  return false;
}

bool Database2::CreateError(const ModuleImplBaseClass *sibling,
                            const int code,
                            const std::string16 message,
                            JsObject **instance) {
  // TODO(dimitri.glazkov): create an instance of the error object
  return false;
}

// TODO(dimitri.glazkov): Add name, origin parameters
Database2TransactionQueue *Database2::GetQueue() {
  return NULL;
}
