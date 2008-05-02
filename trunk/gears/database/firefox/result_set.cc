// Copyright 2005, Google Inc.
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

#include <gecko_sdk/include/nsXPCOM.h>
#include <gecko_sdk/include/nsICategoryManager.h>
#include <gecko_internal/nsIDOMClassInfo.h>
#include <gecko_internal/nsIVariant.h>
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/stopwatch.h"
#include "gears/database/common/database_utils.h"
#include "third_party/sqlite_google/preprocessed/sqlite3.h"

#include "gears/database/firefox/database.h"
#include "gears/database/firefox/result_set.h"


// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_ADDREF(GearsResultSet)
NS_IMPL_RELEASE(GearsResultSet)
NS_INTERFACE_MAP_BEGIN(GearsResultSet)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsResultSetInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsResultSetInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsResultSet)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsResultSetClassName = "GearsResultSet";
const nsCID kGearsResultSetClassId = {0x94e65f73, 0x63d1, 0x443d, {0xa8, 0xa8,
                                      0xb4, 0x7b, 0xae, 0x92, 0x25, 0x1c}};
                                     // {94E65F73-63D1-443d-A8A8-B47BAE92251C}


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

bool GearsResultSet::Finalize() {
  if (statement_) {
    sqlite3 *db = sqlite3_db_handle(statement_);
    int sql_status = sqlite3_finalize(statement_);
    sql_status = SqlitePoisonIfCorrupt(db, sql_status);
    statement_ = NULL;

    LOG(("DB ResultSet Close: %d", sql_status));

    if (sql_status != SQLITE_OK) {
      return false;
    }
  }
  return true;
}

NS_IMETHODIMP GearsResultSet::Field(//PRInt32 index
                                    nsIVariant **retval) {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  JsParamFetcher js_params(this);

  int index;
  if (!js_params.GetAsInt(0, &index)) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  if (statement_ == NULL) {
    RETURN_EXCEPTION(STRING16(L"SQL statement is NULL."));
  }

  if ((index < 0) || (index >= sqlite3_column_count(statement_))) {
    RETURN_EXCEPTION(STRING16(L"Invalid index."));
  }

  std::string16 error_message;
  if (!FieldImpl(&js_params, index, &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }
  RETURN_NORMAL();
}

bool GearsResultSet::FieldImpl(JsParamFetcher *js_params, int index,
                               std::string16 *error_message) {
  assert(statement_ != NULL);

  JsContextPtr js_context = js_params->GetContextPtr();
  JsScopedToken token;

  int column_type = sqlite3_column_type(statement_, index);
  switch (column_type) {
    case SQLITE_INTEGER: {
      sqlite_int64 i64 = sqlite3_column_int64(statement_, index);
      // Warning: the extra check here is because the SpiderMonkey jsval int
      // is essentially only 31 bits, giving less range than INT_MIN..INT_MAX.
      // TODO(cprince): Hide this logic in a shared SetInt(int64) function.
      if ((i64 >= INT_MIN) && (i64 <= INT_MAX) && INT_FITS_IN_JSVAL(i64)) {
        // return an integer
        if (!IntToJsToken(js_context, static_cast<int>(i64), &token)) {
          *error_message = GET_INTERNAL_ERROR_MESSAGE();
          return false;
        }
      } else if ((i64 >= JS_INT_MIN) && (i64 <= JS_INT_MAX)) {
        // return a double, using the mantissa
        if (!DoubleToJsToken(js_context, static_cast<double>(i64), &token)) {
          *error_message = GET_INTERNAL_ERROR_MESSAGE();
          return false;
        }
      } else {
        *error_message = STRING16(L"Integer value is out of range.");
        return false;
      }
      break;
    }
    case SQLITE_FLOAT: {
      double dbl = sqlite3_column_double(statement_, index);
      if (!DoubleToJsToken(js_context, dbl, &token)) {
        *error_message = GET_INTERNAL_ERROR_MESSAGE();
        return false;
      }
      break;
    }
    case SQLITE_TEXT: {
      const char16 *text = static_cast<const char16 *>(
                               sqlite3_column_text16(statement_, index));
      if (!StringToJsToken(js_context, text, &token)) {
        *error_message = GET_INTERNAL_ERROR_MESSAGE();
        return false;
      }
      break;
    }
    case SQLITE_NULL:
      if (!NullToJsToken(js_context, &token)) {
        *error_message = GET_INTERNAL_ERROR_MESSAGE();
        return false;
      }
      break;
    case SQLITE_BLOB:
      // TODO(miket): figure out right way to pass around blobs in variants.
      *error_message = STRING16(L"Data type not supported.");
      return false;
    default:
      *error_message = STRING16(L"Data type not supported.");
      return false;
  }

  js_params->SetReturnValue(token);
  return true;
}

NS_IMETHODIMP GearsResultSet::FieldByName(//const nsAString &field_name
                                          nsIVariant **retval) {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  JsParamFetcher js_params(this);

  std::string16 field_name;
  if (!js_params.GetAsString(0, &field_name)) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  if (statement_ == NULL) {
    RETURN_EXCEPTION(STRING16(L"SQL statement is NULL."));
  }

  // TODO(miket): This is horrible O(n) code but we didn't have a hashtable
  // implementation handy. Fix this!
  int n = sqlite3_column_count(statement_);
  int index;
  for (index = 0; index < n; ++index) {
    const void *column_name = sqlite3_column_name16(statement_, index);
    nsDependentString s(static_cast<const PRUnichar *>(column_name));
    if (field_name == s.get()) {
      break; // found it
    }
  }
  if (index >= n) {
    RETURN_EXCEPTION(STRING16(L"Field name not found."));
  }

  std::string16 error_message;
  if (!FieldImpl(&js_params, index, &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsResultSet::FieldName(PRInt32 index, nsAString &retval) {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  if (statement_ == NULL) {
    RETURN_EXCEPTION(STRING16(L"SQL statement is NULL."));
  }
  if ((index < 0) || (index >= sqlite3_column_count(statement_))) {
    RETURN_EXCEPTION(STRING16(L"Invalid index."));
  }

  const void *column_name = sqlite3_column_name16(statement_, index);
  retval.Assign((nsString::char_type *)column_name);
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsResultSet::FieldCount(PRInt32 *retval) {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  // rs.fieldCount() should never throw. Return 0 if there is no statement.
  if (statement_ == NULL) {
    *retval = 0;
  } else {
    *retval = sqlite3_column_count(statement_);
  }
  RETURN_NORMAL();
}

NS_IMETHODIMP GearsResultSet::Close() {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  if (!Finalize()) {
    RETURN_EXCEPTION(STRING16(L"SQLite finalize() failed."));
  }

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsResultSet::Next() {
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
  sql_status = SqlitePoisonIfCorrupt(sqlite3_db_handle(statement_),
                                     sql_status);
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

NS_IMETHODIMP GearsResultSet::IsValidRow(PRBool *retval) {
  // rs.isValidRow() should never throw. Return false if there is no statement.
  bool valid = false;
  if (statement_ != NULL) {
    valid = is_valid_row_;
  }
  *retval = (valid ? PR_TRUE : PR_FALSE);
  RETURN_NORMAL();
}
