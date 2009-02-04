// Copyright 2007, Google Inc.
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

#ifndef GEARS_LOCALSERVER_NPAPI_UPDATE_TASK_CR_H__
#define GEARS_LOCALSERVER_NPAPI_UPDATE_TASK_CR_H__

#include "gears/base/common/serialization.h"
#include "gears/localserver/common/update_task.h"
#include "gears/base/chrome/module_cr.h"


// TODO(michaeln): renamed to CRUpdateTask
class NPUpdateTask : public UpdateTask {
 public:
  NPUpdateTask(BrowsingContext *context) : UpdateTask(context) {}

  // Overriden to execute update tasks in the plugin process
  virtual bool StartUpdate(ManagedResourceStore *store);
  virtual void AwaitStartup();

 protected:
  // Overriden to ensure only one task per application runs at a time
  virtual void Run();

  virtual void NotifyObservers(Event *event);

 private:
  static bool SetRunningTask(NPUpdateTask *task);
  static void ClearRunningTask(NPUpdateTask *task);
};


// TODO(michaeln): move to SerializableClassId enumeration
#define SERIALIZABLE_AUTOUPDATE_MESSAGE  ((SerializableClassId)1224)

// The message sent from browser process to plugin process for auto updates
class AutoUpdateMessage : public PluginMessage {
 public:
  AutoUpdateMessage() : store_id_(0), context_(0) {}
  AutoUpdateMessage(int64 store_id, CPBrowsingContext context)
      : store_id_(store_id), context_(context) {}

  int64 store_id_;
  CPBrowsingContext context_;

  // PluginMessage override
  virtual void OnMessageReceived();

  // Serailizable overrides
  virtual SerializableClassId GetSerializableClassId() const {
    return SERIALIZABLE_AUTOUPDATE_MESSAGE;
  }
  virtual bool Serialize(Serializer *out) const {
    out->WriteInt64(store_id_);
    out->WriteInt(context_);
    return true;
  }
  virtual bool Deserialize(Deserializer *in) {
    return (in->ReadInt64(&store_id_) &&
            in->ReadInt(reinterpret_cast<int*>(&context_)));
  }

  // Serializable registration
  static Serializable *New() {
    return new AutoUpdateMessage;
  }
  static void Register() {
    Serializable::RegisterClass(SERIALIZABLE_AUTOUPDATE_MESSAGE, New);
  }
};

// TODO(zork): move to SerializableClassId enumeration
#define SERIALIZABLE_AUTOUPDATE_SYNC_MESSAGE  ((SerializableClassId)1229)

// The message sent from a renderer proccess to the browser proccess to start
// an update task.
class AutoUpdateSyncMessage : public PluginSyncMessage {
 public:
  AutoUpdateSyncMessage() : store_id_(0), context_(0) {}
  AutoUpdateSyncMessage(int64 store_id, CPBrowsingContext context)
      : store_id_(store_id), context_(context) {}

  int64 store_id_;
  CPBrowsingContext context_;

  // PluginSyncMessage override
  virtual void OnSyncMessageReceived(std::vector<uint8> *retval);

  // Serailizable overrides
  virtual SerializableClassId GetSerializableClassId() const {
    return SERIALIZABLE_AUTOUPDATE_SYNC_MESSAGE;
  }
  virtual bool Serialize(Serializer *out) const {
    out->WriteInt64(store_id_);
    out->WriteInt(context_);
    return true;
  }
  virtual bool Deserialize(Deserializer *in) {
    return (in->ReadInt64(&store_id_) &&
            in->ReadInt(reinterpret_cast<int*>(&context_)));
  }

  // Serializable registration
  static Serializable *New() {
    return new AutoUpdateSyncMessage;
  }
  static void Register() {
    Serializable::RegisterClass(SERIALIZABLE_AUTOUPDATE_SYNC_MESSAGE, New);
  }
};

// TODO(zork): move to SerializableClassId enumeration
#define SERIALIZABLE_ISUPDATERUNNING_MESSAGE  ((SerializableClassId)1227)

// The message sent from a renderer proccess to the browser proccess to check
// if the specified update task is running.
class IsUpdateRunningMessage : public PluginSyncMessage {
 public:
  IsUpdateRunningMessage() : store_id_(0) {}
  IsUpdateRunningMessage(int64 store_id)
      : store_id_(store_id) {}

  int64 store_id_;

  // PluginMessage override
  virtual void OnSyncMessageReceived(std::vector<uint8> *retval);
  bool IsRunning();

  // Serailizable overrides
  virtual SerializableClassId GetSerializableClassId() const {
    return SERIALIZABLE_ISUPDATERUNNING_MESSAGE;
  }
  virtual bool Serialize(Serializer *out) const {
    out->WriteInt64(store_id_);
    return true;
  }
  virtual bool Deserialize(Deserializer *in) {
    return (in->ReadInt64(&store_id_));
  }

  // Serializable registration
  static Serializable *New() {
    return new IsUpdateRunningMessage;
  }
  static void Register() {
    Serializable::RegisterClass(SERIALIZABLE_ISUPDATERUNNING_MESSAGE, New);
  }
};

// TODO(zork): move to SerializableClassId enumeration
#define SERIALIZABLE_UPDATENOTIFY_MESSAGE  ((SerializableClassId)1228)

// The message sent from the browser process to renderer processes to transfer
// update task events.
class UpdateNotifyMessage : public PluginMessage {
 public:
  UpdateNotifyMessage() : event_(NULL) {}
  UpdateNotifyMessage(const char16 *topic, UpdateTask::Event *event)
      : notification_topic_(topic), event_(event) {}

  UpdateTask::Event *event_;
  std::string16 notification_topic_;

  // PluginMessage override
  virtual void OnMessageReceived();

  // Serailizable overrides
  virtual SerializableClassId GetSerializableClassId() const {
    return SERIALIZABLE_UPDATENOTIFY_MESSAGE;
  }
  virtual bool Serialize(Serializer *out) const {
    if (event_) {
      out->WriteString(notification_topic_.c_str());
      out->WriteObject(event_);
      return true;
    } else {
      return false;
    }
  }
  virtual bool Deserialize(Deserializer *in) {
    return (in->ReadString(&notification_topic_) &&
            in->CreateAndReadObject(reinterpret_cast<NotificationData **>(&event_)));
  }

  // Serializable registration
  static Serializable *New() {
    return new UpdateNotifyMessage;
  }
  static void Register() {
    Serializable::RegisterClass(SERIALIZABLE_UPDATENOTIFY_MESSAGE, New);
  }
};

#endif  // GEARS_LOCALSERVER_NPAPI_UPDATE_TASK_CR_H__
