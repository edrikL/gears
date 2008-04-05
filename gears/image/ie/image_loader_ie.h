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

#ifndef GEARS_IMAGE_IE_IMAGE_LOADER_IE_H__
#define GEARS_IMAGE_IE_IMAGE_LOADER_IE_H__

#ifdef OFFICIAL_BUILD
// The Image API has not been finalized for official builds
#else

#include "genfiles/interfaces.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"


class ATL_NO_VTABLE GearsImageLoader
    : public ModuleImplBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsImageLoader>,
      public IDispatchImpl<GearsImageLoaderInterface> {
 public:
  BEGIN_COM_MAP(GearsImageLoader)
    COM_INTERFACE_ENTRY(GearsImageLoaderInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsImageLoader)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  // End boilerplate code. Begin interface.

  STDMETHOD(createImageFromBlob)(IUnknown *blob,
                                 GearsImageInterface **retval);

  GearsImageLoader() {}
  ~GearsImageLoader() {}


 private:
  DISALLOW_EVIL_CONSTRUCTORS(GearsImageLoader);
};

#endif  // not OFFICIAL_BUILD

#endif  // GEARS_IMAGE_IE_IMAGE_LOADER_IE_H__
