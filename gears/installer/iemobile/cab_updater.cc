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

#include "gears/installer/iemobile/cab_updater.h"

#include "gears/base/common/string16.h"
#include "gears/base/common/thread_locals.h"
#include "gears/base/ie/bho.h"
#include "gears/installer/iemobile/resource.h"

// TODO(andreip): When updating the Gears 'guid' field, consider switching
// 'application' to a GUID that means Pocket IE (to match Firefox pings).
// Also remind cprince to update the stats logic when 'guid' changes.
const char16* kApplicationId = L"%7Bc3fc95dBb-cd75-4f3d-a586-bcb7D004784c%7D";

// The topic for the message system.
const char16* kTopic = STRING16(L"Cab Update Event");
// The key for ThreadLocals.
const ThreadLocals::Slot kThreadLocalKey = ThreadLocals::Alloc();

// The delay before the first update.
const int32 kFirstUpdatePeriod = (1000 * 60 * 2);  // 2 minutes
// The time interval between the rest of the update checks.
const int32 kUpdatePeriod = (1000 * 60 * 60 * 24);  // 24 hours
// A grace period after a connection event. Setting up a new connection
// can take time so we wait a little after the event.
const int32 kGracePeriod = (1000 * 30);  // 30 seconds

CabUpdater::CabUpdater()
    : checker_(PeriodicChecker::CreateChecker()),
      is_showing_update_dialog_(false) {
}

CabUpdater::~CabUpdater() {
  ASSERT(checker_);
  delete checker_;
}

void CabUpdater::SetSiteAndStart(IWebBrowser2* browser) {
  ASSERT(checker_);
  browser_ = browser;
  if (checker_->Init(kFirstUpdatePeriod, kUpdatePeriod, kGracePeriod,
                     kApplicationId, this)) {
    MessageService::GetInstance()->AddObserver(this, kTopic);
    // We get stopped when the DLL unloads.
    ThreadLocals::SetValue(kThreadLocalKey, this, &Stop);
    checker_->Start();
  }
}

void CabUpdater::OnNotify(MessageService *service,
                          const char16 *topic,
                          const NotificationData *data) {
  // The user is already looking at the update dialog. Can only happen
  // in rare circumstances, when an update was found, the browser is running
  // but the user hasn't looked at it in the last 24 hours.
  if (is_showing_update_dialog_) return;

  is_showing_update_dialog_ = true;
  // Get the event in the right format.
  const SerializableString16* update_event =
      static_cast<const SerializableString16*>(data);
  // Check if the browser is busy.
  VARIANT_BOOL is_busy;
  browser_->get_Busy(&is_busy);
  // Detect the browser state.
  READYSTATE state;
  browser_->get_ReadyState(&state);
  // Only show the update dialog if the browser isn't in the middle of a
  // download or showing some other notification to the user.
  if (!is_busy && state == READYSTATE_COMPLETE) {
    HWND browser_window = BrowserHelperObject::GetBrowserWindow();
    if (browser_window && ShowUpdateDialog(browser_window)) {
        // Convert the URL to the format expected by the browser object.
        CComBSTR url(update_event->string_.c_str());
        browser_->Navigate(url, NULL, NULL, NULL, NULL);
    }
  }
  is_showing_update_dialog_ = false;
}

// Internal

// static
void CabUpdater::Stop(void* self) {
  ASSERT(self);
  CabUpdater* updater = reinterpret_cast<CabUpdater*>(self);
  // Stop the periodic checker, without waiting for it to clean up. In general
  // this is dangerous, because the periodic checker's worker thread may attempt
  // to access the object's internals, or call back to this object, after these
  // objects have been deleted. Hovever, in this case, the browser process is
  // being shut down and the checker's thread is likely to be forcefully
  // terminated before it completes, causing this call to block indefinitely if
  // we wait for clean up. Furthermore, any crashes that occur from this point
  // on will not be noticed as the browser shuts down!
  updater->checker_->Stop(false);
  MessageService::GetInstance()->RemoveObserver(updater, kTopic);
}

bool CabUpdater::ShowUpdateDialog(HWND browser_window) {
  HMODULE module = GetModuleHandle(PRODUCT_SHORT_NAME);
  if (!module) return false;
  // Load the dialog strings
  LPCTSTR message = reinterpret_cast<LPCTSTR>(
      LoadString(module,
                 IDS_UPGRADE_MESSAGE,
                 NULL,
                 0));
  LPCTSTR title = reinterpret_cast<LPCTSTR>(
      LoadString(module,
                 IDS_UPGRADE_DIALOG_TITLE,
                 NULL,
                 0));
  // Can't communicate with the user. Something is terribly wrong. Abort.
  if (!message || !title) return false;

  return (IDYES == MessageBox(browser_window,
                              message,
                              title,
                              MB_YESNO | MB_ICONQUESTION));
}

void CabUpdater::UpdateUrlAvailable(const std::string16 &url) {
  // Marshall the callback to the browser thread so that we can show the dialog.
  SerializableString16* update_event = new SerializableString16(url.c_str());
  MessageService::GetInstance()->NotifyObservers(kTopic, update_event);
}
