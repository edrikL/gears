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

#if defined(WIN32) && !defined(WINCE)

#ifdef USING_CCTESTS

#include <algorithm>
#include <set>
#include "gears/base/common/ipc_message_queue.h"
#include "gears/base/common/serialization.h"
#include "gears/base/common/stopwatch.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

//-----------------------------------------------------------------------------
// Functions defined in ipc_message_queue_win32.cc to facillitate testing
//-----------------------------------------------------------------------------

void TestingIpcMessageQueueWin32_GetAllProcesses(
                                    std::vector<IpcProcessId> *processes);
void TestingIpcMessageQueueWin32_DieWhileHoldingWriteLock(IpcProcessId id);
void TestingIpcMessageQueueWin32_DieWhileHoldingRegistryLock();
void TestingIpcMessageQueueWin32_SleepWhileHoldingRegistryLock();
void TestingIpcMessageQueueWin32_GetCounters(IpcMessageQueueCounters *counters,
                                             bool reset);

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

// we want this larger than the capacity of the io buffer, ~8K
static const int kBigPingLength = 18000;


static const char16 *kHello = STRING16(L"hello");
static const char16 *kPing = STRING16(L"ping");
static const char16 *kQuit = STRING16(L"quit");
static const char16 *kBigPing = STRING16(L"bigPing");
static const char16 *kSendManyPings = STRING16(L"sendManyPings");
static const char16 *kSendManyBigPings = STRING16(L"sendManyBigPings");
static const char16 *kDieWhileHoldingWriteLock = STRING16(L"dieWithWriteLock");
static const char16 *kDieWhileHoldingRegistryLock =
                         STRING16(L"dieWithRegistryLock");
static const char16* kQuitWhileHoldingRegistryLock =
                         STRING16(L"quitWithRegistryLock");
