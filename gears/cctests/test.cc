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

#ifdef DEBUG

#include "gears/cctests/test.h"

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/module_wrapper.h"

DECLARE_GEARS_WRAPPER(GearsTest);

template<>
void Dispatcher<GearsTest>::Init() {
  RegisterMethod("runTests", &GearsTest::RunTests);
  RegisterMethod("testPassArguments", &GearsTest::TestPassArguments);
  RegisterMethod("testPassArgumentsOptional",
                 &GearsTest::TestPassArgumentsOptional);
  RegisterMethod("testPassObject", &GearsTest::TestPassObject);
  RegisterMethod("testCreateObject", &GearsTest::TestCreateObject);
  RegisterMethod("testCoerceBool", &GearsTest::TestCoerceBool);
  RegisterMethod("testCoerceInt", &GearsTest::TestCoerceInt);
  RegisterMethod("testCoerceDouble", &GearsTest::TestCoerceDouble);
  RegisterMethod("testCoerceString", &GearsTest::TestCoerceString);
  RegisterMethod("testGetType", &GearsTest::TestGetType);
#ifdef WINCE
  RegisterMethod("removeEntriesFromBrowserCache",
                 &GearsTest::RemoveEntriesFromBrowserCache);
  RegisterMethod("testEntriesPresentInBrowserCache",
                 &GearsTest::TestEntriesPresentInBrowserCache);
#endif
}

#ifdef WIN32
#include <windows.h>  // must manually #include before nsIEventQueueService.h
// the #define max on win32 conflicts with std::numeric_limits<T>::max()
#if defined(WIN32) && defined(max)
#undef max
#endif
#endif

#include <cmath>
#include <limits>
#include <sstream>

#include "gears/base/common/name_value_table_test.h"
#include "gears/base/common/permissions_db.h"
#include "gears/base/common/permissions_db_test.h"
#include "gears/base/common/sqlite_wrapper_test.h"
#include "gears/base/common/string_utils.h"
#ifdef WINCE
#include "gears/base/common/url_utils.h"
#include "gears/base/common/wince_compatibility.h"
#endif
#include "gears/database/common/database_utils_test.h"
#include "gears/localserver/common/http_cookies.h"
#include "gears/localserver/common/http_request.h"
#include "gears/localserver/common/localserver_db.h"
#include "gears/localserver/common/managed_resource_store.h"
#include "gears/localserver/common/manifest.h"
#include "gears/localserver/common/resource_store.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

#if BROWSER_FF
bool TestBlobInputStreamFf();  // from blob_input_stream_ff_test.cc
#endif
bool TestHttpCookies();
bool TestHttpRequest();
bool TestManifest();
bool TestMessageService();  // from message_service_test.cc
bool TestLocalServerDB();
bool TestResourceStore();
bool TestManagedResourceStore();
bool TestParseHttpStatusLine();
bool TestSecurityModel();  // from security_model_test.cc
bool TestFileUtils();  // from file_test.cc
bool TestUrlUtils();  // from url_utils_test.cc
bool TestJsRootedTokenLifetime();  // from base_class_test.cc
bool TestStringUtils();  // from string_utils_test.cc
bool TestSerialization();  // from serialization_test.cc
bool TestCircularBuffer();  // from circular_buffer_test.cc
bool TestRefCount(); // from scoped_refptr_test.cc
#ifndef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
bool TestBufferBlob();  // from blob_test.cc
bool TestSliceBlob();  // from blob_test.cc
#endif  // not OFFICIAL_BUILD
#if defined(WIN32) && !defined(WINCE) && defined(BROWSER_IE)
bool TestIpcMessageQueue();  // from ipc_message_queue_win32_test.cc
#endif

void CreateObjectBool(JsCallContext* context,
                      JsRunnerInterface* js_runner,
                      JsObject* out);
void CreateObjectInt(JsCallContext* context,
                        JsRunnerInterface* js_runner,
                        JsObject* out);
void CreateObjectDouble(JsCallContext* context,
                        JsRunnerInterface* js_runner,
                        JsObject* out);
void CreateObjectString(JsCallContext* context,
                        JsRunnerInterface* js_runner,
                        JsObject* out);
void CreateObjectArray(JsCallContext* context,
                       JsRunnerInterface* js_runner,
                       JsRootedCallback* func,
                       JsObject* out);
void CreateObjectObject(JsCallContext* context,
                        JsRunnerInterface* js_runner,
                        JsObject* out);
void CreateObjectFunction(JsCallContext* context,
                          JsRootedCallback* func,
                          JsObject* out);
void TestObjectBool(JsCallContext* context, const JsObject& obj);
void TestObjectInt(JsCallContext* context, const JsObject& obj);
void TestObjectDouble(JsCallContext* context, const JsObject& obj);
void TestObjectString(JsCallContext* context, const JsObject& obj);
void TestObjectArray(JsCallContext* context,
                     const JsObject& obj,
                     const ModuleImplBaseClass& base);
void TestObjectObject(JsCallContext* context, const JsObject& obj);
void TestObjectFunction(JsCallContext* context,
                        const JsObject& obj,
                        const ModuleImplBaseClass& base);

