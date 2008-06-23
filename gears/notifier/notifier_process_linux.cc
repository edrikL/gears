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
#else
#if defined(LINUX) && !defined(OS_MACOSX)

#include <string>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"
#include "gears/notifier/notifier_process.h"

namespace notifier {

std::string GetSingleInstanceLockFilePath() {
  static std::string lock_file_path;
  if (lock_file_path.empty()) {
    std::string lock_file_path("/tmp/" PRODUCT_SHORT_NAME_ASCII "notifier");
    lock_file_path += IntegerToString(getuid());
    lock_file_path += ".lock";
  }
  return lock_file_path;
}

}

bool NotifierProcess::StartProcess() {
  // TODO (jianli): to be implemented.
  return false;
}

bool NotifierProcess::RegisterProcess() {
  // Nothing to do since it has already been done in
  // LinuxNotifier::CheckSingleInstance
  return true;
}

unsigned int NotifierProcess::FindProcess() {
  unsigned int pid = 0;

  std::string lock_file_path = notifier::GetSingleInstanceLockFilePath();
  int lock_file_fd = open(lock_file_path.c_str(), O_RDONLY);
  if (lock_file_fd > 0) {
    char buf[16] = { 0 };
    int num = read(lock_file_fd, buf, sizeof(buf) - 1);
    if (0 < num && num < static_cast<int>(sizeof(buf) - 1)) {
      int id = atoi(buf);
      if (id > 0 && kill(static_cast<pid_t>(id), 0) == 0) {
        pid = static_cast<unsigned int>(id);
      }
    }
    close(lock_file_fd);
  }

  return pid;
}

#endif  // defined(LINUX) && !defined(OS_MACOSX)
#endif  // OFFICIAL_BUILD
