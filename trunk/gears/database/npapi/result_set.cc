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

#include "gears/database/npapi/result_set.h"

#include "gears/third_party/sqlite_google/preprocessed/sqlite3.h"
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/stopwatch.h"

#include "gears/database/npapi/database.h"

GearsResultSet::GearsResultSet() :
    database_(NULL),
    statement_(NULL),
    is_valid_row_(false) {
}

GearsResultSet::~GearsResultSet() {
  if (statement_) {
    LOG(("~GearsResultSet - was NOT closed by caller\n"));
  }

  Finalize();

  if (database_ != NULL) {
    database_->RemoveResultSet(this);
    database_->Release();
    database_ = NULL;
  }
}

bool GearsResultSet::InitializeResultSet(sqlite3_stmt *statement,
                                         GearsDatabase *db,
                                         std::string16 *error_message) {
  assert(statement);
  assert(db);
  assert(error_message);
  statement_ = statement;
  // convention: call next() when the statement is set
  bool succeeded = NextImpl(error_message); 
  if (!succeeded || sqlite3_column_count(statement_) == 0) {
    // Either an error occurred or this was a command that does
    // not return a row, so we can just close automatically
    Close();
  } else {
    database_ = db;
    database_->AddRef();
    db->AddResultSet(this);
  }
  return succeeded;
}

void GearsResultSet::PageUnloading() {
  if (database_ != NULL) {
    // Don't remove ourselves from the result set, since database_ is going away
    // soon anyway.
    database_->Release();
    database_ = NULL;
  }
}

bool GearsResultSet::Finalize() {
  if (statement_) {
    int sql_status = sqlite3_finalize(statement_);
    statement_ = NULL;

    LOG(("DB ResultSet Close: %d", sql_status));

    if (sql_status != SQLITE_OK) {
      return false;
    }
  }
  return true;
}

void GearsResultSet::FieldImpl(int index) {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  if (statement_ == NULL) {
    RETURN_EXCEPTION(STRING16(L"SQL statement is NULL."));
  }
  if ((index < 0) || (index >= sqlite3_column_count(statement_))) {
    RETURN_EXCEPTION(STRING16(L"Invalid index."));
  }

  int column_type = sqlite3_column_type(statement_, index);
  bool success = false;
  switch (column_type) {
    case SQLITE_INTEGER: {
      sqlite_int64 i64 = sqlite3_column_int64(statement_, index);
      if ((i64 >= INT_MIN) && (i64 <= INT_MAX)) {
        int retval = static_cast<int>(i64);
        success = GetJsRunner()->SetReturnValue(JSPARAM_INT, &retval);
      } else if ((i64 >= JS_INT_MIN) && (i64 <= JS_INT_MAX)) {
        double retval = static_cast<double>(i64);
        success = GetJsRunner()->SetReturnValue(JSPARAM_DOUBLE, &retval);
      } else {
        RETURN_EXCEPTION(STRING16(L"Integer value is out of range."));
      }
      break;
    }
    case SQLITE_FLOAT: {
      double retval = sqlite3_column_double(statement_, index);
      success = GetJsRunner()->SetReturnValue(JSPARAM_DOUBLE, &retval);
      break;
    }
    case SQLITE_TEXT: {
      const void *text = sqlite3_column_text16(statement_, index);
      std::string16 retval(static_cast<const char16 *>(text));
      success = GetJsRunner()->SetReturnValue(JSPARAM_STRING16, &retval);
      break;
    }
    case SQLITE_NULL:
      success = GetJsRunner()->SetReturnValue(JSPARAM_NULL, NULL);
      break;
    case SQLITE_BLOB:
      // TODO(miket): figure out right way to pass around blobs in variants.
      RETURN_EXCEPTION(STRING16(L"Data type not supported."));
    default:
      RETURN_EXCEPTION(STRING16(L"Data type not supported."));
  }

  if (!success) {
    RETURN_EXCEPTION(STRING16(L"Setting variant failed."));
  }

  RETURN_NORMAL();
}

