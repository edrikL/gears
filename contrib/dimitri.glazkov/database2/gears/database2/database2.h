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

#ifndef GEARS_DATABASE2_DATABASE2_H__
#define GEARS_DATABASE2_DATABASE2_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/string16.h"
#include "gears/database2/interpreter.h"
#include "gears/database2/thread_safe_queue.h"

// forward declarations
class Database2Transaction;

typedef Database2ThreadSafeQueue<Database2Transaction> 
    Database2TransactionQueue;

// Implements the HTML5 database interface, which allows the creation of 
// transactions. We also have our own proprietary synchronousTransaction() 
// method. This class also has a reference to a Database2Connection object which
// it shares with all transactions it creates.
class Database2 : public ModuleImplBaseClassVirtual {
public:
  Database2()
    : ModuleImplBaseClassVirtual("Database2") {}

  // transaction(...)
  // IN: SQLTransactionCallback callback, 
  //     optional SQLTransactionErrorCallback errorCallback, 
  //     optional VoidCallback successCallback);
  // OUT: void
  void Transaction(JsCallContext *context);

  // synchronousTransaction(...)
  // IN: function start_callback
  // OUT: void
  void SynchronousTransaction(JsCallContext *context);

  // changeVersion(...)
  // IN: string old version, string new version, optional function callback,
  //     optional function success, optional function failure
  // OUT: void
  void ChangeVersion(JsCallContext *context);

  // TODO(dimitri.glazkov): Add name, origin parameters
  static Database2TransactionQueue *GetQueue();

  static bool Create(const ModuleImplBaseClass *sibling, 
                     const std::string16 &name,
                     const std::string16 &version,
                     Database2 **instance);

  // creates an instance of a SQLError object, using a Javascript object
  static bool CreateError(const ModuleImplBaseClass *sibling, 
                          int code, 
                          std::string16 message,
                          JsObject **instance);

private:
  std::string16 name_;
  std::string16 origin_;
  std::string16 version_;

  // TODO(dimitri.glazkov) merge RefCounted & scoped_refptr into this branch

  // Shared reference to the connection used by all transactions from this
  // database instance. This is initialized during the first transaction.

  //scoped_refptr<Database2Connection> connection_;
  Database2Interpreter interpreter_;
  //scoped_refptr<Database2ThreadedInterpreter> threaded_interpreter_;

  void QueueTransaction(Database2Transaction *transaction);

  DISALLOW_EVIL_CONSTRUCTORS(Database2);
};

#endif // GEARS_DATABASE2_DATABASE2_H__
