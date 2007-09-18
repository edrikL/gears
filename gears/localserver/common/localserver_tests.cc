// Copyright 2006, Google Inc.
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
#ifdef DEBUG

#include "gears/base/common/permissions_db.h"
#include "gears/base/common/permissions_db_test.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/name_value_table_test.h"
#include "gears/base/common/sqlite_wrapper_test.h"
#include "gears/localserver/common/managed_resource_store.h"
#include "gears/localserver/common/http_cookies.h"
#include "gears/localserver/common/http_request.h"
#include "gears/localserver/common/manifest.h"
#include "gears/localserver/common/localserver_db.h"
#include "gears/localserver/common/localserver_tests.h"
#include "gears/localserver/common/resource_store.h"


bool TestSecurityModel();  // from security_model_test.cc
bool TestFileUtils();  // from file_test.cc
bool TestUrlUtils();  // from url_utils_test.cc
bool TestJsRootedTokenLifetime();  // from base_class_test.cc
bool TestStringUtils();  // from string_utils_test.cc


//------------------------------------------------------------------------------
// TestWebCacheAll
//------------------------------------------------------------------------------
bool TestWebCacheAll() {
  // We need permissions to use the localserver.
  SecurityOrigin cc_tests_origin;
  cc_tests_origin.InitFromUrl(STRING16(L"http://cc_tests/"));
  PermissionsDB *permissions = PermissionsDB::GetDB();
  if (!permissions) return false;
  permissions->SetCanAccessGears(cc_tests_origin,
                                 PermissionsDB::PERMISSION_ALLOWED);

  bool ok = true;
  ok &= TestStringUtils();
  ok &= TestFileUtils();
  ok &= TestUrlUtils();
  ok &= TestParseHttpStatusLine();
  ok &= TestHttpRequest();
  ok &= TestHttpCookies();
  ok &= TestSecurityModel();
  ok &= TestSqliteUtilsAll();
  ok &= TestNameValueTableAll();
  ok &= TestPermissionsDBAll();
  ok &= TestLocalServerDB();
  ok &= TestResourceStore();
  ok &= TestManifest();
  ok &= TestManagedResourceStore();
  // TODO(zork): Add this test back in once it doesn't crash the browser.
  //ok &= TestJsRootedTokenLifetime();
  LOG(("TestWebCacheAll - %s\n", ok ? "passed" : "failed"));

  // We have to call GetDB again since TestCapabilitiesDBAll deletes
  // the previous instance.
  permissions = PermissionsDB::GetDB();
  permissions->SetCanAccessGears(cc_tests_origin,
                                 PermissionsDB::PERMISSION_DEFAULT);
  return ok;
}


//------------------------------------------------------------------------------
// TestHttpCookies
//------------------------------------------------------------------------------
bool TestHttpCookies() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestHttpCookies - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

  std::vector<std::string> tokens;
  std::string tokens_string("a,b,c,d");
  TEST_ASSERT(Tokenize(tokens_string, std::string(","), &tokens) == 4);
  TEST_ASSERT(tokens[0] == "a");
  TEST_ASSERT(tokens[1] == "b");
  TEST_ASSERT(tokens[2] == "c");
  TEST_ASSERT(tokens[3] == "d");
  tokens_string = ", aaaa;bbbb ,;cccc;,";
  TEST_ASSERT(Tokenize(tokens_string, std::string(",;"), &tokens) == 3);
  TEST_ASSERT(tokens[0] == " aaaa");
  TEST_ASSERT(tokens[1] == "bbbb ");
  TEST_ASSERT(tokens[2] == "cccc");

  std::string16 name, value;
  const std::string16 kName(STRING16(L"name"));
  const std::string16 kNameEq(STRING16(L"name="));
  const std::string16 kNameEqSp(STRING16(L"name= "));
  ParseCookieNameAndValue(kNameEq, &name, &value);
  TEST_ASSERT(name == kName);
  TEST_ASSERT(value.empty());
  ParseCookieNameAndValue(kNameEqSp, &name, &value);
  TEST_ASSERT(name == kName);
  TEST_ASSERT(value.empty());

  const std::string16 kValue(STRING16(L"value"));
  const std::string16 kName2(STRING16(L"name 2"));
  const std::string16 kValue2(STRING16(L"value 2"));
  const std::string16 kCookie3(STRING16(L"cookie3"));
  CookieMap map;
  std::string16 cookie_string(
        STRING16(L"name=value; name 2 = value 2; cookie3 "));
  ParseCookieString(cookie_string, &map);
  TEST_ASSERT(map.size() == 3);
  TEST_ASSERT(map.GetCookie(kName, &value));
  TEST_ASSERT(value == kValue);
  TEST_ASSERT(map.HasSpecificCookie(kName2, kValue2));
  TEST_ASSERT(map.HasCookie(kCookie3));
  TEST_ASSERT(!map.HasCookie(kCookie3 + kName2));

  TEST_ASSERT(GetCookieString(STRING16(L"http://www.google.com/"),
              &cookie_string));
  ParseCookieString(cookie_string, &map);

  LOG(("TestHttpCookies - passed\n"));
  return true;
}