void GearsTest::RunTests(JsCallContext *context) {
  // We need permissions to use the localserver.
  SecurityOrigin cc_tests_origin;
  cc_tests_origin.InitFromUrl(STRING16(L"http://cc_tests/"));
  PermissionsDB *permissions = PermissionsDB::GetDB();
  if (!permissions) {
    context->SetException(GET_INTERNAL_ERROR_MESSAGE());
    return;
  }

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
  ok &= TestDatabaseUtilsAll();
  ok &= TestLocalServerDB();
  ok &= TestResourceStore();
  ok &= TestManifest();
  ok &= TestManagedResourceStore();
  ok &= TestMessageService();
  ok &= TestSerialization();
  ok &= TestCircularBuffer();
  ok &= TestRefCount();
#ifndef OFFICIAL_BUILD
  // The blob API has not been finalized for official builds
#if BROWSER_FF || BROWSER_IE
  ok &= TestBufferBlob();
  ok &= TestSliceBlob();
#else
  // Blobs not yet implemented for NPAPI.
#endif  // BROWSER_FF || BROWSER_IE
#endif  // not OFFICIAL_BUILD
  // TODO(zork): Add this test back in once it doesn't crash the browser.
  //ok &= TestJsRootedTokenLifetime();

#if defined(WIN32) && !defined(WINCE) && defined(BROWSER_IE)
  ok &= TestIpcMessageQueue();
#endif
#ifndef OFFICIAL_BUILD
#if BROWSER_FF
  ok &= TestBlobInputStreamFf();
#endif
#endif

  // We have to call GetDB again since TestCapabilitiesDBAll deletes
  // the previous instance.
  permissions = PermissionsDB::GetDB();
  permissions->SetCanAccessGears(cc_tests_origin,
                                 PermissionsDB::PERMISSION_NOT_SET);

  context->SetReturnValue(JSPARAM_BOOL, &ok);
}

void GearsTest::TestPassArguments(JsCallContext *context) {
  bool bool_value = false;
  int int_value = 0;
  int64 int64_value = 0;
  double double_value = 0.0;
  std::string16 string_value;

  JsArgument argv[] = {
    {JSPARAM_REQUIRED, JSPARAM_BOOL, &bool_value},
    {JSPARAM_REQUIRED, JSPARAM_INT, &int_value},
    {JSPARAM_REQUIRED, JSPARAM_INT64, &int64_value},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &double_value},
    {JSPARAM_REQUIRED, JSPARAM_STRING16, &string_value}
  };

  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  if (!bool_value) {
    context->SetException(STRING16(L"Incorrect value for parameter 1"));
  } else if (int_value != 42) {
    context->SetException(STRING16(L"Incorrect value for parameter 2"));
  } else if (int64_value != (GG_LONGLONG(1) << 42)) {
    context->SetException(STRING16(L"Incorrect value for parameter 3"));
  } else if (double_value != 88.8) {
    context->SetException(STRING16(L"Incorrect value for parameter 4"));
  } else if (string_value != STRING16(L"hotdog")) {
    context->SetException(STRING16(L"Incorrect value for parameter 5"));
  }
}

void GearsTest::TestPassArgumentsOptional(JsCallContext *context) {
  int int_values[3] = {};

  JsArgument argv[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &(int_values[0])},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &(int_values[1])},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &(int_values[2])}
  };

  int argc = context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) return;

  for (int i = 0; i < argc; ++i) {
    if (int_values[i] != 42) {
      std::string16 error(STRING16(L"Incorrect value for parameter "));
      error += IntegerToString16(i + 1);
      error += STRING16(L".");
      context->SetException(error);
      return;
    }
  }
}

// An object is passed in from JavaScript with properties set to test all the
// JsObject::Get* functions.
void GearsTest::TestPassObject(JsCallContext *context) {
  const int argc = 1;
  JsObject obj;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_OBJECT, &obj } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  TestObjectBool(context, obj);
  if (context->is_exception_set()) return;

  TestObjectInt(context, obj);
  if (context->is_exception_set()) return;

  TestObjectDouble(context, obj);
  if (context->is_exception_set()) return;

  TestObjectString(context, obj);
  if (context->is_exception_set()) return;

  TestObjectArray(context, obj, *this);
  if (context->is_exception_set()) return;

  TestObjectObject(context, obj);
  if (context->is_exception_set()) return;

  TestObjectFunction(context, obj, *this);
}

void GearsTest::TestCreateObject(JsCallContext* context) {
  const int argc = 1;
  JsRootedCallback* func = NULL;
  JsArgument argv[argc] = { { JSPARAM_REQUIRED, JSPARAM_FUNCTION, &func } };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  JsRunnerInterface* js_runner = GetJsRunner();
  if (!js_runner)
    context->SetException(STRING16(L"Failed to get JsRunnerInterface."));

  scoped_ptr<JsObject> js_object(js_runner->NewObject(NULL));
  if (!js_object.get())
    context->SetException(STRING16(L"Failed to create new javascript object."));

  CreateObjectBool(context, js_runner, js_object.get());
  if (context->is_exception_set()) return;

  CreateObjectInt(context, js_runner, js_object.get());
  if (context->is_exception_set()) return;

  CreateObjectDouble(context, js_runner, js_object.get());
  if (context->is_exception_set()) return;

  CreateObjectString(context, js_runner, js_object.get());
  if (context->is_exception_set()) return;

  CreateObjectArray(context, js_runner, func, js_object.get());
  if (context->is_exception_set()) return;

  CreateObjectObject(context, js_runner, js_object.get());
  if (context->is_exception_set()) return;

  CreateObjectFunction(context, func, js_object.get());
  if (context->is_exception_set()) return;

  context->SetReturnValue(JSPARAM_OBJECT, js_object.get());
}

