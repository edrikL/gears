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
// This class contains convenience functions used by the Opera implementation
// of the plugin. It holds the pointer to the API exported by Opera.
//

#ifndef GEARS_BASE_OPERA_OPERA_UTILS_H__
#define GEARS_BASE_OPERA_OPERA_UTILS_H__

#include "gears/base/common/message_queue.h"  // for ThreadId

class OperaGearsAPI;
class OperaWorkerThreadInterface;

class OperaUtils {
 public:
  // Called by Opera when it first loads Gears. opera_api points to an object
  // that implements the Opera API for Gears. browser_thread must be
  // set to the ID of the thread Opera runs on.
  static void Init(OperaGearsAPI *opera_api, ThreadId browser_thread);

  // Returns the pointer to the Opera API for Gears.
  static OperaGearsAPI *GetBrowserApiForGears();

  // Returns the ID of the thread the browser runs on.
  static ThreadId GetBrowserThreadId();

  // Creates and returns a handle to an Opera worker thread.
  static OperaWorkerThreadInterface *CreateWorkerThread();

  // Called to open the Gears settings dialog.
  static void OpenSettingsDialog();

 private:
  // Holds the pointer to the global Opera API for Gears.
  static OperaGearsAPI *opera_api_;
  // Holds the ID of the thread the browser runs on.
  static ThreadId browser_thread_;

  DISALLOW_EVIL_CONSTRUCTORS(OperaUtils);
};

#endif  // GEARS_BASE_OPERA_OPERA_UTILS_H__
