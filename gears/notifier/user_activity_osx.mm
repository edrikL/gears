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
// Definitions for detecting user activity.

#ifdef OFFICIAL_BUILD
  // The notification API has not been finalized for official builds.
#else
#ifdef OS_MACOSX

#import "gears/notifier/user_activity.h"
#import "gears/base/common/common_osx.h"

class MacUserActivityMonitor : public UserActivityMonitor {
 public:
  MacUserActivityMonitor();
  virtual ~MacUserActivityMonitor();

 protected:
  virtual UserMode PlatformDetectUserMode();
  virtual uint32 GetMonitorPowerOffTimeSec();
  virtual uint32 GetUserIdleTimeMs();
  virtual bool IsScreensaverRunning();
  virtual bool IsWorkstationLocked() ;
  virtual bool IsFullScreenMode();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(MacUserActivityMonitor);
};

MacUserActivityMonitor::MacUserActivityMonitor() {
}

MacUserActivityMonitor::~MacUserActivityMonitor() {
}

UserMode MacUserActivityMonitor::PlatformDetectUserMode() {
  // No platform specific detection on OSX - returning USER_MODE_UNKNOWN
  // will make UserActivityMonitor to call "specific" detection methods below.
  return USER_MODE_UNKNOWN;
}

uint32 MacUserActivityMonitor::GetMonitorPowerOffTimeSec() {
  // TODO(dimich): Implement this.
  LOG(("UserActivityMonitor::GetMonitorPowerOffTimeSec: Not Implemented\n"));
  return 0;
}

uint32 MacUserActivityMonitor::GetUserIdleTimeMs() {
  // TODO(dimich): Implement this.
  LOG(("UserActivityMonitor::GetUserIdleTimeMs: Not Implemented\n"));
  return 0;
}

bool MacUserActivityMonitor::IsScreensaverRunning() {
  // TODO(dimich): Implement this.
  LOG(("UserActivityMonitor::IsScreensaverRunning: Not Implemented\n"));
  return false;
}

bool MacUserActivityMonitor::IsWorkstationLocked() {
  // TODO(dimich): Implement this.
  LOG(("UserActivityMonitor::IsWorkstationLocked: Not Implemented\n"));
  return false;
}

bool MacUserActivityMonitor::IsFullScreenMode() {
  // TODO(dimich): Implement this.
  LOG(("UserActivityMonitor::IsFullScreenMode: Not Implemented\n"));
  return false;
}

UserActivityMonitor *UserActivityMonitor::Create() {
  return new MacUserActivityMonitor();
}

#endif  // OS_MACOSX

#endif  // OFFICIAL_BUILD
