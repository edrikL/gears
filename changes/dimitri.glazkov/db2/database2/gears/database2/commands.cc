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

#include "gears/database2/commands.h"
#include "gears/database2/common.h"
#include "gears/database2/result_set2.h"

void Database2BeginCommand::Execute(bool *has_results) {
  set_result(connection()->Begin());
}

void Database2BeginCommand::HandleResults() {
  // the entire synchronous transaction execution cycle is actually controlled
  // from this method
  if (success()) {
    tx()->MarkOpen();
    tx()->InvokeCallback();
    tx()->MarkClosed();
    // implicitly trigger commit
    tx()->ExecuteNextStatement(NULL);
  } else {
    tx()->InvokeErrorCallback();
  }
}

void Database2AsyncExecuteCommand::Execute(bool *has_results) {
  // conn_->Execute(stmt, this);
  // if success and no success callback:
    // tx->ExecuteNextStatement();
    // *has_results = false;
    // return
  // otherwise, delegate to foreground thread (by default)
}

void Database2AsyncExecuteCommand::HandleResults() {
  // create JS objects from collected rows
  // stmt_->InvokeCallback(collected_rows);
  // if statement succeeded and callback failed, queue rollback op
  // else if statement failed and there is no callback, or callback did
  // not return false, queue rollback op
}

void Database2SyncExecuteCommand::Execute(bool *has_results) {
  // TODO(dimitri.glazkov): Collect row results
  if (!set_result(connection()->Execute(statement_->sql(),
      statement_->arguments(), NULL))) {
    connection()->Rollback();
    tx()->MarkClosed();
  }
}

void Database2SyncExecuteCommand::HandleResults() {
  if (success()) {
    scoped_refptr<Database2ResultSet> result_set;
    // TODO(dimitri.glazkov): Pass collected row results
    if (Database2ResultSet::Create(tx(), &result_set)) {
      context_->SetReturnValue(JSPARAM_DISPATCHER_MODULE, result_set.get());
      ReleaseNewObjectToScript(result_set.get());
    } else {
      // unable to create a result_set
      context_->SetException(kInternalError);
    }
  } else {
    // throw SQL error as an exception
    // TODO(dimitri.glazkov): Consider formatting the message to include error
    // code
    context_->SetException(connection()->error_message());
  }
  // since it's not longer needed and we never really owned it anyway
  context_.release();
}

void Database2CommitCommand::Execute(bool *has_results) {
  if (!set_result(connection()->Commit())) {
    connection()->Rollback();
  } else {
    *has_results = tx()->HasSuccessCallback();
  }
}

void Database2CommitCommand::HandleResults() {
  if (success()) {
    tx()->InvokeSuccessCallback();
  } else {
    tx()->InvokeErrorCallback();
  }
}

void Database2RollbackCommand::Execute(bool *has_results) {
  // stub
}

void Database2RollbackCommand::HandleResults() {
  // stub
}


