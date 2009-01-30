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

#include "gears/installer/iemobile/cab_updater_ie.h"

#include "gears/base/common/file.h"
#include "gears/base/common/thread_locals.h"

static const char16 *kGearsCabName = L"gears-wince-opt.cab";
static const int kFirstUpdatePeriod = (1000 * 60 * 2);  // 2 minutes

IECabUpdater::IECabUpdater() {
  checker_ = PeriodicChecker::CreateChecker(NULL);
  first_update_period_ = kFirstUpdatePeriod;
  module_name_ = PRODUCT_SHORT_NAME;
}

// PeriodicChecker::ListenerInterface implementation
void IECabUpdater::UpdateUrlAvailable(const std::string16 &url) {
  // Start the download task
  MutexLock lock(&download_task_mutex_);
  if (download_task_ == NULL) {
    // Cache the temp file we use for the download and start the download.

    if (File::GetBaseTemporaryDirectory(&temp_file_path_)) {
      temp_file_path_ += kGearsCabName;
      download_task_ = DownloadTask::Create(url.c_str(),
                                            temp_file_path_.c_str(),
                                            this,
                                            NULL);
    }
  }
}
