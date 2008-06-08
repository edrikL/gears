// Copyright 2008, Google Inc.
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

#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
int main(int, char **) {
  return 0;
}
#else
#if USING_CCTESTS
#include <string>

#include "gears/notifier/unit_test.h"

#include "gears/base/common/basictypes.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/string16.h"
#include "gears/notifier/balloon_collection_test.h"
#include "gears/notifier/notification_manager_test.h"

class UnitTest {
 public:
  // singleton
  static UnitTest *instance() {
    return &unit_test_;
  }

  void LogError(const std::string16& error) {
    failed_ = true;
    log_.append(error);
    log_.append(STRING16(L"\n"));

    if (print_) {
      std::string error8;
      String16ToUTF8(error.c_str(), &error8);
      printf("%s\n", error8.c_str());
    }
  }

  bool all_passed() const {
    return !failed_;
  }

  // The full results of what was logged.  This is
  // to pass back over the ipc call.
  const std::string16& log() const {
    return log_;
  }

  void set_print(bool print) {
    print_ = print;
  }

 private:
  UnitTest() : failed_(false), print_(false) {
  }

  static UnitTest unit_test_;

  std::string16 log_;
  bool failed_;
  bool print_;
  DISALLOW_EVIL_CONSTRUCTORS(UnitTest);
};

UnitTest UnitTest::unit_test_;

void LogTestError(const std::string16& error) {
  UnitTest::instance()->LogError(error);
}

bool DidAllTestsPass() {
  return UnitTest::instance()->all_passed();
}

const std::string16& GetTestLog() {
  return UnitTest::instance()->log();
}

int main(int, char **) {
  UnitTest::instance()->set_print(true);

  TestNotificationManager();
  TestNotificationManagerDelay();
  TestBalloonCollection();
  if (!DidAllTestsPass()) {
    printf("*** Test failures occurred. ***\n");
    return 1;
  }
  printf("*** All tests passed. ***\n");

  return 0;
}
#else
int main(int, char **) {
  return 0;
}
#endif  // USING_CCTESTS
#endif  // OFFICIAL_BUILD
