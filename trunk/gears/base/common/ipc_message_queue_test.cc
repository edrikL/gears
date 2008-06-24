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

// Enable testing for Win32
#if defined(WIN32) && !defined(WINCE)
#if BROWSER_IE
#define TEST_IPC_MESSAGE_QUEUE 1    // Test peer queue
#else
#define TEST_IPC_MESSAGE_QUEUE 2    // Test system queue
#endif
#endif

// Enable testing for Linux
#if defined(LINUX) && !defined(OS_MACOSX)
#define TEST_IPC_MESSAGE_QUEUE 2    // Test system queue
#endif

#ifdef TEST_IPC_MESSAGE_QUEUE

#ifdef USING_CCTESTS

#include <algorithm>
#include <list>
#include <set>
#include <vector>
#if defined(LINUX) && !defined(OS_MACOSX)
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "gears/base/common/ipc_message_queue.h"
#include "gears/base/common/mutex.h"
#ifdef WIN32
#include "gears/base/common/process_utils_win32.h"
#endif
#include "gears/base/common/serialization.h"
#include "gears/base/common/stopwatch.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "third_party/scoped_ptr/scoped_ptr.h"
#ifdef WIN32
#include <atlbase.h>
#include <windows.h>
#endif

#if TEST_IPC_MESSAGE_QUEUE == 1
#define GetIpcMessageQueue     IpcMessageQueue::GetPeerQueue
#define TEST_SEND_TO_ALL       1
const bool kAsPeer = true;
#elif TEST_IPC_MESSAGE_QUEUE == 2
#define GetIpcMessageQueue     IpcMessageQueue::GetSystemQueue
const bool kAsPeer = false;
#else
#error "Invalid TEST_IPC_MESSAGE_QUEUE."
#endif

//-----------------------------------------------------------------------------
// Functions defined in ipc_message_queue_***.cc to facilitate testing
//-----------------------------------------------------------------------------

#ifdef WIN32
void TestingIpcMessageQueueWin32_GetAllProcesses(
        bool as_peer, std::vector<IpcProcessId> *processes);
void TestingIpcMessageQueueWin32_DieWhileHoldingWriteLock(
        IpcMessageQueue *ipc_message_queue, IpcProcessId id);
void TestingIpcMessageQueueWin32_DieWhileHoldingRegistryLock(
        IpcMessageQueue *ipc_message_queue);
void TestingIpcMessageQueueWin32_SleepWhileHoldingRegistryLock(
        IpcMessageQueue *ipc_message_queue);
void TestingIpcMessageQueueWin32_GetCounters(IpcMessageQueueCounters *counters,
                                             bool reset);
#endif

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

#if defined(LINUX) && !defined(OS_MACOSX)
bool InitIpcSlave();
void RunIpcSlave();
#endif

//-----------------------------------------------------------------------------
// Constants, functions, and classes used by test code in both the master
// and slave processes
//-----------------------------------------------------------------------------

#undef IPC_STRESS

#ifdef IPC_STRESS
static const int kManyPings = 10000;
static const int kManyBigPings = 1000;
static const int kManySlavesCount = 30;  // max supported is 31
#else
static const int kManyPings = 1000;
static const int kManyBigPings = 100;
static const int kManySlavesCount = 10;
#endif

// we want this larger than the capacity of the packet
// (~8K for WIN32, ~2K for Linux)
#ifdef WIN32
static const int kBigPingLength = 18000;
#elif defined(LINUX) && !defined(OS_MACOSX)
static const int kBigPingLength = 5000;
#endif


static const char16 *kHello = STRING16(L"hello");
static const char16 *kPing = STRING16(L"ping");
static const char16 *kQuit = STRING16(L"quit");
static const char16 *kBigPing = STRING16(L"bigPing");
static const char16 *kSendManyPings = STRING16(L"sendManyPings");
static const char16 *kSendManyBigPings = STRING16(L"sendManyBigPings");
static const char16 *kError = STRING16(L"error");
static const char16 *kDoChitChat = STRING16(L"doChitChat");
static const char16 *kChit = STRING16(L"chit");
static const char16 *kChat = STRING16(L"chat");
static const char16 *kDidChitChat = STRING16(L"didChitChat");
#ifdef WIN32
static const char16 *kDieWhileHoldingWriteLock = STRING16(L"dieWithWriteLock");
static const char16 *kDieWhileHoldingRegistryLock =
                         STRING16(L"dieWithRegistryLock");