//------------------------------------------------------------------------------
// Coercion Tests
// TODO(aa): There is no real need to pass the values to coerce from JavaScript.
// Implement BoolToJsToken(), IntToJsToken(), etc and use to create tokens and
// test coercion directly in C++.
//------------------------------------------------------------------------------
// Coerces the first parameter to a bool and ensures the coerced value is equal
// to the expected value.
void GearsTest::TestCoerceBool(JsCallContext *context) {
  JsToken value;
  bool expected_value;
  const int argc = 2;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_TOKEN, &value },
    { JSPARAM_REQUIRED, JSPARAM_BOOL, &expected_value }
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  bool coerced_value;
  if (!JsTokenToBool_Coerce(value, context->js_context(), &coerced_value)) {
    context->SetException(STRING16(L"Could not coerce argument to bool."));
    return;
  }

  bool ok = (coerced_value == expected_value);
  context->SetReturnValue(JSPARAM_BOOL, &ok);
}

// Coerces the first parameter to an int and ensures the coerced value is equal
// to the expected value.
void GearsTest::TestCoerceInt(JsCallContext *context) {
  JsToken value;
  int expected_value;
  const int argc = 2;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_TOKEN, &value },
    { JSPARAM_REQUIRED, JSPARAM_INT, &expected_value },
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  int coerced_value;
  if (!JsTokenToInt_Coerce(value, context->js_context(), &coerced_value)) {
    context->SetException(STRING16(L"Could not coerce argument to int."));
    return;
  }

  bool ok = (coerced_value == expected_value);
  context->SetReturnValue(JSPARAM_BOOL, &ok);
}

// Coerces the first parameter to a double and ensures the coerced value is
// equal to the expected value.
void GearsTest::TestCoerceDouble(JsCallContext *context) {
  JsToken value;
  double expected_value;
  const int argc = 2;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_TOKEN, &value },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &expected_value },
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  double coerced_value;
  if (!JsTokenToDouble_Coerce(value, context->js_context(), &coerced_value)) {
    context->SetException(STRING16(L"Could not coerce argument to double."));
    return;
  }

  bool ok = (coerced_value == expected_value);
  context->SetReturnValue(JSPARAM_BOOL, &ok);
}

// Coerces the first parameter to a string and ensures the coerced value is
// equal to the expected value.
void GearsTest::TestCoerceString(JsCallContext *context) {
  JsToken value;
  std::string16 expected_value;
  const int argc = 2;
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_TOKEN, &value },
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &expected_value },
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  std::string16 coerced_value;
  if (!JsTokenToString_Coerce(value, context->js_context(), &coerced_value)) {
    context->SetException(STRING16(L"Could not coerce argument to string."));
    return;
  }

  bool ok = (coerced_value == expected_value);
  context->SetReturnValue(JSPARAM_BOOL, &ok);
}

// Checks that the second parameter is of the type specified by the first
// parameter using GetType(). First parameter should be one of "bool", "int",
// "double", "string", "null", "undefined", "array", "function", "object".
void GearsTest::TestGetType(JsCallContext *context) {
  std::string16 type;
  const int argc = 1;
  // Don't really care about the actual value of the second parameter
  JsArgument argv[argc] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &type },
  };
  context->GetArguments(argc, argv);
  if (context->is_exception_set()) return;

  bool ok = false;
  JsParamType t = context->GetArgumentType(1);
  if (type == STRING16(L"bool") && t == JSPARAM_BOOL ||
      type == STRING16(L"int") && t == JSPARAM_INT ||
      type == STRING16(L"double") && t == JSPARAM_DOUBLE ||
      type == STRING16(L"string") && t == JSPARAM_STRING16 ||
      type == STRING16(L"null") && t == JSPARAM_NULL ||
      type == STRING16(L"undefined") && t == JSPARAM_UNDEFINED ||
      type == STRING16(L"array") && t == JSPARAM_ARRAY ||
      type == STRING16(L"function") && t == JSPARAM_FUNCTION ||
      type == STRING16(L"object") && t == JSPARAM_OBJECT) {
    ok = true;
  }
  context->SetReturnValue(JSPARAM_BOOL, &ok);
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
  TEST_ASSERT(test_item1.payload.creation_date ==
              test_item2.payload.creation_date);
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
  TEST_ASSERT(test_item3.payload.creation_date ==
              test_item2.payload.creation_date);
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
  version3.session_redirect_url = STRING16(
                                      L"http://cc_tests/session_redirect_url");
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
    STRING16(L"HTTP/1.1 500 REASON: CONTAINING COLON")
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
      delete this;
      LOG(("TestHttpRequest - complete (%d)\n", status));
    }
  }
};

#ifdef BROWSER_FF
// Returns true if the currently executing thread is the main UI thread,
// firefox/mozila has one such very special thread
// See cache_intercept.cc for implementation
bool IsUiThread();
#endif

bool TestHttpRequest() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestHttpRequest - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}

#ifdef BROWSER_FF
  if (!IsUiThread()) {
    return true;
  }
