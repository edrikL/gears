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

static const char16 *kSingleInstanceMutexId =
    L"Local\\929B4371-2230-49b2-819F-6A34D5A11DED";

class Win32Notifier : public Notifier {
 public:
  Win32Notifier();
  virtual bool Initialize();
  virtual int Run();
  virtual void Terminate();

 private:
  bool CheckAlreadyRunning(bool *already_running);

  HANDLE single_instance_mutex_handle_;
  DISALLOW_EVIL_CONSTRUCTORS(Win32Notifier);
};

Win32Notifier::Win32Notifier()
    : single_instance_mutex_handle_(NULL) {
}

bool Win32Notifier::CheckAlreadyRunning(bool *already_running) {
  assert(already_running);

  single_instance_mutex_handle_ = ::CreateMutex(NULL,
                                                false,
                                                kSingleInstanceMutexId);

  DWORD err = ::GetLastError();
  if (err == ERROR_ALREADY_EXISTS) {
    *already_running = true;
    return true;
  }

  if (single_instance_mutex_handle_ == NULL) {
    LOG(("CreateMutex failed with errno=%u", err));
    return false;
  }

  *already_running = false;
  return true;
}

bool Win32Notifier::Initialize() {
  bool already_running = false;
  if (!CheckAlreadyRunning(&already_running)) {
    return false;
  }
  if (already_running) {
    LOG(("Notifier already started. Quit\n"));
    return false;
  }

  return Notifier::Initialize();
}

void Win32Notifier::Terminate() {
  if (single_instance_mutex_handle_) {
    ::ReleaseMutex(single_instance_mutex_handle_);
    single_instance_mutex_handle_ = NULL;
  }
}

int Win32Notifier::Run() {
  running_ = true;
  MSG msg;
  while (running_ && ::GetMessage(&msg, NULL, 0, 0)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
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
  if (notifier.Initialize()) {
    retval = notifier.Run();
    notifier.Terminate();
  }

  LOG(("Gears Notifier terminated.\n"));

  return retval;
}

#endif  // OFFICIAL_BUILD
#endif  // WIN32