static const char16* kQuitWhileHoldingRegistryLock =
                         STRING16(L"quitWithRegistryLock");
#endif

// Allocates block of memory using malloc and initializes the memory
// with an easy to verify pattern
static uint8 *MallocTestData(int length) {
  uint8 *data = static_cast<uint8*>(malloc(length));
  for(int i = 0; i < length; ++i) {
    data[i] = static_cast<uint8>(i % 256);
  }
  return data;
}

static bool VerifyTestData(uint8 *data, int length) {
  if (!data) return false;
  for(int i = 0; i < length; ++i) {
    if (data[i] != static_cast<uint8>(i % 256))
      return false;
  }
  return true;
}


// The message class we send to/from slave processes
class IpcTestMessage : public Serializable {
 public:
  IpcTestMessage() : bytes_length_(0) {}
  IpcTestMessage(const char16 *str) : string_(str), bytes_length_(0) {}
  IpcTestMessage(const char16 *str, int bytes_length)
      : string_(str), bytes_length_(bytes_length),
        bytes_(MallocTestData(bytes_length)) {}

  std::string16 string_;
  int bytes_length_;
  scoped_ptr_malloc<uint8> bytes_;
#ifdef WIN32
  IpcMessageQueueCounters counters_;
#endif

  virtual SerializableClassId GetSerializableClassId() {
    return SERIALIZABLE_IPC_TEST_MESSAGE;
  }
  virtual bool Serialize(Serializer *out) {
    out->WriteString(string_.c_str());

#ifdef WIN32
    TestingIpcMessageQueueWin32_GetCounters(&counters_, false);
    out->WriteBytes(&counters_, sizeof(counters_));
#endif

    out->WriteInt(bytes_length_);
    if (bytes_length_) {
      out->WriteBytes(bytes_.get(), bytes_length_);
    }
    return true;
  }
  virtual bool Deserialize(Deserializer *in) {
    if (!in->ReadString(&string_) ||
#ifdef WIN32
        !in->ReadBytes(&counters_, sizeof(counters_)) ||
#endif
        !in->ReadInt(&bytes_length_)) {
      return false;
    }
    if (bytes_length_) {
      bytes_.reset(static_cast<uint8*>(malloc(bytes_length_)));
      return in->ReadBytes(bytes_.get(), bytes_length_);
    } else {
      bytes_.reset(NULL);
      return true;
    }
  }
  static Serializable *New() {
    return new IpcTestMessage;
  }
  static void RegisterAsSerializable() {
    Serializable::RegisterClass(SERIALIZABLE_IPC_TEST_MESSAGE, New);
  }
};

//-----------------------------------------------------------------------------
// Helpers for use in the "master" process
//-----------------------------------------------------------------------------

#ifdef WIN32
static bool WaitForRegisteredProcesses(int n, int timeout) {
  int64 start_time = GetCurrentTimeMillis();
  std::vector<IpcProcessId> registered_processes;
  while (true) {
    TestingIpcMessageQueueWin32_GetAllProcesses(kAsPeer, &registered_processes);
    if (registered_processes.size() == n)
      return true;
    if (!timeout || GetCurrentTimeMillis() - start_time > timeout)
      return false;
    Sleep(1);
  }
  return false;
}
#endif

std::list<IpcProcessId> g_slave_processes;

struct SlaveProcess {
  IpcProcessId id;
#ifdef WIN32
  CHandle handle;
#endif

  SlaveProcess() : id(0) {}

