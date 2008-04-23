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
#include <atlbase.h>
#include <atlsync.h>
#include <windows.h>
#include "genfiles/product_constants.h"
#include "gears/base/ie/detect_version_collision.h"
#include "gears/ui/ie/string_table.h"

// We run our detection code once at startup
static bool OneTimeDetectVersionCollision();
static bool detected_collision = OneTimeDetectVersionCollision();

bool DetectedVersionCollision() {
  return detected_collision;
}

// We use two named mutex objects to determine if another version of the same
// Gears distribution is running.  The first indicates an instance of ANY
// version is running.  The second indicates an instance of OUR version is
// running.  The base mutex names should not change across versions!
//
// TODO(michaeln): Ideally, these should be per user kernel objects rather
// than per local session to detect when the same user is running Gears in
// another windows session. See //depot/googleclient/ci/common/synchronized.cpp
// for some clues.
static CMutex running_mutex;
static CMutex our_version_running_mutex;

#if BROWSER_NPAPI
// For the Win32 NPAPI distribution.
#define PRODUCT_COLLISION_GUID L"{6E9495F2-005B-4016-9F09-FA9F1F3F2D85}"
#else
// For the Win32 IE/Firefox distribution.
#define PRODUCT_COLLISION_GUID L"{685E0F7D-005A-40a0-B9F8-168FBA824248}"
#endif

static const wchar_t *kMutexName =
    L"IsRunning" PRODUCT_COLLISION_GUID;
static const wchar_t *kOurVersionMutexName =
    L"IsRunning" PRODUCT_COLLISION_GUID L"-" PRODUCT_VERSION_STRING;

// Returns true if we detect that a different version of Gears is running.
// If no collision is detected, leaves mutex handles open to indicate that
// our version is running.  If a collision is detected, this instance of Gears
// will be crippled, so we close all mutex handles so others don't see this
// instance as 'running'. Should only be called once.
static bool OneTimeDetectVersionCollision() {
  // Detect if an instance of any version is running
  if (!running_mutex.Create(NULL, FALSE, kMutexName)) {
    assert(false);
    return true;
  }
  bool already_running = (GetLastError() == ERROR_ALREADY_EXISTS);

  // Detect if another instance of our version is running
  if (!our_version_running_mutex.Create(NULL, FALSE, kOurVersionMutexName)) {
    assert(false);
    running_mutex.Close();
    return true;
  }
  bool our_version_already_running = (GetLastError() == ERROR_ALREADY_EXISTS);

  if (!already_running) {
    // No collision, we are the first instance to run
    assert(!our_version_already_running);
    return false;
  } else if (our_version_already_running) {
    // No collision, other instances of our version are running
    return false;
  } else {
    // A collision with a different version!
    our_version_running_mutex.Close();
    running_mutex.Close();
    return true;
  }
}

// Low tech UI to notify the user
static bool alerted_user = false;

void MaybeNotifyUserOfVersionCollision() {
  assert(detected_collision);
  if (!alerted_user) {
    NotifyUserOfVersionCollision();
  }
}

void NotifyUserOfVersionCollision() {
  assert(detected_collision);
  alerted_user = true;
  const int kMaxStringLength = 256;
  char16 title[kMaxStringLength];
  char16 text[kMaxStringLength];
  if (LoadString(GetModuleHandle(PRODUCT_SHORT_NAME),
                 IDS_VERSION_COLLISION_TITLE, title, kMaxStringLength) &&
      LoadString(GetModuleHandle(PRODUCT_SHORT_NAME),
                 IDS_VERSION_COLLISION_TEXT, text, kMaxStringLength)) {
    MessageBox(NULL, text, title, MB_OK);
  }
}
