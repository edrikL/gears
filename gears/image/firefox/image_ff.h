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

#ifndef GEARS_IMAGE_FIREFOX_IMAGE_FF_H__
#define GEARS_IMAGE_FIREFOX_IMAGE_FF_H__

#ifdef OFFICIAL_BUILD
// The Image API has not been finalized for official builds
#else

#include <nsComponentManagerUtils.h>

#include "genfiles/image.h"
#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/js_runner.h"
#include "gears/blob/blob_ff.h"
#include "gears/image/common/image.h"
#include "gears/third_party/scoped_ptr/scoped_ptr.h"

// Object identifiers
extern const char *kGearsImageClassName;
extern const nsCID kGearsImageClassId;


class GearsImage
    : public ModuleImplBaseClass,
      public GearsImageInterface,
      public GearsImagePvtInterface {
 public:
  NS_DECL_ISUPPORTS
  GEARS_IMPL_BASECLASS
  // End boilerplate code. Begin interface.

  // Creates a GearsImage with the given Image object.  For use from other
  // modules.
  GearsImage() {}

  void Init(Image *img) {
    img_.reset(img);
  }

  NS_IMETHOD GetContents(Image** retval);

  NS_IMETHOD Resize(// PRInt32 width, PRInt32 height
                   );
  NS_IMETHOD Crop(// PRInt32 x, PRInt32 y, PRInt32 width, PRInt32 height
                 );
  NS_IMETHOD Rotate(// PRInt32 degrees
                   );
  NS_IMETHOD FlipHorizontal();
  NS_IMETHOD FlipVertical();
  NS_IMETHOD DrawImage(//GearsImageInterface *image, PRInt32 x, PRInt32 y
                      );
  NS_IMETHOD GetWidth(PRInt32 *retval);
  NS_IMETHOD GetHeight(PRInt32 *retval);
  NS_IMETHOD ToBlob(// const nsAString &type,
                    nsISupports **retval);

 private:
  ~GearsImage() {}

  scoped_ptr<Image> img_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsImage);
};

#endif  // not OFFICIAL_BUILD

#endif  // GEARS_IMAGE_FIREFOX_IMAGE_FF_H__
