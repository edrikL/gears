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

#include "gears/localserver/npapi/managed_resource_store_np.h"
#include "gears/localserver/npapi/resource_store_np.h"

//-----------------------------------------------------------------------------
// CanServeLocally
//-----------------------------------------------------------------------------
void GearsLocalServer::CanServeLocally() {
  bool retval = false;
  GetJsRunner()->SetReturnValue(JSPARAM_BOOL, &retval);
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// CreateManagedStore
//-----------------------------------------------------------------------------
void GearsLocalServer::CreateManagedStore() {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(&name, &required_cookie))
    return;

  ScopedModuleWrapper store_wrapper(CreateGearsManagedResourceStore(this));
  if (!store_wrapper.get())
    return;  // Create function sets an error message.

  JsToken retval = store_wrapper.get()->GetWrapperToken();
  GetJsRunner()->SetReturnValue(JSPARAM_OBJECT_TOKEN, &retval);
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// OpenManagedStore
//-----------------------------------------------------------------------------
void GearsLocalServer::OpenManagedStore() {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(&name, &required_cookie))
    return;

  GetJsRunner()->SetReturnValue(JSPARAM_NULL, NULL);
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// RemoveManagedStore
//-----------------------------------------------------------------------------
void GearsLocalServer::RemoveManagedStore() {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(&name, &required_cookie))
    return;

  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// CreateStore
//-----------------------------------------------------------------------------
void GearsLocalServer::CreateStore() {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(&name, &required_cookie))
    return;

  ScopedModuleWrapper store_wrapper(CreateGearsResourceStore(this));
  if (!store_wrapper.get())
    return;  // Create function sets an error message.

  JsToken retval = store_wrapper.get()->GetWrapperToken();
  GetJsRunner()->SetReturnValue(JSPARAM_OBJECT_TOKEN, &retval);
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// OpenStore
//-----------------------------------------------------------------------------
void GearsLocalServer::OpenStore() {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(&name, &required_cookie))
    return;

  GetJsRunner()->SetReturnValue(JSPARAM_NULL, NULL);
  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

//-----------------------------------------------------------------------------
// RemoveStore
//-----------------------------------------------------------------------------
void GearsLocalServer::RemoveStore() {
  std::string16 name;
  std::string16 required_cookie;
  if (!GetAndCheckParameters(&name, &required_cookie))
    return;

  RETURN_EXCEPTION(STRING16(L"Not Implemented"));
}

//------------------------------------------------------------------------------
// GetAndCheckParameters
//------------------------------------------------------------------------------
bool GearsLocalServer::GetAndCheckParameters(std::string16 *name,
                                             std::string16 *required_cookie) {
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, name },
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, required_cookie },
  };
  int argc = GetJsRunner()->GetArguments(ARRAYSIZE(argv), argv);
  if (argc < 1)
    return false;  // JsRunner sets an error message.

  // Validate parameters
  if (name->empty()) {
    SET_EXCEPTION(STRING16(L"The name parameter is required."));
    return false;
  }

  if (!IsStringValidPathComponent(name->c_str())) {
    std::string16 error(STRING16(L"The name parameter contains invalid "
                                 L"characters: "));
    error += *name;
    error += STRING16(L".");
    SET_EXCEPTION(error.c_str());
    return false;
  }

  return true;
}