#endif

  // Send a request synchronously
  scoped_refptr<HttpRequest> request;
  TEST_ASSERT(HttpRequest::Create(&request));
  request->SetOnReadyStateChange(new TestHttpRequestListener());
  bool ok = request->Open(HttpConstants::kHttpGET,
                          STRING16(L"http://www.google.com/"),
                          false);
  TEST_ASSERT(ok);
  ok = request->Send();
  TEST_ASSERT(ok);

  // Send an async request
  TEST_ASSERT(HttpRequest::Create(&request));
  request->SetOnReadyStateChange(new TestHttpRequestListener());
  ok = request->Open(HttpConstants::kHttpGET,
                     STRING16(L"http://www.google.com/"),
                     true);
  TEST_ASSERT(ok);
  ok = request->Send();
  TEST_ASSERT(ok);
  return true;
}

// JsObject test functions

#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    std::stringstream ss; \
    ss << "TestObject - failed (" << __LINE__ << ": " << __FILE__ \
       << std::endl; \
    std::string message8(ss.str()); \
    std::string16 message16; \
    if (UTF8ToString16(message8.c_str(), message8.size(), &message16)) { \
      context->SetException(message16); \
    } else { \
      context->SetException(STRING16(L"Failed to convert error message.")); \
    } \
  } \
}

void TestObjectBool(JsCallContext* context, const JsObject& obj) {
  bool property_value = false;
  TEST_ASSERT(obj.GetPropertyAsBool(STRING16(L"bool_true"), &property_value));
  TEST_ASSERT(property_value == true);

  property_value = true;
  TEST_ASSERT(obj.GetPropertyAsBool(STRING16(L"bool_false"), &property_value));
  TEST_ASSERT(property_value == false);

  JsArray arr;
  TEST_ASSERT(obj.GetPropertyAsArray(STRING16(L"bool_array"), &arr));

  int length = -1;
  TEST_ASSERT(arr.GetLength(&length));
  TEST_ASSERT(length == 2);

  property_value = false;
  TEST_ASSERT(arr.GetElementAsBool(0, &property_value));
  TEST_ASSERT(property_value == true);

  property_value = true;
  TEST_ASSERT(arr.GetElementAsBool(1, &property_value));
  TEST_ASSERT(property_value == false);
}

const static int int_large = 1073741823;  // 2 ** 30 - 1

void TestObjectInt(JsCallContext* context, const JsObject& obj) {
  // integer (assumed to be tagged 32 bit signed integer,
  //          30 bits magnitude, 1 bit sign, 1 bit tag)
  int property_value = -1;
  TEST_ASSERT(obj.GetPropertyAsInt(STRING16(L"int_0"), &property_value));
  TEST_ASSERT(property_value == 0);

  property_value = -1;
  TEST_ASSERT(obj.GetPropertyAsInt(STRING16(L"int_1"), &property_value));
  TEST_ASSERT(property_value == 1);

  property_value = -1;
  TEST_ASSERT(obj.GetPropertyAsInt(STRING16(L"int_large"), &property_value));
  TEST_ASSERT(property_value == int_large);

  property_value = 1;
  TEST_ASSERT(obj.GetPropertyAsInt(STRING16(L"int_negative_1"),
                                    &property_value));
  TEST_ASSERT(property_value == -1);

  property_value = 1;
  TEST_ASSERT(obj.GetPropertyAsInt(STRING16(L"int_negative_large"),
                                    &property_value));
  TEST_ASSERT(property_value == -int_large);

  JsArray arr;
  TEST_ASSERT(obj.GetPropertyAsArray(STRING16(L"int_array"), &arr));

  int length = -1;
  TEST_ASSERT(arr.GetLength(&length));
  TEST_ASSERT(length == 5);

  property_value = -1;
  TEST_ASSERT(arr.GetElementAsInt(0, &property_value));
  TEST_ASSERT(property_value == 0);

  property_value = -1;
  TEST_ASSERT(arr.GetElementAsInt(1, &property_value));
  TEST_ASSERT(property_value == 1);

  property_value = -1;
  TEST_ASSERT(arr.GetElementAsInt(2, &property_value));
  TEST_ASSERT(property_value == int_large);

  property_value = 1;
  TEST_ASSERT(arr.GetElementAsInt(3, &property_value));
  TEST_ASSERT(property_value == -1);

  property_value = -1;
  TEST_ASSERT(arr.GetElementAsInt(4, &property_value));
  TEST_ASSERT(property_value == -int_large);
}

// Magic number is from:
// http://developer.mozilla.org/en/docs/
//   Core_JavaScript_1.5_Reference:Global_Objects:Number:MIN_VALUE
const static double JS_NUMBER_MIN_VALUE = 5e-324;

