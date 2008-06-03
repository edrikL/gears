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

#include "gears/blob/blob_builder.h"

#include <limits>
#include "gears/base/common/string_utils.h"
#include "gears/blob/blob_interface.h"
#include "gears/blob/buffer_blob.h"
#include "gears/blob/join_blob.h"

namespace {
const std::string::size_type max_data_len =
    std::numeric_limits<std::vector<uint8>::size_type>::max();
}

BlobBuilder::BlobBuilder() : length_(0) {
}

BlobBuilder::~BlobBuilder() {
}

bool BlobBuilder::AddBlob(BlobInterface *blob) {
  if (!IncrementAndCheckSize(blob->Length())) return false;
  if (blob->Length() == 0) return true;
  PushDataAsBlob();
  blob_list_.push_back(blob);
  return true;
}

bool BlobBuilder::AddData(const void *data, int64 length) {
  if (!IncrementAndCheckSize(length)) return false;
  if (length == 0) return true;
  // TODO(bgarcia): handle larger data by splitting into multiple BufferBlobs
  // or creating a FileBlob.  For now, return false if it's too large.
  if (max_data_len - data_.size() < length) return false;
  data_.insert(data_.end(), static_cast<const uint8*>(data),
               static_cast<const uint8*>(data) + length);
  return true;
}

bool BlobBuilder::AddString(const std::string16 &data) {
  std::string utf8_string;
  String16ToUTF8(data.c_str(), &utf8_string);
  return AddData(utf8_string.c_str(), utf8_string.length());
}

void BlobBuilder::CreateBlob(scoped_refptr<BlobInterface> *blob) {
  PushDataAsBlob();
  length_ = 0;
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

void BlobBuilder::PushDataAsBlob() {
  if (!data_.empty()) {
    scoped_refptr<BlobInterface> blob = new BufferBlob(&data_);
    blob_list_.push_back(blob);
  }
}

bool BlobBuilder::IncrementAndCheckSize(int64 size_increment) {
  if (size_increment < 0) {
    return false;
  }
  if (std::numeric_limits<int64>::max() - length_ < size_increment) {
    return false;
  }
  length_ += size_increment;
  return true;
}
