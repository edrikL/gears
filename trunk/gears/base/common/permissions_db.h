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

#ifndef GEARS_BASE_COMMON_PERMISSIONS_DB_H__
#define GEARS_BASE_COMMON_PERMISSIONS_DB_H__

#include <map>
#include "gears/base/common/name_value_table.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/sqlite_wrapper.h"


// This class provides an API to manage the capabilities of pages within
// Gears. Right now, it is a baby API and only manages a single capability:
// the ability to access Gears at all. But we anticipate it growing into a
// bigger API, which would manage more fine-grained capabilities, such as the
// ability to store more than 1MB on disk, etc.
//
// TODO(aa): Think about factoring some of the commonalities between this class
// and WebCacheDB into a common base class.
class PermissionsDB {
 public:
  // The allowable values of a permission.
  enum PermissionValue {
    PERMISSION_DEFAULT = 0,
    PERMISSION_ALLOWED = 1,
    PERMISSION_DENIED = 2
  };

  // Gets a thread-specific PermissionsDB instance.
  static PermissionsDB *GetDB();

  // Sets the Gears access level for a given SecurityOrigin.
  void SetCanAccessGears(const SecurityOrigin &origin, PermissionValue value);

  // Gets the Gears access level for a given SecurityOrigin.
  bool GetCanAccessGears(const SecurityOrigin &origin, PermissionValue *retval);

  // Get all the origins with a specific value.
  bool GetOriginsByValue(PermissionValue value,
                         std::vector<SecurityOrigin> *result);

  // Attempts to enable Gears for a worker with the given SecurityOrigin.
  bool EnableGearsForWorker(const SecurityOrigin &origin);

  // The key used to cache instances of PermissionsDB in ThreadLocals.
  static const std::string kThreadLocalKey;

 private:
  // Private constructor, callers must use GetDB().
  PermissionsDB();

  // Initializes the database. Must be called before other methods.
  bool Init();

  // Creates or upgrades the database to kCurrentVersion.
  bool CreateOrUpgradeDatabase();

  // Creates the database's schema.
  bool CreateDatabase();

  // Upgrade the schema from 1 to 2.
  bool UpgradeFromVersion1ToVersion2();

  // Destructor function called by ThreadLocals to dispose of a thread-specific
  // DB instance when a thread dies.
  static void DestroyDB(void *context);

  // Database we use to store capabilities information.
  SQLDatabase db_;

  // Version metadata for the capabilities database.
  NameValueTable version_table_;

  // Maps origins to ability to access Gears.
  NameValueTable access_table_;

  DISALLOW_EVIL_CONSTRUCTORS(PermissionsDB);
  DECL_SINGLE_THREAD
};

#endif  // GEARS_BASE_COMMON_PERMISSIONS_DB_H__
