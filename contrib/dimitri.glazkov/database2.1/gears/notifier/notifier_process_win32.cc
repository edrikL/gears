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
#ifdef WIN32

#include "gears/base/common/common.h"
#include "gears/base/common/string16.h"
#include "gears/notifier/notifier_process.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

// Work around the header including errors.
#include <atlbase.h>
#include <atlwin.h>
#include <objbase.h>
#include <windows.h>

static const char16 *kNotifierDummyWndClassName =
    L"DAAC6F7D-4BAB-403b-AC79-D09E165BC509";
static const char16 *kNotifierStartUpSyncGateName =
    L"C2304258-5112-4fed-B162-FCA6E224F873";
static const int kNotifierStartUpSyncTimeoutMs = 5000;    // 5s

// Dummy window
class NotifierDummyWindow :
    public CWindowImpl<NotifierDummyWindow> {
 public:
  DECLARE_WND_CLASS(kNotifierDummyWndClassName);

  NotifierDummyWindow() : CWindowImpl() {}

  ~NotifierDummyWindow() {
    if (m_hWnd) {
      DestroyWindow();
    }
  }

  bool NotifierDummyWindow::Initialize() {
    return Create(NULL, 0, L"GearsNotifier", WS_POPUP) != NULL;
  }

  BEGIN_MSG_MAP(NotifierDummyWindow)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  END_MSG_MAP()

  LRESULT OnCreate(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    return 0;
  }

  LRESULT OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    ::PostQuitMessage(0);
    return 0;
  }
};

static scoped_ptr<NotifierDummyWindow> g_dummy_wnd;

class NotifierSyncGate {
 public:
   NotifierSyncGate(const char16 *name) : handle_(NULL), opened_(false) {
    handle_ = ::CreateEvent(NULL, true, false, name);
    assert(handle_);
  }

  ~NotifierSyncGate() {
    if (opened_) {
      BOOL res = ::ResetEvent(handle_);
      assert(res);
    }
    if (handle_) {
      ::CloseHandle(handle_);
    }
  }

  bool Open() {
    assert(handle_);
    assert(!opened_);
    if (!::SetEvent(handle_)) {
      return false;
    }
    opened_ = true;
    return true;
  }

  bool Wait(int timeout_ms) {
    assert(handle_);
    return ::WaitForSingleObject(handle_, timeout_ms) == WAIT_OBJECT_0;
  }

 private:
  HANDLE handle_;
  bool opened_;
};

static scoped_ptr<NotifierSyncGate> g_sync_gate;

bool GetNotifierPath(std::string16 *path) {
  assert(path);

  HKEY hkey;
  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     L"Software\\Google\\Gears",
                     0,
                     KEY_QUERY_VALUE,
                     &hkey) != ERROR_SUCCESS) {
    return false;
  }

  DWORD type = 0;
  char16 install_dir_value[MAX_PATH + 1];
  DWORD size = sizeof(install_dir_value) - sizeof(install_dir_value[0]);
  LONG res = ::RegQueryValueEx(hkey,
                               L"install_dir",
                               NULL,
                               &type,
                               reinterpret_cast<BYTE*>(install_dir_value),
                               &size);
  ::RegCloseKey(hkey);
  if (res != ERROR_SUCCESS) {
    return false;
  }

  install_dir_value[size / sizeof(install_dir_value[0]) + 1] = L'\0';
  path->assign(install_dir_value);
  if (path->at(path->length() - 1) != L'\\') {
    path->append(L"\\");
  }
  path->append(L"Shared\\notifier.exe");

  return true;
}

bool NotifierProcess::StartProcess() {
  std::string16 notifier_path;
  if (!GetNotifierPath(&notifier_path)) {
    return false;
  }

  STARTUPINFO startup_info = {0};
  startup_info.cb = sizeof(startup_info);
  PROCESS_INFORMATION process_info = {0};
  if (!::CreateProcess(notifier_path.c_str(),
                       NULL,
                       NULL,
                       NULL,
                       FALSE,
                       0,
                       NULL,
                       NULL,
                       &startup_info,
                       &process_info)) {
    return false;
  }
  ::CloseHandle(process_info.hProcess);
  ::CloseHandle(process_info.hThread);

  NotifierSyncGate gate(kNotifierStartUpSyncGateName);
  return gate.Wait(kNotifierStartUpSyncTimeoutMs);
}

bool NotifierProcess::RegisterProcess() {
  assert(g_dummy_wnd.get() == NULL);
  g_dummy_wnd.reset(new NotifierDummyWindow);
  if (!g_dummy_wnd->Initialize()) {
    return false;
  }

  g_sync_gate.reset(new NotifierSyncGate(kNotifierStartUpSyncGateName));
  return g_sync_gate->Open();
}

unsigned int NotifierProcess::FindProcess() {
  unsigned int process_id = 0;
  HWND window = ::FindWindow(kNotifierDummyWndClassName, NULL);
  if (window) {
    ::GetWindowThreadProcessId(window, reinterpret_cast<DWORD*>(&process_id));
  }

  return process_id;
}

#endif  // defined(WIN32)
#endif  // OFFICIAL_BUILD
