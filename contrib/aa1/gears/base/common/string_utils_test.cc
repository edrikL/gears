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

#include <string>
#include "gears/base/common/common.h"
#include "gears/base/common/string_utils.h"

// We don't use google3/testing/base/gunit because our tests depend of
// browser specific code that needs to run in the context of the browser.

static bool TestStartsWithAndEndsWith();
static bool TestStrUtilsReplaceAll();
static bool TestStringCompareIgnoreCase();
static bool TestStringUTF8FileToUrl();

bool TestStringUtils() {
  bool ok = true;
  ok &= TestStringCompareIgnoreCase();
  ok &= TestStartsWithAndEndsWith();
  ok &= TestStrUtilsReplaceAll();
  ok &= TestStringUTF8FileToUrl();
  return ok;
}


#if WIN32
#define strcasecmp _stricmp
#endif

static bool TestStringCompareIgnoreCase() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestStringCompareIgnoreCase - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}
  {
    std::string empty_str;
    std::string aaa_str("aaa");
    std::string bbb_str("bbb");
    std::string aaaa_str("aaaa");
    std::string aaa_upper_str = MakeUpperString(aaa_str);

    // First test our assumptions about the proper results by invoking
    // the CRT's impl for 8 bit characters
    TEST_ASSERT(strcasecmp(aaa_str.c_str(), aaa_str.c_str()) == 0);
    TEST_ASSERT(strcasecmp(aaa_str.c_str(), bbb_str.c_str()) < 0);
    TEST_ASSERT(strcasecmp(bbb_str.c_str(), aaa_str.c_str()) > 0);
    TEST_ASSERT(strcasecmp(aaa_str.c_str(), aaaa_str.c_str()) < 0);
    TEST_ASSERT(strcasecmp(aaa_str.c_str(), empty_str.c_str()) > 0);
    TEST_ASSERT(strcasecmp(aaa_str.c_str(), 
                           aaa_upper_str.c_str()) == 0);

    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), aaa_str.c_str()) == 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), bbb_str.c_str()) < 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    bbb_str.c_str(), aaa_str.c_str()) > 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), aaaa_str.c_str()) < 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), empty_str.c_str()) > 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), aaa_upper_str.c_str()) == 0);
  }
  {
    std::string16 empty_str;
    std::string16 aaa_str(STRING16(L"aaa"));
    std::string16 bbb_str(STRING16(L"bbb"));
    std::string16 aaaa_str(STRING16(L"aaaa"));
    std::string16 aaa_upper_str = MakeUpperString(aaa_str);

    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), aaa_str.c_str()) == 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), bbb_str.c_str()) < 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    bbb_str.c_str(), aaa_str.c_str()) > 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), aaaa_str.c_str()) < 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), empty_str.c_str()) > 0);
    TEST_ASSERT(StringCompareIgnoreCase(
                    aaa_str.c_str(), aaa_upper_str.c_str()) == 0);
  }
  return true;
}


static bool TestStrUtilsReplaceAll() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestStrUtilsReplaceAll - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}
  std::string str("bbaaabbaaabb");
  TEST_ASSERT(ReplaceAll(str, std::string("bb"), std::string("cc")) == 3);
  TEST_ASSERT(str == "ccaaaccaaacc");

  TEST_ASSERT(ReplaceAll(str, std::string("cc"), std::string("d")) == 3);
  TEST_ASSERT(str == "daaadaaad");

  TEST_ASSERT(ReplaceAll(str, std::string("d"), std::string("ff")) == 3);
  TEST_ASSERT(str == "ffaaaffaaaff");

  TEST_ASSERT(ReplaceAll(str, std::string("ff"), std::string(1, 0)) == 3);
  TEST_ASSERT(str.length() == 9);
  
  TEST_ASSERT(ReplaceAll(str, std::string(1, 0), std::string("bb")) == 3);
  TEST_ASSERT(str == "bbaaabbaaabb");

  str = "aaaa";
  TEST_ASSERT(ReplaceAll(str, std::string("a"), std::string("aa")) == 4);
  TEST_ASSERT(str == "aaaaaaaa");

  TEST_ASSERT(ReplaceAll(str, std::string("aa"), std::string("a")) == 4);
  TEST_ASSERT(str == "aaaa");

  TEST_ASSERT(ReplaceAll(str, std::string("b"), std::string("c")) == 0);
  TEST_ASSERT(str == "aaaa");

  return true;
}

