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

#ifndef GEARS_DATABASE2_TRANSACTION_H__
#define GEARS_DATABASE2_TRANSACTION_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/js_types.h" // for JsCallContext
#include "gears/base/common/js_runner.h"
#include "gears/database2/thread_safe_queue.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// forward declarations
class Database2;
class Database2Statement;

class Database2Transaction : public ModuleImplBaseClassVirtual {
 public:
  Database2Transaction::Database2Transaction() : 
    ModuleImplBaseClassVirtual("Database2Transaction") {}

  // IN: in string sqlDatabase2Statement, 
  //     in object[] arguments, 
  //     in SQLDatabase2StatementCallback callback, 
  //     in SQLDatabase2StatementErrorCallback errorCallback
  // OUT: void
  void ExecuteSql(JsCallContext *context);

  void Start();

  void ExecuteNextStatement(JsCallContext *context);

  bool HasErrorCallback() const { 
    return error_callback_.get() != NULL
      && !JsTokenIsNullOrUndefined(error_callback_->token()); 
  }

  bool HasSuccessCallback() const {
    return success_callback_.get() != NULL
      && !JsTokenIsNullOrUndefined(success_callback_->token());
  }

  void InvokeCallback();
  void InvokeErrorCallback();
  void InvokeSuccessCallback();

  static bool Create(const Database2 *database,
                     const bool async,
                     JsRootedCallback *callback,
                     JsRootedCallback *error_callback,
                     JsRootedCallback *success_callback,
                     Database2Transaction **instance);

 private:
  bool async_;
  std::string16 old_version_;
  std::string16 new_version_;

  // scoped_refptr<Database2Connection> connection_;
  // scoped_refptr<Database2Interpreter> interpreter_;

  Database2ThreadSafeQueue<Database2Statement> statement_queue_;

  bool is_open_;

  scoped_ptr<JsRootedCallback> callback_;
  scoped_ptr<JsRootedCallback> error_callback_;
  scoped_ptr<JsRootedCallback> success_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(Database2Transaction);
};

#endif // GEARS_DATABASE2_TRANSACTION_H__
