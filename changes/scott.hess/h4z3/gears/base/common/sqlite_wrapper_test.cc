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
#ifdef DEBUG

#include "gears/base/common/string_utils.h"
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/sqlite_wrapper_test.h"


static bool TestSQLDatabaseTransactions();
static bool TestSQLTransaction();
static bool CreateTable(SQLDatabase &db);
static bool InsertRow(SQLDatabase &db);


//------------------------------------------------------------------------------
// TestSqliteUtilsAll
//------------------------------------------------------------------------------
bool TestSqliteUtilsAll() {
  bool ok = true;
  ok &= TestSQLDatabaseTransactions();
  ok &= TestSQLTransaction();
  return ok;
}


//------------------------------------------------------------------------------
// TestSQLDatabaseTransactions
//------------------------------------------------------------------------------
static bool TestSQLDatabaseTransactions() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestSQLDatabaseTransactions - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

  // Put something into the DB using a nested transaction
  {
    SQLDatabase db1;
    TEST_ASSERT(db1.Init(STRING16(L"SqliteUtils_test.db")));

    TEST_ASSERT(SQLITE_OK ==
      sqlite3_exec(db1.GetDBHandle(), 
        "DROP TABLE IF EXISTS test", NULL, NULL, NULL));

    TEST_ASSERT(db1.BeginTransaction());
    TEST_ASSERT(SQLITE_OK ==
      sqlite3_exec(db1.GetDBHandle(), 
        "CREATE TABLE test (val TEXT)", NULL, NULL, NULL));

    TEST_ASSERT(db1.BeginTransaction());
    TEST_ASSERT(SQLITE_OK ==
      sqlite3_exec(db1.GetDBHandle(), 
        "INSERT INTO test VALUES ('foo')", NULL, NULL, NULL));

    TEST_ASSERT(db1.CommitTransaction());
    TEST_ASSERT(db1.CommitTransaction());
  }

  // Now check that it is there
  {
    SQLDatabase db2;
    TEST_ASSERT(db2.Init(STRING16(L"SqliteUtils_test.db")));

    SQLStatement stmt;
    TEST_ASSERT(SQLITE_OK ==
      stmt.prepare16(&db2, STRING16(L"SELECT 1 FROM test WHERE val='foo'")));
    TEST_ASSERT(SQLITE_ROW == stmt.step());
    TEST_ASSERT(1 == stmt.column_int(0));
  }

  LOG(("TestSQLDatabaseTransactions - passed\n"));
  return true;
}


//------------------------------------------------------------------------------
// TestSQLTransaction
//------------------------------------------------------------------------------
static bool TestSQLTransaction() {
  // Put something into the DB using a nested transaction
  {
    SQLDatabase db1;
    TEST_ASSERT(db1.Init(STRING16(L"SqliteUtils_test.db")));

    TEST_ASSERT(SQLITE_OK == 
      sqlite3_exec(db1.GetDBHandle(), "DROP TABLE test", NULL, NULL, NULL));

    TEST_ASSERT(CreateTable(db1));
  }

  // Now verify that the table is not there
  {
    SQLDatabase db2;
    TEST_ASSERT(db2.Init(STRING16(L"SqliteUtils_test.db")));

    SQLStatement stmt;
    TEST_ASSERT(SQLITE_OK == 
      stmt.prepare16(&db2, 
        STRING16(L"SELECT 1 FROM sqlite_master WHERE tbl_name='test'")));
    TEST_ASSERT(SQLITE_DONE == stmt.step());
  }

  LOG(("TestSQLTransaction - passed\n"));
  return true;
}


//------------------------------------------------------------------------------
// CreateTable helper used by TestSQLTransaction
//------------------------------------------------------------------------------
static bool CreateTable(SQLDatabase &db) {
  SQLTransaction tx(&db);
  TEST_ASSERT(tx.Begin());

  TEST_ASSERT(SQLITE_OK == 
    sqlite3_exec(db.GetDBHandle(), 
      "CREATE TABLE test (val TEXT)", NULL, NULL, NULL));

  TEST_ASSERT(InsertRow(db));
  return true;
}


//------------------------------------------------------------------------------
// InsertRow helper used by TestSQLTransaction
//------------------------------------------------------------------------------
static bool InsertRow(SQLDatabase &db) {
  SQLTransaction tx(&db);

  TEST_ASSERT(tx.Begin());

  // There should be an error because test doesn't exist
  TEST_ASSERT(SQLITE_OK == 
    sqlite3_exec(db.GetDBHandle(), 
      "INSERT INTO test VALUES ('foo')", NULL, NULL, NULL));

  // Now roll it back
  tx.Rollback();

  return true;
}

#endif  // DEBUG
