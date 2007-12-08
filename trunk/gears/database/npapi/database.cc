// Copyright 2007, Google Inc.
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

#include "gears/base/common/paths.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/database/common/database_utils.h"
#include "gears/database/npapi/database.h"
#include "gears/database/npapi/result_set.h"

#ifdef DEBUG
Stopwatch GearsDatabase::g_stopwatch_;
#endif // DEBUG

GearsDatabase::GearsDatabase() : db_(NULL) {
}

GearsDatabase::~GearsDatabase() {
  assert(result_sets_.empty());

  if (db_) {
    sqlite3_close(db_);
    db_ = NULL;
  }
}

void GearsDatabase::Open(JsCallContext *context) {
  if (db_) {
    context->SetException(STRING16(L"A database is already open."));
    return;
  }

  // Create an event monitor to close remaining ResultSets when the page
  // unloads.
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(),
                                             JSEVENT_UNLOAD, this));
  }

  // Get parameters.
  std::string16 database_name;
  JsArgument argv[] = {
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &database_name },
  };
  int argc = context->GetArguments(ARRAYSIZE(argv), argv);

  // For now, callers cannot open DBs in other security origins.
  // To support that, parse an 'origin' argument here and call
  // IsOriginAccessAllowed (yet to be written).

  // Open the database.
  if (!OpenSqliteDatabase(database_name.c_str(), EnvPageSecurityOrigin(),
                          &db_)) {
    context->SetException(STRING16(L"Couldn't open SQLite database."));
  }
}

void GearsDatabase::Execute(JsCallContext *context) {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  int sql_status;

  if (!db_) {
    context->SetException(STRING16(L"Database handle was NULL."));
    return;
  }

  // Get parameters.
  std::string16 expr;
  JsToken arg_array;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &expr },
    { JSPARAM_OPTIONAL, JSPARAM_OBJECT_TOKEN, &arg_array },
  };
  int argc = context->GetArguments(ARRAYSIZE(argv), argv);
  if (argc < 1)
    return;  // GetArguments sets an error message.

  // Prepare a statement for execution.

// TODO(cprince): remove #ifdef and string conversion after refactoring LOG().
#ifdef DEBUG
  std::string expr_utf8;
  String16ToUTF8(expr.c_str(), expr.length(), &expr_utf8);
  LOG(("DB Execute: %s", expr_utf8.c_str()));
#endif

  scoped_sqlite3_stmt_ptr stmt;
  sql_status = sqlite3_prepare16_v2(db_, expr.c_str(), -1, &stmt, NULL);
  if ((sql_status != SQLITE_OK) || (stmt.get() == NULL)) {
    std::string16 msg;
    BuildSqliteErrorString(STRING16(L"SQLite prepare() failed."),
                           sql_status, db_, &msg);
    msg += STRING16(L" EXPRESSION: ");
    msg += expr;
    context->SetException(msg.c_str());
    return;
  }

  // Bind parameters
  if (!BindArgsToStatement(context, (argc >= 2) ? &arg_array : NULL,
                           stmt.get())) {
    // BindArgsToStatement already set an exception
    return;
  }

  // Wrap a GearsResultSet around the statement and execute it
  GComPtr<GearsResultSet> result_set(
      CreateModule<GearsResultSet>(EnvPageJsContext()));
  if (!result_set.get())
    return;  // Create function sets an error message.

  if (!result_set->InitBaseFromSibling(this)) {
    context->SetException(STRING16(L"Error initializing base class."));
    return;
  }

  // Note the ResultSet takes ownership of the statement
  std::string16 error_message;
  if (!result_set->InitializeResultSet(stmt.release(), this, &error_message)) {
    context->SetException(error_message.c_str());
    return;
  }

  context->SetReturnValue(JSPARAM_MODULE, result_set.get());
}

