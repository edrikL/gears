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
#ifndef GEARS_NOTIFIER_NOTIFICATION_H__
#define GEARS_NOTIFIER_NOTIFICATION_H__

#include "gears/base/common/serialization.h"

// Describes the information about a notification.
// Note: do not forget to increase kNotificationVersion if you make any change
// to this class.
class Notification : public Serializable {
 public:
  Notification() : version_(kNotificationVersion) {}

  void CopyFrom(const Notification& from) {
    version_ = from.version_;
    title_ = from.title_;
    icon_ = from.icon_;
    service_ = from.service_;
    id_ = from.id_;
    description_ = from.description_;
  }

  virtual SerializableClassId GetSerializableClassId() {
    return SERIALIZABLE_DESKTOP_NOTIFICATION;
  }

  virtual bool Serialize(Serializer *out) {
    out->WriteInt(version_);
    out->WriteString(title_.c_str());
    out->WriteString(icon_.c_str());
    out->WriteString(service_.c_str());
    out->WriteString(id_.c_str());
    out->WriteString(description_.c_str());
    return true;
  }

  virtual bool Deserialize(Deserializer *in) {
    if (!in->ReadInt(&version_) ||
        version_ != kNotificationVersion ||
        !in->ReadString(&title_) ||
        !in->ReadString(&icon_) ||
        !in->ReadString(&service_) ||
        !in->ReadString(&id_) ||
        !in->ReadString(&description_)) {
      return false;
    }
    return true;
  }

  static Serializable *New() {
    return new Notification;
  }

  static void RegisterAsSerializable() {
    Serializable::RegisterClass(SERIALIZABLE_DESKTOP_NOTIFICATION, New);
  }

  const std::string16& title() const { return title_; }
  const std::string16& icon() const { return icon_; }
  const std::string16& service() const { return service_; }
  const std::string16& id() const { return id_; }
  const std::string16& description() const { return description_; }

  void set_title(const std::string16& title) { title_ = title; }
  void set_icon(const std::string16& icon) { icon_ = icon; }
  void set_service(const std::string16& service) { service_ = service; }
  void set_id(const std::string16& id) { id_ = id; }
  void set_description(const std::string16& description) {
    description_ = description;
  }

 private:
  // Bump up the following version number if you make any change to the
  // Notification class.
  static const int kNotificationVersion = 1;

  int version_;
  std::string16 title_;
  std::string16 icon_;
  std::string16 id_;
  std::string16 service_;
  std::string16 description_;

  DISALLOW_EVIL_CONSTRUCTORS(Notification);
};

#endif  // GEARS_NOTIFIER_NOTIFICATION_H__
