// Copyright 2005, Google Inc.
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

#include <nsCOMPtr.h>
#include <nsDebug.h>
#include <nsICategoryManager.h>
#include <nsIFile.h>
#include <nsIProperties.h>
#include <nsIServiceManager.h>
#include <nsIURI.h>
#include <nsServiceManagerUtils.h> // for NS_IMPL_* and NS_INTERFACE_*
#include <nsXPCOM.h>
#include "gears/third_party/gecko_internal/jsapi.h"
#include "gears/third_party/gecko_internal/nsIDOMClassInfo.h" // for *_DOM_CLASSINFO
#include "gears/third_party/gecko_internal/nsIPrincipal.h"
#include "gears/third_party/gecko_internal/nsIScriptSecurityManager.h"
#include "gears/third_party/gecko_internal/nsIScriptNameSpaceManager.h"
#include "gears/third_party/gecko_internal/nsIXPConnect.h"

#include "gears/base/common/paths.h"
#include "gears/base/common/common.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/database/common/database_utils.h"
#include "gears/database/firefox/database.h"
#include "gears/database/firefox/result_set.h"

struct JSContext;

// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_ADDREF(GearsDatabase)
NS_IMPL_RELEASE(GearsDatabase)
NS_INTERFACE_MAP_BEGIN(GearsDatabase)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsDatabaseInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsDatabaseInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsDatabase)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsDatabaseClassName = "GearsDatabase";
const nsCID kGearsDatabaseClassId = {0xbb360bcc, 0xd770, 0x478e, {0xa2, 0x2f,
                                     0x01, 0x0d, 0x2d, 0xe6, 0x2b, 0x13}};
                                    // {BB360BCC-D770-478e-A22F-010D2DE62B13}


#ifdef DEBUG
Stopwatch GearsDatabase::g_stopwatch_;
#endif // DEBUG

GearsDatabase::GearsDatabase(): db_(NULL) {
}

GearsDatabase::~GearsDatabase() {
  assert(result_sets_.empty());

  if (db_) {
    sqlite3_close(db_);
    db_ = NULL;
  }
}

NS_IMETHODIMP GearsDatabase::Open(//OPTIONAL const nsAString &database_name
                                 ) {
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
  // database_name is an optional arg
  std::string16 database_name;

  JsParamFetcher js_params(this);

  if (js_params.IsOptionalParamPresent(0, false)) {
    if (!js_params.GetAsString(0, &database_name)) {
      RETURN_EXCEPTION(STRING16(L"Database name must be a string."));
    }
  }

  // For now, callers cannot open DBs in other security origins.
  // To support that, parse an 'origin' argument here and call
  // IsOriginAccessAllowed (yet to be written).

  // Open the database.
  if (!OpenSqliteDatabase(database_name.c_str(), EnvPageSecurityOrigin(),
                          &db_)) {
    RETURN_EXCEPTION(STRING16(L"Couldn't open SQLite database."));
  }

  const int kSQLiteBusyTimeout = 5000;
  sqlite3_busy_timeout(db_, kSQLiteBusyTimeout);

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsDatabase::Execute(//const nsAString &expr,
                                     //OPTIONAL nsIVariant arg_array,
                                     GearsResultSetInterface **retval) {
#ifdef DEBUG
  ScopedStopwatch scoped_stopwatch(&GearsDatabase::g_stopwatch_);
#endif // DEBUG

  int sql_status;

  *retval = NULL;  // set retval in case we exit early

  if (!db_) {
    RETURN_EXCEPTION(STRING16(L"Database handle was NULL."));
  }

  // Get parameters.
  std::string16 expr;

  JsParamFetcher js_params(this);

  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"The sql parameter is required."));
  } else if (!js_params.GetAsString(0, &expr)) {
    RETURN_EXCEPTION(STRING16(L"The sql parameter must be a string."));
  }


  // Prepare a statement for execution.

// TODO(cprince): remove #ifdef and string conversion after refactoring LOG().
#ifdef DEBUG
  std::string expr_ascii;
  String16ToUTF8(expr.c_str(), expr.length(), &expr_ascii);
  LOG(("DB Execute: %s", expr_ascii.c_str()));
