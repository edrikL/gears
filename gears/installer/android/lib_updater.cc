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

#include "gears/installer/android/lib_updater.h"

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "gears/base/android/java_class.h"
#include "gears/base/android/java_exception_scope.h"
#include "gears/base/android/java_global_ref.h"
#include "gears/base/android/java_jni.h"
#include "gears/base/android/java_local_frame.h"
#include "gears/base/android/java_string.h"
#include "gears/base/common/file.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/stopwatch.h"

#define ANDROID_UID L"%7B99CED90A-A262-42B5-A819-FE32148F2FAB%7D"

static const char16* kUpgradeUrl = reinterpret_cast<const char16*>(
    L"http://tools.google.com/service/update2/ff?"
    L"guid=" ANDROID_UID
    L"&version=" PRODUCT_VERSION_STRING
    L"&application=" ANDROID_UID
    L"&appversion=1.0"
#ifdef OFFICIAL_BUILD
    L"&os=android&dist=google");
#else
    L"&os=android&dist=google&dev=1");
#endif

static const char16* kUpdateTimestampFile =
     STRING16(L"/" PRODUCT_SHORT_NAME L"timestamp");

// The delay before the first update.
static const int kFirstUpdateDelayMs = (1000 * 60 * 2);  // 2 minutes
// The time interval between the rest of the update checks.
static const int kUpdatePeriodMs = (1000 * 60 * 60 * 24);  // 24 hours

// A simple task that checks for a new Gears version.
class VersionCheckTask : public AsyncTask {
 public:
  VersionCheckTask() : AsyncTask(NULL) {}
  bool Init();

  enum {
    VERSION_CHECK_TASK_COMPLETE = 0,
  };

  // Returns the URL of the Gears zip package.
  const char16* Url() const;
  // Returns the version number of Gears on the update server.
  const char16* Version() const;

 private:
  // AsyncTask
  virtual void Run();

  // Parses XML and extracts the Gears version number and download URL.
  bool ExtractVersionAndDownloadUrl(const std::string16& xml);

  // Native callback, invoked if the parsing of the XML succeeds.
  static void SetVersionAndUrl(JNIEnv* env, jclass cls, jstring version,
                               jstring url, jlong self);

  static JNINativeMethod native_methods_[];

  std::string16 url_;
  std::string16 version_;

  DISALLOW_EVIL_CONSTRUCTORS(VersionCheckTask);
};

JNINativeMethod VersionCheckTask::native_methods_[] = {
  {"setVersionAndUrl",
   "(Ljava/lang/String;Ljava/lang/String;J)V",
   reinterpret_cast<void*>(VersionCheckTask::SetVersionAndUrl)
  },
};

static const char* kVersionExtractorClass =
    "com/google/android/gears/VersionExtractor";

static JavaClass::Method methods[] = {
  { JavaClass::kStatic, "extract", "(Ljava/lang/String;J)Z"},
};

enum VersionExtractorMethodID {
  VERSION_EXTRACTOR_METHOD_ID_EXTRACT = 0,
};


//------------------------------------------------------------------------------
// LibUpdater

LibUpdater* LibUpdater::instance_ = NULL;

// static
void LibUpdater::StartUpdateChecks() {
  assert(instance_ == NULL);
  instance_ = new LibUpdater();
}

// static
void LibUpdater::StopUpdateChecks() {
  delete instance_;
  instance_ = NULL;
}

LibUpdater::~LibUpdater() {
  LOG(("Stopping Gears updater..."));
  version_check_task_->Abort();
  timer_event_.Signal();
  AndroidMessageLoop::Stop(thread_id_);
  Join();
  LOG(("Gears updater stopped."));
}

