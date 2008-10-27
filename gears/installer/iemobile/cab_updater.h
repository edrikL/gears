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
//
// This file defines the CabUpdater class, which uses the PeriodicChecker to
// handle automitic updating.
//
// 1. When the Gears BHO is loaded, the CabUpdater is started immediately after
// the HTTP handler is registered. The PeriodicChecker is configured to check 
// for an update after two minutes and every 24 hours thereafter. The
// PeriodicChecker is started.
// 2. If the PeriodicChecker reports a new version, we present the user with a
// dialog, asking if they would like to upgrade.
// 3. If the user agrees, the CabUpdater uses the IWebBrowser2 pointer to make
// the browser download the new CAB.
// 4. The CabUpdater waits for a new update from the PeriodicChecker and
// continues from step 2.

#ifndef GEARS_INSTALLER_IEMOBILE_CAB_UPDATER_H__
#define GEARS_INSTALLER_IEMOBILE_CAB_UPDATER_H__

#include "gears/base/common/common.h"
#include "gears/base/common/message_service.h"
#include "gears/installer/common/download_task.h"
#include "gears/installer/iemobile/periodic_checker.h"

class CabUpdater
    : public MessageObserverInterface,
      public PeriodicChecker::ListenerInterface,
      public DownloadTask::ListenerInterface {
 public:
  CabUpdater();
  virtual ~CabUpdater();
  // Starts the updater.
  void Start(HWND browser_window);

  // MessageObserverInterface

  // Listens and responds to update events (sent as SerializableString16).
  // The NotificationData event object will hold the URL the browser
  // should be pointed to.
  virtual void OnNotify(MessageService *service,
                        const char16 *topic,
                        const NotificationData *data);

 private:
  // Stop method called when the DLL unloads.
  static void Stop(void* self);
  // Shows the dialog saying that an update is available.
  bool ShowUpdateAvailableDialog();
  // Shows the dialog saying that installation failed.
  bool ShowInstallationFailedDialog();

  // PeriodicChecker::ListenerInterface implementation
  virtual void UpdateUrlAvailable(const std::string16 &url);

  // DownloadTask::ListenerInterface implementation
  virtual void DownloadComplete();

  // The periodic update checker. Owned.
  PeriodicChecker* checker_;
  // Is the update dialog showing?
  bool is_showing_update_dialog_;

  // The task to download the new CAB.
  DownloadTask *download_task_;
  Mutex download_task_mutex_;

  // The window that will be used as the parent of the dialogs.
  HWND browser_window_;

  // The temporary file that will be used for the downloaded CAB.
  std::string16 temp_file_path_;

  DISALLOW_EVIL_CONSTRUCTORS(CabUpdater);
};

#endif  // GEARS_INSTALLER_IEMOBILE_CAB_UPDATER_H__
