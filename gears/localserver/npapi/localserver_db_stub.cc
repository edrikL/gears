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

// Temporarily forked from common/localserver_db.cc.  This is just a stub so
// things compile.

#include "gears/localserver/common/localserver_db.h"

#include "gears/base/common/http_utils.h"
#include "gears/base/common/permissions_db.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/stopwatch.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/thread_locals.h"
#include "gears/base/common/url_utils.h"

#if 0
#include "gears/localserver/common/blob_store.h"
#ifdef USE_FILE_STORE
#include "gears/localserver/common/file_store.h"
#endif
#include "gears/localserver/common/http_cookies.h"
#include "gears/localserver/common/managed_resource_store.h"
#include "gears/localserver/common/update_task.h"
#endif

const char16 *WebCacheDB::kFilename = STRING16(L"localserver.db");

// TODO(michaeln): indexes and enforce constraints

// Name of NameValueTable created to store version and browser information
const char16 *kSystemInfoTableName = STRING16(L"SystemInfo");

// Names of various other tables
const char *kServersTable = "Servers";
const char *kVersionsTable = "Versions";
const char *kEntriesTable = "Entries";
const char *kPayloadsTable = "Payloads";
const char *kResponseBodiesTable = "ResponseBodies";

// Key used to store cache instances in ThreadLocals
const std::string kThreadLocalKey("localserver:db");


static const struct {
    const char *table_name;
    const char *columns;
} kWebCacheTables[] =
    {
      { kServersTable,
        "(ServerID INTEGER PRIMARY KEY AUTOINCREMENT,"
        " Enabled INT CHECK(Enabled IN (0, 1)),"
        " SecurityOriginUrl TEXT NOT NULL,"
        " Name TEXT NOT NULL,"
        " RequiredCookie TEXT,"
        " ServerType INT CHECK(ServerType IN (0, 1)),"
        " ManifestUrl TEXT,"
        " UpdateStatus INT CHECK(UpdateStatus IN (0,1,2,3)),"
        " LastUpdateCheckTime INTEGER DEFAULT 0,"
        " ManifestDateHeader TEXT,"
        " LastErrorMessage TEXT)" },

      { kVersionsTable,
        "(VersionID INTEGER PRIMARY KEY AUTOINCREMENT,"
        " ServerID INTEGER NOT NULL,"
        " VersionString TEXT NOT NULL,"
        " ReadyState INTEGER CHECK(ReadyState IN (0, 1)),"
        " SessionRedirectUrl TEXT)" },

      { kEntriesTable,
        "(EntryID INTEGER PRIMARY KEY AUTOINCREMENT,"
        " VersionID INTEGER,"
        " Url TEXT NOT NULL,"
        " Src TEXT,"       // The manifest file entry's src attribute
        " PayloadID INTEGER,"
        " Redirect TEXT,"
        " IgnoreQuery INTEGER CHECK(IgnoreQuery IN (0, 1)))" },

      { kPayloadsTable,
        "(PayloadID INTEGER PRIMARY KEY AUTOINCREMENT,"
        " CreationDate INTEGER,"
        " Headers TEXT,"
        " StatusCode INTEGER,"
        " StatusLine TEXT)" },

      { kResponseBodiesTable,
        "(BodyID INTEGER PRIMARY KEY," // This is the same ID as the payloadID
        " FilePath TEXT,"  // With USE_FILE_STORE, bodies are stored as
        " Data BLOB)" }    // discrete files, otherwise as blobs in the DB
    };

static const int kWebCacheTableCount = ARRAYSIZE(kWebCacheTables);

/* Schema version history

  version 1: Initial version
  version 2: Added the Enabled column to the Servers table
  version 3: IE ONLY: Added the FilePath column to the Payloads table,
             switched to storing file contents in the file system
             rather than in SQLite as a BLOB. Having files on disk
             allows us to satisfies URLMON's interfaces that require
             returning a file path w/o having to create temp files.
             Also there are potential (unrealized) performance gains to be
             found around shortening the length of time the DB file is 
             locked, and performing streaming I/O into and out of the 
             cached files.
  version 4: Added LastErrorMessage column to Servers table
  version 5: Added the ResponseBodies table
  version 6: Added StatusCode and StatusLine columns to the Payloads table
             and the Redirect plus IsUserSpecifiedRedirect columns to Entries
  version 7: Removed VersionReadyState.VERSION_PENDING and all related code
  version 8: Renamed 'SessionValue' column to 'RequiredCookie'
  version 9: Added IgnoreQuery and removed IsUserSpecified columns
  version 10: Added SecurityOriginUrl and removed Domain columns
  version 11: No actual database schema change, but if an origin does not have
              PERMISSION_ALLOWED, the DB should not contain anything for that
              origin. The version was bumped to trigger an upgrade script which
              removes existing data that should not be there.
*/

