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
#include "gears/image/firefox/image_loader_ff.h"

// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_ADDREF(GearsImageLoader)
NS_IMPL_RELEASE(GearsImageLoader)
NS_INTERFACE_MAP_BEGIN(GearsImageLoader)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsImageLoaderInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsImageLoaderInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsImageLoader)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsImageLoaderClassName = "GearsImageLoader";
const nsCID kGearsImageLoaderClassId =
{0x63610164, 0x96ed, 0x4c06, {0x9d, 0xc9, 0x3d, 0x3f, 0xb5, 0xdd, 0x3a, 0xef}};
// {63610164-96ed-4c06-9dc9-3d3fb5dd3aef}

NS_IMETHODIMP
GearsImageLoader::CreateImageFromBlob(nsISupports *blob,
                                      GearsImageInterface **retval) {
  // Extract the blob
  nsresult nr;
  nsCOMPtr<GearsBlobPvtInterface> blob_pvt = do_QueryInterface(blob, &nr);
  if (NS_FAILED(nr) || !blob_pvt) {
    RETURN_EXCEPTION(STRING16(L"Error converting to native class."));
  }
  scoped_refptr<BlobInterface> blob_contents;
  nr = blob_pvt->GetContents(as_out_parameter(blob_contents));
  if (NS_FAILED(nr) || !blob_contents.get()) {
    RETURN_EXCEPTION(STRING16(L"Error getting blob contents."));
  }

  // Create the image
  Image *img = new Image();
  GearsImage *image = new GearsImage();
  nsCOMPtr<GearsImageInterface> image_external = image;
  image->Init(img);
  std::string16 error;
  if (!img->Init(blob_contents.get(), &error)) {
    RETURN_EXCEPTION(error.c_str());
  }
  if (!image->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Initializing base class failed."));
  }

  // Return the image
  NS_ADDREF(*retval = image_external);
  RETURN_NORMAL();
}

#endif  // not OFFICIAL_BUILD