//------------------------------------------------------------------------------
// TestManifest
//------------------------------------------------------------------------------
bool TestManifest() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestManifest - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

  const char16 *manifest_url = STRING16(L"http://cc_tests/manifest.json");
  const std::string16 expected_version = STRING16(L"expected_version");
  const std::string16 expected_redirect(
                 STRING16(L"http://cc_tests.other_origin/redirectUrl"));
  const char16 *json16 = STRING16(
    L"{ 'betaManifestVersion': 1, \n"
    L"  'version': 'expected_version', \n"
    L"  'redirectUrl': 'http://cc_tests.other_origin/redirectUrl', \n"
    L"  'entries': [ \n"
    L"       { 'url': 'test_url', 'src': 'test_src' }, \n"
    L"       { 'url': 'test_url2' }, \n"
    L"       { 'url': 'test_url3', 'ignoreQuery': true}, \n"
    L"       { 'url': 'test_redirect_url', 'redirect': 'test_url3?blah' } \n"
    L"     ] \n"
    L"}");

  std::string json8;
  TEST_ASSERT(String16ToUTF8(json16, &json8));
  ReplaceAll(json8, std::string("'"), std::string("\""));

  Manifest manifest;
  bool ok = manifest.Parse(manifest_url, json8.c_str(), json8.length());
  TEST_ASSERT(ok);
  TEST_ASSERT(manifest.IsValid());
  TEST_ASSERT(expected_version == manifest.GetVersion());
  TEST_ASSERT(expected_redirect == manifest.GetRedirectUrl());
  TEST_ASSERT(manifest.GetEntries()->size() == 4);

  const Manifest::Entry *entry1 = &manifest.GetEntries()->at(0);
  TEST_ASSERT(entry1->url == STRING16(L"http://cc_tests/test_url"));
  TEST_ASSERT(entry1->src == STRING16(L"http://cc_tests/test_src"));
  TEST_ASSERT(entry1->redirect.empty());
  TEST_ASSERT(!entry1->ignore_query);

  const Manifest::Entry *entry2 = &manifest.GetEntries()->at(1);
  TEST_ASSERT(entry2->url == STRING16(L"http://cc_tests/test_url2"));
  TEST_ASSERT(entry2->src.empty());
  TEST_ASSERT(entry2->redirect.empty());

  const Manifest::Entry *entry3 = &manifest.GetEntries()->at(2);
  TEST_ASSERT(entry3->url == STRING16(L"http://cc_tests/test_url3"));
  TEST_ASSERT(entry3->src.empty());
  TEST_ASSERT(entry3->redirect.empty());
  TEST_ASSERT(entry3->ignore_query);

  const Manifest::Entry *entry4 = &manifest.GetEntries()->at(3);
  TEST_ASSERT(entry4->url == STRING16(L"http://cc_tests/test_redirect_url"));
  TEST_ASSERT(entry4->src.empty());
  TEST_ASSERT(entry4->redirect == STRING16(L"http://cc_tests/test_url3?blah"));


  const char *json_not_an_object = "\"A string, but we need an object\"";
  Manifest manifest_should_not_parse;
  ok = manifest_should_not_parse.Parse(manifest_url, json_not_an_object);
  TEST_ASSERT(!ok);

  LOG(("TestManifest - passed\n"));
  return true;
}