  // Start a slave process via rundll32.exe
  bool Start() {
#ifdef WIN32
    wchar_t module_path[MAX_PATH];  // folder + filename
    if (0 == GetModuleFileNameW(GetGearsModuleHandle(),
                                module_path, MAX_PATH)) {
      return false;
    }

    // get a version without spaces, to use it as a command line argument
    wchar_t module_short_path[MAX_PATH];
    if (0 == GetShortPathNameW(module_path, module_short_path, MAX_PATH)) {
      return false;
    }

    std::wstring command;
    command += L"rundll32.exe ";
    command += module_short_path;
    command += L",RunIpcSlave";

    // execute the rundll32 command
    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si;
    GetStartupInfo(&si);
    BOOL ok = CreateProcessW(NULL, (LPWSTR)command.c_str(),
                             NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    DWORD err = ::GetLastError();
    if (!ok) return false;
    CloseHandle(pi.hThread);
    handle.m_h = pi.hProcess;
    id = pi.dwProcessId;
    g_slave_processes.push_back(id);
    return true;
#elif defined(LINUX) && !defined(OS_MACOSX)
    pid_t pid = fork();
    if (pid == -1) {
      LOG(("fork failed with errno=%d\n", errno));
      return false;
    } else if (pid == 0) {
      // Child process
      if (!InitIpcSlave()) {
        return false;
      }
      RunIpcSlave();
      return true;
    } else {
      // Parent process
      id = pid;
      g_slave_processes.push_back(id);
      return true;
    }
#endif
  }

  // Waits until the slave's process id is in the IPCQueue registry
  bool WaitTillRegistered(int timeout) {
#ifdef WIN32
    assert(id);
    assert(handle.m_h);
    int64 start_time = GetCurrentTimeMillis();
    std::vector<IpcProcessId> registered_processes;
    while (true) {
      TestingIpcMessageQueueWin32_GetAllProcesses(kAsPeer,
                                                  &registered_processes);
      if (std::find(registered_processes.begin(),
                    registered_processes.end(),
                    id) !=
          registered_processes.end()) {
        return true;
      }
      if (GetCurrentTimeMillis() - start_time > timeout)
        return false;
      Sleep(1);
    }
#elif defined(LINUX) && !defined(OS_MACOSX)
    return true;
#endif
  }

  // Waits until the slave process terminates
  bool WaitForExit(int timeout) {
    g_slave_processes.remove(id);
#ifdef WIN32
    assert(id);
    assert(handle.m_h);
    return WaitForSingleObject(handle, timeout) == WAIT_OBJECT_0;
#elif defined(LINUX) && !defined(OS_MACOSX)
    assert(id);
    LOG(("Waiting for process %u\n", id));
    while (timeout > 0) {
      if (waitpid(id, NULL, WNOHANG) > 0) {
        return true;
      }
      usleep(1000);
      timeout--;
    }
    return false;
#endif
  }
};


// The handler we register to receive kIpcQueue_TestMessage's sent
// to us from our slave processes
class MasterMessageHandler : public IpcMessageQueue::HandlerInterface {
 public:
  virtual void HandleIpcMessage(IpcProcessId source_process_id,
                                int message_type,
                                const IpcMessageData *message_data) {
    MutexLock locker(&lock_);
    if (save_messages_) {
      const IpcTestMessage *test_message =
                static_cast<const IpcTestMessage*>(message_data);
      ++num_received_messages_;

      std::string narrow_string;
      String16ToUTF8(test_message->string_.c_str(), &narrow_string);
      LOG(("master received %s\n", narrow_string.c_str()));

      source_ids_.push_back(source_process_id);
      message_strings_.push_back(test_message->string_);
#ifdef WIN32
      last_received_counters_ = test_message->counters_;
#endif

      if (test_message->string_ == kBigPing &&
          !VerifyTestData(test_message->bytes_.get(),
                          test_message->bytes_length_)) {
        ++num_invalid_big_pings_;
        LOG(("invalid big ping data\n"));
      }
    }
  }

  bool WaitForMessages(int n) {
    assert(save_messages_);
    MutexLock locker(&lock_);
    num_messages_waiting_to_receive_ = n;
    wait_for_messages_start_time_ = GetCurrentTimeMillis();
    lock_.Await(Condition(this, &MasterMessageHandler::HasMessagesOrTimedout));
    return num_received_messages_ == num_messages_waiting_to_receive_;
  }

  bool HasMessagesOrTimedout() {
    return num_received_messages_ == num_messages_waiting_to_receive_ ||
           (GetCurrentTimeMillis() - wait_for_messages_start_time_) > 30000;
  }

