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
#include "gears/localserver/common/localserver_db.h"

static const char16 *kDatabaseName = STRING16(L"permissions.db");
static const char16 *kVersionTableName = STRING16(L"VersionInfo");
static const char16 *kVersionKeyName = STRING16(L"Version");
static const char16 *kAccessTableName = STRING16(L"Access");
static const int kCurrentVersion = 2;
static const int kOldestUpgradeableVersion = 1;


const std::string PermissionsDB::kThreadLocalKey("base:permissions");


PermissionsDB *PermissionsDB::GetDB() {
  if (ThreadLocals::HasValue(kThreadLocalKey)) {
    return reinterpret_cast<PermissionsDB*>(
        ThreadLocals::GetValue(kThreadLocalKey));
  }

  PermissionsDB *db = new PermissionsDB();

  // If we can't initialize, we store NULL in the map so that we don't keep
  // trying to Init() over and over.
  if (!db->Init()) {
    delete db;
    db = NULL;
  }

  ThreadLocals::SetValue(kThreadLocalKey, db, &DestroyDB);
  return db;
}


void PermissionsDB::DestroyDB(void *context) {
  PermissionsDB *db = reinterpret_cast<PermissionsDB*>(context);
  if (db) {
    delete db;
  }
}


PermissionsDB::PermissionsDB()
    : version_table_(&db_, kVersionTableName),
      access_table_(&db_, kAccessTableName),
      shortcut_table_(&db_) {
}


