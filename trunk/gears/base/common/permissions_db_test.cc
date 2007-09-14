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


bool TestPermissionsDBAll() {
// TODO(aa): Refactor into a common location for all the internal tests.
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestPermissionsDBAll - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

  PermissionsDB *permissions = PermissionsDB::GetDB();
  TEST_ASSERT(permissions);

  // Set some permissions
  SecurityOrigin foo;
  foo.InitFromUrl(STRING16(L"http://unittest.foo.example.com"));
  permissions->SetCanAccessGears(foo, PermissionsDB::PERMISSION_ALLOWED);

  SecurityOrigin bar;
  bar.InitFromUrl(STRING16(L"http://unittest.bar.example.com"));
  permissions->SetCanAccessGears(bar, PermissionsDB::PERMISSION_DENIED);

  // Get the threadlocal instance and make sure we got the same instance back
  TEST_ASSERT(PermissionsDB::GetDB() == permissions);

  // Now destory the threadlocal instance and get a new one to test whether our
  // values were saved.
  ThreadLocals::DestroyValue(PermissionsDB::kThreadLocalKey);
  permissions = PermissionsDB::GetDB();

  TEST_ASSERT(PermissionsDB::PERMISSION_ALLOWED ==
              permissions->GetCanAccessGears(foo));
  TEST_ASSERT(PermissionsDB::PERMISSION_DENIED ==
              permissions->GetCanAccessGears(bar));

  // Try searching by default value (should not be allowed).
  std::vector<SecurityOrigin> list;
  TEST_ASSERT(!permissions->GetOriginsByValue(
      PermissionsDB::PERMISSION_DEFAULT, &list));

  // Now try resetting
  permissions->SetCanAccessGears(bar, PermissionsDB::PERMISSION_DEFAULT);
  TEST_ASSERT(PermissionsDB::PERMISSION_DEFAULT ==
              permissions->GetCanAccessGears(bar));

  LOG(("TestPermissionsDBAll - passed\n"));
  return true;
}
