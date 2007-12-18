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

#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsILocalFile.h>
#include <nsMemory.h>
#include <nsXPCOM.h>
#include "gecko_internal/nsIDOMClassInfo.h"
#include "gecko_internal/nsIScriptNameSpaceManager.h"

#include "gears/base/common/thread_locals.h"
#include "gears/channel/firefox/channel.h"
#include "gears/database/firefox/database.h"
#include "gears/database/firefox/result_set.h"
#include "gears/desktop/desktop_ff.h"
#include "gears/factory/firefox/factory.h"
#include "gears/httprequest/firefox/httprequest_ff.h"
#include "gears/localserver/firefox/cache_intercept.h"
#include "gears/localserver/firefox/localserver_ff.h"
#include "gears/localserver/firefox/managed_resource_store_ff.h"
#include "gears/localserver/firefox/resource_store_ff.h"
#include "gears/localserver/firefox/file_submitter_ff.h"
#include "gears/timer/timer.h"
#include "gears/ui/firefox/ui_utils.h"
#include "gears/workerpool/firefox/workerpool.h"

#ifdef DEBUG
#include "gears/cctests/test_ff.h"
#endif

//-----------------------------------------------------------------------------

// TODO(cprince): can remove this when switch to google3 logging
#ifdef PR_LOGGING
PRLogModuleInfo *gLog = PR_NewLogModule(PRODUCT_SHORT_NAME_ASCII);
#endif

//-----------------------------------------------------------------------------

#define SINGLETON_CONSTRUCTOR(class_)                                  \
static NS_METHOD class_##Constructor(nsISupports *outer,               \
                                     const nsIID &iid,                 \
                                     void **result) {                  \
  if (outer)                                                           \
    return NS_ERROR_NO_AGGREGATION;                                    \
  static class_ *service = nsnull;                                     \
  if (!service) {                                                      \
    service = new class_();                                            \
    if (!service)                                                      \
      return NS_ERROR_OUT_OF_MEMORY;                                   \
    NS_ADDREF(service);  /* hold reference for lifetime of app */      \
  }                                                                    \
  return service->QueryInterface(iid, result);                         \
}

#define SINGLETON_CONSTRUCTOR_INIT(class_)                             \
static NS_METHOD class_##Constructor(nsISupports *outer,               \
                                     const nsIID &iid,                 \
                                     void **result) {                  \
  if (outer)                                                           \
    return NS_ERROR_NO_AGGREGATION;                                    \
  static class_ *service = nsnull;                                     \
  if (!service) {                                                      \
    service = new class_();                                            \
    if (!service)                                                      \
      return NS_ERROR_OUT_OF_MEMORY;                                   \
    NS_ADDREF(service);  /* hold reference for lifetime of app */      \
    nsresult rv = service->Init();                                     \
    if (NS_FAILED(rv)) {                                               \
      NS_RELEASE(service);                                             \
      return rv;                                                       \
    }                                                                  \
  }                                                                    \
  return service->QueryInterface(iid, result);                         \
}

//-----------------------------------------------------------------------------


const char *kDomciExtensionContractId = "@google.com/" PRODUCT_SHORT_NAME_ASCII
                                        "/domciextension;1";
const char *kDomciExtensionClassName = "DomciExtension";
const nsCID kDomciExtensionClassId = {0x71f2446a, 0x81ed, 0x4345, {0x8d, 0xdb,
                                      0xd6, 0x9b, 0xd5, 0xc3, 0xc7, 0x32}};
                                     // {71F2446A-81ED-4345-8DDB-D69BD5C3C732}

