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
//
// Definitions for detecting user activity.

#ifndef GEARS_NOTIFIER_USER_ACTIVITY_H__
#define GEARS_NOTIFIER_USER_ACTIVITY_H__

// Categorization of the user mode
enum UserMode {
  // User is away
  // * A screen saver is displayed.
  // * The machine is locked.
  // * Non-active Fast User Switching session is on (Windows XP & Vista).
  USER_AWAY_MODE = 0,

  // User is idle
  USER_IDLE_MODE,

  // User is interrupted by some critical event
  // * Laptop power is being suspended.
  USER_INTERRUPTED_MODE,

  // User is in not idle
  USER_NORMAL_MODE,

  // User is in full-screen presentation mode
  // * A full-screen application is running.
  // * A full-screen (exclusive mode) Direct3D application is running
  //  (Windows XP & Vista).
  // * The user activates Windows presentation settings (Windows Vista).
  USER_PRESENTATION_MODE,

  // User mode is undetermined
  USER_MODE_UNKNOWN,
};

class UserActivityInterface {
 public:
  virtual UserMode CheckUserActivity() = 0;
};

// Is user active?
inline bool IsActiveUserMode(UserMode user_mode) {
  return user_mode == USER_NORMAL_MODE;
}

#endif  // GEARS_NOTIFIER_USER_ACTIVITY_H__
