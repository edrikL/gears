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

#ifdef USING_CCTESTS

#include "gears/base/common/ipc_message_queue_test.h"

#include <stdio.h>
#include <stdlib.h>

#include "gears/base/common/common.h"
#include "gears/base/common/paths.h"
#include "gears/base/common/process_utils_win32.h"
#include "gears/base/common/stopwatch.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"

// Work around the header including errors.
#ifdef WIN32
#include <atlbase.h>
#include <windows.h>
#endif

void TestingIpcMessageQueueWin32_GetAllProcesses(
        std::vector<IpcProcessId> *processes);

//-----------------------------------------------------------------------------
// Master process test code
//-----------------------------------------------------------------------------

bool WaitForRegisteredProcesses(int n, int timeout) {
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

bool ValidateRegisteredProcesses(int n, const IpcProcessId *process_ids) {
  std::vector<IpcProcessId> registered_processes;
  TestingIpcMessageQueueWin32_GetAllProcesses(&registered_processes);
  if (registered_processes.size() == n) {
    if (process_ids) {
      for (int i = 0; i < n; ++i) {
        if (registered_processes[i] != process_ids[i]) {
          return false;
        }
      }
    }
    return true;
  }
  return false;
}

bool MasterMessageHandler::WaitForMessages(int n) {
  assert(save_messages_);
  MutexLock locker(&lock_);
  num_messages_waiting_to_receive_ = n;
  wait_for_messages_start_time_ = GetCurrentTimeMillis();
  lock_.Await(Condition(this, &MasterMessageHandler::HasMessagesOrTimedout));
  return num_received_messages_ == num_messages_waiting_to_receive_;
}

bool GetSlavePath(std::string16 *slave_path) {
  // We use ./run_gears_dll.exe <function_in_gears_dll_to_call>
  assert(slave_path);
  char16 module_path[MAX_PATH] = {0};  // folder + filename
  if (0 == ::GetModuleFileName(GetGearsModuleHandle(),
                               module_path, MAX_PATH)) {
    return false;
  }
  PathRemoveFileSpec(module_path);
  PathAppend(module_path, L"run_gears_dll.exe");

  // Get a version without spaces, to use it as a command line argument.
  char16 module_short_path[MAX_PATH] = {0};
  if (0 == GetShortPathNameW(module_path, module_short_path, MAX_PATH)) {
    return false;
  }

  *slave_path = module_short_path;
  *slave_path += L" RunIpcSlave";
  return true;
}

bool SlaveProcess::Start() {
  // Get the command.
  std::string16 slave_path;
  if (!GetSlavePath(&slave_path)) {
    return false;
  }

  // Execute the command.
  PROCESS_INFORMATION pi = { 0 };
  STARTUPINFO si;
  GetStartupInfo(&si);
  if (!::CreateProcess(NULL, &slave_path.at(0),
                       NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    return false;
  }
  ::CloseHandle(pi.hProcess);
  ::CloseHandle(pi.hThread);

  id_ = pi.dwProcessId;
  slave_processes_.push_back(id_);

  return true;
}

bool SlaveProcess::WaitTillRegistered(int timeout) {
  assert(id_);

  int64 start_time = GetCurrentTimeMillis();  
  std::vector<IpcProcessId> registered_processes;
  while (GetCurrentTimeMillis() - start_time < timeout) {
    TestingIpcMessageQueueWin32_GetAllProcesses(&registered_processes);
    if (std::find(registered_processes.begin(),
                  registered_processes.end(),
                  id_) != registered_processes.end()) {
      return true;
    }

    Sleep(1);
  }
  return false;
}

bool SlaveProcess::WaitForExit(int timeout) {
  assert(id_);

  HANDLE handle = ::OpenProcess(SYNCHRONIZE, false, id_);
  if (!handle) {
    return true;
  }

  bool rv = WaitForSingleObject(handle, timeout) == WAIT_OBJECT_0;
  ::CloseHandle(handle);
  return rv;
}


//-----------------------------------------------------------------------------
// Slave process test code
//-----------------------------------------------------------------------------

static SlaveMessageHandler g_slave_handler;

IpcMessageQueue *SlaveMessageHandler::GetIpcMessageQueue() const {
  return IpcMessageQueue::GetPeerQueue();
}

void SlaveMessageHandler::TerminateSlave() {
  g_slave_handler.set_done(true);
  LOG(("Terminating slave process %u\n", ::GetCurrentProcessId()));
}

bool InitSlave() {
  LOG(("Initializing slave process %u\n", ::GetCurrentProcessId()));

  std::vector<IpcProcessId> processes;
  TestingIpcMessageQueueWin32_GetAllProcesses(&processes);

  g_slave_handler.set_parent_process_id(processes.empty() ? 0 : processes[0]);

  // Create the new ipc message queue for the child process.
  IpcTestMessage::RegisterAsSerializable();
  IpcMessageQueue *ipc_message_queue = g_slave_handler.GetIpcMessageQueue();
  if (!ipc_message_queue) {
    return false;
  }
  ipc_message_queue->RegisterHandler(kIpcQueue_TestMessage, &g_slave_handler);

  // Send a hello message to the master process to tell that the child process
  // has been started.
  ipc_message_queue->Send(g_slave_handler.parent_process_id(),
                          kIpcQueue_TestMessage,
                          new IpcTestMessage(GetHelloMessage()));

  LOG(("Slave process sent hello message to master process %u\n",
       g_slave_handler.parent_process_id()));

  return true;
}

void RunSlave() {
  LOG(("Running slave process %u\n", ::GetCurrentProcessId()));

  // Loop until either done_ or our parent process terminates. While we're
  // looping the ipc worker thread will call HandleIpcMessage.
  ATL::CHandle parent_process(
                  ::OpenProcess(SYNCHRONIZE, FALSE,
                                g_slave_handler.parent_process_id()));
  while (::WaitForSingleObject(parent_process, 1000) == WAIT_TIMEOUT &&
         !g_slave_handler.done()) {
  }

  LOG(("Slave process %u exitting\n", ::GetCurrentProcessId()));
}

int SlaveMain() {
  if (!InitSlave()) {
    return 1;
  }
  RunSlave();
  return 0;
}

#ifdef BROWSER_NONE

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  return SlaveMain();
}

#elif BROWSER_IE

// This is the function that run_gears_dll.exe calls
extern "C" __declspec(dllexport) int __cdecl RunIpcSlave() {
  SlaveMain();
  return 0;
}

#endif  // BROWSER_NONE

#endif  // USING_CCTESTS

