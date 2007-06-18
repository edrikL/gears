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

#include <assert.h>
#include <inttypes.h>
#include <semaphore.h>
#include <syslimits.h>

#include "gears/localserver/safari/update_task_sf.h"

// The docs claim that SEM_NAME_LEN should be defined.  It is not.  However,
// by looking at the xnu source (bsd/kern/posix_sem.c), it defines PSEMNAMLEN
// to be 31 characters.  We'll use that value.
#define MAX_SEM_NAME_LEN 31

inline void GetUpdateTaskSemaphoreName(int64 store_server_id, char *buffer) {
  // Since the max # of digits for a 64 bit value is 19 (9223372036854775807),
  // ensure that we don't go over that by prefixing the value with SFUT::
  snprintf(buffer, MAX_SEM_NAME_LEN, "SFUT::%lld", store_server_id);
}

void SFUpdateTask::Run() {
  // We use a global semaphore to ensure that only one update task per captured
  // application executes at a time. Since there can be multiple Safari and
  // worker pool processes, this semaphore needs to work across process
  // boundaries.
  char semaphore_name[MAX_SEM_NAME_LEN];
  GetUpdateTaskSemaphoreName(store_.GetServerID(), semaphore_name); 
  sem_t *global_semaphore = sem_open(semaphore_name, O_CREAT, 0600, 1);
  if (global_semaphore != (sem_t *)SEM_FAILED) {
    if (sem_trywait(global_semaphore) == 0) {
      UpdateTask::Run();
      sem_unlink(semaphore_name);
      sem_close(global_semaphore);
      return;
    } else {
      LOG(("SFUpdateTask - not running, another task is already running\n"));
      sem_close(global_semaphore);
    }
  } else {
    LOG(("SFUpdateTask - failed to start, unable to create semaphore\n"));
  }
  NotifyTaskComplete(false);
}

// Returns true if an UpdateTask for the given store is running
// Platform-specific implementation. See declaration in update_task.h.
bool UpdateTask::IsUpdateTaskForStoreRunning(int64 store_server_id) {
  // We consider the existence of a semaphore having right name as proof
  // positive of a running update task
  char semaphore_name[MAX_SEM_NAME_LEN];
  GetUpdateTaskSemaphoreName(store_server_id, semaphore_name); 
  bool result = false;
  sem_t *global_semaphore = sem_open(semaphore_name, 0);
  if (global_semaphore != (sem_t *)SEM_FAILED) {
    sem_close(global_semaphore);
    result = true;
  }
  return result;
}

// Platform-specific implementation. See declaration in update_task.h.
UpdateTask *UpdateTask::CreateUpdateTask() {
  return new SFUpdateTask();
}