bool GearsDatabase::BindArgsToStatement(JsCallContext *context,
                                        const JsToken *arg_array,
                                        sqlite3_stmt *stmt) {
  int num_args_expected = sqlite3_bind_parameter_count(stmt);
  int num_args = 0;

  JsArray array;

  if (arg_array && (!array.SetArray(*arg_array, EnvPageJsContext()) ||
                    !array.GetLength(&num_args))) {
    context->SetException(STRING16(L"Invalid SQL parameters array."));
    return false;
  }

  if (num_args_expected != num_args) {
    context->SetException(STRING16(L"Wrong number of SQL parameters."));
    return false;
  }

  for (int i = 0; i < num_args; i++) {
    int sql_index = i + 1; // sql parameters are 1-based
    int sql_status = SQLITE_ERROR;
    std::string16 arg_str;
    int arg_int;
    double arg_double;
    if (array.GetElementAsString(i, &arg_str)) {
// TODO(cprince): remove #ifdef and string conversion after refactoring LOG().
#ifdef DEBUG
      std::string str_utf8;
      String16ToUTF8(arg_str.c_str(), arg_str.length(), &str_utf8);
      LOG(("        Parameter %i: %s", i, str_utf8.c_str()));
#endif
      sql_status = sqlite3_bind_text16(
          stmt, sql_index, arg_str.c_str(), -1,
          SQLITE_TRANSIENT); // so SQLite copies string immediately
    } else if (array.GetElementAsInt(i, &arg_int)) {
      LOG(("        Parameter %i: %i", i, arg_int));
      sql_status = sqlite3_bind_int(stmt, sql_index, arg_int);
    } else if (array.GetElementAsDouble(i, &arg_double)) {
      LOG(("        Parameter %i: %lf", i, arg_double));
      sql_status = sqlite3_bind_double(stmt, sql_index, arg_double);
    } else {
      // TODO(mpcomplete): figure out what else we need to support here.
      std::string16 error =
          STRING16(L"SQL parameter ") + IntegerToString16(i) +
          STRING16(L" has unknown type.");
      context->SetException(error.c_str());
      return false;
    }

    if (sql_status != SQLITE_OK) {
      context->SetException(STRING16(L"Could not bind arguments to expression."));
      return false;
    }
  }

  return true;
}

void GearsDatabase::Close(JsCallContext *context) {
  if (!CloseInternal()) {
    context->SetException(STRING16(L"SQLite close() failed."));
  }
}

void GearsDatabase::GetLastInsertRowId(JsCallContext *context) {
  sqlite_int64 rowid = sqlite3_last_insert_rowid(db_);
  if ((rowid < JS_INT_MIN) || (rowid > JS_INT_MAX)) {
    context->SetException(STRING16(L"lastInsertRowId is out of range."));
    return;
  }
  double retval = static_cast<double>(rowid);
  context->SetReturnValue(JSPARAM_DOUBLE, &retval);
}

void GearsDatabase::AddResultSet(GearsResultSet *rs) {
  result_sets_.insert(rs);
}

void GearsDatabase::RemoveResultSet(GearsResultSet *rs) {
  assert(result_sets_.find(rs) != result_sets_.end());

  result_sets_.erase(rs);
}


bool GearsDatabase::CloseInternal() {
  if (db_) {
    for (std::set<GearsResultSet *>::iterator result_set = result_sets_.begin();
         result_set != result_sets_.end();
         ++result_set) {
      (*result_set)->Finalize();
    }

    int sql_status = sqlite3_close(db_);
    db_ = NULL;
    if (sql_status != SQLITE_OK) {
      return false;
    }
  }
  return true;
}

void GearsDatabase::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);

  CloseInternal();

  // When the page unloads, NPAPI plugins are unloaded.  When that happens,
  // objects are cleaned up and deleted regardless of reference count, so we
  // can't ensure that the ResultSets are deleted first.  So we give them a
  // chance to do cleanup while the GearsDatabase object still exists.
  for (std::set<GearsResultSet *>::iterator result_set = result_sets_.begin();
       result_set != result_sets_.end();
       ++result_set) {
    (*result_set)->PageUnloading();
  }
  result_sets_.clear();
}

#ifdef DEBUG
void GearsDatabase::GetExecuteMsec(JsCallContext *context) {
  int retval = GearsDatabase::g_stopwatch_.GetElapsed();
  context->SetReturnValue(JSPARAM_INT, &retval);
}
#endif
