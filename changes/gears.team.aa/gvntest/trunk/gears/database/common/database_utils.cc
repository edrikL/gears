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

  int sql_status = sqlite3_open16(filename.c_str(), db);
  if (sql_status != SQLITE_OK) {
    return false; // error = "SQLite open() failed."
  }

  // Set reasonable defaults.
  sql_status = sqlite3_exec(*db,
                            "PRAGMA encoding = 'UTF-8';"
                            "PRAGMA auto_vacuum = 0;"
                            "PRAGMA cache_size = 2048;"
                            "PRAGMA page_size = 4096;"
                            "PRAGMA synchronous = NORMAL;",
                            NULL, NULL, NULL
                            );
  if (sql_status != SQLITE_OK) {
    sqlite3_close(*db);
    return false;
  }

  sql_status = sqlite3_set_authorizer(*db, ForbidAllPragmas, NULL);
  if (sql_status != SQLITE_OK) {
    sqlite3_close(*db);
    return false;
  }

  return true;
}
