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

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/base/common/base_class.h"

#include "gears/database2/database2.h"
#include "gears/database2/transaction.h"

DECLARE_GEARS_WRAPPER(Database2);

void Dispatcher<Database2>::Init() {
  RegisterMethod("transaction", &Database2::Transaction);
  RegisterMethod("synchronousTransaction", &Database2::SynchronousTransaction);
  RegisterMethod("changeVersion", &Database2::ChangeVersion);
}

void Database2::Transaction(JsCallContext *context) {
  // IN: SQLTransactionCallback callback, 
  //     optional SQLTransactionErrorCallback errorCallback, 
  //     optional VoidCallback successCallback
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
  // OUT: void
}

void Database2::SynchronousTransaction(JsCallContext *context) {
  // IN: SQLSynchronousTransactionCallback
  JsRootedCallback *callback = NULL;

  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &callback }
  };

  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  // populate with interpreter (not threaded)

  // sync transaction never enters the queue, there's no need for it
  // but it still checks for pending transactions in the queue
  // to prevent deadlocks
  //if (GetQueue(/* name, origin */)->empty()) {
  //  tx->Start();
  //}
  //else {
  // explicitly disallow nested transactions, thus one can't
  // start a synchronous transaction while another transaction is still open
  // set exception INVALID_STATE_ERR
  //}
  // OUT: void
}

void Database2::ChangeVersion(JsCallContext *context) {
  // IN: in DOMString oldVersion, in DOMString newVersion, 
  //  in SQLTransactionCallback callback, 
  //  in SQLTransactionErrorCallback errorCallback, 
  //  optional in VoidCallback successCallback
  std::string16 oldVersion;
  std::string16 newVersion;
  JsRootedCallback *callback = NULL;
  JsRootedCallback *errorCallback = NULL;
  JsRootedCallback *successCallback = NULL;

  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &oldVersion },
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &newVersion },
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &callback },
    { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &errorCallback },
    { JSPARAM_OPTIONAL, JSPARAM_FUNCTION, &successCallback }
  };

  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  // populate callbacks
  // populate with threaded interpreter
  // + populate version
  // ...

  // OUT: void
}

void Database2::QueueTransaction(Database2Transaction *transaction) {
  // Database2TransactionQueue queue = GetQueue(/* name, origin */);  
  // returns true, if first iterm in queue
  // bool first = queue->empty();
  // sketch only, consider making a single method to prevent race conditions
  // queue->Push(tx);
  // if (first) {
  // start executing transaction, if first
  // tx->Start();
  // }
}

bool Database2::Create(const ModuleImplBaseClass *sibling, 
                       const std::string16 &name,
                       const std::string16 &version,
                       Database2 **instance) {
  Database2 *database = CreateModule<Database2>(sibling->GetJsRunner());
  if (database && database->InitBaseFromSibling(sibling)) {
    *instance = database;
    return true;
  }
  return false;
}

Database2TransactionQueue *Database2::GetQueue(/* name, origin */) {
  return NULL;
}