void TestObjectDouble(JsCallContext* context, const JsObject& obj) {
  // JavaScript interprets 1.0 as an integer.
  // This is why 1 is 1.01.
  double property_value = -1.0;
  TEST_ASSERT(obj.GetPropertyAsDouble(STRING16(L"double_0"), &property_value));
  TEST_ASSERT(property_value == 0.01);

  property_value = -1.0;
  TEST_ASSERT(obj.GetPropertyAsDouble(STRING16(L"double_1"), &property_value));
  TEST_ASSERT(property_value == 1.01);

  property_value = -1;
  TEST_ASSERT(obj.GetPropertyAsDouble(STRING16(L"double_large"),
                                      &property_value));
  TEST_ASSERT(property_value == std::numeric_limits<double>::max());

  property_value = 1;
  TEST_ASSERT(obj.GetPropertyAsDouble(STRING16(L"double_negative_1"),
                                    &property_value));
  TEST_ASSERT(property_value == -1.01);

  property_value = 1;
  TEST_ASSERT(obj.GetPropertyAsDouble(STRING16(L"double_negative_large"),
                                    &property_value));
  TEST_ASSERT(property_value == JS_NUMBER_MIN_VALUE);

  JsArray arr;
  TEST_ASSERT(obj.GetPropertyAsArray(STRING16(L"double_array"), &arr));

  int length = -1;
  TEST_ASSERT(arr.GetLength(&length));
  TEST_ASSERT(length == 5);

  property_value = -1;
  TEST_ASSERT(arr.GetElementAsDouble(0, &property_value));
  TEST_ASSERT(property_value == 0.01);

  property_value = -1;
  TEST_ASSERT(arr.GetElementAsDouble(1, &property_value));
  TEST_ASSERT(property_value == 1.01);

  property_value = -1;
  TEST_ASSERT(arr.GetElementAsDouble(2, &property_value));
  TEST_ASSERT(property_value == std::numeric_limits<double>::max());

  property_value = 1;
  TEST_ASSERT(arr.GetElementAsDouble(3, &property_value));
  TEST_ASSERT(property_value == -1.01);

  property_value = 1;
  TEST_ASSERT(arr.GetElementAsDouble(4, &property_value));
  TEST_ASSERT(property_value == JS_NUMBER_MIN_VALUE);
}

void TestObjectString(JsCallContext* context, const JsObject& obj) {
  std::string16 property_value = STRING16(L"not empty");
  TEST_ASSERT(obj.GetPropertyAsString(STRING16(L"string_0"), &property_value));
  TEST_ASSERT(property_value.empty());

  property_value = STRING16(L"");
  TEST_ASSERT(obj.GetPropertyAsString(STRING16(L"string_1"), &property_value));
  TEST_ASSERT(property_value == STRING16(L"a"));

  property_value = STRING16(L"");
  TEST_ASSERT(obj.GetPropertyAsString(STRING16(L"string_many"),
                                      &property_value));
  const static std::string16 string_many(
      STRING16(L"asdjh1)!(@#*h38ind89!03234bnmd831%%%*&*jdlwnd8893asd1233"));
  TEST_ASSERT(property_value == string_many);

  JsArray arr;
  TEST_ASSERT(obj.GetPropertyAsArray(STRING16(L"string_array"), &arr));

  int length = -1;
  TEST_ASSERT(arr.GetLength(&length));
  TEST_ASSERT(length == 3);

  property_value = STRING16(L"");
  TEST_ASSERT(arr.GetElementAsString(0, &property_value));
  TEST_ASSERT(property_value.empty());

  property_value = STRING16(L"");
  TEST_ASSERT(arr.GetElementAsString(1, &property_value));
  TEST_ASSERT(property_value == STRING16(L"a"));

  property_value = STRING16(L"");
  TEST_ASSERT(arr.GetElementAsString(2, &property_value));
  TEST_ASSERT(property_value == string_many);
}

// The property with property_name is expected to be expected_length and contain
// integers where index == int value. I.e. length 3 is expected to be [0, 1, 2].
static bool ValidateGeneratedArray(const JsArray& arr, int expected_length) {
  int length = -1;
  if (!arr.GetLength(&length))
    return false;

  if (length != expected_length)
    return false;

  for (int i = 0; i < length; ++i) {
    int current_element = -1;
    if (!arr.GetElementAsInt(i, &current_element))
      return false;

    if (current_element != i)
      return false;
  }

  return true;
}

static bool ValidateGeneratedArray(const JsObject& obj,
                                   const std::string16& property_name,
                                   int expected_length) {
  JsArray arr;
  if (!obj.GetPropertyAsArray(property_name, &arr))
    return false;

  return ValidateGeneratedArray(arr, expected_length);
}

