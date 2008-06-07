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

#ifdef OFFICIAL_BUILD
// The blob API has not been finalized for official builds
#else
#include "gears/blob/blob_builder.h"

#include <limits>
#include "gears/base/common/file.h"
#include "gears/base/common/string_utils.h"
#include "gears/blob/blob_interface.h"
#include "gears/blob/buffer_blob.h"
#include "gears/blob/file_blob.h"
#include "gears/blob/join_blob.h"

namespace {
const int64 kMaxBufferSize = 1024 * 1024; // 1MB
}

BlobBuilder::BlobBuilder() {
}

BlobBuilder::~BlobBuilder() {
}

bool BlobBuilder::AddBlob(BlobInterface *blob) {
  if (blob->Length() < 0) return false;
  if (blob->Length() == 0) return true;
  PushDataAsBlob();
  blob_list_.push_back(blob);
  return true;
}

bool BlobBuilder::AddData(const void *data, int64 length) {
  if (length < 0) return false;
  if (length == 0) return true;
  if (!file_.get() && (length + data_.size() > kMaxBufferSize)) {
    file_.reset(File::CreateNewTempFile());
    if (!data_.empty()) {
      int64 result = file_->Write(&data_[0], data_.size());
      if (result != length) return false;
      data_.clear();
    }
  }

  if (file_.get()) {
    int64 pos = file_->Tell();
    int64 result = file_->Write(static_cast<const uint8*>(data), length);
    if (result != length) {
      file_->Truncate(pos);
      file_->Seek(pos, File::SEEK_FROM_START);
      return false;
    }
  } else {
    data_.insert(data_.end(), static_cast<const uint8*>(data),
                 static_cast<const uint8*>(data) + length);
  }
  return true;
}

bool BlobBuilder::AddString(const std::string16 &data) {
  std::string utf8_string;
  String16ToUTF8(data.c_str(), &utf8_string);
  return AddData(utf8_string.c_str(), utf8_string.length());
}

void BlobBuilder::CreateBlob(scoped_refptr<BlobInterface> *blob) {
  PushDataAsBlob();
  if (blob_list_.size() == 0) {
    *blob = new EmptyBlob;
  } else if (blob_list_.size() == 1) {
    *blob = blob_list_.back();
    blob_list_.pop_back();
  } else {
    *blob = new JoinBlob(blob_list_);
    blob_list_.clear();
  }
}

int64 BlobBuilder::Length() const {
  int64 length(0);
  if (file_.get()) {
    length = file_->Size();
    // TODO(bgarcia): If something bad happened to the file, is there
    //                anything we can do to recover from this situation?
    assert(length != -1);
  }
  length += data_.size();
  for (unsigned i = 0; i < blob_list_.size();  ++i) {
    length += blob_list_[i]->Length();
  }
  return length;
}

void BlobBuilder::PushDataAsBlob() {
  if (!data_.empty()) {
    assert(!file_.get());
    blob_list_.push_back(new BufferBlob(&data_));
    data_.clear();
  } else if (file_.get()) {
    file_->Flush();
    blob_list_.push_back(new FileBlob(file_.release()));
  }
}

#endif
