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

#include <algorithm>
#include "gears/base/common/permissions_db.h"
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/thread_locals.h"

// For use with sort().
static bool SecurityOriginLT(const SecurityOrigin &a,
                             const SecurityOrigin &b) {
  return a.url() < b.url();
}

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

  TEST_ASSERT(permissions->GetCanAccessGears(foo) ==
      PermissionsDB::PERMISSION_ALLOWED);
  TEST_ASSERT(permissions->GetCanAccessGears(bar) ==
      PermissionsDB::PERMISSION_DENIED);

  // Try searching by default value (should not be allowed).
  std::vector<SecurityOrigin> list;
  TEST_ASSERT(!permissions->GetOriginsByValue(
      PermissionsDB::PERMISSION_DEFAULT, &list));

  // Now try resetting
  permissions->SetCanAccessGears(bar, PermissionsDB::PERMISSION_DEFAULT);
  TEST_ASSERT(permissions->GetCanAccessGears(bar) ==
      PermissionsDB::PERMISSION_DEFAULT);

  // TODO(shess) Constants for later comparison.
  const std::string16 kFooTest1(STRING16(L"Test"));
  const std::string16 kFooTest1Url(STRING16(L"http://www.foo.com/Test.html"));
  const std::string16
      kFooTest1IcoUrl(STRING16(L"http://www.foo.com/Test.ico"));
  const std::string16 kFooTest1Msg(STRING16(L"This is the message."));

  const std::string16 kFooTest2(STRING16(L"Another"));
  const std::string16
      kFooTest2Url(STRING16(L"http://www.foo.com/Another.html"));
  const std::string16
      kFooTest2IcoUrl(STRING16(L"http://www.foo.com/Another.ico"));
  const std::string16 kFooTest2Msg(STRING16(L"This is another message."));

  const std::string16 kBarTest(STRING16(L"Test"));
  const std::string16 kBarTestUrl(STRING16(L"http://www.bar.com/Test.html"));
  const std::string16 kBarTestIcoUrl(STRING16(L"http://www.bar.com/Test.ico"));
  const std::string16 kBarTestMsg(STRING16(L"This is a message."));

  // Load up some shortcuts.
  TEST_ASSERT(
      permissions->SetShortcut(foo, kFooTest1.c_str(), kFooTest1Url.c_str(),
                               kFooTest1IcoUrl.c_str(), kFooTest1Msg.c_str()));

  TEST_ASSERT(
      permissions->SetShortcut(foo, kFooTest2.c_str(), kFooTest2Url.c_str(),
                               kFooTest2IcoUrl.c_str(), kFooTest2Msg.c_str()));

  TEST_ASSERT(
      permissions->SetShortcut(bar, kBarTest.c_str(), kBarTestUrl.c_str(),
                               kBarTestIcoUrl.c_str(), kBarTestMsg.c_str()));

  // Expect 2 origins with shortcuts.
  list.clear();
  TEST_ASSERT(permissions->GetOriginsWithShortcuts(&list));
  TEST_ASSERT(list.size() == 2);
  std::sort(list.begin(), list.end(), SecurityOriginLT);
  TEST_ASSERT(list[0].url() == bar.url());
  TEST_ASSERT(list[1].url() == foo.url());

  // Expect 2 shortcuts for origin foo.
  std::vector<std::string16> names;
  TEST_ASSERT(permissions->GetOriginShortcuts(foo, &names));
  TEST_ASSERT(names.size() == 2);
  std::sort(names.begin(), names.end());
  TEST_ASSERT(names[0] == kFooTest2);
  TEST_ASSERT(names[1] == kFooTest1);

  // Test a specific shortcut to see if the data comes back right.
  std::string16 app_url, ico_url, msg;
  TEST_ASSERT(permissions->GetShortcut(foo, kFooTest2.c_str(),
                                       &app_url, &ico_url, &msg));
  TEST_ASSERT(app_url == kFooTest2Url);
  TEST_ASSERT(ico_url == kFooTest2IcoUrl);
  TEST_ASSERT(msg == kFooTest2Msg);

  // Test that deleting a specific shortcut doesn't impact other
  // shortcuts.
  TEST_ASSERT(permissions->DeleteShortcut(foo, kFooTest2.c_str()));

  list.clear();
  TEST_ASSERT(permissions->GetOriginsWithShortcuts(&list));
  TEST_ASSERT(list.size() == 2);
  std::sort(list.begin(), list.end(), SecurityOriginLT);
  TEST_ASSERT(list[0].url() == bar.url());
  TEST_ASSERT(list[1].url() == foo.url());

  names.clear();
  TEST_ASSERT(permissions->GetOriginShortcuts(foo, &names));
  LOG(("names.size() == %d\n", names.size()));
  TEST_ASSERT(names.size() == 1);
  TEST_ASSERT(names[0] == kFooTest1);

  // Test that deleting a specific origin's shortcuts doesn't impact
  // other origins.
  TEST_ASSERT(permissions->DeleteShortcuts(foo));

  list.clear();
  TEST_ASSERT(permissions->GetOriginsWithShortcuts(&list));
  TEST_ASSERT(list.size() == 1);
  TEST_ASSERT(list[0].url() == bar.url());

  LOG(("TestPermissionsDBAll - passed\n"));
  return true;
}
