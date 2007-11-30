// Copyright 2006, Google Inc.
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

#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h" // for IsStringValidPathComponent()
#include "gears/database/common/database_utils.h"

// NOTE(shess) This may interact with various SQLite features.  For
// instance, VACUUM is implemented in terms of more basic SQLite
// features, such as PRAGMA (or ATTACH, which Gears also disables).
static int ForbidAllPragmas(void *userData, int iType,
                            const char *zPragma, const char *zArg,
                            const char *zDatabase, const char *zView) {
  if (iType == SQLITE_PRAGMA) {
    // TODO(shess): Could test zPragma for the specific pragma being
    // accessed, and allow some of them.  Could allow zArg==NULL
    // through to enable read-only access.  Deny-all for now to leave
    // open the possibility of compiling the pragma code out entirely.
    return SQLITE_DENY;
  }
  return SQLITE_OK;
}

bool OpenSqliteDatabase(const char16 *name, const SecurityOrigin &origin,
                        sqlite3 **db) {
  std::string16 filename;

  // Setup the directory.
  if (!GetDataDirectory(origin, &filename)) { return false; }

  // Ensure directory exists; sqlite_open does not do this.
  if (!File::RecursivelyCreateDir(filename.c_str())) { return false; }

  // Open the SQLite database.
  if (!AppendDataName(name, kDataSuffixForDatabase, &filename)) {
    return false;
  }

  sqlite3 *temp_db = NULL;
  int sql_status = sqlite3_open16(filename.c_str(), &temp_db);
  if (sql_status != SQLITE_OK) {
    // sqlite3_close() should be called after sqlite3_open() failures.
    // The DB handle may be valid or NULL, sqlite3_close() handles
    // either.
    sqlite3_close(temp_db);
    return false; // error = "SQLite open() failed."
  }

  // Set the busy timeout value before executing any SQL statements.
  // With the timeout value set, SQLite will wait and retry if another
  // thread has the database locked rather than immediately fail with an
  // SQLITE_BUSY error.
  const int kSQLiteBusyTimeout = 5000;
  sqlite3_busy_timeout(temp_db, kSQLiteBusyTimeout);

  // Set reasonable defaults.
  sql_status = sqlite3_exec(temp_db,
                            "PRAGMA encoding = 'UTF-8';"
                            "PRAGMA auto_vacuum = 1;"
                            "PRAGMA cache_size = 2048;"
                            "PRAGMA page_size = 4096;"
                            "PRAGMA synchronous = NORMAL;",
                            NULL, NULL, NULL
                            );
  if (sql_status != SQLITE_OK) {
    sqlite3_close(temp_db);
    return false;
  }

  sql_status = sqlite3_set_authorizer(temp_db, ForbidAllPragmas, NULL);
  if (sql_status != SQLITE_OK) {
    sqlite3_close(temp_db);
    return false;
  }

  *db = temp_db;
  return true;
}

bool GetColumnNames(sqlite3_stmt *statement,
                    std::vector<std::string16> *column_names,
                    std::string16 *error) {
  int num_columns = sqlite3_column_count(statement);

  column_names->clear();
  for (int i = 0; i < num_columns; ++i) {
    const char16 *column_name = static_cast<const char16 *>(
                                    sqlite3_column_name16(statement, i));
    if (!column_name) {
      *error = STRING16(L"Could not get column name for column '");
      *error += IntegerToString16(i);
      *error += STRING16(L"'.");
      return false;
    }
    column_names->push_back(column_name);
  }

  return true;
}

bool PopulateJsObjectFromRow(JsRunnerInterface *js_runner,
                             JsToken js_object,
                             sqlite3_stmt *statement,
                             std::vector<std::string16> *column_names,
                             std::string16 *error) {
  for (size_t i = 0; i < column_names->size(); ++i) {
    // NOTE: we cannot cache the first row's column types because SQLite does
    // not guarantee that column types are consistent across rows.
    int column_type = sqlite3_column_type(statement, i);
    const char16 *column_name = (*column_names)[i].c_str();
    bool success = false;

    switch (column_type) {
      case SQLITE_INTEGER: {
        int64 val = sqlite3_column_int64(statement, i);
        // Even though SetPropertyInt64 detects this and fails, check here so
        // we give a useful error message.
        if (val < JS_INT_MIN || val > JS_INT_MAX) {
          *error = STRING16(L"Integer value for column '");
          *error += column_name;
          *error += STRING16(L"' is outside of supported range.");
          return false;
        }
        success = js_runner->SetPropertyInt64(js_object, column_name, val);
        break;
      }
      case SQLITE_FLOAT:
        success = js_runner->SetPropertyDouble(js_object, column_name,
                                sqlite3_column_double(statement, i));
        break;
      case SQLITE_TEXT: {
        const char16 *value = static_cast<const char16 *>(
                                  sqlite3_column_text16(statement, i));
        success = js_runner->SetPropertyString(js_object, column_name, value);
        break;
      }
      case SQLITE_NULL:
        success = js_runner->SetPropertyNull(js_object, column_name);
        break;
      default:
        *error = STRING16(L"Unsupported datatype for column '");
        *error += column_name;
        *error += STRING16(L"'.");
        return false;
    }

    if (!success) {
      *error = STRING16(L"Internal error setting property for column '");
      *error += column_name;
      *error += STRING16(L"'.");
      return false;
    }
  }

  return true;
}
