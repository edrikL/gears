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

#include <nsCOMPtr.h>
#include <nsDebug.h>
#include <nsICategoryManager.h>
#include <nsIFile.h>
#include <nsIProperties.h>
#include <nsIServiceManager.h>
#include <nsIURI.h>
#include <nsServiceManagerUtils.h>  // for NS_IMPL_* and NS_INTERFACE_*
#include <nsXPCOM.h>
#include <gecko_sdk/include/nspr.h>
#include <gecko_sdk/include/nsServiceManagerUtils.h>
#include <gecko_sdk/include/nsCOMPtr.h>
#include <gecko_internal/jsapi.h>
#include <gecko_internal/nsIDOMClassInfo.h>

#include "gears/base/common/paths.h"
#include "gears/base/common/common.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/sqlite_wrapper.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"

#include "gears/image/firefox/image_ff.h"

struct JSContext;

// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_ADDREF(GearsImage)
NS_IMPL_RELEASE(GearsImage)
NS_INTERFACE_MAP_BEGIN(GearsImage)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsImageInterface)
  NS_INTERFACE_MAP_ENTRY(GearsImagePvtInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsImageInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsImage)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsImageClassName = "GearsImage";
const nsCID kGearsImageClassId = {0xef496b80, 0xa2fa, 0x11dc, {0x81, 0x32,
                                  0x5e, 0xeb, 0x55, 0xd8, 0x95, 0x93}};
                              // {EF496B80-A2FA-11DC-8132-5EEB55D89593}

NS_IMETHODIMP GearsImage::GetContents(Image** retval) {
  assert(img_ != NULL);
  *retval = img_.get();
  RETURN_NORMAL();
}


NS_IMETHODIMP GearsImage::Resize(// PRInt32 width,
                                 // PRInt32 height
                                ) {
  assert(img_ != NULL);
  int width, height;
  JsParamFetcher js_params(this);
  if (js_params.GetCount(false) < 2) {
    RETURN_EXCEPTION(STRING16(L"resize() requires 2 parameters."));
  } else if (!js_params.GetAsInt(0, &width)) {
    RETURN_EXCEPTION(STRING16(L"The width parameter must be an int."));
  } else if (!js_params.GetAsInt(1, &height)) {
    RETURN_EXCEPTION(STRING16(L"The height parameter must be an int."));
  }

  std::string16 error;
  if (!img_->Resize(width, height, &error)) {
    RETURN_EXCEPTION(error.c_str());
  }

  RETURN_NORMAL();
}


NS_IMETHODIMP GearsImage::Crop(// PRInt32 x,
                               // PRInt32 y,
                               // PRInt32 width,
                               // PRInt32 height
                               ) {
  assert(img_ != NULL);
  int x, y, width, height;
  JsParamFetcher js_params(this);
  if (js_params.GetCount(false) < 4) {
    RETURN_EXCEPTION(STRING16(L"crop() requires 4 parameters."));
  } else if (!js_params.GetAsInt(0, &x)) {
    RETURN_EXCEPTION(STRING16(L"The x parameter must be an int."));
  } else if (!js_params.GetAsInt(1, &y)) {
    RETURN_EXCEPTION(STRING16(L"The y parameter must be an int."));
  } else if (!js_params.GetAsInt(2, &width)) {
    RETURN_EXCEPTION(STRING16(L"The width parameter must be an int."));
  } else if (!js_params.GetAsInt(3, &height)) {
    RETURN_EXCEPTION(STRING16(L"The height parameter must be an int."));
  }

  std::string16 error;
  if (!img_->Crop(x, y, width, height, &error)) {
    RETURN_EXCEPTION(error.c_str());
  }

  RETURN_NORMAL();
}


NS_IMETHODIMP GearsImage::Rotate(// PRInt32 degrees
                                 ) {
  assert(img_ != NULL);
  int degrees;
  JsParamFetcher js_params(this);
  if (js_params.GetCount(false) < 1) {
    RETURN_EXCEPTION(STRING16(L"rotate() requires 1 parameter."));
  } else if (!js_params.GetAsInt(0, &degrees)) {
    RETURN_EXCEPTION(STRING16(L"The degrees parameter must be an int."));
  }
  std::string16 error;
  if (!img_->Rotate(degrees, &error)) {
    RETURN_EXCEPTION(error.c_str());
  }

  RETURN_NORMAL();
}


