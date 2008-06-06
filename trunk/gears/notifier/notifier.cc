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

int main(int, char **) {
  return 0;
}
#else
#include "gears/notifier/notifier.h"

#include "gears/base/common/serialization.h"
#include "gears/base/common/string_utils.h"
#include "gears/notifier/notification.h"
#include "gears/notifier/notifier_process.h"
#if USING_CCTESTS
#include "gears/notifier/unit_test.h"
#endif  // USING_CCTESTS


Notifier::Notifier()
  : running_(false) {
}

bool Notifier::Initalize() {
  Notification::RegisterAsSerializable();
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

int Notifier::Run() {
  while (running_) {
#if defined(WIN32) || defined(WINCE)
    Sleep(100);
#elif defined(LINUX) || defined(MAC)
    sched_yield();
#endif
  }
  return 0;
}

void Notifier::Terminate() {
}

void Notifier::HandleIpcMessage(IpcProcessId source_process_id,
                                int message_type,
                                const IpcMessageData *message_data) {
  switch (message_type) {
    case kDesktop_AddNotification: {
      const Notification *notification =
          static_cast<const Notification*>(message_data);
      AddNotification(notification);
      break;
    }

    case kDesktop_RemoveNotification: {
      const Notification *notification =
          static_cast<const Notification*>(message_data);
      RemoveNotification(notification->id());
      break;
    }

    default:
      assert(false);
      break;
  }
}

void Notifier::AddNotification(const Notification *notification) {
  assert(notification);

  LOG(("Add notification %S - %S\n",
       notification->id().c_str(),
       notification->title().c_str()));
}

void Notifier::RemoveNotification(const std::string16 &notification_id) {
  LOG(("Remove notification %S\n", notification_id.c_str()));
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  int argc;
  char16 **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!argv) { return __LINE__; }  // return line as a ghetto error code
  LocalFree(argv);  // MSDN says to free 'argv', using LocalFree().

  LOG(("Gears Notifier started.\n"));

  Notifier notifier;

  int retval = -1;
  if (notifier.Initalize()) {
    retval = notifier.Run();
    notifier.Terminate();
  }

  LOG(("Gears Notifier terminated.\n"));

  return retval;
}

#endif  // OFFICIAL_BUILD
