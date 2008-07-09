// Copyright 2006, Google Inc.
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

#ifndef GEARS_BASE_FIREFOX_FACTORY_H__
#define GEARS_BASE_FIREFOX_FACTORY_H__

#include <vector>
#include "genfiles/factory.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/html_event_monitor.h"
#include "gears/base/common/permissions_db.h"


// Object identifiers
extern const char *kGearsFactoryContractId;
extern const char *kGearsFactoryClassName;
extern const nsCID kGearsFactoryClassId;


// The factory class satisfies requests for Gears objects matching specific
// versions. Gears offers multiple side-by-side versions of components so that
// applications can request specific component configurations. We didn't want
// to change class/method signatures each time a version changed, so we use
// this factory class to choose at runtime which version of an object to
// instantiate.

class GearsFactory
    : public ModuleImplBaseClass,
      public GearsFactoryInterface {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  GearsFactory();

  NS_IMETHOD Create(//const nsAString &object
                    //const nsAString &version
                    nsISupports **retval);
  NS_IMETHOD GetBuildInfo(nsAString &retval);
  NS_IMETHOD GetVersion(nsAString &retval);

  // bool getPermission(string siteName, string imageUrl, string extraMessage)
  NS_IMETHOD GetPermission(//const nsAString &siteName
                           //const nsAString &imageUrl
                           //const nsAString &extraMessage
                           PRBool *retval);
  // readonly bool hasPermission
  NS_IMETHOD GetHasPermission(PRBool *retval);


  // Non-scriptable methods
  void SuspendObjectCreation();
  void ResumeObjectCreationAndUpdatePermissions();

 private:
  // Helper method used by Create(). Returns true if the module was created
  // successfully. Returns false and an empty error message if the module is
  // unknown. Returns false and an error message for other errors.
  bool CreateDispatcherModule(const std::string16 &object_name,
                              JsParamFetcher *js_params, std::string16 *error);

  // A factory starts out operational, but it can be put in a "suspended" state,
  // unable to create objects.  This is important for some use cases, like
  // cross-origin workers.
  bool is_creation_suspended_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsFactory);
};


#endif // GEARS_BASE_FIREFOX_FACTORY_H__