//------------------------------------------------------------------------------
// TestResourceStore
//------------------------------------------------------------------------------
bool TestResourceStore() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestResourceStore - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}
  const char16 *name = STRING16(L"name");
  const char16 *url1 = STRING16(L"http://cc_tests/url1");
  const char16 *url2 = STRING16(L"http://cc_tests/url2");
  const char16 *url3 = STRING16(L"http://cc_tests/url3");
  const char16 *required_cookie = STRING16(L"required_cookie");
  const char *data1 = "Hello world";
  const char16 *headers1 =
      STRING16(L"Content-Type: text/plain\r\nContent-Length: 11\r\n\r\n");

  SecurityOrigin security_origin;
  TEST_ASSERT(security_origin.InitFromUrl(url1));

  ResourceStore wcs;
  TEST_ASSERT(wcs.CreateOrOpen(security_origin, name, required_cookie));

  ResourceStore::Item item1;
  item1.entry.url = url1;
  item1.payload.headers = headers1;
  item1.payload.data.reset(new std::vector<uint8>);
  item1.payload.data->assign(data1, data1 + strlen(data1));
  item1.payload.status_line = STRING16(L"HTTP/1.0 200 OK");
  item1.payload.status_code = HttpConstants::HTTP_OK;
  TEST_ASSERT(wcs.PutItem(&item1));

  TEST_ASSERT(wcs.IsCaptured(url1));

  std::string16 headers;
  TEST_ASSERT(wcs.GetAllHeaders(url1, &headers));
  TEST_ASSERT(headers == item1.payload.headers);

  std::string16 content_type;
  TEST_ASSERT(wcs.GetHeader(url1, STRING16(L"Content-Type"), &content_type));
  TEST_ASSERT(content_type == STRING16(L"text/plain"));

  ResourceStore::Item test_item1;
  TEST_ASSERT(wcs.GetItem(url1, &test_item1));

  TEST_ASSERT(wcs.Copy(url1, url2));

  ResourceStore::Item test_item2;
  TEST_ASSERT(wcs.GetItem(url2, &test_item2));
  TEST_ASSERT(test_item1.entry.id != test_item2.entry.id);
  TEST_ASSERT(test_item1.entry.payload_id == test_item2.entry.payload_id);
  TEST_ASSERT(test_item1.entry.src == test_item2.entry.src);
  TEST_ASSERT(test_item1.entry.url != test_item2.entry.url);
  TEST_ASSERT(test_item1.entry.version_id == test_item2.entry.version_id);
  TEST_ASSERT(test_item1.payload.id == test_item2.payload.id);
  TEST_ASSERT(test_item1.payload.creation_date == test_item2.payload.creation_date);
  TEST_ASSERT(test_item1.payload.headers == test_item2.payload.headers);
  TEST_ASSERT(test_item1.payload.status_line == test_item2.payload.status_line);
  TEST_ASSERT(test_item1.payload.status_code == test_item2.payload.status_code);

  TEST_ASSERT(wcs.Rename(url2, url3));

  ResourceStore::Item test_item3;
  TEST_ASSERT(wcs.GetItem(url3, &test_item3));
  TEST_ASSERT(test_item3.entry.id == test_item2.entry.id);
  TEST_ASSERT(test_item3.entry.payload_id == test_item2.entry.payload_id);
  TEST_ASSERT(test_item3.entry.src == test_item2.entry.src);
  TEST_ASSERT(test_item3.entry.url != test_item2.entry.url);
  TEST_ASSERT(test_item3.entry.version_id == test_item2.entry.version_id);
  TEST_ASSERT(test_item3.payload.id == test_item2.payload.id);
  TEST_ASSERT(test_item3.payload.creation_date == test_item2.payload.creation_date);
  TEST_ASSERT(test_item3.payload.headers == test_item2.payload.headers);
  TEST_ASSERT(test_item3.payload.status_line == test_item2.payload.status_line);
  TEST_ASSERT(test_item3.payload.status_code == test_item2.payload.status_code);

  LOG(("TestResourceStore - passed\n"));
  return true;
}