void TestObjectArray(JsCallContext* context,
                     const JsObject& obj,
                     const ModuleImplBaseClass& base) {
  TEST_ASSERT(ValidateGeneratedArray(obj, STRING16(L"array_0"), 0));
  TEST_ASSERT(ValidateGeneratedArray(obj, STRING16(L"array_1"), 1));
  TEST_ASSERT(ValidateGeneratedArray(obj, STRING16(L"array_8"), 8));
  TEST_ASSERT(ValidateGeneratedArray(obj, STRING16(L"array_10000"), 10000));

  JsArray array_many_types;
  TEST_ASSERT(obj.GetPropertyAsArray(STRING16(L"array_many_types"),
                                     &array_many_types));
  int array_many_types_length = -1;
  TEST_ASSERT(array_many_types.GetLength(&array_many_types_length));
  TEST_ASSERT(array_many_types_length == 7);

  // index 0
  bool bool_false = true;
  TEST_ASSERT(array_many_types.GetElementAsBool(0, &bool_false));
  TEST_ASSERT(bool_false == false);

  // index 1
  bool bool_true = false;
  TEST_ASSERT(array_many_types.GetElementAsBool(1, &bool_true));
  TEST_ASSERT(bool_true == true);

  // index 2
  int int_2 = -1;
  TEST_ASSERT(array_many_types.GetElementAsInt(2, &int_2));
  TEST_ASSERT(int_2 == 2);

  // index 3
  std::string16 string_3;
  TEST_ASSERT(array_many_types.GetElementAsString(3, &string_3));
  TEST_ASSERT(string_3 == STRING16(L"3"));

  // index 4
  double double_4 = -1;
  TEST_ASSERT(array_many_types.GetElementAsDouble(4, &double_4));
  TEST_ASSERT(double_4 == 4.01);

  // index 5
  JsArray array_5;
  TEST_ASSERT(array_many_types.GetElementAsArray(5, &array_5));
  TEST_ASSERT(ValidateGeneratedArray(array_5, 5));

  // index 6
  JsRootedCallback* function_6 = NULL;
  TEST_ASSERT(array_many_types.GetElementAsFunction(6, &function_6));
  JsRunnerInterface* js_runner = base.GetJsRunner();
  TEST_ASSERT(js_runner);
  JsRootedToken* retval = NULL;
  TEST_ASSERT(js_runner->InvokeCallback(function_6,
                                        0, NULL, &retval));
  TEST_ASSERT(retval);
  std::string16 string_retval;
  JsContextPtr js_context = NULL;
#ifdef BROWSER_FF
  js_context = base.EnvPageJsContext();
#endif
  TEST_ASSERT(JsTokenToString_NoCoerce(retval->token(), js_context,
                                       &string_retval));
  TEST_ASSERT(string_retval == STRING16(L"i am a function"));
}

void TestObjectObject(JsCallContext* context, const JsObject& obj) {
  JsObject child_obj;
  TEST_ASSERT(obj.GetPropertyAsObject(STRING16(L"obj"), &child_obj));

  bool bool_true = false;
  TEST_ASSERT(child_obj.GetPropertyAsBool(STRING16(L"bool_true"), &bool_true));
  TEST_ASSERT(bool_true == true);

  int int_0 = -1;
  TEST_ASSERT(child_obj.GetPropertyAsInt(STRING16(L"int_0"), &int_0));
  TEST_ASSERT(int_0 == 0);

  double double_0 = -1.0;
  TEST_ASSERT(child_obj.GetPropertyAsDouble(STRING16(L"double_0"), &double_0));
  TEST_ASSERT(double_0 == 0.01);

  std::string16 string_0(STRING16(L"not empty"));
  TEST_ASSERT(child_obj.GetPropertyAsString(STRING16(L"string_0"), &string_0));
  TEST_ASSERT(string_0.empty());

  TEST_ASSERT(ValidateGeneratedArray(child_obj, STRING16(L"array_0"), 0));
}

void TestObjectFunction(JsCallContext* context,
                        const JsObject& obj,
                        const ModuleImplBaseClass& base) {
  JsRootedCallback* function = NULL;
  TEST_ASSERT(obj.GetPropertyAsFunction(STRING16(L"func"), &function));
  JsRunnerInterface* js_runner = base.GetJsRunner();
  TEST_ASSERT(js_runner);
  JsRootedToken* retval = NULL;
  TEST_ASSERT(js_runner->InvokeCallback(function, 0, NULL, &retval));
  TEST_ASSERT(retval);
  std::string16 string_retval;
  JsContextPtr js_context = NULL;
#ifdef BROWSER_FF
  js_context = base.EnvPageJsContext();
#endif
  TEST_ASSERT(JsTokenToString_NoCoerce(retval->token(), js_context,
                                       &string_retval));
  TEST_ASSERT(string_retval == STRING16(L"i am a function"));
}

#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    std::stringstream ss; \
    ss << "CreateObject - failed (" << __LINE__ << ": " << __FILE__ \
       << std::endl; \
    std::string message8(ss.str()); \
    printf("%s\n", message8.c_str()); \
    std::string16 message16; \
    if (UTF8ToString16(message8.c_str(), message8.size(), &message16)) { \
      context->SetException(message16); \
    } else { \
      context->SetException(STRING16(L"Failed to convert error message.")); \
    } \
  } \
}

void CreateObjectBool(JsCallContext* context,
                      JsRunnerInterface* js_runner,
                      JsObject* out) {
  TEST_ASSERT(out->SetPropertyBool(STRING16(L"bool_true"), true));
  TEST_ASSERT(out->SetPropertyBool(STRING16(L"bool_false"), false));
  JsArray* bool_array = js_runner->NewArray();
  TEST_ASSERT(bool_array);
  TEST_ASSERT(bool_array->SetElementBool(0, true));
  TEST_ASSERT(bool_array->SetElementBool(1, false));
  TEST_ASSERT(out->SetPropertyArray(STRING16(L"bool_array"), bool_array));
}