  void ClearSavedMessages() {
    MutexLock locker(&lock_);
    source_ids_.clear();
    message_strings_.clear();
    num_received_messages_ = 0;
    num_invalid_big_pings_ = 0;
#ifdef WIN32
    TestingIpcMessageQueueWin32_GetCounters(NULL, true);
#endif
  }

  void SetSaveMessages(bool save) {
    MutexLock locker(&lock_);
    save_messages_ = save;
  }

  int CountSavedMessages(IpcProcessId source, const char16 *str) {
    MutexLock locker(&lock_);
    int count = 0;
    for (int i = 0; i < num_received_messages_; ++i) {
      bool match = true;
      if (source != 0) {
        match = (source_ids_[i] == source);
      }
      if (match && str) {
        match = (message_strings_[i] == str);
      }
      if (match) {
        ++count;
      }
    }
    return count;
  }

  Mutex lock_;
  bool save_messages_;
  int num_received_messages_;
  int num_invalid_big_pings_;
  std::vector<IpcProcessId> source_ids_;
  std::vector<std::string16> message_strings_;
#ifdef WIN32
  IpcMessageQueueCounters last_received_counters_;
#endif

 private:
  int64 wait_for_messages_start_time_;
  int num_messages_waiting_to_receive_;
};


static MasterMessageHandler g_master_handler;

void QuitAllSlaveProcesses(IpcMessageQueue* ipc_message_queue) {
  g_master_handler.SetSaveMessages(false);
  g_master_handler.ClearSavedMessages();
  if (ipc_message_queue) {
    for (std::list<IpcProcessId>::const_iterator iter =
              g_slave_processes.begin();
         iter != g_slave_processes.end();
         ++iter) {
      ipc_message_queue->Send(*iter,
                              kIpcQueue_TestMessage,
                              new IpcTestMessage(kQuit));
    }
  }
}

bool TestIpcMessageQueue(std::string16 *error) {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestIpcMessageQueue - failed (%d)\n", __LINE__)); \
    QuitAllSlaveProcesses(ipc_message_queue); \
    *error += STRING16(L"TestIpcMessageQueue - failed. "); \
    return false; \
  } \
}

#if BROWSER_FF
  // TODO(levin): This test is currently busted for Firefox on some machines
  // and basically just creates noise in determining if something is broken
  // or not, so we'll disable it for now until the root problem can be figured
  // out.
#else
  assert(error);

  // Register our message class and handler
  IpcTestMessage::RegisterAsSerializable();
  IpcMessageQueue *ipc_message_queue = GetIpcMessageQueue();
  TEST_ASSERT(ipc_message_queue);
  ipc_message_queue->RegisterHandler(kIpcQueue_TestMessage, &g_master_handler);

  // Tell any straglers from a previous run to quit
#ifdef TEST_SEND_TO_ALL
  g_master_handler.SetSaveMessages(false);
  g_master_handler.ClearSavedMessages();
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kQuit),
                               false);
  TEST_ASSERT(WaitForRegisteredProcesses(1, 2000));
#endif

  g_master_handler.SetSaveMessages(true);
  g_master_handler.ClearSavedMessages();

  // These test assume that the current process is the only process with an
  // ipc queue that is currently executing.
#ifdef WIN32
  std::vector<IpcProcessId> registered_processes;
  TestingIpcMessageQueueWin32_GetAllProcesses(kAsPeer, &registered_processes);
  TEST_ASSERT(registered_processes.size() == 1);
  TEST_ASSERT(registered_processes[0] ==
              ipc_message_queue->GetCurrentIpcProcessId());
#endif

  // Start two slave processes, a and b
  SlaveProcess process_a, process_b;
  TEST_ASSERT(process_a.Start());
  TEST_ASSERT(process_a.WaitTillRegistered(30000));
  TEST_ASSERT(process_b.Start());
  TEST_ASSERT(process_b.WaitTillRegistered(30000));

  // The registry should now contain three processes
#ifdef WIN32
  TestingIpcMessageQueueWin32_GetAllProcesses(kAsPeer, &registered_processes);
  TEST_ASSERT(registered_processes.size() == 3);
  TEST_ASSERT(registered_processes[0] ==
              ipc_message_queue->GetCurrentIpcProcessId());
  TEST_ASSERT(registered_processes[1] == process_a.id);
  TEST_ASSERT(registered_processes[2] == process_b.id);
