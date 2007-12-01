// Copyright 2005, Google Inc.
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

#ifndef GEARS_LOCALSERVER_NPAPI_LOCALSERVER_NP_H__
#define GEARS_LOCALSERVER_NPAPI_LOCALSERVER_NP_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"

//-----------------------------------------------------------------------------
// GearsLocalServer
//-----------------------------------------------------------------------------
class GearsLocalServer : public ModuleImplBaseClass {
 public:
  // need a default constructor to instance objects from the Factory
  GearsLocalServer() {}

  // IN: string url
  // OUT: bool retval
  void CanServeLocally(JsCallContext *context);

  // IN: string name
  // OPTIONAL IN: required_cookie
  // OUT: GearsManagedResourceStore *retval
  void CreateManagedStore(JsCallContext *context);

  // IN: string name
  // OPTIONAL IN: required_cookie
  // OUT: GearsManagedResourceStore *retval
  void OpenManagedStore(JsCallContext *context);

  // IN: string name
  // OPTIONAL IN: required_cookie
  void RemoveManagedStore(JsCallContext *context);

  // IN: string name
  // OPTIONAL IN: required_cookie
  // OUT: GearsResourceStore *retval
  void CreateStore(JsCallContext *context);

  // IN: string name
  // OPTIONAL IN: required_cookie
  // OUT: GearsResourceStore *retval
  void OpenStore(JsCallContext *context);

  // IN: string name
  // OPTIONAL IN: required_cookie
  void RemoveStore(JsCallContext *context);

 private:
  bool GetAndCheckParameters(JsCallContext *context, std::string16 *name,
                             std::string16 *required_cookie);

  DISALLOW_EVIL_CONSTRUCTORS(GearsLocalServer);
};

ModuleWrapperBaseClass *CreateGearsLocalServer(ModuleImplBaseClass *sibling);

#endif // GEARS_LOCALSERVER_NPAPI_RESOURCE_STORE_NP_H__
