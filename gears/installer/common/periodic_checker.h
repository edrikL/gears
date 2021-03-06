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
// PROCUREMENT OF SBUSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This file defines the PeriodicChecker class. This class makes regular
// requests to a server to check for upgrades available for the specified
// application. On receiving a response from the server, this class parses the
// response to extract the latest available version and its download URL. If a
// new version is available, a callback is made to the listener with the
// download URL of the new version.

#ifndef GEARS_INSTALLER_COMMON_PERIODIC_CHECKER_H__
#define GEARS_INSTALLER_COMMON_PERIODIC_CHECKER_H__

#include "gears/base/common/atl_headers_win32.h"
#include "gears/base/common/browsing_context.h"
#include <gears/base/common/string16.h>
#include "gears/base/common/wince_compatibility.h"

// An implemnentation detail of PeriodicChecker.
class VersionFetchTask;

class PeriodicChecker : public CWindowImpl<PeriodicChecker> {
 public:
  class ListenerInterface {
   public:
    virtual void UpdateUrlAvailable(const std::string16 &url) = 0;
  };

  // Simple factory method.
  static PeriodicChecker *CreateChecker(BrowsingContext *browsing_context);
  // Initializes the checker and sets the desired times between update checks.
  // Also sets the application ID sent to the server for update checks and the
  // listener for callbacks.
  bool Init(int first_period, int normal_period, int grace_period,
            const std::string16 &guid, ListenerInterface *listener);
  // Starts the checker thread.
  void Start();
  // Stops the checker thread. Parmeter specifies whether this call should block
  // waiting for the checker to clean up. In general, specifying false here is
  // dangerous. See CabUpdater::Stop for details of why this flag is required.
  void Stop(bool wait_for_cleanup);
  ~PeriodicChecker();

  // TODO(andreip): aggregate all Gears WM_USER messages in the same
  // header to avoid overlap.
  static const int kVersionFetchTaskMessageId = 0;
  static const int WM_FETCH_TASK_COMPLETE =
      WM_USER + kVersionFetchTaskMessageId;

 private:
  // CWindowImpl
  BEGIN_MSG_MAP(PeriodicChecker)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_FETCH_TASK_COMPLETE, OnFetchTaskComplete)
  END_MSG_MAP()

  // Internal
  PeriodicChecker(BrowsingContext *browsing_context);
  // Message handlers.
  LRESULT OnTimer(UINT message, WPARAM unused1, LPARAM unused2, BOOL &handled);
  LRESULT OnFetchTaskComplete(UINT message, WPARAM success, LPARAM unused,
                              BOOL &handled);

  // This message handler is different because it needs to be registered at
  // runtime with RegisterWindowMessage (it's comming from the connection
  // manager process and that API only exposes a string).
  void OnConnectionStatusChanged();
  // Thread entry.
  static unsigned int _stdcall ThreadMain(void* checker);
  // Runs the Windows message loop machinery (essentially a lot of boilerplate).
  void RunMessageLoop();
  // Timer utilitiy.
  void ResetTimer(int period);

  // The delay before the first check.
  int first_period_;
  // The normal time interval between checks.
  int normal_period_;
  // The time interval to wait after a connection up event.
  int grace_period_;
  // The GUID to include in the request query parameters.
  std::string16 guid_;
  // The listener for callbacks
  ListenerInterface *listener_;
  // Events used for shutting down.
  CEvent stop_event_;
  CEvent thread_complete_event_;
#ifdef DEBUG
  // Whether we should wait for the thread to clean up during shutdown.
  bool wait_for_cleanup_;
#endif
  // The thread handle.
  HANDLE thread_;
  // The window handle.
  HWND window_;
  // The timer ID.
  int timer_;
  // The task responsible for doing the HTTP requests to upgrade URL.
  VersionFetchTask *task_;
  // Was this a normal timer event?
  bool normal_timer_event_;
  // The message ID used for listening to connection status events.
  uint32 con_mgr_msg_;

  scoped_refptr<BrowsingContext> browsing_context_;

  DISALLOW_EVIL_CONSTRUCTORS(PeriodicChecker);
};

#endif  // GEARS_INSTALLER_COMMON_PERIODIC_CHECKER_H__