NS_DOMCI_EXTENSION(Scour)
  // "Under The Hood":
  // The entries below form the body of this extension's RegisterDOMCI method.
  // For each block, if the string in line 1 matches the RegisterDOMCI argument,
  // RegisterDOMClassInfo will be called for each of the enclosed IIDs.
  // RegisterDOMClassInfo calls into gNameSpaceManager->RegisterDOMCIData.

  // factory
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsFactory)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsFactoryInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsFactory, PR_TRUE,
                                             &kGearsFactoryClassId)

  // database
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsDatabase)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsDatabaseInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsDatabase, PR_TRUE,
                                             &kGearsDatabaseClassId)
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsResultSet)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsResultSetInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsResultSet, PR_TRUE,
                                             &kGearsResultSetClassId)

  // desktop
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsDesktop)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsDesktopInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsDesktop, PR_TRUE,
                                             &kGearsDesktopClassId)

  // httprequest
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsHttpRequest)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsHttpRequestInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsHttpRequest, PR_TRUE,
                                             &kGearsHttpRequestClassId)

  // workerpool
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsWorkerPool)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsWorkerPoolInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsWorkerPool, PR_TRUE,
                                             &kGearsWorkerPoolClassId)

  // localserver
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsLocalServer)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsLocalServerInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsLocalServer, PR_TRUE,
                                             &kGearsLocalServerClassId)
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsManagedResourceStore)
     NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsManagedResourceStoreInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsManagedResourceStore, PR_TRUE,
                                             &kGearsManagedResourceStoreClassId)
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsResourceStore)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsResourceStoreInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsResourceStore, PR_TRUE,
                                             &kGearsResourceStoreClassId)
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsFileSubmitter)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsFileSubmitterInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsFileSubmitter, PR_TRUE,
                                             &kGearsFileSubmitterClassId)

#ifdef DEBUG
  // test
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsTest)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsTestInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsTest, PR_TRUE,
                                             &kGearsTestClassId)
#endif

  // timer
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsTimer)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsTimerInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsTimer, PR_TRUE,
                                             &kGearsTimerClassId)

  // channel
  NS_DOMCI_EXTENSION_ENTRY_BEGIN(GearsChannel)
    NS_DOMCI_EXTENSION_ENTRY_INTERFACE(GearsChannelInterface)
  NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GearsChannel, PR_TRUE,
                                             &kGearsChannelClassId)

NS_DOMCI_EXTENSION_END

static NS_METHOD ScourRegisterSelf(nsIComponentManager *compMgr,
                                   nsIFile *path,
                                   const char *loaderStr,
                                   const char *type,
                                   const nsModuleComponentInfo *info) {
  LOG(("RegisterSelf()\n"));

  // Trigger early initialization of our cache interceptor.
  nsCOMPtr<nsICategoryManager> catMgr =
      do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  if (!catMgr)
    return NS_ERROR_UNEXPECTED;

  // The cache intercept component needs to be initialized at xpcom startup
  // time (so that it can override the default cache implementation).
  catMgr->AddCategoryEntry(NS_XPCOM_STARTUP_OBSERVER_ID,
                           kCacheInterceptClassName,
                           kCacheInterceptContractId,
                           PR_TRUE, PR_TRUE, NULL);


  const struct {
    const char *className;
    const char *interfaceName;
    const char *interfaceIDStr;
  } jsDOMClasses[] = {
    // factory
    { kGearsFactoryClassName, "GearsFactoryInterface",
      GEARSFACTORYINTERFACE_IID_STR },
    // database
    { kGearsDatabaseClassName, "GearsDatabaseInterface",
      GEARSDATABASEINTERFACE_IID_STR },
    { kGearsResultSetClassName, "GearsResultSetInterface",
      GEARSRESULTSETINTERFACE_IID_STR },
    // desktop
    { kGearsDesktopClassName, "GearsDesktopInterface",
      GEARSDESKTOPINTERFACE_IID_STR },
    // httprequest
    { kGearsHttpRequestClassName, "GearsHttpRequestInterface",
      GEARSHTTPREQUESTINTERFACE_IID_STR },
    // workerpool
    { kGearsWorkerPoolClassName, "GearsWorkerPoolInterface",
      GEARSWORKERPOOLINTERFACE_IID_STR },
    // localserver
    { kGearsLocalServerClassName, "GearsLocalServerInterface",
      GEARSLOCALSERVERINTERFACE_IID_STR },
    { kGearsManagedResourceStoreClassName, "GearsManagedResourceStoreInterface",
      GEARSMANAGEDRESOURCESTOREINTERFACE_IID_STR },
    { kGearsResourceStoreClassName, "GearsResourceStoreInterface",
      GEARSRESOURCESTOREINTERFACE_IID_STR },
    { kGearsFileSubmitterClassName, "GearsFileSubmitterInterface",
      GEARSFILESUBMITTERINTERFACE_IID_STR },

#ifdef DEBUG
    // test
    { kGearsTestClassName, "GearsTestInterface",
      GEARSTESTINTERFACE_IID_STR },
#endif
    // timer
    { kGearsTimerClassName, "GearsTimerInterface",
      GEARSTIMERINTERFACE_IID_STR },
    // channel
    { kGearsChannelClassName, "GearsChannelInterface",
      GEARSCHANNELINTERFACE_IID_STR }
  };

  for (size_t i = 0; i < NS_ARRAY_LENGTH(jsDOMClasses); ++i) {
    catMgr->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                             jsDOMClasses[i].className,
                             kDomciExtensionContractId,
                             PR_TRUE, PR_TRUE, NULL);
    // AddCategoryEntry(JAVASCRIPT_DOM_INTERFACE, ...) does not seem to be
    // necessary for our Gears interfaces.
  }

  return NS_OK;
}


