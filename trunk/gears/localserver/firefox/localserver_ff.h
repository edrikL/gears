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

#ifndef GEARS_LOCALSERVER_FIREFOX_LOCALSERVER_FF_H__
#define GEARS_LOCALSERVER_FIREFOX_LOCALSERVER_FF_H__

#include "genfiles/localserver.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"


// Object identifiers
extern const char *kGearsLocalServerClassName;
extern const nsCID kGearsLocalServerClassId;


//-----------------------------------------------------------------------------
// GearsLocalServer
//-----------------------------------------------------------------------------
class GearsLocalServer
    : public ModuleImplBaseClass,
      public GearsLocalServerInterface {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  // need a default constructor to instance objects from the Factory
  GearsLocalServer() {}

  NS_IMETHOD CanServeLocally(
      //const nsAString &url,
      PRBool *retval
      );
  NS_IMETHOD CreateManagedStore(
      //const nsAString &name
      //OPTIONAL in AString required_cookie
      GearsManagedResourceStoreInterface **retval
      );
  NS_IMETHOD OpenManagedStore(
      //const nsAString &name
      //OPTIONAL in AString required_cookie
      GearsManagedResourceStoreInterface **retval
      );
  NS_IMETHOD RemoveManagedStore(
      //const nsAString &name
      //OPTIONAL in AString required_cookie
      );
  NS_IMETHOD CreateStore(
      //const nsAString &name
      //OPTIONAL in AString required_cookie
      GearsResourceStoreInterface **retval
      );
  NS_IMETHOD OpenStore(
      //const nsAString &name
      //OPTIONAL in AString required_cookie
      GearsResourceStoreInterface **retval
      );
  NS_IMETHOD RemoveStore(
      //const nsAString &name
      //OPTIONAL in AString required_cookie
      );

 private:
  bool GetAndCheckParameters(bool has_string_retval,
                                 std::string16 *name,
                                 std::string16 *required_cookie,
                                 std::string16 *error_message);

  DISALLOW_EVIL_CONSTRUCTORS(GearsLocalServer);
};


#endif // GEARS_LOCALSERVER_FIREFOX_RESOURCE_STORE_FF_H__