#endif

  // Each should send a "hello" message on startup, we wait for that to know
  // that they are ready to receive IpcTestMessages.
  TEST_ASSERT(g_master_handler.WaitForMessages(2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_a.id, kHello) == 1);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_b.id, kHello) == 1);

  // Broadcast a "ping" and wait for expected responses
  g_master_handler.ClearSavedMessages();
#ifdef TEST_SEND_TO_ALL
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kPing),
                               false);
#else
  ipc_message_queue->Send(process_a.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kPing));
  ipc_message_queue->Send(process_b.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kPing));
#endif
  TEST_ASSERT(g_master_handler.WaitForMessages(2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_a.id, kPing) == 1);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_b.id, kPing) == 1);

  // Broadcast a "bigPing" and wait for expected responses
  g_master_handler.ClearSavedMessages();
#ifdef TEST_SEND_TO_ALL
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kBigPing, kBigPingLength),
                               false);
#else
  ipc_message_queue->Send(process_a.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kBigPing, kBigPingLength));
  ipc_message_queue->Send(process_b.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kBigPing, kBigPingLength));
#endif
  TEST_ASSERT(g_master_handler.WaitForMessages(2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_a.id, kBigPing) ==
              1);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_b.id, kBigPing) ==
              1);

  // Tell them to quit, wait for them to do so, and verify they are no
  // longer in the registry
#ifdef TEST_SEND_TO_ALL
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kQuit),
                               false);
#else
  ipc_message_queue->Send(process_a.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kQuit));
  ipc_message_queue->Send(process_b.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kQuit));
#endif
  TEST_ASSERT(process_a.WaitForExit(30000));
  TEST_ASSERT(process_b.WaitForExit(30000));
#ifdef WIN32
  TEST_ASSERT(WaitForRegisteredProcesses(1, 30000));
#endif

  // Start a new process c
  g_master_handler.ClearSavedMessages();
  SlaveProcess process_c;
  TEST_ASSERT(process_c.Start());
  TEST_ASSERT(process_c.WaitTillRegistered(30000));
  TEST_ASSERT(g_master_handler.WaitForMessages(1));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_c.id, kHello) == 1);

  // Flood it with many small ping messages and wait for the expected responses
  g_master_handler.ClearSavedMessages();
  for (int i = 0; i < kManyPings; ++i) {
    ipc_message_queue->Send(process_c.id,
                            kIpcQueue_TestMessage,
                            new IpcTestMessage(kPing));
  }
  TEST_ASSERT(g_master_handler.WaitForMessages(kManyPings));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_c.id, kPing) ==
              kManyPings);

  // Flood it with a smaller number of large messages.
  g_master_handler.ClearSavedMessages();
  for (int i = 0; i < kManyBigPings; ++i) {
    ipc_message_queue->Send(process_c.id,
                            kIpcQueue_TestMessage,
                            new IpcTestMessage(kBigPing, kBigPingLength));
  }
  TEST_ASSERT(g_master_handler.WaitForMessages(kManyBigPings));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_c.id, kBigPing) ==
              kManyBigPings);
  TEST_ASSERT(g_master_handler.num_invalid_big_pings_ == 0);

  // Start a new process d
  g_master_handler.ClearSavedMessages();
  SlaveProcess process_d;
  TEST_ASSERT(process_d.Start());
  TEST_ASSERT(process_d.WaitTillRegistered(30000));
  TEST_ASSERT(g_master_handler.WaitForMessages(1));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_d.id, kHello) == 1);

  // Tell c and d to send us many small pings at the same time.
  g_master_handler.ClearSavedMessages();
#ifdef TEST_SEND_TO_ALL
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kSendManyPings),
                               false);
#else
  ipc_message_queue->Send(process_c.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kSendManyPings));
  ipc_message_queue->Send(process_d.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kSendManyPings));
#endif
  TEST_ASSERT(g_master_handler.WaitForMessages(kManyPings * 2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_c.id, kPing) ==
             kManyPings);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_d.id, kPing) ==
              kManyPings);

  // Tell c and d to send us many big pings at the same time.
  g_master_handler.ClearSavedMessages();
