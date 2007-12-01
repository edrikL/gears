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

#ifndef GEARS_BASE_NPAPI_FACTORY_H__
#define GEARS_BASE_NPAPI_FACTORY_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"


class JsRunnerInterface;

// The factory class satisfies requests for Gears objects matching specific
// versions. Gears offers multiple side-by-side versions of components so that
// applications can request specific component configurations. We didn't want
// to change class/method signatures each time a version changed, so we use
// this factory class to choose at runtime which version of an object to
// instantiate.

class GearsFactory
    : public ModuleImplBaseClass {
 public:
  GearsFactory();

  // IN: string object, string version
  void Create(JsCallContext *context);

  // OUT: string build_info
  void GetBuildInfo(JsCallContext *context);

  // OUT: string version_string
  void GetVersion(JsCallContext *context);

  // Non-scriptable methods
  void SuspendObjectCreation();
  void ResumeObjectCreationAndUpdatePermissions();

 private:
  // friends for exposing 'is_permission_*' fields
  friend class PoolThreadsManager;
  friend bool HasPermissionToUseGears(GearsFactory *factory,
                                      const char16 *custom_icon_url,
                                      const char16 *custom_name,
                                      const char16 *custom_message);

  // A factory starts out operational, but it can be put in a "suspended" state,
  // unable to create objects.  This is important for some use cases, like
  // cross-origin workers.
  bool is_creation_suspended_;

  // We remember for the life of this page even if 'remember_choice'
  // is not chosen. The page and it's workers will be allowed to use
  // Gears without prompts until the page is unloaded.
  //
  // TODO(cprince): move this into ModuleImplBaseClass to auto-pass permission
  // data around. Do that when we have a single instance of the info per page,
  // because multiple copies can get out of sync (permission data is mutable).
  bool is_permission_granted_;
  bool is_permission_value_from_user_; // user prompt, or persisted DB value

  DISALLOW_EVIL_CONSTRUCTORS(GearsFactory);
};


#endif // GEARS_BASE_NPAPI_FACTORY_H__
