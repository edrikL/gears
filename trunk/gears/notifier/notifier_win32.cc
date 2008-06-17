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

#ifdef WIN32

#ifdef OFFICIAL_BUILD

#include <windows.h>
// The notification API has not been finalized for official builds.
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  return 0;
}

#else

#include "gears/notifier/notifier.h"

class Win32Notifier : public Notifier {
 public:
  Win32Notifier();
  virtual int Run();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Win32Notifier);
};

Win32Notifier::Win32Notifier() {
}

int Win32Notifier::Run() {
  running_ = true;
  while (running_) {
    Sleep(100);
  }
  return 0;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  int argc;
  char16 **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!argv) { return __LINE__; }  // return line as a lame error code
  LocalFree(argv);  // MSDN says to free 'argv', using LocalFree().

  LOG(("Gears Notifier started.\n"));

  Win32Notifier notifier;

  int retval = -1;
  if (notifier.Initalize()) {
    retval = notifier.Run();
    notifier.Terminate();
  }

  LOG(("Gears Notifier terminated.\n"));

  return retval;
}

#endif  // OFFICIAL_BUILD
#endif  // WIN32