#ifdef TEST_SEND_TO_ALL
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kSendManyBigPings),
                               false);
#else
  ipc_message_queue->Send(process_c.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kSendManyBigPings));
  ipc_message_queue->Send(process_d.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kSendManyBigPings));
#endif
  TEST_ASSERT(g_master_handler.WaitForMessages(kManyBigPings * 2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_c.id, kBigPing) ==
              kManyBigPings);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_d.id, kBigPing) ==
              kManyBigPings);
  TEST_ASSERT(g_master_handler.num_invalid_big_pings_ == 0);

  // Tell c to send us many big pings and d to send us many small pings
  g_master_handler.ClearSavedMessages();
  ipc_message_queue->Send(process_c.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kSendManyBigPings));
  ipc_message_queue->Send(process_d.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kSendManyPings));
  TEST_ASSERT(g_master_handler.WaitForMessages(kManyBigPings + kManyPings));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_c.id, kBigPing) ==
              kManyBigPings);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_d.id, kPing) ==
              kManyPings);
  TEST_ASSERT(g_master_handler.num_invalid_big_pings_ == 0);

#ifdef WIN32
  // Tell 'd' to die while it has the write lock on our queue
  ipc_message_queue->Send(process_d.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kDieWhileHoldingWriteLock));
  TEST_ASSERT(process_d.WaitForExit(30000));

  // Ping 'c' and make sure we still get a response on our queue
  g_master_handler.ClearSavedMessages();
  ipc_message_queue->Send(process_c.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kPing));
  TEST_ASSERT(g_master_handler.WaitForMessages(1));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_c.id, kPing) == 1);

  // Tell 'c' to die while it has the registry lock
  ipc_message_queue->Send(process_c.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kDieWhileHoldingRegistryLock));
  TEST_ASSERT(process_c.WaitForExit(30000));

  // Test that a running process, ours, will cleanup the registry after others
  // have crashed without removing themselves from the registry.
  TEST_ASSERT(WaitForRegisteredProcesses(1, 30000));
#else
  // Tell 'c' and 'd' to die.
  ipc_message_queue->Send(process_c.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kQuit));
  ipc_message_queue->Send(process_d.id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kQuit));
  TEST_ASSERT(process_c.WaitForExit(30000));
  TEST_ASSERT(process_d.WaitForExit(30000));
#endif

  // Start many processes
#if defined(WIN32) && BROWSER_IE
  g_master_handler.ClearSavedMessages();
  SlaveProcess many_slaves[kManySlavesCount];
  for (int i = 0; i < kManySlavesCount; ++i) {
    TEST_ASSERT(many_slaves[i].Start());
  }
  for (int i = 0; i < kManySlavesCount; ++i) {
    TEST_ASSERT(many_slaves[i].WaitTillRegistered(30000));
  }
  TEST_ASSERT(g_master_handler.WaitForMessages(kManySlavesCount));
  TEST_ASSERT(g_master_handler.CountSavedMessages(0, kHello) ==
              kManySlavesCount);

  // Have them chit-chat with one another
  g_master_handler.ClearSavedMessages();
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kDoChitChat),
                               false);
  TEST_ASSERT(g_master_handler.WaitForMessages(kManySlavesCount * 2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(0, kChit) ==
              kManySlavesCount);
  TEST_ASSERT(g_master_handler.CountSavedMessages(0, kDidChitChat) ==
              kManySlavesCount);
  TestingIpcMessageQueueWin32_GetAllProcesses(kAsPeer, &registered_processes);
  TEST_ASSERT(registered_processes.size() == kManySlavesCount + 1);
  for (int i = 0; i < kManySlavesCount; ++i) {
    TEST_ASSERT(2 == g_master_handler.CountSavedMessages(
                                          many_slaves[i].id, NULL));
  }

  // Tell the first to quit in a would be dead lock situation
  ipc_message_queue->Send(many_slaves[0].id,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kQuitWhileHoldingRegistryLock));

  // Tell the remainder to quit normally
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kQuit),
                               false);
  for (int i = 0; i < kManySlavesCount; ++i) {
    TEST_ASSERT(many_slaves[i].WaitForExit(30000));
  }

  TEST_ASSERT(WaitForRegisteredProcesses(1, 30000));
  TestingIpcMessageQueueWin32_GetAllProcesses(kAsPeer, &registered_processes);
  TEST_ASSERT(registered_processes[0] ==
              ipc_message_queue->GetCurrentIpcProcessId());