NS_IMETHODIMP GearsImage::FlipHorizontal() {
  assert(img_ != NULL);
  std::string16 error;
  if (!img_->FlipHorizontal(&error)) {
    RETURN_EXCEPTION(error.c_str());
  }
  RETURN_NORMAL();
}


NS_IMETHODIMP GearsImage::FlipVertical() {
  assert(img_ != NULL);
  std::string16 error;
  if (!img_->FlipVertical(&error)) {
    RETURN_EXCEPTION(error.c_str());
  }
  RETURN_NORMAL();
}


NS_IMETHODIMP GearsImage::DrawImage(//GearsImageInterface *image,
                                    //PRInt32 x,
                                    //PRInt32 y
                                   ) {
  assert(img_ != NULL);

  nsCOMPtr<nsISupports> isupports;
  int x, y;
  JsParamFetcher js_params(this);
  if (js_params.GetCount(false) < 3) {
    RETURN_EXCEPTION(STRING16(L"drawImage() requires 3 parameters."));
  } else if (!js_params.GetAsModule(0, getter_AddRefs(isupports))) {
    RETURN_EXCEPTION(STRING16(L"The image parameter must be an image object."));
  } else if (!js_params.GetAsInt(1, &x)) {
    RETURN_EXCEPTION(STRING16(L"The x parameter must be an int."));
  } else if (!js_params.GetAsInt(2, &y)) {
    RETURN_EXCEPTION(STRING16(L"The y parameter must be an int."));
  }

  // Get the image
  nsresult nr;
  nsCOMPtr<GearsImagePvtInterface> image_pvt =
      do_QueryInterface(isupports, &nr);
  if (NS_FAILED(nr) || !image_pvt) {
    RETURN_EXCEPTION(STRING16(L"The image parameter must be an image object."));
  }
  Image *img;
  nr = image_pvt->GetContents(&img);
  if (NS_FAILED(nr) || !img) {
    RETURN_EXCEPTION(STRING16(L"Error getting image data."));
  }

  std::string16 error;
  if (!img_->DrawImage(img, x, y, &error)) {
    RETURN_EXCEPTION(error.c_str());
  }
  RETURN_NORMAL();
}


NS_IMETHODIMP GearsImage::GetWidth(PRInt32 *retval) {
  assert(img_ != NULL);
  *retval = img_->Width();
  RETURN_NORMAL();
}


NS_IMETHODIMP GearsImage::GetHeight(PRInt32 *retval) {
  assert(img_ != NULL);
  *retval = img_->Height();
  RETURN_NORMAL();
}


NS_IMETHODIMP GearsImage::ToBlob(// const nsAString &type,
                                 nsISupports **retval) {
  assert(img_ != NULL);

  bool type_given = false;
  std::string16 type;
  JsParamFetcher js_params(this);
  if (!js_params.IsOptionalParamPresent(1, false)) {
    // Type is not given
  } else if (!js_params.GetAsString(0, &type)) {
    type_given = true;
    RETURN_EXCEPTION(STRING16(L"The type parameter must be a string."));
  }

  GearsBlob *blob = new GearsBlob();
  nsCOMPtr<GearsBlobInterface> blob_external = blob;

  if (!type_given) {
    blob->Reset(img_->ToBlob());
  } else if (type == STRING16(L"image/png")) {
    blob->Reset(img_->ToBlob(Image::FORMAT_PNG));
  } else if (type == STRING16(L"image/gif")) {
    blob->Reset(img_->ToBlob(Image::FORMAT_GIF));
  } else if (type == STRING16(L"image/jpeg")) {
    blob->Reset(img_->ToBlob(Image::FORMAT_JPEG));
  } else {
    RETURN_EXCEPTION(STRING16(L"Format must be either 'image/png', "
                              L"'image/gif' or 'image/jpeg'"));
  }

  if (!blob->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Initializing base class failed."));
  }
  NS_ADDREF(*retval = blob_external);
  RETURN_NORMAL();
}

#endif  // not OFFICIAL_BUILD