static bool TestStartsWithAndEndsWith() {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestBeginsWithAndEndsWith - failed (%d)\n", __LINE__)); \
    return false; \
  } \
}
  // std::string16 tests
  {
    const std::string16 prefix(STRING16(L"prefix"));
    const std::string16 suffix(STRING16(L"suffix"));
    const std::string16 test(STRING16(L"prefix_string_suffix"));
    TEST_ASSERT(StartsWith(test, prefix));
    TEST_ASSERT(EndsWith(test, suffix));
    TEST_ASSERT(!StartsWith(test, suffix));
    TEST_ASSERT(!EndsWith(test, prefix));
    TEST_ASSERT(!StartsWith(prefix, test));
    TEST_ASSERT(!EndsWith(suffix, test));
  }
  // std::string tests
  {
    const std::string prefix("prefix");
    const std::string suffix("suffix");
    const std::string test("prefix_string_suffix");
    TEST_ASSERT(StartsWith(test, prefix));
    TEST_ASSERT(EndsWith(test, suffix));
    TEST_ASSERT(!StartsWith(test, suffix));
    TEST_ASSERT(!EndsWith(test, prefix));
    TEST_ASSERT(!StartsWith(prefix, test));
    TEST_ASSERT(!EndsWith(suffix, test));
  }
  LOG(("TestStartsWithAndEndsWith - passed\n"));
  return true;
}

static bool TestStringUTF8FileToUrl() {
#undef TEST_ASSERT
#define TEST_ASSERT(b,test_name) \
{ \
  if (!(b)) { \
  LOG(("TestStringUTF8FileToUrl: %s - failed (%d)\n", test_name, __LINE__)); \
    return false; \
  } \
}
  struct URLCase {
    const char *input;
    const char *expected;
    const char *test_name;
  } cases[] = {
    {"c:/Dead/Beef.txt", "file:///c:/Dead/Beef.txt", "No escapes"},
    {"c:\\Dead\\Beef.txt", "file:///c:/Dead/Beef.txt", "Backslash"},
    {"c:/Dead/Beef/42;.txt", "file:///c:/Dead/Beef/42%3B.txt", "Semicolon"},
    {"c:/Dead/Beef/42#{}.txt", "file:///c:/Dead/Beef/42%23%7B%7D.txt",
      "Disallowed Characters"},
    {"c:/Dead/Beef/牛肉.txt",
      "file:///c:/Dead/Beef/%E7%89%9B%E8%82%89.txt",
      "Non-Ascii Characters"}
  };

  struct URLCase directory_cases[] = {
    {"c:/Dead/Beef/", "file:///c:/Dead/Beef/", "Trailing slash"},
    {"c:\\Dead\\Beef\\", "file:///c:/Dead/Beef/", "Trailing backslash"},
    {"c:/Dead/Beef", "file:///c:/Dead/Beef/", "No trailing slash"},
    {"c:\\Dead\\Beef", "file:///c:/Dead/Beef/", "No trailing backslash"},
  };

  for (unsigned int i = 0; i < ARRAYSIZE(cases); ++i) {
    std::string input(cases[i].input);
    std::string output(UTF8PathToUrl(input, false));
    TEST_ASSERT(output == cases[i].expected, cases[i].test_name);
  }

  for (unsigned int i = 0; i < ARRAYSIZE(directory_cases); ++i) {
    std::string input(directory_cases[i].input);
    std::string output(UTF8PathToUrl(input, true));
    TEST_ASSERT(output == directory_cases[i].expected,
                directory_cases[i].test_name);
  }

  LOG(("TestStringUTF8FileToUrl - passed\n"));
  return true;
}
#endif  // DEBUG
