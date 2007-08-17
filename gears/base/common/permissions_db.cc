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

#include "gears/base/common/permissions_db.h"
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/thread_locals.h"

static const char16 *kDatabaseName = STRING16(L"permissions.db");
static const char16 *kVersionTableName = STRING16(L"VersionInfo");
static const char16 *kVersionKeyName = STRING16(L"Version");
static const char16 *kAccessTableName = STRING16(L"Access");
static const int kCurrentVersion = 2;
static const int kOldestUpgradeableVersion = 1;


const std::string CapabilitiesDB::kThreadLocalKey("base:permissions");


CapabilitiesDB *CapabilitiesDB::GetDB() {
  if (ThreadLocals::HasValue(kThreadLocalKey)) {
    return reinterpret_cast<CapabilitiesDB*>(
        ThreadLocals::GetValue(kThreadLocalKey));
  }

  CapabilitiesDB *db = new CapabilitiesDB();

  // If we can't initialize, we store NULL in the map so that we don't keep
  // trying to Init() over and over.
  if (!db->Init()) {
    delete db;
    db = NULL;
  }

  ThreadLocals::SetValue(kThreadLocalKey, db, &DestroyDB);
  return db;
}


void CapabilitiesDB::DestroyDB(void *context) {
  CapabilitiesDB *db = reinterpret_cast<CapabilitiesDB*>(context);
  if (db) {
    delete db;
  }
}


CapabilitiesDB::CapabilitiesDB()
    : version_table_(&db_, kVersionTableName),
      access_table_(&db_, kAccessTableName) {
}


bool CapabilitiesDB::Init() {
  // Initialize the database and tables
  if (!db_.Init(kDatabaseName)) {
    return false;
  }

  // Examine the contents of the database and determine if we have to
  // instantiate or updgrade the schema.
  int version = 0;
  version_table_.GetInt(kVersionKeyName, &version);

  // if its the version we're expecting, great
  if (version == kCurrentVersion) {
    return true;
  }

  // We have to either create or upgrade the database
  if (!CreateOrUpgradeDatabase()) {
    return false;
  }

  return true;
}


const CapabilitiesDB::CapabilityStatus CapabilitiesDB::GetCanAccessGears(
    const SecurityOrigin &origin) {
  int result = 0;
  if (access_table_.GetInt(origin.url().c_str(), &result)) {
    return static_cast<CapabilityStatus>(result);
  } else {
    return CAPABILITY_DEFAULT;
  }
}


void CapabilitiesDB::SetCanAccessGears(
    const SecurityOrigin &origin,
    CapabilitiesDB::CapabilityStatus status) {
  if (origin.url().empty()) {
    assert(false);
    return;
  }

  if (status == CAPABILITY_DEFAULT) {
    access_table_.Clear(origin.url().c_str());
  } else if (status == CAPABILITY_ALLOWED || status == CAPABILITY_DENIED) {
    access_table_.SetInt(origin.url().c_str(), status);
  } else {
    LOG(("CapabilitiesDB::SetCanAccessGears invalid status: %d", status));
    assert(false);
  }
}


bool CapabilitiesDB::GetOriginsByStatus(
    CapabilitiesDB::CapabilityStatus status,
    std::vector<SecurityOrigin> *result) {
  if (CAPABILITY_ALLOWED != status &&
      CAPABILITY_DENIED != status) {
    LOG(("Unexpected status: %d", status));
    return false;
  }

  // TODO(aa): Refactor into NameValueTable::FindNamesByIntValue().
  std::string16 sql(STRING16(L"SELECT Name FROM "));
  sql += kAccessTableName;
  sql += STRING16(L" WHERE Value = ? ORDER BY Name ASC");

  SQLStatement statement;
  if (SQLITE_OK != statement.prepare16(&db_, sql.c_str())) {
    return false;
  }

  if (SQLITE_OK != statement.bind_int(0, status)) {
    return false;
  }

  int rv;
  while (SQLITE_DONE != (rv = statement.step())) {
    if (SQLITE_ROW != rv) {
      LOG(("CapabilitiesDB::ListGearsAccess: Could not iterate. Error was: %d",
           sqlite3_errcode(db_.GetDBHandle())));
      return false;
    }

    SecurityOrigin origin;
    if (!origin.InitFromUrl(statement.column_text16_safe(0))) {
      LOG(("CapabilitiesDB::ListGearsAccess: InitFromUrl() failed."));
      // If we can't initialize a single URL, don't freak out. Try to do the
      // other ones.
      continue;
    }
    result->push_back(origin);
  }

  return true;
}


bool CapabilitiesDB::CreateOrUpgradeDatabase() {
  // Doing this in a transaction effectively locks the database file and
  // ensures that this is synchronized across all threads and processes
  SQLTransaction transaction(&db_);
  if (!transaction.Begin()) {
    return false;
  }

  // Now that we have locked the database, fetch the version again
  int version = 0;
  version_table_.GetInt(kVersionKeyName, &version);

  if (version == kCurrentVersion) {
    // some other thread/process has performed the create or upgrade already
    return true;
  }

  if (0 == version) {
    // There is no database. Create one.
    if (!CreateDatabase()) {
      return false;
    }
  } else if (version < kOldestUpgradeableVersion || version > kCurrentVersion) {
    LOG(("Unexpected version: %d", version));
    return false;
  } else {
    // We only have one historical version at this time.
    assert(version == 1);
    if (!UpgradeFromVersion1ToVersion2()) {
      return false;
    }
  }

  if (!transaction.Commit()) {
    return false;
  }

  return true;
}


bool CapabilitiesDB::CreateDatabase() {
  ASSERT_SINGLE_THREAD();

  SQLTransaction transaction(&db_);
  if (!transaction.Begin()) {
    return false;
  }

  if (!db_.DropAllObjects()) {
    return false;
  }

  // Create our two tables
  if (!version_table_.MaybeCreateTable() ||
      !access_table_.MaybeCreateTable()) {
    return false;
  }

  // set the current version
  if (!version_table_.SetInt(kVersionKeyName, kCurrentVersion)) {
    return false;
  }

  if (!transaction.Commit()) {
    return false;
  }

  return true;
}


bool CapabilitiesDB::UpgradeFromVersion1ToVersion2() {
  SQLTransaction transaction(&db_);
  if (!transaction.Begin()) {
    return false;
  }

  // There was a bug in v1 of this db where we inserted some corrupt UTF-8
  // characters into the db. This was pre-release, so it's not worth trying
  // to clean it up. Instead just remove old permissions.
  int rv = sqlite3_exec(db_.GetDBHandle(), "DELETE FROM ScourAccess", NULL,
                        NULL, NULL);

  if (SQLITE_OK != rv) {
    return false;
  }

  if (!version_table_.SetInt(kVersionKeyName, 2)) {
    return false;
  }

  return transaction.Commit();
}
