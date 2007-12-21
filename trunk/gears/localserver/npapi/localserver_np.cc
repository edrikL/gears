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

#include "gears/localserver/npapi/localserver_np.h"

#include "gears/base/common/paths.h"
#include "gears/base/npapi/module_wrapper.h"
#include "gears/localserver/npapi/managed_resource_store_np.h"
#include "gears/localserver/npapi/resource_store_np.h"

DECLARE_GEARS_WRAPPER(GearsLocalServer);

// static
void Dispatcher<GearsLocalServer>::Init() {
  RegisterMethod("canServeLocally", &GearsLocalServer::CanServeLocally);
  RegisterMethod("createManagedStore", &GearsLocalServer::CreateManagedStore);
  RegisterMethod("openManagedStore", &GearsLocalServer::OpenManagedStore);
  RegisterMethod("removeManagedStore", &GearsLocalServer::RemoveManagedStore);
  RegisterMethod("createStore", &GearsLocalServer::CreateStore);
  RegisterMethod("openStore", &GearsLocalServer::OpenStore);
  RegisterMethod("removeStore", &GearsLocalServer::RemoveStore);
}

//-----------------------------------------------------------------------------
// CanServeLocally
//-----------------------------------------------------------------------------
void GearsLocalServer::CanServeLocally(JsCallContext *context) {
  bool retval = false;
  context->SetReturnValue(JSPARAM_BOOL, &retval);
  context->SetException(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// CreateManagedStore
//-----------------------------------------------------------------------------
void GearsLocalServer::CreateManagedStore(JsCallContext *context) {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(context, &name, &required_cookie))
    return;

  GComPtr<GearsManagedResourceStore> store(
        CreateModule<GearsManagedResourceStore>(EnvPageJsContext()));
  if (!store.get())
    return;  // Create function sets an error message.

  if (!store->InitBaseFromSibling(this)) {
    context->SetException(STRING16(L"Error initializing base class."));
    return;
  }

  context->SetReturnValue(JSPARAM_MODULE, store.get());
  context->SetException(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// OpenManagedStore
//-----------------------------------------------------------------------------
void GearsLocalServer::OpenManagedStore(JsCallContext *context) {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(context, &name, &required_cookie))
    return;

  context->SetReturnValue(JSPARAM_NULL, NULL);
  context->SetException(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// RemoveManagedStore
//-----------------------------------------------------------------------------
void GearsLocalServer::RemoveManagedStore(JsCallContext *context) {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(context, &name, &required_cookie))
    return;

  context->SetException(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// CreateStore
//-----------------------------------------------------------------------------
void GearsLocalServer::CreateStore(JsCallContext *context) {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(context, &name, &required_cookie))
    return;

  GComPtr<GearsResourceStore> store(
        CreateModule<GearsResourceStore>(EnvPageJsContext()));
  if (!store.get())
    return;  // Create function sets an error message.

  if (!store->InitBaseFromSibling(this)) {
    context->SetException(STRING16(L"Error initializing base class."));
    return;
  }

  context->SetReturnValue(JSPARAM_MODULE, store.get());
  context->SetException(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// OpenStore
//-----------------------------------------------------------------------------
void GearsLocalServer::OpenStore(JsCallContext *context) {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(context, &name, &required_cookie))
    return;

  context->SetReturnValue(JSPARAM_NULL, NULL);
  context->SetException(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// RemoveStore
//-----------------------------------------------------------------------------
void GearsLocalServer::RemoveStore(JsCallContext *context) {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(context, &name, &required_cookie))
    return;

  context->SetException(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetAndCheckParameters
//------------------------------------------------------------------------------
bool GearsLocalServer::GetAndCheckParameters(JsCallContext *context,
                                             std::string16 *name,
                                             std::string16 *required_cookie) {
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, name },
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, required_cookie },
  };
  int argc = context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set())
    return false;

  // Validate parameters
  if (name->empty()) {
    context->SetException(STRING16(L"The name parameter is required."));
    return false;
  }

  std::string16 error_message;
  if (!IsUserInputValidAsPathComponent(*name, &error_message)) {
    context->SetException(error_message.c_str());
    return false;
  }

  return true;
}
