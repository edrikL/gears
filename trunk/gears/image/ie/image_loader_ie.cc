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

#include "gears/image/common/image.h"
#include "gears/image/ie/image_ie.h"
#include "gears/image/ie/image_loader_ie.h"

HRESULT GearsImageLoader::createImageFromBlob(IUnknown *blob,
                                              GearsImageInterface **retval) {
  // Extract the blob
  CComQIPtr<GearsBlobPvtInterface> blob_pvt(blob);
  if (!blob_pvt) {
    RETURN_EXCEPTION(STRING16(L"Error converting to native class."));
  }
  VARIANT var;
  HRESULT hr = blob_pvt->get_contents(&var);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Error getting blob contents."));
  }
  scoped_refptr<BlobInterface> blob_contents(
      reinterpret_cast<BlobInterface*>(var.byref));
  blob_contents->Unref();

  // Create the image
  CComObject<GearsImage> *image = NULL;
  hr = CComObject<GearsImage>::CreateInstance(&image);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Could not create GearsImage."));
  }
  CComPtr<CComObject<GearsImage> > image_ptr(image);
  Image *image_contents = new Image();
  image->Init(image_contents);
  std::string16 error;
  if (!image_contents->Init(blob_contents.get(), &error)) {
    RETURN_EXCEPTION(error.c_str());
  }
  if (!image->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Initializing base class failed."));
  }

  // Return the image
  CComQIPtr<GearsImageInterface> image_external = image;
  if (!image_external) {
    RETURN_EXCEPTION(STRING16(L"Could not get GearsImage interface."));
  }
  *retval = image_external.Detach();
  RETURN_NORMAL();
}

#endif  // not OFFICIAL_BUILD
