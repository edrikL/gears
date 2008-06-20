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
// The notification API has not been finalized for official builds.
#else
#include "gears/notifier/notifier.h"

#include "gears/base/common/security_model.h"
#include "gears/base/common/serialization.h"
#include "gears/base/common/string_utils.h"
#include "gears/notifier/notification.h"
#include "gears/notifier/notification_manager.h"
#include "gears/notifier/notifier_process.h"
#include "gears/notifier/user_activity.h"
#if USING_CCTESTS
#include "gears/notifier/unit_test.h"
#endif  // USING_CCTESTS
#include "third_party/glint/include/platform.h"
#include "third_party/glint/include/work_item.h"

// TODO(dimich): implement this.
class UserActivityMonitor : public UserActivityInterface {
 public:
  UserActivityMonitor() {
  }
  virtual UserMode CheckUserActivity() {
    return USER_NORMAL_MODE;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(UserActivityMonitor);
};

// Posted as workitem from IPC worker thread to main UI thread.
class NotificationTask : public glint::WorkItem {
 public:
  enum Action { ACTION_ADD, ACTION_DELETE };
  NotificationTask(Notifier *notifier,
                   const GearsNotification &notification,
                   Action action)
    : action_(action),
      notifier_(notifier) {
    assert(notifier);
    notification_.CopyFrom(notification);
  }

  void Post() {
    glint::platform()->PostWorkItem(NULL, this);
  }

  virtual void Run() {
    if (action_ == ACTION_ADD) {
      notifier_->AddNotification(notification_);
    } else if (action_ == ACTION_DELETE) {
      notifier_->RemoveNotification(notification_.security_origin(),
                                    notification_.id());
    } else {
      assert(false);
    }
  }
 private:
  Action action_;
  Notifier *notifier_;
  GearsNotification notification_;
};

Notifier::Notifier()
  : running_(false),
    notification_manager_(new NotificationManager(new UserActivityMonitor())) {
}

Notifier::~Notifier() {
}

bool Notifier::Initialize() {
  GearsNotification::RegisterAsSerializable();
  IpcMessageQueue *ipc_message_queue = IpcMessageQueue::GetSystemQueue();
  if (!ipc_message_queue) {
    return false;
  }
  ipc_message_queue->RegisterHandler(kDesktop_AddNotification, this);
  ipc_message_queue->RegisterHandler(kDesktop_RemoveNotification, this);

  if (!NotifierProcess::RegisterProcess()) {
    return false;
  }

  running_ = true;

  return true;
}

void Notifier::Terminate() {
}

// This call comes on a worker thread that services the inter-process
// communication mechanism. So we need to make copies of all receipts and
// ship over by FedEx.
void Notifier::HandleIpcMessage(IpcProcessId source_process_id,
                                int message_type,
                                const IpcMessageData *message_data) {
  switch (message_type) {
    case kDesktop_AddNotification: {
      const GearsNotification *notification =
          static_cast<const GearsNotification*>(message_data);
      (new NotificationTask(this,
                            *notification,
                            NotificationTask::ACTION_ADD))->Post();
      break;
    }

    case kDesktop_RemoveNotification: {
      const GearsNotification *notification =
          static_cast<const GearsNotification*>(message_data);
      (new NotificationTask(this,
                            *notification,
                            NotificationTask::ACTION_DELETE))->Post();
      break;
    }

    default:
      assert(false);
      break;
  }
}

void Notifier::AddNotification(const GearsNotification &notification) {
  LOG(("Add notification %S - %S ($S)\n",
       notification.security_origin().url().c_str(),
       notification.id().c_str(),
       notification.title().c_str()));
  notification_manager_->Add(notification);
}

void Notifier::RemoveNotification(const SecurityOrigin &security_origin,
                                  const std::string16 &id) {
  LOG(("Remove notification %S - %S\n", security_origin.url().c_str(),
       id.c_str()));
  notification_manager_->Delete(security_origin, id);
}

#endif  // OFFICIAL_BUILD