//------------------------------------------------------------------------------
// TestManagedResourceStore
//------------------------------------------------------------------------------
bool TestManagedResourceStore() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestManagedResourceStore - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

  const char *manifest1_json =
    "{ \"betaManifestVersion\": 1, "
    "  \"version\": \"test_version\", "
    "  \"redirectUrl\": \"redirectUrl\", "
    "  \"entries\": [ "
    "       { \"url\": \"test_url\", \"src\": \"test_src\" }, "
    "       { \"url\": \"test_url2\" }, "
    "       { \"url\": \"test_url3\", \"ignoreQuery\": true } "
    "     ]"
    "}";

  const char16 *manifest_url = STRING16(L"http://cc_tests/manifest.json");
  const char16 *name = STRING16(L"name");
  const char16 *required_cookie = STRING16(L"user=joe");

  SecurityOrigin security_origin;
  TEST_ASSERT(security_origin.InitFromUrl(manifest_url));

  // Clear out data from previous test runs
  int64 existing_store_id = WebCacheDB::kInvalidID;
  if (ManagedResourceStore::ExistsInDB(security_origin, name,
                                       required_cookie, &existing_store_id)) {
    ManagedResourceStore remover;
    TEST_ASSERT(remover.Open(existing_store_id));
    TEST_ASSERT(remover.Remove());
  }
  // Bring the ManagedResourceStore thru the states that it will go thru
  // during an install / update process, and verify that it works as expected.

  ManagedResourceStore app;
  TEST_ASSERT(app.CreateOrOpen(security_origin, name, required_cookie));

  // Ensure it looks freshly created
  TEST_ASSERT(app.StillExistsInDB());
  TEST_ASSERT(!app.HasVersion(WebCacheDB::VERSION_CURRENT));
  TEST_ASSERT(!app.HasVersion(WebCacheDB::VERSION_DOWNLOADING));

  // Test Get/Set UpdateInfo
  WebCacheDB::UpdateStatus update_status;
  int64 last_time;
  TEST_ASSERT(app.GetUpdateInfo(&update_status, &last_time, NULL, NULL));
  TEST_ASSERT(update_status == WebCacheDB::UPDATE_OK);
  TEST_ASSERT(last_time == 0);
  TEST_ASSERT(app.SetUpdateInfo(WebCacheDB::UPDATE_FAILED, 1, NULL, NULL));
  TEST_ASSERT(app.GetUpdateInfo(&update_status, &last_time, NULL, NULL));
  TEST_ASSERT(update_status == WebCacheDB::UPDATE_FAILED);
  TEST_ASSERT(last_time == 1);

  // Add a version in the "downloading" state

  Manifest manifest1;
  TEST_ASSERT(manifest1.Parse(manifest_url, manifest1_json));

  int64 manifest1_version_id;
  TEST_ASSERT(app.AddManifestAsDownloadingVersion(&manifest1,
                                                  &manifest1_version_id));
  TEST_ASSERT(app.HasVersion(WebCacheDB::VERSION_DOWNLOADING));

  std::string16 version_string;
  TEST_ASSERT(app.GetVersionString(WebCacheDB::VERSION_DOWNLOADING,
                                   &version_string));
  TEST_ASSERT(version_string == manifest1.GetVersion());

  // Transition to current

  TEST_ASSERT(app.SetDownloadingVersionAsCurrent());
  TEST_ASSERT(app.HasVersion(WebCacheDB::VERSION_CURRENT));
  TEST_ASSERT(!app.HasVersion(WebCacheDB::VERSION_DOWNLOADING));

  LOG(("TestManagedResourceStore - passed\n"));
  return true;
}