// AsyncTask
void LibUpdater::HandleEvent(int msg_code,
                             int msg_param,
                             AsyncTask *source) {
  if (source && source == version_check_task_.get()) {
    assert(VersionCheckTask::VERSION_CHECK_TASK_COMPLETE == msg_code);
    bool success = static_cast<bool>(msg_param);
    if (success) {
      // TODO(andreip): Check version number and start download if needed.
      std::string version;
      String16ToUTF8(version_check_task_->Version(), &version);
      LOG(("The server returned: %s", version.c_str()));
    }

    // We finished processing an update. Record the timestamp and wait
    // some time before trying again.
    RecordUpdateTimestamp();
    if (timer_event_.WaitWithTimeout(kUpdatePeriodMs)) {
      // timer_event_ must have been signaled directly in the destructor.
      return;
    }

    // kUpdatePeriodMs has elapsed. Start the version check task again.
    StartVersionCheckTask();
  }
}

LibUpdater::LibUpdater() : version_check_task_(new VersionCheckTask()) {
  thread_id_ = Start();
  assert(thread_id_ != 0);
}

// Thread
void LibUpdater::Run() {
  LOG(("Gears updater started."));
  int delay = 0;
  // Determine what the initial delay should be.
  if (!MillisUntilNextUpdateCheck(&delay)) {
    LOG(("Run: Failed to get the initial delay."));
    delay = kFirstUpdateDelayMs;
  }

  // Wait for delay millis.
  if (timer_event_.WaitWithTimeout(delay)) {
    // event_ was signaled, destructor must have been called.
    return;
  }

  // Start the version check task.
  if (!StartVersionCheckTask()) {
    return;
  }

  // Run the message loop.
  AndroidMessageLoop::Start();
}

bool LibUpdater::StartVersionCheckTask() {
  // Initialize the task.
  if (!version_check_task_->Init()) {
    // The task could not be initialized.
    return false;
  }
  // Set ourselves as the listener.
  version_check_task_->SetListener(this);
  // Start the task.
  return version_check_task_->Start();
}

bool LibUpdater::RecordUpdateTimestamp() {
  std::string16 timestamp_file_path;
  if (!GetBaseResourcesDirectory(&timestamp_file_path)) {
    LOG(("RecordUpdateTimestamp: Could not get resources directory."));
    return false;
  }

  timestamp_file_path += kUpdateTimestampFile;

  // Create the timestamp file if it does not exist and open it.
  // This will set / update its modified time.
  scoped_ptr<File> timestamp_file(
      File::Open(timestamp_file_path.c_str(), File::WRITE, File::NEVER_FAIL));

  if (!timestamp_file.get()) {
    LOG(("RecordUpdateTimestamp: Failed to create or open timestamp file."));
    return false;
  }

  if (!timestamp_file->Truncate(0)) {
    LOG(("RecordUpdateTimestamp: Failed to touch timestamp file."));
  }

  return true;
}

bool LibUpdater::MillisUntilNextUpdateCheck(int *milliseconds_out) {
  std::string16 timestamp_file_path;
  if (!GetBaseResourcesDirectory(&timestamp_file_path)) {
    LOG(("MillisUntilNextUpdateCheck: Could not get resources directory."));
    return false;
  }

  timestamp_file_path += kUpdateTimestampFile;

  std::string timestamp_file_path_utf8;
  if (!String16ToUTF8(timestamp_file_path.c_str(), &timestamp_file_path_utf8)) {
    LOG(("MillisUntilNextUpdateCheck: Could not convert path strings."));
    return false;
  }

  struct stat statbuf;
  if (stat(timestamp_file_path_utf8.c_str(), &statbuf)) {
    LOG(("MillisUntilNextUpdateCheck: Could not stat timestamp file: %s",
         timestamp_file_path_utf8.c_str()));
    return false;
  }

  int64 last_update_check_millis = statbuf.st_mtime * 1000LL;
  int64 now = GetCurrentTimeMillis();
  int64 elapsed = now - last_update_check_millis;

  if (elapsed < 0) {
    // Bizarre situation when the timestamp file appears to have been modified
    // in the future.
    LOG(("MillisUntilNextUpdateCheck: update timestamp from the future!"));
    *milliseconds_out = kFirstUpdateDelayMs;
    return true;
  }

  if (elapsed > kUpdatePeriodMs) {
    // An update is overdue.
    *milliseconds_out = kFirstUpdateDelayMs;
    return true;
  }

  *milliseconds_out = kUpdatePeriodMs - static_cast<int>(elapsed);

  if (*milliseconds_out < kFirstUpdateDelayMs) {
    // Avoid starting the update immediately so that we don't slow
    // down the browser.
    *milliseconds_out = kFirstUpdateDelayMs;
  }

  return true;
}