#endif

  TEST_ASSERT(g_master_handler.CountSavedMessages(0, kError) == 0);

  LOG(("TestIpcMessageQueue - passed\n"));
  g_master_handler.SetSaveMessages(false);
  g_master_handler.ClearSavedMessages();
#endif  // BROWSER_FF
  return true;
}


//-----------------------------------------------------------------------------
// Slave process test code
//-----------------------------------------------------------------------------

// The handler we register to receive kIpcQueue_TestMessage's sent
// to us from our main process
class SlaveMessageHandler : public IpcMessageQueue::HandlerInterface {
 public:
  SlaveMessageHandler() : done_(false),
                          chits_received_(0),
                          chats_received_(0),
                          parent_process_id_(0) {}
  virtual void HandleIpcMessage(IpcProcessId source_process_id,
                                int message_type,
                                const IpcMessageData *message_data);

  bool done_;
  int chits_received_;
  int chats_received_;
  std::set<IpcProcessId> chit_sources_;
  std::set<IpcProcessId> chat_sources_;
  IpcProcessId parent_process_id_;
};
static SlaveMessageHandler g_slave_handler;


#ifdef WIN32

// This is the function that rundll32 calls when we launch other processes.
extern "C"
__declspec(dllexport) void __cdecl RunIpcSlave(HWND window,
                                               HINSTANCE instance,
                                               LPSTR command_line,
                                               int command_show) {
  LOG(("RunIpcSlave called\n"));
  IpcTestMessage::RegisterAsSerializable();
  IpcMessageQueue *ipc_message_queue = GetIpcMessageQueue();
  ipc_message_queue->RegisterHandler(kIpcQueue_TestMessage, &g_slave_handler);

  std::vector<IpcProcessId> registered_processes;
  TestingIpcMessageQueueWin32_GetAllProcesses(kAsPeer, &registered_processes);
  assert(registered_processes.size() > 0);
  g_slave_handler.parent_process_id_ = registered_processes[0];
  ipc_message_queue->Send(g_slave_handler.parent_process_id_,
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kHello));
  LOG(("RunIpcSlave sent hello message to master process\n"));

  // Loop until either done_ or our parent process terminates. While we're
  // looping the ipc worker thread will call HandleIpcMessage.
  ATL::CHandle parent_process(
                  ::OpenProcess(SYNCHRONIZE, FALSE,
                                g_slave_handler.parent_process_id_));
  while (WaitForSingleObject(parent_process, 1) == WAIT_TIMEOUT &&
         !g_slave_handler.done_) {
  }
  LOG(("RunIpcSlave exitting\n"));
}

#elif defined(LINUX) && !defined(OS_MACOSX)

bool InitIpcSlave() {
  LOG(("Slave process %u is started\n", getpid()));

  // Create the new ipc message queue for the child process.
  IpcTestMessage::RegisterAsSerializable();
  IpcMessageQueue *ipc_message_queue = GetIpcMessageQueue();
  if (!ipc_message_queue) {
    return false;
  }
  ipc_message_queue->RegisterHandler(kIpcQueue_TestMessage, &g_slave_handler);

  // Send a hello message to the parent process to tell that the child process
  // has been started.
  ipc_message_queue->Send(getppid(),
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(kHello));

  LOG(("Slave process %u sent hello message to master process\n", getpid()));

  return true;
}

void RunIpcSlave() {
  while (kill(getppid(), 0) != -1 && !g_slave_handler.done_) {
    usleep(1000);
  }

  LOG(("Slave process %u exitting\n", getpid()));

  raise(SIGTERM);
}

#endif


