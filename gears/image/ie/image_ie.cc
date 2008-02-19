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

#ifdef OFFICIAL_BUILD
// The Image API has not been finalized for official builds
#else

#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"

#include "gears/blob/blob_ie.h"
#include "gears/image/ie/image_ie.h"


STDMETHODIMP GearsImage::get_contents(VARIANT *retval) {
  assert(img_ != NULL);
  retval->vt = VT_BYREF;
  retval->byref = img_.get();
  RETURN_NORMAL();
}

STDMETHODIMP GearsImage::resize(int width, int height) {
  assert(img_ != NULL);
  std::string16 error;
  if (!img_->Resize(width, height, &error)) {
    RETURN_EXCEPTION(error.c_str());
  }

  RETURN_NORMAL();
}


STDMETHODIMP GearsImage::crop(int x, int y, int width, int height) {
  assert(img_ != NULL);
  std::string16 error;
  if (!img_->Crop(x, y, width, height, &error)) {
    RETURN_EXCEPTION(error.c_str());
  }

  RETURN_NORMAL();
}


STDMETHODIMP GearsImage::rotate(int degrees) {
  assert(img_ != NULL);
  std::string16 error;
  if (!img_->Rotate(degrees, &error)) {
    RETURN_EXCEPTION(error.c_str());
  }

  RETURN_NORMAL();
}


STDMETHODIMP GearsImage::flipHorizontal() {
  assert(img_ != NULL);
  std::string16 error;
  if (!img_->FlipHorizontal(&error)) {
    RETURN_EXCEPTION(error.c_str());
  }
  RETURN_NORMAL();
}


STDMETHODIMP GearsImage::flipVertical() {
  assert(img_ != NULL);
  std::string16 error;
  if (!img_->FlipVertical(&error)) {
    RETURN_EXCEPTION(error.c_str());
  }
  RETURN_NORMAL();
}


STDMETHODIMP GearsImage::get_width(int *retval) {
  assert(img_ != NULL);
  *retval = img_->Width();
  RETURN_NORMAL();
}


STDMETHODIMP GearsImage::get_height(int *retval) {
  assert(img_ != NULL);
  *retval = img_->Height();
  RETURN_NORMAL();
}


STDMETHODIMP GearsImage::drawImage(IUnknown *image, int x, int y) {
  assert(img_ != NULL);

  // Get the image
  CComQIPtr<GearsImagePvtInterface> image_pvt(image);
  if (!image_pvt) {
    RETURN_EXCEPTION(STRING16(L"The image parameter must be an image object."));
  }
  VARIANT var;
  HRESULT hr = image_pvt->get_contents(&var);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Error getting image contents."));
  }
  Image *img = reinterpret_cast<Image*>(var.byref);

  std::string16 error;
  if (!img_->DrawImage(img, x, y, &error)) {
    RETURN_EXCEPTION(error.c_str());
  }
  RETURN_NORMAL();
}


STDMETHODIMP GearsImage::toBlob(const VARIANT *type,
                                IUnknown **retval) {
  assert(img_ != NULL);

  bool type_given = false;
  CComBSTR type_bstr(L"");
  if (ActiveXUtils::OptionalVariantIsPresent(type)) {
    if (type->vt != VT_BSTR) {
      RETURN_EXCEPTION(STRING16(L"The type parameter must be a string."));
    } else {
      type_given = true;
      type_bstr = type->bstrVal;
    }
  }

  CComObject<GearsBlob> *blob = NULL;
  HRESULT hr = CComObject<GearsBlob>::CreateInstance(&blob);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Could not create GearsBlob."));
  }
  CComPtr<CComObject<GearsBlob> > blob_ptr(blob);

  if (!type_given) {
    blob->Reset(img_->ToBlob());
  } else if (std::string16(type_bstr) == STRING16(L"image/png")) {
    blob->Reset(img_->ToBlob(Image::FORMAT_PNG));
  } else if (std::string16(type_bstr) == STRING16(L"image/gif")) {
    blob->Reset(img_->ToBlob(Image::FORMAT_GIF));
  } else if (std::string16(type_bstr) == STRING16(L"image/jpeg")) {
    blob->Reset(img_->ToBlob(Image::FORMAT_JPEG));
  } else {
    RETURN_EXCEPTION(STRING16(L"Format must be either 'image/png', "
                              L"'image/gif' or 'image/jpeg'"));
  }

  if (!blob->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Initializing base class failed."));
  }
  CComQIPtr<GearsBlobInterface> blob_external = blob;
  if (!blob_external) {
    RETURN_EXCEPTION(STRING16(L"Could not get GearsBlob interface."));
  }
  *retval = blob_external.Detach();
  RETURN_NORMAL();
}

#endif  // not OFFICIAL_BUILD