void GearsResultSet::Field() {
  // Get parameters.
  int index;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &index },
  };
  int argc = GetJsRunner()->GetArguments(ARRAYSIZE(argv), argv);
  if (argc < 1)
    return;  // JsRunner sets an error message.

  FieldImpl(index);  // sets the return value/error.
}

void GearsResultSet::FieldByName() {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  if (statement_ == NULL) {
    RETURN_EXCEPTION(STRING16(L"SQL statement is NULL."));
  }

  // Get parameters.
  std::string16 field_name;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &field_name },
  };
  int argc = GetJsRunner()->GetArguments(ARRAYSIZE(argv), argv);
  if (argc < 1)
    return;  // JsRunner sets an error message.

  // TODO(miket): This is horrible O(n) code but we didn't have a hashtable
  // implementation handy. Fix this!
  int n = sqlite3_column_count(statement_);
  int i;
  for (i = 0; i < n; ++i) {
    const void *column_name = sqlite3_column_name16(statement_, i);
    std::string16 s(static_cast<const char16 *>(column_name));
    if (field_name == s) {
      break;  // found it
    }
  }
  if (i >= n) {
    RETURN_EXCEPTION(STRING16(L"Field name not found."));
  }

  FieldImpl(i);  // sets the return value/error.
}

void GearsResultSet::FieldName() {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  if (statement_ == NULL) {
    RETURN_EXCEPTION(STRING16(L"SQL statement is NULL."));
  }

  // Get parameters.
  int index;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &index },
  };
  int argc = GetJsRunner()->GetArguments(ARRAYSIZE(argv), argv);
  if (argc < 1)
    return;  // JsRunner sets an error message.

  if ((index < 0) || (index >= sqlite3_column_count(statement_))) {
    RETURN_EXCEPTION(STRING16(L"Invalid index."));
  }

  const void *column_name = sqlite3_column_name16(statement_, index);
  std::string16 retval(static_cast<const char16 *>(column_name));
  GetJsRunner()->SetReturnValue(JSPARAM_STRING16, &retval);
  RETURN_NORMAL();
}

void GearsResultSet::FieldCount() {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  // rs.fieldCount() should never throw. Return 0 if there is no statement.
  int retval = statement_ ? sqlite3_column_count(statement_) : 0;

  GetJsRunner()->SetReturnValue(JSPARAM_INT, &retval);
  RETURN_NORMAL();
}

void GearsResultSet::Close() {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  if (!Finalize()) {
    RETURN_EXCEPTION(STRING16(L"SQLite finalize() failed."));
  }

  RETURN_NORMAL();
}

void GearsResultSet::Next() {
  if (!statement_) {
    RETURN_EXCEPTION(STRING16(L"Called Next() with NULL statement."));
  }
  std::string16 error_message;
  if (!NextImpl(&error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }
  RETURN_NORMAL();
}

bool GearsResultSet::NextImpl(std::string16 *error_message) {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG
  assert(statement_);
  assert(error_message);
  int sql_status = sqlite3_step(statement_);
  LOG(("GearsResultSet::next() sqlite3_step returned %d", sql_status));
  switch (sql_status) {
    case SQLITE_ROW:
      is_valid_row_ = true;
      break;
    case SQLITE_BUSY:
      // If there was a timeout (SQLITE_BUSY) the SQL row cursor did not
      // advance, so we don't reset is_valid_row_. If it was valid prior to
      // this call, it's still valid now.
      break;
    default:
      is_valid_row_ = false;
      break;
  }
  bool succeeded = (sql_status == SQLITE_ROW) ||
                   (sql_status == SQLITE_DONE) ||
                   (sql_status == SQLITE_OK);
  if (!succeeded) {
    BuildSqliteErrorString(STRING16(L"Database operation failed."),
                           sql_status, sqlite3_db_handle(statement_),
                           error_message);
  }
  return succeeded;
}

void GearsResultSet::IsValidRow() {
  // rs.isValidRow() should never throw. Return false if there is no statement.
  bool valid = false;
  if (statement_ != NULL) {
    valid = is_valid_row_;
  }

  GetJsRunner()->SetReturnValue(JSPARAM_BOOL, &valid);
  RETURN_NORMAL();
}
