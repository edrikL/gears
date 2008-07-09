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

#ifndef GEARS_FACTORY_IE_FACTORY_H__
#define GEARS_FACTORY_IE_FACTORY_H__

#include <objsafe.h>
#include "genfiles/product_constants.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/permissions_db.h"
#include "gears/base/ie/resource.h"  // for .rgs resource ids (IDR_*)
#include "genfiles/interfaces.h"


// The factory class satisfies requests for Gears objects matching specific
// versions. Gears offers multiple side-by-side versions of components so that
// applications can request specific component configurations. We didn't want
// to change class/method signatures each time a version changed, so we use
// this factory class to choose at runtime which version of an object to
// instantiate.

class ATL_NO_VTABLE GearsFactory
    : public ModuleImplBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsFactory, &CLSID_GearsFactory>,
      public IObjectWithSiteImpl<GearsFactory>,
      public IObjectSafetyImpl<GearsFactory,
                               INTERFACESAFE_FOR_UNTRUSTED_CALLER +
                               INTERFACESAFE_FOR_UNTRUSTED_DATA>,
      public IDispatchImpl<GearsFactoryInterface> {
 public:
  BEGIN_COM_MAP(GearsFactory)
    COM_INTERFACE_ENTRY(GearsFactoryInterface)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  DECLARE_REGISTRY_RESOURCEID(IDR_GEARSFACTORY)
  // End boilerplate code.

  GearsFactory();

  // GearsFactoryInterface methods
  STDMETHOD(create)(const BSTR object, const VARIANT *version,
                    IDispatch **retval);
  STDMETHOD(getBuildInfo)(BSTR *retval);
  STDMETHOD(get_version)(BSTR *retval);

  // bool getPermission(string siteName, string imageUrl, string extraMessage)
  STDMETHOD(getPermission)(const VARIANT *site_name,
                           const VARIANT *image_url,
                           const VARIANT *extra_message,
                           VARIANT_BOOL *retval);
  // readonly bool hasPermission
  STDMETHOD(get_hasPermission)(VARIANT_BOOL *retval);

  // Hook into SetSite() to do some init work that requires the site.
  STDMETHOD(SetSite)(IUnknown *site);

#ifdef WINCE
  // Set the page's script engine. This is only required for WinCE.
  // TODO(steveblock): Ideally, we would avoid the need for this method and
  // somehow pass this pointer to the GearsFactory constructor. This could
  // either be through ActiveXObject() or through the OBJECT tag's parameters.
  // However, it looks like neither approach is possible.
  STDMETHOD(privateSetGlobalObject)(/* [in] */ IDispatch *js_dispatch);
#endif

  // Non-scriptable methods
  void SuspendObjectCreation();
  void ResumeObjectCreationAndUpdatePermissions();

 private:
  // Helper method used by Create(). Returns true if the module was created
  // successfully. Returns false and an empty error message if the module is
  // unknown. Returns false and an error message for other errors.
  bool CreateDispatcherModule(const std::string16 &object_name,
                              IDispatch **retval, std::string16 *error);

  // A factory starts out operational, but it can be put in a "suspended" state,
  // unable to create objects.  This is important for some use cases, like
  // cross-origin workers.
  bool is_creation_suspended_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsFactory);
};

OBJECT_ENTRY_AUTO(__uuidof(GearsFactory), GearsFactory)

#endif  // GEARS_FACTORY_IE_FACTORY_H__