// The names of values stored in the system_info table
static const char16 *kSchemaVersionName = STRING16(L"version");
static const char16 *kSchemaBrowserName = STRING16(L"browser");

// The values stored in the system_info table
const int kCurrentVersion = 11;
#if BROWSER_IE
static const char16 *kCurrentBrowser = STRING16(L"ie");
#elif BROWSER_FF
static const char16 *kCurrentBrowser = STRING16(L"firefox");
#elif BROWSER_NPAPI
static const char16 *kCurrentBrowser = STRING16(L"npapi");
#elif BROWSER_SAFARI
static const char16 *kCurrentBrowser = STRING16(L"safari");
#else
#error "BROWSER_?? not defined."
#endif


//------------------------------------------------------------------------------
// constructor
//------------------------------------------------------------------------------
WebCacheDB::WebCacheDB()
    : system_info_table_(&db_, kSystemInfoTableName), 
      response_bodies_store_(NULL) {
  // When parameter binding multiple parameters, we frequently use a scheme
  // of OR'ing return values together for testing for an error once after
  // all rv |= bind_foo() assignments have been made. This relies on
  // SQLITE_OK being 0.
  assert(SQLITE_OK == 0);
}


//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
bool WebCacheDB::Init() {
  return false;
}


//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
WebCacheDB::~WebCacheDB() {
}


//------------------------------------------------------------------------------
// Creates or upgrades the database to kCurrentVersion
//------------------------------------------------------------------------------
bool WebCacheDB::CreateOrUpgradeDatabase() {
  return false;
}


//------------------------------------------------------------------------------
// CreateDatabase
//------------------------------------------------------------------------------
bool WebCacheDB::CreateDatabase() {
  return false;
}


//------------------------------------------------------------------------------
// CreateTables
//------------------------------------------------------------------------------
bool WebCacheDB::CreateTables() {
  return false;
}

/*
Sample upgrade function
//------------------------------------------------------------------------------
// UpgradeFromXToY
//------------------------------------------------------------------------------
bool WebCacheDB::UpgradeFromXToY() {
  const char *kUpgradeCommands[] = {
      // schema upgrade statements go here
      "UPDATE SystemInfo SET value=Y WHERE name=\"version\""
  };
  const int kUpgradeCommandsCount = ARRAYSIZE(kUpgradeCommands);

  return ExecuteSqlCommandsInTransaction(kUpgradeCommands,
                                         kUpgradeCommandsCount);
}
*/

//------------------------------------------------------------------------------
// UpgradeFrom10To11
//------------------------------------------------------------------------------
bool WebCacheDB::UpgradeFrom10To11() {
  return false;
}

//------------------------------------------------------------------------------
// ExecuteSqlCommandsInTransaction
//------------------------------------------------------------------------------
bool WebCacheDB::ExecuteSqlCommandsInTransaction(const char *commands[],
                                                 int count) {
  return false;
}

//------------------------------------------------------------------------------
// ExecuteSqlCommands
//------------------------------------------------------------------------------
bool WebCacheDB::ExecuteSqlCommands(const char *commands[], int count) {
  return false;
}


//------------------------------------------------------------------------------
// CanService
//------------------------------------------------------------------------------
bool WebCacheDB::CanService(const char16 *url) {
  return false;
}


//------------------------------------------------------------------------------
// Service
//------------------------------------------------------------------------------
bool WebCacheDB::Service(const char16 *url, bool head_only,
                         PayloadInfo *payload) {
  return false;
}


//------------------------------------------------------------------------------
// Service
//------------------------------------------------------------------------------
bool WebCacheDB::Service(const char16 *url,
                         int64 *payload_id_out,
                         std::string16 *redirect_url_out) {
  return false;
}


//------------------------------------------------------------------------------
// MaybeInitiateUpdateTask
//------------------------------------------------------------------------------
static Mutex last_auto_update_lock;
typedef std::map< int64, int64 > Int64Map;
static Int64Map last_auto_update_map;

void WebCacheDB::MaybeInitiateUpdateTask(int64 server_id) {
}

//------------------------------------------------------------------------------
// InsertPayload
//------------------------------------------------------------------------------
bool WebCacheDB::InsertPayload(int64 server_id,
                               const char16 *url,
                               PayloadInfo *payload) {
  return false;
}


