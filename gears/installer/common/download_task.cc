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

#include "gears/installer/common/download_task.h"

#include "gears/base/common/file.h"
#include "gears/blob/blob_interface.h"
#include "gears/blob/blob_utils.h"

static const int kBufferSizeBytes = 32 * 1024;  // 32 Kb

// static
DownloadTask *DownloadTask::Create(const char16* url,
                                   const char16* save_path,
                                   ListenerInterface *listener) {
  scoped_ptr<DownloadTask> task(new DownloadTask(url, save_path, listener));
  assert(task.get());
  if (!task.get() || !task->Init() || !task->Start()) {
    return NULL;
  }
  return task.release();
}

void DownloadTask::StopThreadAndDelete() {
  Abort();
  DeleteWhenDone();
}

DownloadTask::DownloadTask(const char16* url,
                           const char16* save_path,
                           ListenerInterface *listener)
    : AsyncTask(NULL),
      url_(url),
      save_path_(save_path),
      listener_(listener) {
  assert(listener_);
}

// AsyncTask implementation
void DownloadTask::Run() {
  WebCacheDB::PayloadInfo payload;
  scoped_refptr<BlobInterface> payload_data;
  bool was_redirected;
  bool success = false;
  if (AsyncTask::HttpGet(url_.c_str(),
                         true,
                         NULL,
                         NULL,
                         NULL,
                         &payload,
                         &payload_data,
                         &was_redirected,
                         NULL,  // full_redirect_url
                         NULL)) {  // error_msg
    // payload_data can be empty in case of a 30x response.
    // The update server does not redirect, so we treat this as an error.
    if (!was_redirected &&
        payload.PassesValidationTests(NULL) &&
        payload_data->Length() &&
        SaveToFile(payload_data.get())) {
      success = true;
    }
  }
  listener_->DownloadComplete(success);
}

bool DownloadTask::SaveToFile(BlobInterface *data) {
  assert(data != NULL);
  std::string16 directory;
  if (!File::GetParentDirectory(save_path_, &directory)) {
    LOG(("DownloadTask::SaveToFile : Failed to get parent directory."));
    return false;
  }
  if (!File::RecursivelyCreateDir(directory.c_str())) {
    LOG(("DownloadTask::SaveToFile : Could not create save directory."));
    return false;
  }

  scoped_ptr<File> file(File::Open(save_path_.c_str(),
                                   File::WRITE, File::NEVER_FAIL));
  if (!file.get()) {
    LOG(("DownloadTask::SaveToFile : Could not create file."));
    return false;
  }

  scoped_array<uint8> buffer;
  buffer.reset(new uint8[kBufferSizeBytes]);
  int64 offset = 0;
  int64 counter = 0;
  do {
    counter = data->Read(buffer.get(), offset, kBufferSizeBytes);
    if (counter > 0) {
      file->Write(buffer.get(), counter);
      offset += counter;
    }
  } while (kBufferSizeBytes == counter);

  if (counter < 0) {
    assert(counter == -1);
    LOG(("DownloadTask::SaveToFile : Could not save to temp directory."));
    return false;
  }

  return true;
}