bool PermissionsDB::Init() {
  // Initialize the database and tables
  if (!db_.Open(kDatabaseName)) {
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

  // Doing this in a transaction effectively locks the database file and
  // ensures that this is synchronized across all threads and processes
  SQLTransaction transaction(&db_, "PermissionsDB::Init");
  if (!transaction.Begin()) {
    return false;
  }

  // Fetch the version again in case someone else beat us to the
  // upgrade.
  version_table_.GetInt(kVersionKeyName, &version);
  if (version == kCurrentVersion) {
    return true;
  }

  if (0 == version) {
    // No database in place, create it.
    //
    // TODO(shess) Verify that this is true.  Is it _no_ database, or
    // is there a database which didn't have a version?  The latter
    // case would be masked by the CREATE IF NOT EXISTS statements
    // we're using.
    if (!CreateDatabase()) {
      return false;
    }
  } else {
    if (!UpgradeToVersion2()) {
      return false;
    }
  }

  // Double-check that we ended up with the right version.
  version_table_.GetInt(kVersionKeyName, &version);
  if (version != kCurrentVersion) {
    return false;
  }

  return transaction.Commit();
}


PermissionsDB::PermissionValue PermissionsDB::GetCanAccessGears(
                                   const SecurityOrigin &origin) {
  int retval_int = PERMISSION_DEFAULT;
  access_table_.GetInt(origin.url().c_str(), &retval_int);
  return static_cast<PermissionsDB::PermissionValue>(retval_int);
}


void PermissionsDB::SetCanAccessGears(const SecurityOrigin &origin,
                                      PermissionsDB::PermissionValue value) {
  if (origin.url().empty()) {
    assert(false);
    return;
  }

  if (value == PERMISSION_DEFAULT) {
    access_table_.Clear(origin.url().c_str());
  } else if (value == PERMISSION_ALLOWED || value == PERMISSION_DENIED) {
    access_table_.SetInt(origin.url().c_str(), value);
  } else {
    LOG(("PermissionsDB::SetCanAccessGears invalid value: %d", value));
    assert(false);
  }

  if (value == PERMISSION_DENIED || value == PERMISSION_DEFAULT) {
    WebCacheDB *webcacheDB = WebCacheDB::GetDB();
    if (webcacheDB) {
      webcacheDB->DeleteServersForOrigin(origin);
    }
  }
}


bool PermissionsDB::GetOriginsByValue(PermissionsDB::PermissionValue value,
                                      std::vector<SecurityOrigin> *result) {
  if (PERMISSION_ALLOWED != value && PERMISSION_DENIED != value) {
    LOG(("Unexpected value: %d", value));
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

  if (SQLITE_OK != statement.bind_int(0, value)) {
    return false;
  }

  int rv;
  while (SQLITE_DONE != (rv = statement.step())) {
    if (SQLITE_ROW != rv) {
      LOG(("PermissionsDB::ListGearsAccess: Could not iterate. Error was: %d",
           db_.GetErrorCode()));
      return false;
    }

    SecurityOrigin origin;
    if (!origin.InitFromUrl(statement.column_text16_safe(0))) {
      LOG(("PermissionsDB::ListGearsAccess: InitFromUrl() failed."));
      // If we can't initialize a single URL, don't freak out. Try to do the
      // other ones.
      continue;
    }
    result->push_back(origin);
  }

  return true;
}


bool PermissionsDB::EnableGearsForWorker(const SecurityOrigin &origin) {
  SQLTransaction transaction(&db_, "PermissionsDB::EnableGearsForWorker");
  if (!transaction.Begin()) {
    return false;
  }

  switch (GetCanAccessGears(origin)) {
    case PERMISSION_ALLOWED:
      return true;
    case PERMISSION_DENIED:
      return false;
    case PERMISSION_DEFAULT:
      if (!access_table_.SetInt(origin.url().c_str(), PERMISSION_ALLOWED)) {
        return false;
      }
      return transaction.Commit();
    default:
      LOG(("Unexpected permission value"));
      return false;
  }
}

bool PermissionsDB::SetShortcut(const SecurityOrigin &origin,
                                const char16 *name, const char16 *app_url,
                                const std::vector<std::string16> &icon_urls,
                                const char16 *msg) {
  return shortcut_table_.SetShortcut(origin.url().c_str(), name,
                                     app_url, icon_urls, msg);
}

bool PermissionsDB::GetOriginsWithShortcuts(
    std::vector<SecurityOrigin> *result) {

  std::vector<std::string16> origin_urls;
  if (!shortcut_table_.GetOriginsWithShortcuts(&origin_urls)) {
    return false;
  }

  for (size_t ii = 0; ii < origin_urls.size(); ++ii) {
    SecurityOrigin origin;
    if (!origin.InitFromUrl(origin_urls[ii].c_str())) {
      LOG(("PermissionsDB::GetOriginsWithShortcuts: InitFromUrl() failed."));
      // If we can't initialize a single URL, don't freak out. Try to do the
      // other ones.
      continue;
    }
    result->push_back(origin);
  }
  return true;
}

bool PermissionsDB::GetOriginShortcuts(const SecurityOrigin &origin,
                                       std::vector<std::string16> *names) {
  return shortcut_table_.GetOriginShortcuts(origin.url().c_str(), names);
}

bool PermissionsDB::GetShortcut(const SecurityOrigin &origin,
                                const char16 *name, std::string16 *app_url,
                                std::vector<std::string16> *icon_urls,
                                std::string16 *msg) {
  return shortcut_table_.GetShortcut(origin.url().c_str(), name,
                                     app_url, icon_urls, msg);
}

bool PermissionsDB::DeleteShortcut(const SecurityOrigin &origin,
                                   const char16 *name) {
  return shortcut_table_.DeleteShortcut(origin.url().c_str(), name);
}

bool PermissionsDB::DeleteShortcuts(const SecurityOrigin &origin) {
  return shortcut_table_.DeleteShortcuts(origin.url().c_str());
}

bool PermissionsDB::CreateDatabase() {
  ASSERT_SINGLE_THREAD();

  SQLTransaction transaction(&db_, "PermissionsDB::CreateDatabase");
  if (!transaction.Begin()) {
    return false;
  }

  if (!db_.DropAllObjects()) {
    return false;
  }

  if (!version_table_.MaybeCreateTable() ||
      !access_table_.MaybeCreateTable()) {
    return false;
  }

  // set the current version
  if (!version_table_.SetInt(kVersionKeyName, kCurrentVersion)) {
    return false;
  }

  return transaction.Commit();
}

bool PermissionsDB::UpgradeToVersion2() {
  SQLTransaction transaction(&db_, "PermissionsDB::UpgradeToVersion2");
  if (!transaction.Begin()) {
    return false;
  }

  int version = 0;
  version_table_.GetInt(kVersionKeyName, &version);

  if (version != 1) {
    LOG(("PermissionsDB::UpgradeToVersion2 unexpected version: %d", version));
    return false;
  }

  // There was a bug in v1 of this db where we inserted some corrupt UTF-8
  // characters into the db. This was pre-release, so it's not worth trying
  // to clean it up. Instead just remove old permissions.
  //
  // TODO(shess) I'm inclined to say "DROP TABLE IF EXISTS
  // ScourAccess".  Or, since this was from a pre-release schema,
  // "upgrade" version 1 by calling CreateDatabase(), which will drop
  // all existing tables.
  if (SQLITE_OK != db_.Execute("DELETE FROM ScourAccess")) {
    return false;
  }

  if (!version_table_.SetInt(kVersionKeyName, 2)) {
    return false;
  }

  return transaction.Commit();
}

bool PermissionsDB::UpgradeToVersion3() {
  SQLTransaction transaction(&db_, "PermissionsDB::UpgradeToVersion3");
  if (!transaction.Begin()) {
    return false;
  }

  int version = 0;
  version_table_.GetInt(kVersionKeyName, &version);

  if (version < 2) {
    if (!UpgradeToVersion2()) {
      return false;
    }
    version_table_.GetInt(kVersionKeyName, &version);
  }

  if (version != 2) {
    LOG(("PermissionsDB::UpgradeToVersion3 unexpected version: %d", version));
    return false;
  }

  if (!shortcut_table_.UpgradeToVersion3()) {
    return false;
  }

  if (!version_table_.SetInt(kVersionKeyName, 3)) {
    return false;
  }

  return transaction.Commit();
}

bool PermissionsDB::UpgradeToVersion4() {
  SQLTransaction transaction(&db_, "PermissionsDB::UpgradeToVersion4");
  if (!transaction.Begin()) {
    return false;
  }

  int version = 0;
  version_table_.GetInt(kVersionKeyName, &version);

  if (version < 3) {
    if (!UpgradeToVersion3()) {
      return false;
    }
    version_table_.GetInt(kVersionKeyName, &version);
  }

  if (version != 3) {
    LOG(("PermissionsDB::UpgradeToVersion4 unexpected version: %d", version));
    return false;
  }

  if (!shortcut_table_.UpgradeFromVersion3ToVersion4()) {
    return false;
  }

  if (!version_table_.SetInt(kVersionKeyName, 4)) {
    return false;
  }

  return transaction.Commit();
}