//------------------------------------------------------------------------------
// FindPayload
//------------------------------------------------------------------------------
bool WebCacheDB::FindPayload(int64 id,
                             PayloadInfo *payload,
                             bool info_only) {
  return false;
}


//------------------------------------------------------------------------------
// ReadPayloadInfo
//------------------------------------------------------------------------------
bool WebCacheDB::ReadPayloadInfo(SQLStatement &stmt,
                                 PayloadInfo *payload,
                                 bool info_only) {
  return false;
}

//------------------------------------------------------------------------------
// FindServer
//------------------------------------------------------------------------------
bool WebCacheDB::FindServer(int64 server_id, ServerInfo *server) {
  return false;
}


//------------------------------------------------------------------------------
// FindServer
//------------------------------------------------------------------------------
bool WebCacheDB::FindServer(const SecurityOrigin &security_origin,
                            const char16 *name,
                            const char16 *required_cookie,
                            ServerType server_type,
                            ServerInfo *server) {
  return false;
}


//------------------------------------------------------------------------------
// FindServersForOrigin
//------------------------------------------------------------------------------
bool WebCacheDB::FindServersForOrigin(const SecurityOrigin &origin,
                                      std::vector<ServerInfo> *servers) {
  return false;
}


//------------------------------------------------------------------------------
// DeleteServersForOrigin
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteServersForOrigin(const SecurityOrigin &origin) {
  return false;
}


//------------------------------------------------------------------------------
// InsertServer
//------------------------------------------------------------------------------
bool WebCacheDB::InsertServer(ServerInfo *server) {
  return false;
}

//------------------------------------------------------------------------------
// UpdateServer
//------------------------------------------------------------------------------
bool WebCacheDB::UpdateServer(int64 id,
                              const char16 *manifest_url) {
  return false;
}


//------------------------------------------------------------------------------
// UpdateServer
//------------------------------------------------------------------------------
bool WebCacheDB::UpdateServer(int64 id,
                              UpdateStatus update_status,
                              int64 last_update_check_time,
                              const char16* manifest_date_header,
                              const char16* error_message) {
  return false;
}


//------------------------------------------------------------------------------
// UpdateServer
//------------------------------------------------------------------------------
bool WebCacheDB::UpdateServer(int64 id, bool enabled) {
  return false;
}


//------------------------------------------------------------------------------
// DeleteServer
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteServer(int64 id) {
  return false;
}


//------------------------------------------------------------------------------
// FindVersion
//------------------------------------------------------------------------------
bool WebCacheDB::FindVersion(int64 server_id,
                             VersionReadyState ready_state,
                             VersionInfo *version) {
  return false;
}

//------------------------------------------------------------------------------
// FindVersion
//------------------------------------------------------------------------------
bool WebCacheDB::FindVersion(int64 server_id,
                             const char16 *version_string,
                             VersionInfo *version) {
  return false;
}


//------------------------------------------------------------------------------
// FindVersions
//------------------------------------------------------------------------------
bool WebCacheDB::FindVersions(int64 server_id,
                              std::vector<VersionInfo> *versions) {
  return false;
}

//------------------------------------------------------------------------------
// ReadServerInfo
//------------------------------------------------------------------------------
void WebCacheDB::ReadServerInfo(SQLStatement &stmt, ServerInfo *server){
}

//------------------------------------------------------------------------------
// ReadVersionInfo
//------------------------------------------------------------------------------
void WebCacheDB::ReadVersionInfo(SQLStatement &stmt, VersionInfo *version) {
}


//------------------------------------------------------------------------------
// InsertVersion
//------------------------------------------------------------------------------
bool WebCacheDB::InsertVersion(VersionInfo *version) {
  return false;
}

//------------------------------------------------------------------------------
// UpdateVersion
//------------------------------------------------------------------------------
bool WebCacheDB::UpdateVersion(int64 id,
                               VersionReadyState ready_state) {
  return false;
}

//------------------------------------------------------------------------------
// DeleteVersion
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteVersion(int64 id) {
  return false;
}

//------------------------------------------------------------------------------
// DeleteVersions
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteVersions(int64 server_id) {
  return false;
}

//------------------------------------------------------------------------------
// DeleteVersions
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteVersions(std::vector<int64> *version_ids) {
  return false;
}


//------------------------------------------------------------------------------
// InsertEntry
//------------------------------------------------------------------------------
bool WebCacheDB::InsertEntry(EntryInfo *entry) {
  return false;
}

//------------------------------------------------------------------------------
// DeleteEntry
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteEntry(int64 id) {
  return false;
}

