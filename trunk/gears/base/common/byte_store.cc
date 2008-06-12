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

#include <limits>

#include "gears/base/common/byte_store.h"

#include "gears/base/common/string_utils.h"
#include "gears/blob/blob_interface.h"

namespace {
const int64 kMaxBufferSize = 1024 * 1024;  // 1MB

// Presents a blob interface of a snapshot of a ByteStore.
// Data can continue to be added to the ByteStore, but this blob will only
// present the data that existed at construction time.
class ByteStoreBlob : public BlobInterface {
 public:
  explicit ByteStoreBlob(ByteStore* byte_store)
      : byte_store_(byte_store), length_(byte_store->Length()) {
  }

  int64 Length() const {
    return length_;
  }

  int64 Read(uint8 *destination, int64 offset, int64 max_bytes) const {
    if (offset >= length_) {
      return 0;
    } else if (offset + max_bytes >= length_) {
      max_bytes = length_ - offset;
    }
    return byte_store_->Read(destination, offset, max_bytes);
  }

 private:
  scoped_refptr<ByteStore> byte_store_;
  int64 length_;
};
}  // namespace

ByteStore::ByteStore() : file_op_(File::WRITE) {
}

ByteStore::~ByteStore() {
}

// We keep appending to data_ until we reach kMaxBufferSize, at which
// point we move everything to file_.
bool ByteStore::AddData(const void *data, int64 length) {
  if (length < 0) return false;
  if (length == 0) return true;

  MutexLock lock(&mutex_);

  if (!file_.get() && (length + data_.size() <= kMaxBufferSize)) {
    data_.insert(data_.end(), static_cast<const uint8*>(data),
                 static_cast<const uint8*>(data) + length);
    return true;
  }
  // Stored in a file.
  if (!file_.get()) {
    file_.reset(File::CreateNewTempFile());
    if (!data_.empty()) {
      int64 result = file_->Write(&data_[0], data_.size());
      if (result != data_.size()) {
        file_.reset();
        return false;
      }
      data_.clear();
    }
  }
  if (file_op_ == File::READ) {
    file_->Seek(0, File::SEEK_FROM_END);
    file_op_ = File::WRITE;
  }
  int64 pos = file_->Tell();
  int64 result = file_->Write(static_cast<const uint8*>(data), length);
  if (result != length) {
    file_->Truncate(pos);
    file_->Seek(0, File::SEEK_FROM_END);
    return false;
  }
  return true;
}

bool ByteStore::AddString(const std::string16 &data) {
  std::string utf8_string;
  String16ToUTF8(data.c_str(), &utf8_string);
  return AddData(utf8_string.data(), utf8_string.length());
}

void ByteStore::CreateBlob(scoped_refptr<BlobInterface> *blob) {
  blob->reset(new ByteStoreBlob(this));
}

int64 ByteStore::Length() const {
  MutexLock lock(&mutex_);

  if (!file_.get()) {
    return data_.size();
  }
  // Stored in a file.
  assert(data_.empty());
  if (file_op_ == File::WRITE) {
    file_->Flush();
    file_op_ = File::READ;
  }
  return file_->Size();
}

int64 ByteStore::Read(uint8 *destination, int64 offset, int64 max_bytes) const {
  if (offset < 0 || max_bytes < 0) {
    return -1;
  }

  MutexLock lock(&mutex_);

  if (!file_.get()) {
    if (data_.size() <= offset) {
      return 0;
    }
    if (data_.size() - offset < max_bytes) {
      max_bytes = data_.size() - offset;
    }
    if (max_bytes > std::numeric_limits<size_t>::max()) {
      max_bytes = std::numeric_limits<size_t>::max();
    }
  
    assert(offset <= kint32max);
    memcpy(destination, &data_[static_cast<int>(offset)],
                        static_cast<size_t>(max_bytes));
    return max_bytes;
  }
  // Stored in a file.
  assert(data_.empty());
  if (file_op_ == File::WRITE) {
    // The subsequent call to Seek will do a flush as well.
    file_op_ = File::READ;
  }
  bool result = file_->Seek(offset, File::SEEK_FROM_START);
  if (!result) {
    return -1;
  }
  return file_->Read(destination, max_bytes);
}