// -----------------------------------------------------------------------------
// VersionCheckTask

bool VersionCheckTask::Init() {
  if (is_initialized_) return true;
  return AsyncTask::Init();
}

const char16* VersionCheckTask::Version() const {
  return version_.c_str();
}

const char16* VersionCheckTask::Url() const {
  return url_.c_str();
}

// AsyncTask
void VersionCheckTask::Run() {
  WebCacheDB::PayloadInfo payload;
  bool was_redirected;
  std::string16 error_msg;
  std::string16 url;
  bool success = false;
  if (AsyncTask::HttpGet(kUpgradeUrl,
                         true,
                         NULL,
                         NULL,
                         NULL,
                         &payload,
                         &was_redirected,
                         &url,
                         &error_msg)) {
    std::string16 xml;
    // payload.data can be empty in case of a 30x response.
    // The update server does not redirect, so we treat this as an error.
    if (payload.data->size() &&
        UTF8ToString16(reinterpret_cast<const char*>(&(*payload.data)[0]),
                       payload.data->size(),
                       &xml)) {
      if (ExtractVersionAndDownloadUrl(xml)) {
        success = true;
      }
    }
  }
  NotifyListener(VERSION_CHECK_TASK_COMPLETE, success);
}

bool VersionCheckTask::ExtractVersionAndDownloadUrl(const std::string16& xml) {
  JavaExceptionScope scope;
  JavaLocalFrame frame;
  JavaClass extractor_java_class;

  if (!extractor_java_class.FindClass(kVersionExtractorClass)) {
    LOG(("Could not find the VersionExtractor class.\n"));
    assert(false);
    return false;
  }

  // Register the native callback.
  jniRegisterNativeMethods(JniGetEnv(),
                           kVersionExtractorClass,
                           native_methods_,
                           NELEM(native_methods_));


  // Get the Java method ID.
  if (!extractor_java_class.GetMultipleMethodIDs(methods, NELEM(methods))) {
    LOG(("Could not find the VersionExtractor methods.\n"));
    assert(false);
    return false;
  }

  jlong native_object_ptr = reinterpret_cast<jlong>(this);
  jboolean result = JniGetEnv()->CallStaticBooleanMethod(
      extractor_java_class.Get(),
      methods[VERSION_EXTRACTOR_METHOD_ID_EXTRACT].id,
      JavaString(xml).Get(),
      native_object_ptr);

  return result;
}


// static
void VersionCheckTask::SetVersionAndUrl(JNIEnv* env, jclass cls,
                                        jstring version, jstring url,
                                        jlong self) {

  JavaString version_string(version);
  JavaString url_string(url);
  VersionCheckTask* task = reinterpret_cast<VersionCheckTask*>(self);

  // version, url and self must not be NULL. Currently there seems to
  // be a bug in JNI that manifests itself by creating jstrings that
  // point to a string containing the word "null" instead of the
  // jstrings being NULL themselves. TODO(andreip): remove the
  // comparison against "null" once the bug is fixed.
  assert(version_string.Get() != NULL &&
         version_string.ToString8().compare("null") != 0);

  assert(url_string.Get() != NULL &&
         url_string.ToString8().compare("null") != 0);

  assert(task);

  task->version_ = version_string.ToString16();
  task->url_ = url_string.ToString16();
}