// We need a NS_DECL_DOM_CLASSINFO for each
// NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO in the codebase.
//
// These macros inform the DOM about every Gears class it might see
// (indicating, for example, what methods and properties exist).
//
// "Under The Hood":
// NS_DECL_DOM_CLASSINFO instances a global variable.  Its argument must match
// the argument to NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO.  The value
// also gets string-ified and affects the class name exposed to JavaScript.

// factory
NS_DECL_DOM_CLASSINFO(GearsFactory)
// database
NS_DECL_DOM_CLASSINFO(GearsDatabase)
NS_DECL_DOM_CLASSINFO(GearsResultSet)
// desktop
NS_DECL_DOM_CLASSINFO(GearsDesktop)
// httprequest
NS_DECL_DOM_CLASSINFO(GearsHttpRequest)
// workerpool
NS_DECL_DOM_CLASSINFO(GearsWorkerPool)
// localserver
NS_DECL_DOM_CLASSINFO(GearsLocalServer)
NS_DECL_DOM_CLASSINFO(GearsManagedResourceStore)
NS_DECL_DOM_CLASSINFO(GearsResourceStore)
NS_DECL_DOM_CLASSINFO(GearsFileSubmitter)

#ifdef DEBUG
// test
NS_DECL_DOM_CLASSINFO(GearsTest)
#endif

// timer
NS_DECL_DOM_CLASSINFO(GearsTimer)
// channel
NS_DECL_DOM_CLASSINFO(GearsChannel)

nsresult PR_CALLBACK ScourModuleConstructor(nsIModule *self) {
  return ThreadLocals::HandleModuleConstructed();
}


void PR_CALLBACK ScourModuleDestructor(nsIModule *self) {
  // We need a NS_IF_RELEASE for each
  // NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO in the codebase.

  // factory
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsFactory));
  // database
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsDatabase));
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsResultSet));
  // desktop
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsDesktop));
  // httprequest
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsHttpRequest));
  // workerpool
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsWorkerPool));
  // localserver
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsLocalServer));
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsManagedResourceStore));
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsResourceStore));

#ifdef DEBUG
  // test
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsTest));
#endif

  // timer
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsTimer));
  // channel
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsChannel));
#ifdef DEBUG
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GearsFileSubmitter));
#endif
}


// Define nsFactory constructors for certain classes.
// These constructors are referenced in components[] below.
//
// We do not need to define a factory constructor for Gears objects that
// should only be created via GearsFactory (rather than instanced directly).
//
// IMPORTANT: objects that derive from ModuleImplBaseClass should not use
// singleton init because their state (like security origin) will not get
// updated when the page changes!

SINGLETON_CONSTRUCTOR(CacheIntercept)
SINGLETON_CONSTRUCTOR(GearsUiUtils)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(GearsFactory, InitBaseFromDOM)
// On Firefox this C++ Factory name affects the script-visible name.
// It may be possible to use modified Mozilla macros to break the dependency,
// but we don't have that ability today.


static const nsModuleComponentInfo components[] = {
  // internal plumbing
  { kDomciExtensionClassName, // [0] fields could be any description string
    kDomciExtensionClassId,
    kDomciExtensionContractId,
    NS_DOMCI_EXTENSION_CONSTRUCTOR(Scour) },

  { kCacheInterceptClassName,
    kCacheInterceptClassId,
    kCacheInterceptContractId,
    CacheInterceptConstructor,
    ScourRegisterSelf }, // register all components in one go

  // ui
  { kGearsUiUtilsClassName,
    kGearsUiUtilsClassId,
    kGearsUiUtilsContractId,
    GearsUiUtilsConstructor },

  // factory
  { kGearsFactoryClassName,
    kGearsFactoryClassId,
    kGearsFactoryContractId,
    GearsFactoryConstructor }
};


NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(gears_module, components,
                                   ScourModuleConstructor,
                                   ScourModuleDestructor)