//------------------------------------------------------------------------------
// TestLocalServerDB
//------------------------------------------------------------------------------
bool TestLocalServerDB() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestWebCacheDB - failed (%d)\n", __LINE__)); \
    SetFakeCookieString(NULL, NULL); \
    return false; \
  } \
}

  const char16 *name = STRING16(L"name");
  const char16 *required_cookie = STRING16(L"user=joe");
  const char16 *testurl = STRING16(L"http://cc_tests/url");

  SecurityOrigin security_origin;
  TEST_ASSERT(security_origin.InitFromUrl(testurl));

  WebCacheDB *db = WebCacheDB::GetDB();
  TEST_ASSERT(db);

  // delete existing info from a previous test run
  WebCacheDB::ServerInfo existing_server;
  if (db->FindServer(security_origin, name, required_cookie,
                     WebCacheDB::MANAGED_RESOURCE_STORE,
                     &existing_server)) {
    db->DeleteServer(existing_server.id);
  }

  // insert a server
  WebCacheDB::ServerInfo server;
  server.server_type = WebCacheDB::MANAGED_RESOURCE_STORE;
  server.security_origin_url = security_origin.url();
  server.name = name;
  server.required_cookie = required_cookie;
  server.manifest_url = STRING16(L"http://cc_tests/manifest_url");
  TEST_ASSERT(db->InsertServer(&server));

  // insert a current version1 that specifies a redirect
  WebCacheDB::VersionInfo version1;
  version1.server_id = server.id;
  version1.version_string = STRING16(L"version_string");
  version1.ready_state = WebCacheDB::VERSION_CURRENT;
  version1.session_redirect_url = STRING16(L"http://cc_tests/redirect_url");
  TEST_ASSERT(db->InsertVersion(&version1));

  // insert an entry with a bogus payload id for the current version
  WebCacheDB::EntryInfo entry;
  entry.version_id = version1.id;
  entry.url = testurl;
  entry.payload_id = kint64max;
  TEST_ASSERT(db->InsertEntry(&entry));

  // we should be able to service a request for testurl, the response
  // should redirect to our session_redirect_url
  SetFakeCookieString(testurl, NULL);
  TEST_ASSERT(db->CanService(testurl));

  WebCacheDB::PayloadInfo payload;
  TEST_ASSERT(db->Service(testurl, true, &payload));
  TEST_ASSERT(payload.IsHttpRedirect());
  std::string16 test_redirect_url;
  TEST_ASSERT(payload.GetHeader(HttpConstants::kLocationHeader,
                                &test_redirect_url) &&
              test_redirect_url == version1.session_redirect_url);

  // insert a downloaded version2 w/o a redirect
  WebCacheDB::VersionInfo version2;
  version2.server_id = server.id;
  version2.version_string = STRING16(L"version_string2");
  version2.ready_state = WebCacheDB::VERSION_DOWNLOADING;
  TEST_ASSERT(db->InsertVersion(&version2));

  // insert an entry for version2
  WebCacheDB::EntryInfo entry2;
  entry2.version_id = version2.id;
  entry2.url = testurl;
  entry2.payload_id = kint64max;
  TEST_ASSERT(db->InsertEntry(&entry2));

  // we should still be able to service a request for testurl from version1
  TEST_ASSERT(db->CanService(testurl));

  // delete version1
  TEST_ASSERT(db->DeleteVersion(version1.id));

  // we shouldn't be able to service a request for testurl
  TEST_ASSERT(!db->CanService(testurl));

  // now make the ready version current
  TEST_ASSERT(db->UpdateVersion(version2.id, WebCacheDB::VERSION_CURRENT));

  // we should still not be able to service a request for testurl as there
  // is no session yet and version2 does not have a redirect_url
  TEST_ASSERT(!db->CanService(testurl));

  // now set the required cookie (fake)
  SetFakeCookieString(testurl, required_cookie);

  // we should be able to service a request for testurl again
  TEST_ASSERT(db->CanService(testurl));

  // clear the cookie string for our testurl (fake)
  SetFakeCookieString(testurl, NULL);

  // off again
  TEST_ASSERT(!db->CanService(testurl));

  // delete version2
  TEST_ASSERT(db->DeleteVersion(version2.id));

  // insert version3 for that server that requires a session and has a redirect
  WebCacheDB::VersionInfo version3;
  version3.server_id = server.id;
  version3.version_string = STRING16(L"version_string3");
  version3.ready_state = WebCacheDB::VERSION_CURRENT;
  version3.session_redirect_url = STRING16(L"http://cc_tests/session_redirect_url");
  TEST_ASSERT(db->InsertVersion(&version3));

  // insert an entry for the ready version
  WebCacheDB::EntryInfo entry3;
  entry3.version_id = version3.id;
  entry3.url = testurl;
  entry3.ignore_query = true;
  entry3.payload_id = kint64max;
  TEST_ASSERT(db->InsertEntry(&entry3));

  // this entry s/b hit for request with arbitrary query parameters
  std::string16 testurl_query(testurl);
  testurl_query += STRING16(L"?blah");

  // on again, s/b responding with redirects
  SetFakeCookieString(testurl, NULL);
  TEST_ASSERT(db->CanService(testurl));
  SetFakeCookieString(testurl_query.c_str(), NULL);
  TEST_ASSERT(db->CanService(testurl_query.c_str()));

  // still on, s/b responding with payloads
  SetFakeCookieString(testurl, required_cookie);
  TEST_ASSERT(db->CanService(testurl));
  SetFakeCookieString(testurl_query.c_str(), required_cookie);
  TEST_ASSERT(db->CanService(testurl_query.c_str()));

  // delete the server altogether
  TEST_ASSERT(db->DeleteServer(server.id));

  // we shouldn't be able to service a request for the test url
  TEST_ASSERT(!db->CanService(testurl));
  TEST_ASSERT(!db->CanService(testurl_query.c_str()));

  SetFakeCookieString(NULL, NULL);

  // we made it thru as expected
  LOG(("TestWebCacheDB - passed\n"));
  return true;
}

