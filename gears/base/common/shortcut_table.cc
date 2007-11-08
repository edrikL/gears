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

#include "gears/base/common/shortcut_table.h"

ShortcutTable::ShortcutTable(SQLDatabase *db)
    : db_(db) {
}

bool ShortcutTable::MaybeCreateTable() {
  const char *sql =
      "CREATE TABLE IF NOT EXISTS Shortcut ("
      " Origin TEXT, Name TEXT, "
      " AppUrl TEXT, IcoUrl TEXT, Msg TEXT, "
      " PRIMARY KEY (Origin, Name)"
      ")";

  if (SQLITE_OK != db_->Execute(sql)) {
    LOG(("ShortcutTable::MaybeCreateTable create "
         "unable to execute: %d", db_->GetErrorCode()));
    return false;
  }

  return true;
}

bool ShortcutTable::SetShortcut(const char16 *origin, const char16 *name,
                                const char16 *app_url, const char16 *ico_url,
                                const char16 *msg) {
  const char16 *sql = STRING16(L"INSERT OR REPLACE INTO Shortcut "
                               L"(Origin, Name, AppUrl, IcoUrl, Msg) "
                               L"VALUES (?, ?, ?, ?, ?)");

  SQLStatement statement;
  if (SQLITE_OK != statement.prepare16(db_, sql)) {
    LOG(("ShortcutTable::SetShortcut unable to prepare: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(0, origin)) {
    LOG(("ShortcutTable::SetShortcut unable to bind origin: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(1, name)) {
    LOG(("ShortcutTable::SetShortcut unable to bind name: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(2, app_url)) {
    LOG(("ShortcutTable::SetShortcut unable to bind app_url: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(3, ico_url)) {
    LOG(("ShortcutTable::SetShortcut unable to bind ico_url: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(4, msg)) {
    LOG(("ShortcutTable::SetShortcut unable to bind msg: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_DONE != statement.step()) {
    LOG(("ShortcutTable::SetShortcut unable to step: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  return true;
}

bool ShortcutTable::
GetOriginsWithShortcuts(std::vector<std::string16> *result) {
  const char16 *sql = STRING16(L"SELECT DISTINCT(Origin) FROM Shortcut");

  SQLStatement statement;
  if (SQLITE_OK != statement.prepare16(db_, sql)) {
    LOG(("ShortcutTable::GetOriginsWithShortcuts unable to prepare: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  std::vector<std::string16> temp;
  int rv;
  while (SQLITE_ROW == (rv = statement.step())) {
    std::string16 origin(statement.column_text16_safe(0));
    temp.push_back(origin);
  }

  if (SQLITE_DONE != rv) {
    LOG(("ShortcutTable::GetOriginsWithShortcuts unable to step: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  result->swap(temp);
  return true;
}

bool ShortcutTable::GetOriginShortcuts(const char16 *origin,
                                       std::vector<std::string16> *names) {
  const char16 *sql = STRING16(L"SELECT Name FROM Shortcut WHERE Origin = ?");

  SQLStatement statement;
  if (SQLITE_OK != statement.prepare16(db_, sql)) {
    LOG(("ShortcutTable::GetOriginShortcuts unable to prepare: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(0, origin)) {
    LOG(("ShortcutTable::GetOriginShortcuts unable to bind origin: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  std::vector<std::string16> temp;
  int rv;
  while (SQLITE_ROW == (rv = statement.step())) {
    std::string16 name(statement.column_text16_safe(0));
    temp.push_back(name);
  }

  if (SQLITE_DONE != rv) {
    LOG(("ShortcutTable::GetOriginShortcuts unable to step: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  names->swap(temp);
  return true;
}

bool ShortcutTable::GetShortcut(const char16 *origin, const char16 *name,
                                std::string16 *app_url, std::string16 *ico_url,
                                std::string16 *msg) {
  const char16 *sql = STRING16(L"SELECT AppUrl, IcoUrl, Msg "
                               L"FROM Shortcut WHERE Origin = ? AND Name = ?");

  SQLStatement statement;
  if (SQLITE_OK != statement.prepare16(db_, sql)) {
    LOG(("ShortcutTable::GetShortcut unable to prepare: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(0, origin)) {
    LOG(("ShortcutTable::GetShortcut unable to bind origin: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(1, name)) {
    LOG(("ShortcutTable::GetShortcut unable to bind name: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_ROW != statement.step()) {
    LOG(("ShortcutTable::GetShortcut got too few results: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  *app_url = statement.column_text16_safe(0);
  *ico_url = statement.column_text16_safe(1);
  *msg = statement.column_text16_safe(2);

  if (SQLITE_DONE != statement.step()) {
    LOG(("ShortcutTable::GetShortcut got too many results: %d\n",
         db_->GetErrorCode()));
  }

  return true;
}

bool ShortcutTable::DeleteShortcut(const char16 *origin, const char16 *name) {
  const char16 *sql = STRING16(L"DELETE FROM Shortcut "
                               L"WHERE Origin = ? AND Name = ?");

  SQLStatement statement;
  if (SQLITE_OK != statement.prepare16(db_, sql)) {
    LOG(("ShortcutTable::DeleteShortcut unable to prepare: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(0, origin)) {
    LOG(("ShortcutTable::DeleteShortcut unable to bind origin: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(1, name)) {
    LOG(("ShortcutTable::DeleteShortcut unable to bind name: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_DONE != statement.step()) {
    LOG(("ShortcutTable::DeleteShortcut unable to step: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  return true;
}

bool ShortcutTable::DeleteShortcuts(const char16 *origin) {
  const char16 *sql = STRING16(L"DELETE FROM Shortcut WHERE Origin = ?");

  SQLStatement statement;
  if (SQLITE_OK != statement.prepare16(db_, sql)) {
    LOG(("ShortcutTable::DeleteShortcuts unable to prepare: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_OK != statement.bind_text16(0, origin)) {
    LOG(("ShortcutTable::DeleteShortcuts unable to bind origin: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  if (SQLITE_DONE != statement.step()) {
    LOG(("ShortcutTable::DeleteShortcuts unable to step: %d\n",
         db_->GetErrorCode()));
    return false;
  }

  return true;
}
