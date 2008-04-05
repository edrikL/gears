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

#ifndef GEARS_IMAGE_IE_IMAGE_IE_H__
#define GEARS_IMAGE_IE_IMAGE_IE_H__

#ifdef OFFICIAL_BUILD
// The Image API has not been finalized for official builds
#else

#include "genfiles/interfaces.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"

#include "gears/image/common/image.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"


class ATL_NO_VTABLE GearsImage
    : public ModuleImplBaseClass,
      public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<GearsImage>,
      public GearsImagePvtInterface,
      public IDispatchImpl<GearsImageInterface> {
 public:
  BEGIN_COM_MAP(GearsImage)
    COM_INTERFACE_ENTRY(GearsImageInterface)
    COM_INTERFACE_ENTRY(GearsImagePvtInterface)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(GearsImage)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  // End boilerplate code. Begin interface.

  // Need a default constructor to CreateInstance objects in IE
  GearsImage() : img_(NULL) {}
  ~GearsImage() {}

  void Init(Image *img) {
    img_.reset(img);
  }

  STDMETHOD(get_contents)(VARIANT *retval);

  STDMETHOD(resize)(int width, int height);
  STDMETHOD(crop)(int xl, int y, int width, int height);
  STDMETHOD(rotate)(int degrees);
  STDMETHOD(flipHorizontal)();
  STDMETHOD(flipVertical)();
  STDMETHOD(drawImage)(IUnknown *image, int x, int y);
  STDMETHOD(get_width)(int *retval);
  STDMETHOD(get_height)(int *retval);
  STDMETHOD(toBlob)(const VARIANT *type, IUnknown **retval);

 private:
  scoped_ptr<Image> img_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsImage);
};

#endif  // not OFFICIAL_BUILD

#endif  // GEARS_IMAGE_IE_IMAGE_IE_H__
