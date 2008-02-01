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

#ifdef WINCE
#include <windows.h>  // must be first
#include <winioctl.h>
#include "gears/installer/iemobile/process_restarter.h"
#include "gears/installer/iemobile/resource.h"  // for restart dialog strings

static const char16* kInternetExplorer = L"iexplore.exe";
static const char16* kGearsSite = L"http://gears.google.com";

HINSTANCE module_instance;
int _charmax = 255;

extern "C" {
BOOL WINAPI DllMain(HANDLE instance, DWORD reason, LPVOID reserved) {
  module_instance = static_cast<HINSTANCE>(instance);
  return TRUE;
}

// The functions below are called during the Gears installation process.

// TODO(andreip): remove this solution if we ever figure out a way to make
// IE reload BHOs at runtime, so that our extensions to the Tools menu
// would be visible without having to restart the browser.

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
  // Find out if IE Mobile is running.
  ProcessRestarter restarter(kInternetExplorer);
  if (restarter.IsProcessRunning()) {
    LPCTSTR message = reinterpret_cast<LPCTSTR>(LoadString(
        module_instance,
        IDS_RESTART_QUESTION,
        NULL,
        0));
    LPCTSTR title = reinterpret_cast<LPCTSTR>(LoadString(
        module_instance,
        IDS_RESTART_DIALOG_TITLE,
        NULL,
        0));
    // Can't communicate with the user. Something is terribly wrong. Abort.
    if (!message || !title) return kCancel;

    // Ask the user if she wants to proceed.
    int please_restart_iemobile = MessageBox(parent_window,
                                             message,
                                             title,
                                             MB_YESNO | MB_ICONQUESTION);
    if (please_restart_iemobile == IDYES) {
      // Try killing the process.
      if (FAILED(restarter.KillTheProcess(
              10,
              ProcessRestarter::KILL_METHOD_1_WINDOW_MESSAGE |
              ProcessRestarter::KILL_METHOD_3_TERMINATE_PROCESS,
              NULL))) {
        // Unfortunately we failed, so inform the user and abort.
        LPCTSTR fail_message = reinterpret_cast<LPCTSTR>(LoadString(
            module_instance,
            IDS_STOP_FAILURE_MESSAGE,
            NULL,
            0));

        if (!fail_message) return kCancel;

        MessageBox(parent_window, fail_message, title, MB_OK);
        return kCancel;
      }
      return kContinue;
    } else {
      // The user chose to cancel the installation.
      return kCancel;
    }
  }
  // IE Mobile wasn't running.
  return kContinue;
}

// This is called after installation completed. We start Internet Explorer
// and point it to http://gears.google.com.
__declspec(dllexport) InstallerActions Install_Exit(
    HWND    parent_window,
    LPCTSTR final_install_dir,
    WORD    failed_directories,
    WORD    failed_files,
    WORD    failed_registry_keys,
    WORD    failed_registry_values,
    WORD    failed_shortcuts) {
  ProcessRestarter restarter(kInternetExplorer);
  if (FAILED(restarter.StartTheProcess(kGearsSite))) {
    // Unfortunately we failed, so inform the user.
    LPCTSTR title = reinterpret_cast<LPCTSTR>(LoadString(
        module_instance,
        IDS_RESTART_DIALOG_TITLE,
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
#endif  // #ifdef WINCE