#endif

  scoped_sqlite3_stmt_ptr stmt;
  sql_status = sqlite3_prepare16_v2(db_, expr.c_str(), -1, &stmt, NULL);
  if ((sql_status != SQLITE_OK) || (stmt.get() == NULL)) {
    std::string16 msg;
    BuildSqliteErrorString(STRING16(L"SQLite prepare() failed."),
                           sql_status, db_, &msg);
    msg += STRING16(L" EXPRESSION: ");
    msg += expr;
    RETURN_EXCEPTION(msg.c_str());
  }

  // Bind parameters

  int num_args_expected = sqlite3_bind_parameter_count(stmt.get());
  int num_args = 0;
  JsToken arg_array;

  if (js_params.IsOptionalParamPresent(1, false)) {
    if (!js_params.GetAsArray(1, &arg_array, &num_args)) {
      RETURN_EXCEPTION(STRING16(L"Invalid SQL parameters array."));
    }
  }

  if (num_args_expected != num_args) {
    RETURN_EXCEPTION(
      STRING16(L"Wrong number of SQL parameters."));
  }

  JsContextPtr cx = js_params.GetContextPtr();

  for (int i = 0; i < num_args_expected; ++i) {
    JsToken arg;
    int sql_index = i + 1; // sql parameters are 1-based
    js_params.GetFromArrayAsToken(arg_array, i, &arg);

    // TODO(michaeln): perhaps add cases for additional types rather than
    // using string conversion for them so sqlite is aware of the actual
    // types being bound to parameters.
    JSType type = JS_TypeOfValue(cx, arg);
    switch (type) {
      case JSTYPE_NULL:
        LOG(("        Parameter %i: null", i));
        sql_status = sqlite3_bind_null(stmt.get(), sql_index);
        break;

      // This case handles JavaScript developers passing null as a
      // substitution parameter into our Execute method. Those
      // parameters manifest themselves JSTYPE_OBJECT rather than
      // JSTYPE_NULL. We look explicitly for the string value "null",
      // which corresponds to evaluating 'String(null)' in JavaScript.
      // We interpret these values as null values in the database rather
      // than the string "null".
      case JSTYPE_OBJECT: {
        JSString *js_str = JS_ValueToString(cx, arg);
        std::string16 str(reinterpret_cast<char16 *>(JS_GetStringChars(js_str)),
                          JS_GetStringLength(js_str));
        if (str == STRING16(L"null")) {
          LOG(("        Parameter %i: null", i));
          sql_status = sqlite3_bind_null(stmt.get(), sql_index);
        } else {
// TODO(cprince): remove #ifdef and string conversion after refactoring LOG().
#ifdef DEBUG
          std::string str_ascii;
          String16ToUTF8(str.c_str(), str.length(), &str_ascii);
          LOG(("        Parameter %i: %s", i, str_ascii.c_str()));
#endif
          sql_status = sqlite3_bind_text16(stmt.get(), sql_index, str.c_str(), 
                                           -1, SQLITE_TRANSIENT);
        }
        break;
      }

      default: {
        JSString *js_str = JS_ValueToString(cx, arg);
        std::string16 str(reinterpret_cast<char16 *>(JS_GetStringChars(js_str)),
                          JS_GetStringLength(js_str));
// TODO(cprince): remove #ifdef and string conversion after refactoring LOG().
#ifdef DEBUG
          std::string str_ascii;
          String16ToUTF8(str.c_str(), str.length(), &str_ascii);
          LOG(("        Parameter %i: %s", i, str_ascii.c_str()));
#endif
        sql_status = sqlite3_bind_text16(
            stmt.get(), sql_index, str.c_str(), -1,
            SQLITE_TRANSIENT); // so SQLite copies string immediately
        break;
      }
    } // END: switch(JS_TypeOfValue(cx, t))

    if (sql_status != SQLITE_OK) {
      RETURN_EXCEPTION(STRING16(L"Could not bind arguments to expression."));
    }

  } // END: for (num_args_expected)


  // Wrap a GearsResultSet around the statement and execute it
  nsCOMPtr<GearsResultSet> rs_internal(new GearsResultSet());
  if (!rs_internal->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Initializing base class failed."));
  }

  // Note the ResultSet takes ownership of the statement
  std::string16 error_message;
  if (!rs_internal->InitializeResultSet(stmt.release(), this, &error_message)) {
    RETURN_EXCEPTION(error_message.c_str());
  }

  NS_ADDREF(*retval = rs_internal);

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsDatabase::Close() {
  if (!CloseInternal()) {
    RETURN_EXCEPTION(STRING16(L"SQLite close() failed."));
  }

  RETURN_NORMAL();
}

NS_IMETHODIMP GearsDatabase::GetLastInsertRowId(PRInt64 *retval) {
  if (!db_) {
    RETURN_EXCEPTION(STRING16(L"Database handle was NULL."));
  }

  *retval = sqlite3_last_insert_rowid(db_);
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
  if (db_) {
    for (std::set<GearsResultSet *>::iterator result_set = result_sets_.begin();
         result_set != result_sets_.end();
         ++result_set) {
      (*result_set)->Finalize();
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
NS_IMETHODIMP GearsDatabase::GetExecuteMsec(PRInt32 *retval) {
  *retval = GearsDatabase::g_stopwatch_.GetElapsed();
  RETURN_NORMAL();
}
#endif
