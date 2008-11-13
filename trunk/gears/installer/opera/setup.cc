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

#if defined(OS_WINCE)

#include <windows.h>  // must be first
#include <winioctl.h>
#include "gears/installer/common/process_restarter.h"
#include "gears/installer/common/resource.h"  // for restart dialog strings

static const char16* kOperaProcessName = L"Opera9.exe";
static const char16* kOperaLaunchExe = L"\\Program Files\\9.5 Beta\\OperaL.exe";
static const char16* kGearsSite = L"http://gears.google.com/done.html";

HINSTANCE module_instance;
int _charmax = 255;

extern "C" {
BOOL WINAPI DllMain(HANDLE instance, DWORD reason, LPVOID reserved) {
  module_instance = static_cast<HINSTANCE>(instance);
  return TRUE;
}

// This defines a return type for the intaller functions. The pattern
// in <ce_setup.h> is that 0 represents the positive action (continue,
// done), while 1 is the negative action (cancel, uninstall).
enum InstallerActions {
  kContinue = 0,
  kCancel,
};

// This is called before the installation begins.
__declspec(dllexport) InstallerActions Install_Init(
    HWND parent_window,
    BOOL is_first_call,
    BOOL is_previously_installed,
    LPCTSTR installation_directory) {
  // We only handle the first call.
  if (!is_first_call) return kContinue;
  ProcessRestarter running_process(kOperaProcessName, NULL);
  running_process.KillTheProcess(
      1000,
      ProcessRestarter::KILL_METHOD_1_WINDOW_MESSAGE |
      ProcessRestarter::KILL_METHOD_3_TERMINATE_PROCESS,
      NULL);
  return kContinue;
}

// This is called after installation completed. We start Opera Mobile and point
// it to the Gears site.
__declspec(dllexport) InstallerActions Install_Exit(
    HWND    parent_window,
    LPCTSTR final_install_dir,
    WORD    failed_directories,
    WORD    failed_files,
    WORD    failed_registry_keys,
    WORD    failed_registry_values,
    WORD    failed_shortcuts) {
  ProcessRestarter running_process(kOperaProcessName, NULL);
  bool is_running = false;
  HRESULT result = running_process.IsProcessRunning(&is_running);
  if ((SUCCEEDED(result) && is_running) || FAILED(result)) {
    // We could not kill Opera Mobile or we could not even tell if it was
    // running. Tell the user he needs to reboot.
    LPCTSTR title = reinterpret_cast<LPCTSTR>(LoadString(
        module_instance,
        IDS_INSTALLING_DIALOG_TITLE,
        NULL,
        0));
    LPCTSTR fail_message = reinterpret_cast<LPCTSTR>(LoadString(
        module_instance,
        IDS_REBOOT_MESSAGE,
        NULL,
        0));

    if (!fail_message || !title) return kContinue;

    MessageBox(parent_window, fail_message, title, MB_OK | MB_ICONEXCLAMATION);
  } else {
    ProcessRestarter restart_process(kOperaLaunchExe, NULL);
    if (FAILED(restart_process.StartTheProcess(kGearsSite))) {
      // Unfortunately we failed, so inform the user.
      LPCTSTR title = reinterpret_cast<LPCTSTR>(LoadString(
          module_instance,
          IDS_INSTALLING_DIALOG_TITLE,
          NULL,
          0));
      LPCTSTR fail_message = reinterpret_cast<LPCTSTR>(LoadString(
          module_instance,
          IDS_START_FAILURE_MESSAGE,
          NULL,
          0));

      if (!fail_message || !title) return kContinue;

      MessageBox(parent_window, fail_message, title, MB_OK);
    }
  }
  return kContinue;
}

// This is called before the uninstall begins.
__declspec(dllexport) InstallerActions Uninstall_Init(HWND parent_window,
                                                      LPCTSTR install_dir) {
  // Continue, please.
  return kContinue;
}

// This is called after the uninstall completed.
__declspec(dllexport) InstallerActions Uninstall_Exit(HWND parent_window) {
  // Not interested.
  return kContinue;
}

};  // extern "C"
#endif  // defined(OS_WINCE)
