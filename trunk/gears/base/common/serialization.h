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

#ifndef GEARS_BASE_COMMON_SERIALIZATION_H__
#define GEARS_BASE_COMMON_SERIALIZATION_H__

#include <map>
#include <vector>

#include "gears/base/common/int_types.h"
#include "gears/base/common/mutex.h"
#include "gears/base/common/string16.h"

enum SerializableClassId {
  SERIALIZABLE_NULL,
  SERIALIZABLE_TEST,
  SERIALIZABLE_UPDATE_TASK_ERROR_EVENT,
  SERIALIZABLE_UPDATE_TASK_PROGRESS_EVENT,
  SERIALIZABLE_UPDATE_TASK_COMPLETION_EVENT,
};

class Serializable;
class Serializer;
class Deserializer;

typedef Serializable *(*SerializableFactoryMethod)(void);

// Interface for serializeable objects.
class Serializable {
 public:
  static void RegisterClass(SerializableClassId class_id,
                            SerializableFactoryMethod factory);
  static Serializable *CreateClass(SerializableClassId class_id);

  virtual ~Serializable() {};

  // Returns the class id.
  virtual SerializableClassId GetSerializableClassId() = 0;

  // Write the serialized object.  The class id and data size are not written.
  virtual bool Serialize(Serializer *out) = 0;

  // Read the serialized data into this object.  The class id and size are
  // not read.
  virtual bool Deserialize(Deserializer *in) = 0;
 private:
  static std::map<SerializableClassId, SerializableFactoryMethod> constructors_;
  static Mutex constructors_lock_;
};

// Wraps a buffer and manages writes to it from a serializable object.
class Serializer {
 public:
  // Construct a Serializer that will write data to the provided buffer.
  Serializer(std::vector<uint8> *buf) : buffer_(buf) {}

  // Serialize the provided object to the internal buffer.  If this is NULL,
  // SERIALIZABLE_NULL will be written with a size of 0.
  bool WriteObject(Serializable *obj);

  void WriteInt(const int data);
  void WriteBool(const bool data);
  void WriteString(const char16 *data);
  void WriteBytes(const void *data, size_t length);

 private:
  std::vector<uint8> *buffer_;
};

// Wraps a buffer and manages extracting serialized data from it.
class Deserializer {
 public:
  // Create a Deserializer that will extract objects from the provided
  // buffer.
  Deserializer(uint8 *buf, size_t len) :
    buffer_(buf), length_(len), read_pos_(0) {}

  // Fill the provided pointer with a new object as read from the
  // serialized stream.  This can be NULL, if a NULL object was written.
  bool CreateAndReadObject(Serializable **out);

  bool ReadInt(int *output);
  bool ReadBool(bool *output);
  bool ReadString(std::string16 *output);
  bool ReadBytes(void *output, size_t length);

 private:
  uint8 *buffer_;
  size_t length_;
  size_t read_pos_;
};

#endif  // GEARS_BASE_COMMON_SERIALIZATION_H__
