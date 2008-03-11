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

#ifndef GEARS_BASE_COMMON_SHORTCUT_TABLE_H__
#define GEARS_BASE_COMMON_SHORTCUT_TABLE_H__

#include "gears/base/common/sqlite_wrapper.h"

// This class provides an API to manage the shortcuts for origins.
// Shortcuts are uniquely identified by their origin url plus their
// name.
class ShortcutTable {
 public:
  ShortcutTable(SQLDatabase *db);

  // Creates the table if it doesn't already exist.
  bool MaybeCreateTable();

  // Upgrade to version 3 schema (this table did not previously
  // exist).
  bool UpgradeToVersion3();

  // Upgrade version 3 schema to version 4.
  bool UpgradeFromVersion3ToVersion4();

  // Add (or overwrite) a shortcut for origin/name, with app_url,
  // icon_urls, and msg as data.
  bool SetShortcut(const char16 *origin, const char16 *name,
                   const char16 *app_url,
                   const std::vector<std::string16> &icon_urls,
                   const char16 *msg);

  // Get the set of origins which have shortcuts.
  bool GetOriginsWithShortcuts(std::vector<std::string16> *result);

  // Get the set of named shortcuts for a specific origin.
  bool GetOriginShortcuts(const char16 *origin,
                          std::vector<std::string16> *names);

  // Get the data for a specific shortcut.
  bool GetShortcut(const char16 *origin, const char16 *name,
                   std::string16 *app_url,
                   std::vector<std::string16> *icon_urls,
                   std::string16 *msg);

  // Delete a specific shortcut.
  bool DeleteShortcut(const char16 *origin, const char16 *name);

  // Delete all shortcuts for an origin.
  bool DeleteShortcuts(const char16 *origin);

 private:
  // A pointer to the SQLDatabase our table lives in.
  SQLDatabase *db_;

  DISALLOW_EVIL_CONSTRUCTORS(ShortcutTable);
};

#endif  // GEARS_BASE_COMMON_SHORTCUT_TABLE_H__
