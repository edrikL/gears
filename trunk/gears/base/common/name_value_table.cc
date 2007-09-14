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

#include "gears/base/common/name_value_table.h"
#include "gears/base/common/sqlite_wrapper.h"


NameValueTable::NameValueTable(SQLDatabase *db, const char16 *table_name)
    : db_(db), table_name_(table_name) {
}


bool NameValueTable::MaybeCreateTable() {
  // No datatype on Value column -- we use SQLite's "manifest typing" to store
  // values of different types in here.
  std::string16 sql = STRING16(L"CREATE TABLE IF NOT EXISTS ");
  sql += table_name_;
  sql += STRING16(L" (Name TEXT UNIQUE, Value)");

  SQLStatement statement;
  if (SQLITE_OK != statement.prepare16(db_, sql.c_str())) {
    LOG(("NameValueTable::MaybeCreateTable unable to prepare statement: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }

  if (SQLITE_DONE != statement.step()) {
    LOG(("NameValueTable::MaybeCreateTable unable to step statement: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }

  return true;
}


bool NameValueTable::SetInt(const char16 *name, int value) {
  SQLStatement statement;
  if (!PrepareSetStatement(statement, name)) {
    return false;
  }

  if (SQLITE_OK != statement.bind_int(1, value)) {
    LOG(("NameValueTable::SetInt unable to bind int: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }

  if (SQLITE_DONE != statement.step()) {
    LOG(("NameValueTable::SetInt unable to step statement: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }

  return true;
}


bool NameValueTable::GetInt(const char16* name, int *value) {
  assert(value);
  if (!value) {
    return false;
  }

  SQLStatement statement;
  if (!PrepareGetStatement(statement, name)) {
    return false;
  }

  int result = statement.step();
  if (SQLITE_ROW == result) {
    (*value) = statement.column_int(0);
    return true;
  } else if (SQLITE_DONE == result) {
    return false;
  } else {
    LOG(("NameValueTable::GetInt unable to step statement: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }
}


bool NameValueTable::SetString(const char16 *name, const char16 *value) {
  SQLStatement statement;
  if (!PrepareSetStatement(statement, name)) {
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(1, value)) {
    LOG(("NameValueTable::SetString unable to bind int: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }

  if (SQLITE_DONE != statement.step()) {
    LOG(("NameValueTable::SetString unable to step statement: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }

  return true;
}


bool NameValueTable::GetString(const char16 *name, std::string16 *value) {
  assert(value);
  if (!value) {
    return false;
  }

  SQLStatement statement;
  if (!PrepareGetStatement(statement, name)) {
    return false;
  }

  int result = statement.step();
  if (SQLITE_ROW == result) {
    (*value) = statement.column_text16(0);
    return true;
  } else if (SQLITE_DONE == result) {
    return false;
  } else {
    LOG(("NameValueTable::GetString unable to step statement: %d\n",
      sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }
}


bool NameValueTable::HasName(const char16 *name, bool *retval) {
  assert(name);
  if (!name) {
    return false;
  }

  const char16 *sql_prefix = STRING16(L"SELECT 1 FROM ");
  const char16 *sql_suffix = STRING16(L" WHERE Name = ?");

  SQLStatement statement;
  if (!PrepareStatement(statement, sql_prefix, sql_suffix, name)) {
    return false;
  }

  int result = statement.step();
  if (SQLITE_ROW == result) {
    *retval = true;
    return true;
  } else if (SQLITE_DONE == result) {
    *retval = false;
    return true;
  } else {
    LOG(("NameValueTable::HasName unable to step statement: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }
}


bool NameValueTable::Clear(const char16 *name) {
  assert(name);
  if (!name) {
    return false;
  }

  const char16 *sql_prefix = STRING16(L"DELETE FROM ");
  const char16 *sql_suffix = STRING16(L" WHERE Name = ?");

  SQLStatement statement;
  if (!PrepareStatement(statement, sql_prefix, sql_suffix, name)) {
    return false;
  }

  if (SQLITE_DONE != statement.step()) {
    LOG(("NameValueTable::Clear unable to step: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }

  return true;
}


bool NameValueTable::PrepareStatement(SQLStatement &statement,
                                      const char16 *prefix,
                                      const char16 *suffix,
                                      const char16 *name) {
  assert(prefix);
  assert(suffix);
  assert(name);
  if (!prefix || !suffix || !name) {
    return false;
  }

  std::string16 sql(prefix);
  sql += table_name_;
  sql += suffix;

  if (SQLITE_OK != statement.prepare16(db_, sql.c_str())) {
    LOG(("NameValueTable::PrepareStatement unable to prepare statement: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(0, name)) {
    LOG(("NameValueTable::PrepareStatement unable to bind name: %d\n",
         sqlite3_errcode(db_->GetDBHandle())));
    return false;
  }

  return true;
}