void SlaveMessageHandler::HandleIpcMessage(
                              IpcProcessId source_process_id,
                              int message_type,
                              const IpcMessageData *message_data) {
  assert(message_type == kIpcQueue_TestMessage);
  const IpcTestMessage *test_message = static_cast<const IpcTestMessage*>
                                          (message_data);

  std::string narrow_string;
  String16ToUTF8(test_message->string_.c_str(), &narrow_string);
  LOG(("slave received %s\n", narrow_string.c_str()));

  IpcMessageQueue *ipc_message_queue = GetIpcMessageQueue();

  if (test_message->string_ == kPing) {
    ipc_message_queue->Send(source_process_id,
                            kIpcQueue_TestMessage,
                            new IpcTestMessage(kPing));

  } else if (test_message->string_ == kQuit) {
    done_ = true;

#ifdef WIN32
  } else if (test_message->string_ == kQuitWhileHoldingRegistryLock) {
    done_ = true;
    TestingIpcMessageQueueWin32_SleepWhileHoldingRegistryLock(
        ipc_message_queue);

  } else if (test_message->string_ == kDieWhileHoldingWriteLock) {
    TestingIpcMessageQueueWin32_DieWhileHoldingWriteLock(ipc_message_queue,
                                                         source_process_id);

  } else if (test_message->string_ == kDieWhileHoldingRegistryLock) {
    TestingIpcMessageQueueWin32_DieWhileHoldingRegistryLock(ipc_message_queue);
#endif

  } else if (test_message->string_ == kBigPing) {
    if (test_message->bytes_length_ == kBigPingLength &&
        VerifyTestData(test_message->bytes_.get(),
                       test_message->bytes_length_)) {
      ipc_message_queue->Send(source_process_id,
                              kIpcQueue_TestMessage,
                              new IpcTestMessage(kBigPing,
                                                 kBigPingLength));
    } else {
      ipc_message_queue->Send(source_process_id,
                              kIpcQueue_TestMessage,
                              new IpcTestMessage(kError));
    }


  } else if (test_message->string_ == kSendManyPings) {
    for (int i = 0; i < kManyPings; ++i) {
      ipc_message_queue->Send(source_process_id,
                              kIpcQueue_TestMessage,
                              new IpcTestMessage(kPing));
    }

  } else if (test_message->string_ == kSendManyBigPings) {
    for (int i = 0; i < kManyBigPings; ++i) {
      ipc_message_queue->Send(source_process_id,
                              kIpcQueue_TestMessage,
                              new IpcTestMessage(kBigPing,
                                                 kBigPingLength));
    }

  } else if (test_message->string_ == kDoChitChat) {
    ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                                 new IpcTestMessage(kChit),
                                 false);

  } else if (test_message->string_ == kChit) {
    ++chits_received_;
    chit_sources_.insert(source_process_id);
    ipc_message_queue->Send(source_process_id,
                            kIpcQueue_TestMessage,
                            new IpcTestMessage(kChat));
    if (chits_received_ > kManySlavesCount - 1) {
      ipc_message_queue->Send(parent_process_id_,
                              kIpcQueue_TestMessage,
                              new IpcTestMessage(kError));
    } else if ((chats_received_ == kManySlavesCount - 1) &&
               (chits_received_ == kManySlavesCount - 1) &&
               (chat_sources_.size() == kManySlavesCount - 1) &&
               (chit_sources_.size() == kManySlavesCount - 1)) {
      ipc_message_queue->Send(parent_process_id_,
                              kIpcQueue_TestMessage,
                              new IpcTestMessage(kDidChitChat));
    }

  } else if (test_message->string_ == kChat) {
    ++chats_received_;
    chat_sources_.insert(source_process_id);
    if (chits_received_ > kManySlavesCount - 1) {
      ipc_message_queue->Send(parent_process_id_,
                              kIpcQueue_TestMessage,
                              new IpcTestMessage(kError));
    } else if ((chats_received_ == kManySlavesCount - 1) &&
               (chits_received_ == kManySlavesCount - 1) &&
               (chat_sources_.size() == kManySlavesCount - 1) &&
               (chit_sources_.size() == kManySlavesCount - 1)) {
      ipc_message_queue->Send(parent_process_id_,
                              kIpcQueue_TestMessage,
                              new IpcTestMessage(kDidChitChat));
    }

  }
}

#endif  // USING_CCTESTS

#endif  // TEST_IPC_MESSAGE_QUEUE