bool TestParseHttpStatusLine() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestParseHttpStatusLine - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}
  std::string16 good(STRING16(L"HTTP/1.1 200 OK"));
  std::string16 version, text;
  int code;
  TEST_ASSERT(ParseHttpStatusLine(good, &version, &code, &text));
  TEST_ASSERT(version == STRING16(L"HTTP/1.1"));
  TEST_ASSERT(code == 200);
  TEST_ASSERT(text == STRING16(L"OK"));
  TEST_ASSERT(ParseHttpStatusLine(good, &version, NULL, NULL));
  TEST_ASSERT(ParseHttpStatusLine(good, NULL, &code, NULL));
  TEST_ASSERT(ParseHttpStatusLine(good, NULL, NULL, &text));

  const char16 *acceptable[] = {
    STRING16(L"HTTP/1.0 200"), // no status
    STRING16(L"HTTP 200 ABBREVIATED VERSION"),
  };
  for (size_t i = 0; i < ARRAYSIZE(acceptable); ++i) {
    std::string16 acceptable_str(acceptable[i]);
    TEST_ASSERT(ParseHttpStatusLine(acceptable_str, NULL, NULL, NULL));
  }

  const char16 *bad[] = {
    STRING16(L" HTTP/1.1 200 SPACE AT START"),
    STRING16(L"WTFP/1.1 200 WRONG SCHEME"),
    STRING16(L"HTTP/1.1 2 CODE TOO SMALL"),
    STRING16(L"HTTP/1.0 2000 CODE TOO BIG"),
    STRING16(L"HTTP/1.0 NO CODE"),
    STRING16(L"complete_gibberish"),
    STRING16(L""), // an empty string
    STRING16(L"    \t \t  "), // whitespace only
  };
  for (size_t i = 0; i < ARRAYSIZE(bad); ++i) {
    std::string16 bad_str(bad[i]);
    TEST_ASSERT(!ParseHttpStatusLine(bad_str, NULL, NULL, NULL));
  }
  LOG(("TestParseHttpStatusLine - passed\n"));
  return true;
}


class TestHttpRequestListener : public HttpRequest::ReadyStateListener {
  virtual void ReadyStateChanged(HttpRequest *source) {
    HttpRequest::ReadyState state = HttpRequest::UNINITIALIZED;
    source->GetReadyState(&state);
    if (state == HttpRequest::COMPLETE) {
      int status = 0;
      std::string16 headers;
      std::string16 body;
      source->GetStatus(&status);
      source->GetAllResponseHeaders(&headers);
      source->GetResponseBodyAsText(&body);
      source->SetOnReadyStateChange(NULL);
      source->ReleaseReference();
      delete this;
      LOG(("TestHttpRequest - complete (%d)\n", status));
    }
  }
};

bool TestHttpRequest() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestHttpRequest - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}
  // A request is created with a refcount of one, our listener will
  // release this reference upon completion. If something goes wrong
  // we leak, not good in a real program but fine for this test.
  HttpRequest *request = HttpRequest::Create();
  request->SetOnReadyStateChange(new TestHttpRequestListener());
  bool ok = request->Open(HttpConstants::kHttpGET,
                          STRING16(L"http://www.google.com/"),
                          true);
  TEST_ASSERT(ok);

  ok = request->Send();
  TEST_ASSERT(ok);
  return true;
}

#endif  // DEBUG
