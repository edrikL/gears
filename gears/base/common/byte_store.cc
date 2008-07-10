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

#include "gears/base/common/byte_store.h"

#include "gears/base/common/string_utils.h"
#include "gears/blob/blob_interface.h"

const int64 kMaxBufferSize = 1024 * 1024;  // 1MB

// Presents a blob interface of a snapshot of a ByteStore.
// Data can continue to be added to the ByteStore, but this blob will only
// present the data that existed at construction time.
class ByteStore::Blob : public BlobInterface {
 public:
  explicit Blob(ByteStore* byte_store)
      : byte_store_(byte_store), length_(byte_store->Length()) {
  }

  virtual int64 Length() const {
    return length_;
  }

  virtual int64 Read(uint8 *destination, int64 offset, int64 max_bytes) const {
    if (offset >= length_) {
      return 0;
    } else if (offset + max_bytes >= length_) {
      max_bytes = length_ - offset;
    }
    return byte_store_->Read(destination, offset, max_bytes);
  }

  virtual bool GetDataElements(std::vector<DataElement> *elements) const {
    if (length_ == 0) {
      return true;  // don't add an element for no data
    }

    DataElement element;
    byte_store_->GetDataElement(&element);
    if (element.type() == DataElement::TYPE_FILE &&
        element.file_path().empty()) {
      return false;  // nameless temp files (posix) not supported
    }
    element.TrimToLength(length_);
    elements->push_back(element);
    return true;
  }

 private:
  scoped_refptr<ByteStore> byte_store_;
  int64 length_;
};


ByteStore::ByteStore()
    : file_op_(File::WRITE), is_finalized_(false), preserve_data_(false),
      async_add_length_(0) {
}

ByteStore::~ByteStore() {
}

// We keep appending to data_ until we reach kMaxBufferSize, at which
// point we move everything to file_.
bool ByteStore::AddData(const void *data, int64 length) {
  if (length < 0) return false;
  if (length == 0) return true;

  MutexLock lock(&mutex_);
  assert(!is_finalized_);
  assert(async_add_length_ == 0);

  if (IsUsingData() && (length + data_.size() <= kMaxBufferSize)) {
    data_.insert(data_.end(), static_cast<const uint8*>(data),
                 static_cast<const uint8*>(data) + length);
    return true;
  }
  return AddDataToFile(data, length);
}

int64 ByteStore::AddDataDirect(Writer *writer, int64 max_length) {
  if (max_length < 0) return 0;

  MutexLock lock(&mutex_);
  assert(!is_finalized_);
  assert(async_add_length_ == 0);

  if (IsUsingData() && (max_length + data_.size() <= kMaxBufferSize)) {
    int offset(data_.size());
    data_.resize(data_.size() + static_cast<unsigned>(max_length));
    int64 total_bytes_added(0);
    while (max_length > 0) {
      int64 bytes_added = writer->WriteToBuffer(&data_[offset], max_length);
      if (bytes_added == Writer::ASYNC) {
        async_add_length_ = max_length;
        return total_bytes_added;
      }
      assert(bytes_added >= 0);
      if (bytes_added == 0) break;
      total_bytes_added += bytes_added;
      offset += static_cast<int>(bytes_added);
      max_length -= bytes_added;
    }
    data_.resize(offset);
    return total_bytes_added;
  }

  // Too large for our memory buffer.  Write data to a temporary buffer and
  // then copy to a file.
  int64 buffer_size(std::min(max_length, kMaxBufferSize));
  if (buffer_.size() < static_cast<size_t>(buffer_size)) {
    buffer_.resize(static_cast<size_t>(buffer_size));
  }
  int64 total_bytes_added(0);
  while (max_length > 0) {
    int64 bytes_added = writer->WriteToBuffer(&buffer_[0], buffer_size);
    if (bytes_added == Writer::ASYNC) {
      // Create the file if it doesn't yet exist.
      AddDataToFile(&buffer_[0], 0);
      async_add_length_ = buffer_size;
      return total_bytes_added;
    }
    assert(bytes_added >= 0);
    if (bytes_added == 0) break;
    if (!AddDataToFile(&buffer_[0], bytes_added)) {
      break;
    }
    total_bytes_added += bytes_added;
    max_length -= bytes_added;
    buffer_size = std::min(max_length, buffer_size);
  }
  return total_bytes_added;
}

void ByteStore::AddDataDirectFinishAsync(int64 length) {
  MutexLock lock(&mutex_);
  assert(async_add_length_ > 0);
  assert(length <= async_add_length_);
  assert(!is_finalized_);

  if (IsUsingData()) {
    data_.resize(data_.size() -
                 static_cast<size_t>(async_add_length_ - length));
  } else {
    AddDataToFile(&buffer_[0], length);
  }
  async_add_length_ = 0;
}