void CreateObjectInt(JsCallContext* context,
                     JsRunnerInterface* js_runner,
                     JsObject* out) {
  TEST_ASSERT(out->SetPropertyInt(STRING16(L"int_0"), 0));
  TEST_ASSERT(out->SetPropertyInt(STRING16(L"int_1"), 1));
  TEST_ASSERT(out->SetPropertyInt(STRING16(L"int_large"), int_large));
  TEST_ASSERT(out->SetPropertyInt(STRING16(L"int_negative_1"), -1));
  TEST_ASSERT(out->SetPropertyInt(STRING16(L"int_negative_large"),
                                  -int_large));
  JsArray* int_array = js_runner->NewArray();
  TEST_ASSERT(int_array);
  TEST_ASSERT(int_array->SetElementInt(0, 0));
  TEST_ASSERT(int_array->SetElementInt(1, 1));
  TEST_ASSERT(int_array->SetElementInt(2, int_large));
  TEST_ASSERT(int_array->SetElementInt(3, -1));
  TEST_ASSERT(int_array->SetElementInt(4, -int_large));
  TEST_ASSERT(out->SetPropertyArray(STRING16(L"int_array"), int_array));
}

void CreateObjectDouble(JsCallContext* context,
                        JsRunnerInterface* js_runner,
                        JsObject* out) {
  TEST_ASSERT(out->SetPropertyDouble(STRING16(L"double_0"), 0.01));
  TEST_ASSERT(out->SetPropertyDouble(STRING16(L"double_1"), 1.01));
  TEST_ASSERT(out->SetPropertyDouble(STRING16(L"double_large"),
                                     std::numeric_limits<double>::max()));
  TEST_ASSERT(out->SetPropertyDouble(STRING16(L"double_negative_1"), -1.01));
  TEST_ASSERT(out->SetPropertyDouble(STRING16(L"double_negative_large"),
                                     JS_NUMBER_MIN_VALUE));
  JsArray* double_array = js_runner->NewArray();
  TEST_ASSERT(double_array);
  TEST_ASSERT(double_array->SetElementDouble(0, 0.01));
  TEST_ASSERT(double_array->SetElementDouble(1, 1.01));
  TEST_ASSERT(double_array->SetElementDouble(2,
      std::numeric_limits<double>::max()));
  TEST_ASSERT(double_array->SetElementDouble(3, -1.01));
  TEST_ASSERT(double_array->SetElementDouble(4, JS_NUMBER_MIN_VALUE));
  TEST_ASSERT(out->SetPropertyArray(STRING16(L"double_array"), double_array));
}

void CreateObjectString(JsCallContext* context,
                        JsRunnerInterface* js_runner,
                        JsObject* out) {
  std::string16 string_0;
  std::string16 string_1(STRING16(L"a"));
  std::string16 string_many(
    STRING16(L"asdjh1)!(@#*h38ind89!03234bnmd831%%%*&*jdlwnd8893asd1233"));
  TEST_ASSERT(out->SetPropertyString(STRING16(L"string_0"), string_0));
  TEST_ASSERT(out->SetPropertyString(STRING16(L"string_1"), string_1));
  TEST_ASSERT(out->SetPropertyString(STRING16(L"string_many"), string_many));
  JsArray* string_array = js_runner->NewArray();
  TEST_ASSERT(string_array);
  TEST_ASSERT(string_array->SetElementString(0, string_0));
  TEST_ASSERT(string_array->SetElementString(1, string_1));
  TEST_ASSERT(string_array->SetElementString(2, string_many));
  TEST_ASSERT(out->SetPropertyArray(STRING16(L"string_array"), string_array));
}

static bool FillTestArray(const int n, JsArray* arr) {
  for (int i = 0; i < n; ++i)
    if (!arr->SetElementInt(i, i)) return false;
  return true;
}

void CreateObjectArray(JsCallContext* context,
                       JsRunnerInterface* js_runner,
                       JsRootedCallback* func,
                       JsObject* out) {
  JsArray* array_0 = js_runner->NewArray();
  TEST_ASSERT(array_0);
  TEST_ASSERT(out->SetPropertyArray(STRING16(L"array_0"), array_0));

  JsArray* array_1 = js_runner->NewArray();
  TEST_ASSERT(array_1);
  TEST_ASSERT(FillTestArray(1, array_1));
  TEST_ASSERT(out->SetPropertyArray(STRING16(L"array_1"), array_1));

  JsArray* array_8 = js_runner->NewArray();
  TEST_ASSERT(array_8);
  TEST_ASSERT(FillTestArray(8, array_8));
  TEST_ASSERT(out->SetPropertyArray(STRING16(L"array_8"), array_8));

  JsArray* array_10000 = js_runner->NewArray();
  TEST_ASSERT(array_10000);
  TEST_ASSERT(FillTestArray(10000, array_10000));
  TEST_ASSERT(out->SetPropertyArray(STRING16(L"array_10000"), array_10000));

  JsArray* array_many_types = js_runner->NewArray();
  TEST_ASSERT(array_many_types);
  TEST_ASSERT(array_many_types->SetElementBool(0, false));
  TEST_ASSERT(array_many_types->SetElementBool(1, true));
  TEST_ASSERT(array_many_types->SetElementInt(2, 2));
  TEST_ASSERT(array_many_types->SetElementString(3, STRING16(L"3")));
  TEST_ASSERT(array_many_types->SetElementDouble(4, 4.01));
  JsArray* array_5 = js_runner->NewArray();
  TEST_ASSERT(array_5);
  TEST_ASSERT(FillTestArray(5, array_5));
  TEST_ASSERT(array_many_types->SetElementArray(5, array_5));
  TEST_ASSERT(array_many_types->SetElementFunction(6, func));
  TEST_ASSERT(out->SetPropertyArray(STRING16(L"array_many_types"),
                                    array_many_types));
}

