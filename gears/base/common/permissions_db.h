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
//
// TODO(aa): "Capabilities" and "Status" were both poor naming choices. Rename.
// Perhaps to "Permission" and "PermissionValue"?
class CapabilitiesDB {
 public:
  // The capabilities a page can have.
  enum CapabilityStatus {
    CAPABILITY_DEFAULT = 0,
    CAPABILITY_ALLOWED = 1,
    CAPABILITY_DENIED = 2
  };

  // Gets a thread-specific CapabilitiesDB instance.
  static CapabilitiesDB *GetDB();

  // Sets the Gears access level for a given SecurityOrigin.
  void SetCanAccessGears(const SecurityOrigin &origin, CapabilityStatus status);

  // Gets the Gears access level for a given SecurityOrigin.
  const CapabilityStatus GetCanAccessGears(const SecurityOrigin &origin);

  // Get all the origins with a specific status.
  bool GetOriginsByStatus(CapabilityStatus status,
                          std::vector<SecurityOrigin> *result);

  // The key used to cache instances of CapabilitiesDB in ThreadLocals.
  static const std::string kThreadLocalKey;

 private:
  // Private constructor, callers must use GetDB().
  CapabilitiesDB();

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

  DISALLOW_EVIL_CONSTRUCTORS(CapabilitiesDB);
  DECL_SINGLE_THREAD
};

#endif  // GEARS_BASE_COMMON_PERMISSIONS_DB_H__
