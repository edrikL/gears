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

#include "gears/database2/connection.h"
#include "gears/database2/statement.h"
#include "gears/base/common/exception_handler_win32.h"
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
// TODO(aa): A little ugly to include code from database1 this way. Refactor
// after integration dispatcher-based Database1 from trunk.
#include "gears/database/common/database_utils.h"

// Open filename as a SQLite database, and setup appropriately for Gears use. 
// Returns SQLITE_OK in case of success, otherwise returns the error code sqlite
// returned.  The caller must arrange to eventually call sqlite3_close() on the
// handle returned in *db even if an error is returned.
//
// Note: we *do not* set a busy timeout because in Database2 we ensure that all
// database access is serial at the API layer.
//
// TODO(aa): Refactor to share with Database1 after integration dispatcher-based
// Database1.
static int OpenAndSetupDatabase(const std::string16 &filename, sqlite3 **db) {
  assert(*db == NULL);

  int sql_status = sqlite3_open16(filename.c_str(), db);
  if (sql_status != SQLITE_OK) {
    return sql_status;
  }

  // Set reasonable defaults.
  sql_status = sqlite3_exec(*db,
                            "PRAGMA encoding = 'UTF-8';"
                            "PRAGMA auto_vacuum = 1;"
                            "PRAGMA cache_size = 2048;"
                            "PRAGMA page_size = 4096;"
                            "PRAGMA synchronous = NORMAL;",
                            NULL, NULL, NULL
                            );
  if (sql_status != SQLITE_OK) {
    return sql_status;
  }

  sql_status = sqlite3_set_authorizer(*db, ForbidAllPragmas, NULL);
  if (sql_status != SQLITE_OK) {
    return sql_status;
  }

  return SQLITE_OK;
}

bool Database2Connection::Execute(const std::string16 &statement,
                                  Database2Values *arguments,
                                  Database2RowHandlerInterface *row_handler) {
  if (!OpenIfNecessary()) return false;

  // if (bogus_version_) {
   // set error code to "version mismatch" (error code 2)
  // }
  // prepare
  // step, for each row, call row_handler->HandleRow(..);
   // if error, set error code and message, return false;
  // return true upon success
  return true;
}

bool Database2Connection::Begin() {
  if (!OpenIfNecessary()) return false;

  // execute BEGIN    
  // if error, set error code and message, return false
  // read actual_version, if doesn't match expected_version_, 
    // set bogus_version_ flag
  // return true upon success
  return true;
}

void Database2Connection::Rollback() {
  assert(handle_);

  // execute ROLLBACK
  // don't remember or handle errors
}

bool Database2Connection::Commit() {
  assert(handle_);

  // execute COMMIT
  // if error, set error code and message, return false
  // return true upon success
  return true;
}

bool Database2Connection::OpenIfNecessary() {
  if (handle_) { return true; }  // already opened

  // Setup the directory.
  std::string16 dirname;
  if (!GetDataDirectory(origin_, &dirname)) {
    error_message_ = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  // Ensure directory exists; sqlite_open does not do this.
  if (!File::RecursivelyCreateDir(dirname.c_str())) {
    error_message_ = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  std::string16 full_path(dirname);
  full_path += kPathSeparator;
  full_path += filename_;

  sqlite3 *temp_db = NULL;
  int sql_status = OpenAndSetupDatabase(full_path, &temp_db);
  if (sql_status != SQLITE_OK) {
    sql_status = SqlitePoisonIfCorrupt(temp_db, sql_status);
    if (sql_status == SQLITE_CORRUPT) {
      database2_metadata_->MarkDatabaseCorrupt(origin_, filename_);
      ExceptionManager::ReportAndContinue();
      error_message_ = GET_INTERNAL_ERROR_MESSAGE();
    } else {
      error_message_ = GET_INTERNAL_ERROR_MESSAGE();
    }

    sqlite3_close(temp_db);
    return false;
  }

  handle_ = temp_db;
  return true;
}