static const char16 *kError = STRING16(L"error");
static const char16 *kDoChitChat = STRING16(L"doChitChat");
static const char16 *kChit = STRING16(L"chit");
static const char16 *kChat = STRING16(L"chat");
static const char16 *kDidChitChat = STRING16(L"didChitChat");

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
  IpcMessageQueueCounters counters_;

  virtual SerializableClassId GetSerializableClassId() {
    return SERIALIZABLE_IPC_TEST_MESSAGE;
  }
  virtual bool Serialize(Serializer *out) {
    out->WriteString(string_.c_str());

    TestingIpcMessageQueueWin32_GetCounters(&counters_, false);
    out->WriteBytes(&counters_, sizeof(counters_));

    out->WriteInt(bytes_length_);
    if (bytes_length_) {
      out->WriteBytes(bytes_.get(), bytes_length_);
    }
    return true;
  }
  virtual bool Deserialize(Deserializer *in) {
    if (!in->ReadString(&string_) ||
        !in->ReadBytes(&counters_, sizeof(counters_)) ||
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

static bool WaitForRegisteredProcesses(int n, int timeout) {
  int64 start_time = GetCurrentTimeMillis();
  std::vector<IpcProcessId> registered_processes;
  while (true) {
    TestingIpcMessageQueueWin32_GetAllProcesses(&registered_processes);
    if (registered_processes.size() == n)
      return true;
    if (!timeout || GetCurrentTimeMillis() - start_time > timeout)
      return false;
    Sleep(1);
  }
  return false;
}

static HMODULE GetModuleHandleFromAddress(void *address) {
  MEMORY_BASIC_INFORMATION mbi;
  SIZE_T result = VirtualQuery(address, &mbi, sizeof(mbi));
  return static_cast<HMODULE>(mbi.AllocationBase);
}

// Gets the handle to the currently executing module.
static HMODULE GetCurrentModuleHandle() {
  // pass a pointer to the current function
  return GetModuleHandleFromAddress(GetCurrentModuleHandle);
}

struct SlaveProcess {
  IpcProcessId id;
  CHandle handle;

  SlaveProcess() : id(0) {}

  // Start a slave process via rundll32.exe
  bool Start() {
    wchar_t module_path[MAX_PATH];  // folder + filename
    if (0 == GetModuleFileNameW(GetCurrentModuleHandle(),
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
    return true;
  }

  // Waits until the slave's process id is in the IPCQueue registry
  bool WaitTillRegistered(int timeout) {
    assert(id);
    assert(handle.m_h);
    int64 start_time = GetCurrentTimeMillis();
    std::vector<IpcProcessId> registered_processes;
    while (true) {
      TestingIpcMessageQueueWin32_GetAllProcesses(&registered_processes);
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
  }

  // Waits until the slave process terminates
  bool WaitForExit(int timeout) {
    assert(id);
    assert(handle.m_h);
    return WaitForSingleObject(handle, timeout) == WAIT_OBJECT_0;
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

      LOG(("master received %S\n", test_message->string_.c_str()));

      source_ids_.push_back(source_process_id);
      message_strings_.push_back(test_message->string_);
      last_received_counters_ = test_message->counters_;

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
    TestingIpcMessageQueueWin32_GetCounters(NULL, true);
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
  IpcMessageQueueCounters last_received_counters_;

 private:
  int64 wait_for_messages_start_time_;
  int num_messages_waiting_to_receive_;
};


static MasterMessageHandler g_master_handler;

bool TestIpcMessageQueue(std::string16 *error) {
#undef TEST_ASSERT
#define TEST_ASSERT(b) \
{ \
  if (!(b)) { \
    LOG(("TestIpcMessageQueue - failed (%d)\n", __LINE__)); \
    g_master_handler.SetSaveMessages(false); \
    g_master_handler.ClearSavedMessages(); \
    if (ipc_message_queue) \
      ipc_message_queue->SendToAll(kIpcQueue_TestMessage, \
                                   new IpcTestMessage(kQuit), \
                                   false); \
    assert(error); \
    *error += STRING16(L"TestIpcMessageQueue - failed. "); \
    return false; \
  } \
}

  // Register our message class and handler
  IpcTestMessage::RegisterAsSerializable();
  g_master_handler.SetSaveMessages(false);
  g_master_handler.ClearSavedMessages();
  IpcMessageQueue *ipc_message_queue = IpcMessageQueue::GetPeerQueue();
  TEST_ASSERT(ipc_message_queue);
  ipc_message_queue->RegisterHandler(kIpcQueue_TestMessage, &g_master_handler);

  // Tell any straglers from a previous run to quit
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kQuit),
                               false);
  TEST_ASSERT(WaitForRegisteredProcesses(1, 2000));

  g_master_handler.SetSaveMessages(true);
  g_master_handler.ClearSavedMessages();

  // These test assume that the current process is the only process with an
  // ipc queue that is currently executing.
  std::vector<IpcProcessId> registered_processes;
  TestingIpcMessageQueueWin32_GetAllProcesses(&registered_processes);
  TEST_ASSERT(registered_processes.size() == 1);
  TEST_ASSERT(registered_processes[0] == 
              ipc_message_queue->GetCurrentIpcProcessId());
  
  // Start two slave processes, a and b
  SlaveProcess process_a, process_b;
  TEST_ASSERT(process_a.Start());
  TEST_ASSERT(process_a.WaitTillRegistered(30000));
  TEST_ASSERT(process_b.Start());
  TEST_ASSERT(process_b.WaitTillRegistered(30000));

  // The registry should now contain three processes
  TestingIpcMessageQueueWin32_GetAllProcesses(&registered_processes);
  TEST_ASSERT(registered_processes.size() == 3);
  TEST_ASSERT(registered_processes[0] ==
              ipc_message_queue->GetCurrentIpcProcessId());
  TEST_ASSERT(registered_processes[1] == process_a.id);
  TEST_ASSERT(registered_processes[2] == process_b.id);

  // Each should send a "hello" message on startup, we wait for that to know
  // that they are ready to receive IpcTestMessages.
  TEST_ASSERT(g_master_handler.WaitForMessages(2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_a.id, kHello) == 1);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_b.id, kHello) == 1);

  // Broadcast a "ping" and wait for expected responses
  g_master_handler.ClearSavedMessages();
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kPing),
                               false);
  TEST_ASSERT(g_master_handler.WaitForMessages(2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_a.id, kPing) == 1);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_b.id, kPing) == 1);

  // Broadcast a "bigPing" and wait for expected responses
  g_master_handler.ClearSavedMessages();
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kBigPing, kBigPingLength),
                               false);
  TEST_ASSERT(g_master_handler.WaitForMessages(2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_a.id, kBigPing) ==
              1);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_b.id, kBigPing) ==
              1);

  // Tell them to quit, wait for them to do so, and verify they are no
  // longer in the registry
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kQuit),
                               false);
  TEST_ASSERT(process_a.WaitForExit(30000));
  TEST_ASSERT(process_b.WaitForExit(30000));
  TEST_ASSERT(WaitForRegisteredProcesses(1, 30000));

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
    ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                                 new IpcTestMessage(kBigPing, kBigPingLength),
                                 false);
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
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kSendManyPings),
                               false);
  TEST_ASSERT(g_master_handler.WaitForMessages(kManyPings * 2));
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_c.id, kPing) ==
             kManyPings);
  TEST_ASSERT(g_master_handler.CountSavedMessages(process_d.id, kPing) ==
              kManyPings);

  // Tell c and d to send us many big pings at the same time.
  g_master_handler.ClearSavedMessages();
  ipc_message_queue->SendToAll(kIpcQueue_TestMessage,
                               new IpcTestMessage(kSendManyBigPings),
                               false);
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

  // Start many processes
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
  TestingIpcMessageQueueWin32_GetAllProcesses(&registered_processes);
  TEST_ASSERT(registered_processes.size() == kManySlavesCount + 1);
  for (int i = 1; i <= kManySlavesCount; ++i) {
    TEST_ASSERT(2 == g_master_handler.CountSavedMessages(
                                          registered_processes[i], NULL));
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
  TestingIpcMessageQueueWin32_GetAllProcesses(&registered_processes);
  TEST_ASSERT(registered_processes[0] ==
              ipc_message_queue->GetCurrentIpcProcessId());

  TEST_ASSERT(g_master_handler.CountSavedMessages(0, kError) == 0);

  LOG(("TestIpcMessageQueue - passed\n"));
  g_master_handler.SetSaveMessages(false);
  g_master_handler.ClearSavedMessages();
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


// This is the function that rundll32 calls when we launch other processes.
extern "C"
__declspec(dllexport) void __cdecl RunIpcSlave(HWND window,
                                               HINSTANCE instance,
                                               LPSTR command_line,
                                               int command_show) {
  LOG(("RunIpcSlave called\n"));
  IpcTestMessage::RegisterAsSerializable();
  IpcMessageQueue *ipc_message_queue = IpcMessageQueue::GetPeerQueue();
  ipc_message_queue->RegisterHandler(kIpcQueue_TestMessage, &g_slave_handler);

  std::vector<IpcProcessId> registered_processes;
  TestingIpcMessageQueueWin32_GetAllProcesses(&registered_processes);
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


void SlaveMessageHandler::HandleIpcMessage(
                              IpcProcessId source_process_id,
                              int message_type,
                              const IpcMessageData *message_data) {
  assert(message_type == kIpcQueue_TestMessage);
  const IpcTestMessage *test_message = static_cast<const IpcTestMessage*>
                                          (message_data);

   LOG(("slave received %S\n", test_message->string_.c_str()));

  if (test_message->string_ == kPing) {
    IpcMessageQueue::GetPeerQueue()->Send(source_process_id,
                                         kIpcQueue_TestMessage,
                                         new IpcTestMessage(kPing));

  } else if (test_message->string_ == kQuit) {
    done_ = true;

  } else if (test_message->string_ == kQuitWhileHoldingRegistryLock) {
    done_ = true;
    TestingIpcMessageQueueWin32_SleepWhileHoldingRegistryLock();

  } else if (test_message->string_ == kDieWhileHoldingWriteLock) {
    TestingIpcMessageQueueWin32_DieWhileHoldingWriteLock(source_process_id);

  } else if (test_message->string_ == kDieWhileHoldingRegistryLock) {
    TestingIpcMessageQueueWin32_DieWhileHoldingRegistryLock();

  } else if (test_message->string_ == kBigPing) {
    if (test_message->bytes_length_ == kBigPingLength &&
        VerifyTestData(test_message->bytes_.get(),
                       test_message->bytes_length_)) {
      IpcMessageQueue::GetPeerQueue()->Send(source_process_id,
                                           kIpcQueue_TestMessage,
                                           new IpcTestMessage(kBigPing,
                                                              kBigPingLength));
    } else {
      IpcMessageQueue::GetPeerQueue()->Send(source_process_id,
                                           kIpcQueue_TestMessage,
                                           new IpcTestMessage(kError));
    }


  } else if (test_message->string_ == kSendManyPings) {
    for (int i = 0; i < kManyPings; ++i) {
      IpcMessageQueue::GetPeerQueue()->Send(source_process_id,
                                           kIpcQueue_TestMessage,
                                           new IpcTestMessage(kPing));
    }

  } else if (test_message->string_ == kSendManyBigPings) {
    for (int i = 0; i < kManyBigPings; ++i) {
      IpcMessageQueue::GetPeerQueue()->Send(source_process_id,
                                           kIpcQueue_TestMessage,
                                           new IpcTestMessage(kBigPing,
                                                              kBigPingLength));
    }

  } else if (test_message->string_ == kDoChitChat) {
    IpcMessageQueue::GetPeerQueue()->SendToAll(kIpcQueue_TestMessage,
                                              new IpcTestMessage(kChit),
                                              false);

  } else if (test_message->string_ == kChit) {
    ++chits_received_;
    chit_sources_.insert(source_process_id);
    IpcMessageQueue::GetPeerQueue()->Send(source_process_id,
                                         kIpcQueue_TestMessage,
                                         new IpcTestMessage(kChat));
    if (chits_received_ > kManySlavesCount - 1) {
      IpcMessageQueue::GetPeerQueue()->Send(parent_process_id_,
                                           kIpcQueue_TestMessage,
                                           new IpcTestMessage(kError));
    } else if ((chats_received_ == kManySlavesCount - 1) &&
               (chits_received_ == kManySlavesCount - 1) &&
               (chat_sources_.size() == kManySlavesCount - 1) &&
               (chit_sources_.size() == kManySlavesCount - 1)) {
      IpcMessageQueue::GetPeerQueue()->Send(parent_process_id_,
                                           kIpcQueue_TestMessage,
                                           new IpcTestMessage(kDidChitChat));
    }

  } else if (test_message->string_ == kChat) {
    ++chats_received_;
    chat_sources_.insert(source_process_id);
    if (chits_received_ > kManySlavesCount - 1) {
      IpcMessageQueue::GetPeerQueue()->Send(parent_process_id_,
                                           kIpcQueue_TestMessage,
                                           new IpcTestMessage(kError));
    } else if ((chats_received_ == kManySlavesCount - 1) &&
               (chits_received_ == kManySlavesCount - 1) &&
               (chat_sources_.size() == kManySlavesCount - 1) &&
               (chit_sources_.size() == kManySlavesCount - 1)) {
      IpcMessageQueue::GetPeerQueue()->Send(parent_process_id_,
                                           kIpcQueue_TestMessage,
                                           new IpcTestMessage(kDidChitChat));
    }

  }
}

#endif

#endif  // defined(WIN32) && !defined(WINCE)
