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

#include "gears/base/common/paths.h"
#include "gears/base/common/common.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/database/common/database_utils.h"
#include "gears/database/npapi/database.h"

// TODO(mpcomplete): implement me
//#include "gears/database/npapi/result_set.h"
class GearsResultSet {};

#ifdef DEBUG
Stopwatch GearsDatabase::g_stopwatch_;
#endif // DEBUG

GearsDatabase::GearsDatabase() : db_(NULL) {
}

GearsDatabase::~GearsDatabase() {
  assert(result_sets_.empty());

  if (db_) {
    sqlite3_close(db_);
    db_ = NULL;
  }
}

void GearsDatabase::Open() {
  if (db_) {
    RETURN_EXCEPTION(STRING16(L"A database is already open."));
  }

  // Create an event monitor to close remaining ResultSets when the page
  // unloads.
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(),
                                             JSEVENT_UNLOAD, this));
  }

  // Get parameters.
  std::string16 database_name;
  JsArgument argv[] = {
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &database_name },
  };
  int argc = GetJsRunner()->GetArguments(ARRAYSIZE(argv), argv);

  // For now, callers cannot open DBs in other security origins.
  // To support that, parse an 'origin' argument here and call
  // IsOriginAccessAllowed (yet to be written).

  // Open the database.
  if (!OpenSqliteDatabase(database_name.c_str(), EnvPageSecurityOrigin(),
                          &db_)) {
    RETURN_EXCEPTION(STRING16(L"Couldn't open SQLite database."));
  }

  // TODO(mpcomplete): remove
  std::string16 retval = database_name + STRING16(L": success!");
  JsReturnValue js_retval = { JSPARAM_STRING16, &retval };
  GetJsRunner()->SetReturnValue(js_retval);

  RETURN_NORMAL();
}

void GearsDatabase::Execute() {
  // TODO(mpcomplete): implement me
  RETURN_NORMAL();
}

void GearsDatabase::Close() {
  if (!CloseInternal()) {
    RETURN_EXCEPTION(STRING16(L"SQLite close() failed."));
  }

  RETURN_NORMAL();
}

void GearsDatabase::GetLastInsertRowId() {
  // TODO(mpcomplete): no way to do int64 in NPAPI.. is that a problem?
  int retval = static_cast<int>(sqlite3_last_insert_rowid(db_));
  JsReturnValue js_retval = { JSPARAM_INT, &retval };
  GetJsRunner()->SetReturnValue(js_retval);

  RETURN_NORMAL();
}

void GearsDatabase::AddResultSet(GearsResultSet *rs) {
  result_sets_.insert(rs);
}

void GearsDatabase::RemoveResultSet(GearsResultSet *rs) {
  assert(result_sets_.find(rs) != result_sets_.end());

  result_sets_.erase(rs);
}


bool GearsDatabase::CloseInternal() {
  // TODO(mpcomplete): implement me
  if (db_) {
    for (std::set<GearsResultSet *>::iterator result_set = result_sets_.begin();
         result_set != result_sets_.end();
         ++result_set) {
      //(*result_set)->Finalize();
    }

    int sql_status = sqlite3_close(db_);
    db_ = NULL;
    if (sql_status != SQLITE_OK) {
      return false;
    }
  }
  return true;
}

void GearsDatabase::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);

  CloseInternal();
}

#ifdef DEBUG
void GearsDatabase::GetExecuteMsec() {
  int retval = GearsDatabase::g_stopwatch_.GetElapsed();
  JsParamToSend js_retval = { JSPARAM_INT, &retval };
  GetJsRunner()->SetReturnValue(js_retval);
  RETURN_NORMAL();
}
#endif