//------------------------------------------------------------------------------
// DeleteEntries
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteEntries(int64 version_id) {
  return false;}


//------------------------------------------------------------------------------
// DeleteEntries
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteEntries(std::vector<int64> *version_ids) {
  return false;
}

//------------------------------------------------------------------------------
// FindEntry
//------------------------------------------------------------------------------
bool WebCacheDB::FindEntry(int64 version_id,
                           const char16 *url,
                           EntryInfo *entry) {
  return false;
}

//------------------------------------------------------------------------------
// FindEntriesHavingNoResponse
//------------------------------------------------------------------------------
bool WebCacheDB::FindEntriesHavingNoResponse(int64 version_id,
                                             std::vector<EntryInfo> *entries)
{
  return false;
}


//------------------------------------------------------------------------------
// ReadEntryInfo
//------------------------------------------------------------------------------
void WebCacheDB::ReadEntryInfo(SQLStatement &stmt, EntryInfo *entry) {
}


//------------------------------------------------------------------------------
// DeleteEntry
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteEntry(int64 version_id, const char16 *url) {
  return false;
}

//------------------------------------------------------------------------------
// CountEntries
//------------------------------------------------------------------------------
bool WebCacheDB::CountEntries(int64 version_id, int64 *count) {
  return false;
}

//------------------------------------------------------------------------------
// UpdateEntry
//------------------------------------------------------------------------------
bool WebCacheDB::UpdateEntry(int64 version_id,
                             const char16 *orig_url,
                             const char16 *new_url) {
  return false;
}

//------------------------------------------------------------------------------
// UpdateEntriesWithNewPayload
//------------------------------------------------------------------------------
bool WebCacheDB::UpdateEntriesWithNewPayload(int64 version_id,
                                             const char16 *url,
                                             int64 payload_id,
                                             const char16 *redirect_url) {
  return false;
}

//------------------------------------------------------------------------------
// FindMostRecentPayload
//------------------------------------------------------------------------------
bool WebCacheDB::FindMostRecentPayload(int64 server_id,
                                       const char16 *url,
                                       PayloadInfo *payload) {
  return false;
}


//------------------------------------------------------------------------------
// DeleteUnreferencedPayload
// TODO(michaeln): This is terribly inefficient. In general, deleting stale
// payloads could be done lazily. Even if done lazily, these SQL operations
// are not good. Make this better.
//------------------------------------------------------------------------------
bool WebCacheDB::DeleteUnreferencedPayloads() {
  return false;
}


#ifdef USE_FILE_STORE
//------------------------------------------------------------------------------
// Called after a top transaction has begun
//------------------------------------------------------------------------------
void WebCacheDB::OnBegin() {
}

//------------------------------------------------------------------------------
// Called after a top transaction has been commited
//------------------------------------------------------------------------------
void WebCacheDB::OnCommit() {
}

//------------------------------------------------------------------------------
// Called after a top transaction has been rolledback
//------------------------------------------------------------------------------
void WebCacheDB::OnRollback() {
}
#endif


//------------------------------------------------------------------------------
// GetDB
//------------------------------------------------------------------------------
// static
WebCacheDB *WebCacheDB::GetDB() {
  return NULL;
}


//------------------------------------------------------------------------------
// Destructor function called by ThreadLocals to dispose of a thread-specific
// DB instance when a thread dies.
//------------------------------------------------------------------------------
// static
void WebCacheDB::DestroyDB(void* pvoid) {
}


//------------------------------------------------------------------------------
// PayloadInfo::GetHeader
//------------------------------------------------------------------------------
bool WebCacheDB::PayloadInfo::GetHeader(const char16* name,
                                        std::string16 *value) {
  return false;
}

bool WebCacheDB::PayloadInfo::IsHttpRedirect() {
  // TODO(michaeln): what about other redirects:
  // 300(multiple), 303(posts), 307(temp)
  return (status_code == HttpConstants::HTTP_FOUND) ||
         (status_code == HttpConstants::HTTP_MOVED);
}

bool WebCacheDB::PayloadInfo::ConvertToHtmlRedirect(bool head_only) {
  return false;

}

void WebCacheDB::PayloadInfo::SynthesizeHtmlRedirect(const char16 *location,
                                                     bool head_only) {
}

bool 
WebCacheDB::PayloadInfo::CanonicalizeHttpRedirect(const char16 *base_url) {
  return false;
}

bool 
WebCacheDB::PayloadInfo::SynthesizeHttpRedirect(const char16 *base_url,
                                                const char16 *location) {
  return false;

}

bool WebCacheDB::PayloadInfo::PassesValidationTests() {
  return false;
}