void CreateObjectObject(JsCallContext* context,
                        JsRunnerInterface* js_runner,
                        JsObject* out) {
  JsObject* obj = js_runner->NewObject(NULL);
  TEST_ASSERT(obj);
  TEST_ASSERT(obj->SetPropertyBool(STRING16(L"bool_true"), true));
  TEST_ASSERT(obj->SetPropertyInt(STRING16(L"int_0"), 0));
  TEST_ASSERT(obj->SetPropertyDouble(STRING16(L"double_0"), 0.01));
  TEST_ASSERT(obj->SetPropertyString(STRING16(L"string_0"), STRING16(L"")));
  JsArray* array_0 = js_runner->NewArray();
  TEST_ASSERT(array_0);
  TEST_ASSERT(obj->SetPropertyArray(STRING16(L"array_0"), array_0));
  TEST_ASSERT(out->SetPropertyObject(STRING16(L"obj"), obj));
}

void CreateObjectFunction(JsCallContext* context,
                          JsRootedCallback* func,
                          JsObject* out) {
  TEST_ASSERT(out->SetPropertyFunction(STRING16(L"func"), func));
}

#ifdef WINCE
// These methods are used by the JavaScript testBrowserCache test.

bool GetJsArrayAsStringVector(const JsArray &js_array,
                              std::vector<std::string16> *strings) {
  int array_size;
  if (!js_array.GetLength(&array_size)) {
    return false;
  }
  std::string16 url;
  for (int i = 0; i < array_size; ++i) {
    js_array.GetElementAsString(i, &url);
    strings->push_back(url);
  }
  return true;
}

void GearsTest::RemoveEntriesFromBrowserCache(JsCallContext *context) {
  bool ok = false;
  context->SetReturnValue(JSPARAM_BOOL, &ok);
  JsArray js_array;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_ARRAY, &js_array }
  };
  if (context->GetArguments(ARRAYSIZE(argv), argv) != ARRAYSIZE(argv)) {
    assert(context->is_exception_set());
    return;
  }
  std::vector<std::string16> urls;
  if (!GetJsArrayAsStringVector(js_array, &urls)) {
    context->SetException(STRING16(L"Failed to get urls."));
    return;
  }
  for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
    std::string16 full_url;
    if (!ResolveAndNormalize(
             EnvPageLocationUrl().c_str(), urls[i].c_str(), &full_url)) {
      context->SetException(STRING16(L"Failed to resolve URL ") + urls[i]);
      return;
    }
    scoped_array<INTERNET_CACHE_ENTRY_INFO> info(
        GetEntryInfoTest(full_url.c_str()));
    if (info.get()) {
      if (DeleteUrlCacheEntry(full_url.c_str()) == FALSE) {
        context->SetException(
            STRING16(L"Failed to remove browser cache entry for ") +
            full_url +
            STRING16(L"."));
        return;
      }
    }
  }
  ok = true;
  context->SetReturnValue(JSPARAM_BOOL, &ok);
}

#undef TEST_ASSERT
#define TEST_ASSERT(test, message) \
{ \
  if (!(test)) { \
    std::stringstream ss; \
    context->SetException(message); \
    return; \
  } \
}

void GearsTest::TestEntriesPresentInBrowserCache(JsCallContext *context) {
  bool ok = false;
  context->SetReturnValue(JSPARAM_BOOL, &ok);
  JsArray js_array;
  bool should_be_present;
  bool should_be_bogus = true;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_ARRAY, &js_array },
    { JSPARAM_REQUIRED, JSPARAM_BOOL, &should_be_present },
    { JSPARAM_OPTIONAL, JSPARAM_BOOL, &should_be_bogus }
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) {
    return;
  }
  std::vector<std::string16> urls;
  if (!GetJsArrayAsStringVector(js_array, &urls)) {
    context->SetException(STRING16(L"Failed to get urls."));
    return;
  }
  for (int i = 0; i < static_cast<int>(urls.size()); ++i) {
    std::string16 full_url;
    if (!ResolveAndNormalize(
             EnvPageLocationUrl().c_str(), urls[i].c_str(), &full_url)) {
      context->SetException(STRING16(L"Failed to resolve URL ") + urls[i]);
      return;
    }
    scoped_array<INTERNET_CACHE_ENTRY_INFO> info(
        GetEntryInfoTest(full_url.c_str()));
    if (should_be_present) {
      TEST_ASSERT(info.get(),
                  STRING16(L"No browser cache entry for ") +
                  full_url +
                  STRING16(L"."));
      bool is_bogus = IsEntryBogusTest(info.get());
      TEST_ASSERT(!(should_be_bogus && !is_bogus),
                  STRING16(L"Browser cache entry for ") +
                  full_url +
                  STRING16(L" should be bogus but is not."));
      TEST_ASSERT(!(!should_be_bogus && is_bogus),
                  STRING16(L"Browser cache entry for ") +
                  full_url +
                  STRING16(L" should not be bogus but is."));
    } else {
      TEST_ASSERT(!info.get(),
                  STRING16(L"Spurious browser cache entry for ") +
                  full_url +
                  STRING16(L"."));
    }
  }
  ok = true;
  context->SetReturnValue(JSPARAM_BOOL, &ok);
}
#endif

#endif  // DEBUG
