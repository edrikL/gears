// Copyright 2009, Google Inc.
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

#include <assert.h>
#include <vector>

#include "gears/localserver/ie/http_intercept.h"

#include "gears/base/common/mutex.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/async_task.h"
#include "gears/localserver/common/localserver_db.h"
#include "gears/ui/common/alert_dialog.h"

#ifdef USE_HTTP_HANDLER_APP
#include "gears/localserver/ie/http_handler_app.h"
#else
#include "gears/localserver/ie/http_handler_patch.h"
#endif

// {B320A625-02E1-4cc8-A864-04E825297A20}
const GUID SID_HttpInterceptBypass =
{ 0xb320a625, 0x2e1, 0x4cc8,
  { 0xa8, 0x64, 0x4, 0xe8, 0x25, 0x29, 0x7a, 0x20 } };

// {2C3ED4D8-F7A1-48b4-A979-5B197C15D0FC}
const GUID IID_HttpInterceptBypass =
{ 0x2c3ed4d8, 0xf7a1, 0x48b4,
  { 0xa9, 0x79, 0x5b, 0x19, 0x7c, 0x15, 0xd0, 0xfc } };


HRESULT InitializeHttpInterception() {
#ifdef USE_HTTP_HANDLER_APP
  return HttpHandlerAPP::Install();
#else
  return HttpHandlerPatch::Install();
#endif
}
 
void SetBypassHttpInterception() {
  HttpHandlerBase::SetBypassCache();
}

#ifdef OS_WINCE
// These are a noop on windows mobile
void HttpInterceptCheck::StartCheck(const char16 *url) {
}
void HttpInterceptCheck::FinishCheck() {
}
#ifdef DEBUG
void HttpInterceptCheck::ResetHasWarned() {
}
#endif
#else  // OS_WINCE

// Shows an alert on a seperate thread.
class WarningTask : public AsyncTask {
 public:
  static bool HasWarned() {
    return has_warned_;
  }

#ifdef DEBUG
  static void ResetHasWarned() {
    has_warned_ = false;
  }
#endif

  static void WarnIfNeeded() {
    MutexLock lock(&has_warned_mutex_);
    if (!has_warned_) {
      has_warned_ = true;
      WarningTask *task = new WarningTask();
      task->StartIt();
      task->DeleteWhenDone();
    }
  }

 private:
  WarningTask() : AsyncTask(NULL) {
  }

  void StartIt() {
    Init();
    Start();
  }

  virtual void Run() {
    AlertDialog::ShowModal(kAlertIncompatibilityDetected);
  }

  static bool has_warned_;
  static Mutex has_warned_mutex_;
};

bool WarningTask::has_warned_ = false;
Mutex WarningTask::has_warned_mutex_;

void HttpInterceptCheck::StartCheck(const char16 *url) {
  assert(url);
  assert(!is_checking_);
  is_checking_ = true;
  origin_.InitFromUrl(url);
  handler_started_count_ = HttpHandlerBase::GetStartedCount();
}

void HttpInterceptCheck::FinishCheck() {
  assert(is_checking_);
  is_checking_ = false;

  if (WarningTask::HasWarned()) {
    return;  // Only warn one time
  }
  if (!origin_.initialized()) {
    return;  // Not a supported url scheme
  }
  if (handler_started_count_ != HttpHandlerBase::GetStartedCount()) {
    return;  // We intercepted something
  }
  if (origin_.scheme() != HttpConstants::kHttpScheme &&
      origin_.scheme() != HttpConstants::kHttpsScheme) {
    return;  // We only expect to intercept http and https
  }

  WebCacheDB *db = WebCacheDB::GetDB();
  if (!db) {
    return;  // An indication of a corrupt database
    // TODO(michaeln): also warn for this type of error?
  }

  // We defer warning until requests into origins that are
  // represented in the localserver are observed.
  std::vector<WebCacheDB::ServerInfo> servers;
  db->FindServersForOrigin(origin_, &servers);
  if (servers.empty()) {
    return;  // No resources captured for this origin
  }

  WarningTask::WarnIfNeeded();
}

#ifdef DEBUG
// static
void HttpInterceptCheck::ResetHasWarned() {
  WarningTask::ResetHasWarned();
}
#endif
#endif  // OS_WINCE