// Writes data to a temporary file.  Will create & open a file if not already
// done.  After creating & opening a file, will copy current contents of data_
// to the file before adding the new data.
//
// If called with length == 0, will create a temp file and transfer
// the current contents of data_ (if any) to the file.
//
// Expects the caller to be holding mutex_.
bool ByteStore::AddDataToFile(const void *data, int64 length) {
  if (!file_.get()) {
    file_.reset(File::CreateNewTempFile());
    if (!data_.empty()) {
      int64 result = file_->Write(&data_[0], data_.size());
      if (result != data_.size()) {
        file_.reset();
        return false;
      }
    }
    if (!preserve_data_) {
      data_.clear();
      // Note, if the flag is set, we keep data_ around so ptrs returned
      // via GetDataElement remain valid for the life of the BlobStore.
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
  blob->reset(new ByteStore::Blob(this));
}

void ByteStore::Finalize() {
  MutexLock lock(&mutex_);
  is_finalized_ = true;
}

void ByteStore::GetDataElement(DataElement *element) {
  assert(element);

  MutexLock lock(&mutex_);
  if (!is_finalized_ && IsUsingData() && !preserve_data_) {
    // We reserve space so the vector won't get reallocated as
    // new data is added.
    data_.reserve(kMaxBufferSize);
    preserve_data_ = true;
  }

  if (IsUsingFile()) {
    file_->Flush();
    element->SetToFilePath(file_->GetFilePath());
  } else if (!data_.empty()) {
    element->SetToBytes(&data_[0], static_cast<int>(data_.size()));
  } else {
    element->SetToBytes(NULL, 0);
  }
}

int64 ByteStore::Length() const {
  MutexLock lock(&mutex_);

  if (IsUsingData()) {
    return data_.size() - async_add_length_;
  }
  // Stored in a file.
  assert(data_.empty() || preserve_data_);
  if (file_op_ == File::WRITE) {
    file_->Flush();
    file_op_ = File::READ;
  }
  return file_->Size();
}

int64 ByteStore::Read(uint8 *destination,
                      int64 offset, int64 max_length) const {
  if (offset < 0 || max_length < 0) {
    return -1;
  }

  MutexLock lock(&mutex_);

  if (IsUsingData()) {
    if (data_.size() <= offset) {
      return 0;
    }
    if (data_.size() - offset < max_length) {
      max_length = data_.size() - offset;
    }
    memcpy(destination, &data_[static_cast<int>(offset)],
                        static_cast<size_t>(max_length));
    return max_length;
  }
  // Stored in a file.
  assert(data_.empty() || preserve_data_);
  if (file_op_ == File::WRITE) {
    // The subsequent call to Seek will do a flush as well.
    file_op_ = File::READ;
  }
  bool result = file_->Seek(offset, File::SEEK_FROM_START);
  if (!result) {
    return -1;
  }
  return file_->Read(destination, max_length);
}

int64 ByteStore::ReadDirect(Reader *reader,
                            int64 offset, int64 max_length) const {
  if (offset < 0 || max_length < 0) {
    return -1;
  }
  {
    MutexLock lock(&mutex_);
    if (IsUsingData()) {
      if (data_.size() <= offset) {
        return 0;
      }
      if (data_.size() - offset < max_length) {
        max_length = data_.size() - offset;
      }
      int64 total_bytes_read(0);
      int pos(static_cast<int>(offset));
      while (max_length > 0) {
        int64 bytes_read = reader->ReadFromBuffer(&data_[pos], max_length);
        assert(bytes_read >= 0);
        if (bytes_read == 0) break;
        assert(bytes_read <= max_length);
        total_bytes_read += bytes_read;
        pos += static_cast<int>(bytes_read);
        max_length -= bytes_read;
      }
      return total_bytes_read;
    }
  }

  // Create a temporary buffer and call Read()
  int64 buffer_size(std::min(max_length, kMaxBufferSize));
  if (buffer_.size() < static_cast<size_t>(buffer_size)) {
    buffer_.resize(static_cast<size_t>(buffer_size));
  }
  int64 total_bytes_read(0);
  while (max_length > 0) {
    int64 bytes_read1 = Read(&buffer_[0], offset, buffer_size);
    if (bytes_read1 <= 0) break;  // No more data available, or error.
    int64 bytes_read = reader->ReadFromBuffer(&buffer_[0], bytes_read1);
    assert(bytes_read >= 0);
    if (bytes_read == 0) break;
    total_bytes_read += bytes_read;
    offset += bytes_read;
    max_length -= bytes_read;
    buffer_size = std::min(max_length, buffer_size);
  }
  return total_bytes_read;
}

void ByteStore::Reserve(int64 length) {
  MutexLock lock(&mutex_);
  if (IsUsingFile()) return;
  if (length > kMaxBufferSize) {
    uint8 data;
    AddDataToFile(&data, 0);
  } else {
    data_.reserve(static_cast<size_t>(length));
  }
}
