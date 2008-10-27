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

#include "gears/installer/common/installer_utils.h"

#include "gears/installer/common/process_restarter.h"

static const char16 *kWceloadExe = L"wceload.exe";
static const char16 *kWceloadWindowName = L"Installation";

bool InstallCab(const char16 *path) {
  // Only one instance of wceload can run at any one time, so we must kill any
  // existing process before we start our process. Note that this will fail on
  // some devices irrespective of the kill method or timeout, because we do do
  // not have permission to get process IDs.
  ProcessRestarter wceload(kWceloadExe, kWceloadWindowName);
  if (FAILED(wceload.KillTheProcess(
      1000,
      ProcessRestarter::KILL_METHOD_1_WINDOW_MESSAGE |
      ProcessRestarter::KILL_METHOD_2_THREAD_MESSAGE |
      ProcessRestarter::KILL_METHOD_3_TERMINATE_PROCESS,
      NULL))) {
    return false;
  }
  std::string16 arguments;
  arguments += L"\"";
  arguments += path;
  arguments += L"\"";
  // Ideally, we'd run the install process without any UI. However, on some
  // (Smartphone/Standard) devices, running the installer without UI causes it
  // to fail. It seems that this is because these devices try to display a
  // 'previous version will be removed' dialog during installation. Surpressing
  // this causes the installation to fail, and the device hangs on the 'must
  // restart device' dialog. (These devices also fail to restart IE
  // automatically, so we prompt for a manual restart). Also, some devices may
  // not support the '/noui' flag.
  //
  // TODO(steveblock): Revisit this issue to look for a better solution.
  //arguments += L" /noui";  // Do not show any UI
  HRESULT result = wceload.StartTheProcess(arguments);
  return SUCCEEDED(result);
}
