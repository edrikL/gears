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

#ifndef GEARS_DATABASE_COMMON_DATABASE_UTILS_H__
#define GEARS_DATABASE_COMMON_DATABASE_UTILS_H__

#include "gears/base/common/js_runner.h"
#include "gears/base/common/string16.h"
#include "gears/third_party/sqlite_google/preprocessed/sqlite3.h"

// Opens the SQLite database for the given {name, SecurityOrigin} pair.
// Creates the database (and appropriate directories) if necessary.
// Stores a pointer to the opened database in 'db'.
//
// All parameters are required, but 'name' can be the empty string.
//
// Returns 'true' if the function succeeds.
bool OpenSqliteDatabase(const char16 *name, const SecurityOrigin &origin,
                        sqlite3 **db);

bool GetColumnNames(sqlite3_stmt *statement,
                    std::vector<std::string16> *column_names,
                    std::string16 *error);

bool PopulateJsObjectFromRow(JsRunnerInterface *js_runner,
                             JsToken js_object,
                             sqlite3_stmt *statement,
                             std::vector<std::string16> *column_names,
                             std::string16 *error);

#endif // GEARS_DATABASE_COMMON_DATABASE_UTILS_H__
