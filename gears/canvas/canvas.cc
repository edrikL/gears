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

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/base/common/string_utils.h"
#include "gears/blob/blob.h"
#include "gears/canvas/blob_backed_skia_input_stream.h"
#include "gears/canvas/blob_backed_skia_output_stream.h"
#include "gears/canvas/canvas_rendering_context_2d.h"
#include "third_party/skia/include/SkCanvas.h"
#include "third_party/skia/include/SkImageDecoder.h"
#include "third_party/skia/include/SkStream.h"

DECLARE_GEARS_WRAPPER(GearsCanvas);
const std::string GearsCanvas::kModuleName("GearsCanvas");
namespace {
const SkBitmap::Config skia_config = SkBitmap::kARGB_8888_Config;
}

GearsCanvas::GearsCanvas()
    : ModuleImplBaseClassVirtual(kModuleName),
      rendering_context_(NULL),
      skia_bitmap_(new SkBitmap) {
  // Initial dimensions as per the HTML5 canvas spec.
  skia_bitmap_->setConfig(skia_config, 300, 150);
  skia_canvas_.reset(new SkCanvas(*skia_bitmap_));
}

template<>
void Dispatcher<GearsCanvas>::Init() {
  RegisterMethod("load", &GearsCanvas::Load);
  RegisterMethod("toBlob", &GearsCanvas::ToBlob);
  RegisterMethod("clone", &GearsCanvas::Clone);
  RegisterProperty("originalPalette", &GearsCanvas::GetOriginalPalette, NULL);
  RegisterMethod("crop", &GearsCanvas::Crop);
  RegisterMethod("resize", &GearsCanvas::Resize);
  RegisterProperty("width", &GearsCanvas::GetWidth, &GearsCanvas::SetWidth);
  RegisterProperty("height", &GearsCanvas::GetHeight, &GearsCanvas::SetHeight);
  RegisterMethod("getContext", &GearsCanvas::GetContext);
}

void GearsCanvas::Load(JsCallContext *context) {
  ModuleImplBaseClass *other_module;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_DISPATCHER_MODULE, &other_module },
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  assert(other_module);
  if (GearsBlob::kModuleName != other_module->get_module_name()) {
    context->SetException(STRING16(L"Argument must be a Blob."));
    return;
  }
  scoped_refptr<BlobInterface> blob;
  static_cast<GearsBlob*>(other_module)->GetContents(&blob);
  assert(blob.get());
  
  BlobBackedSkiaInputStream blob_stream(blob.get());
  if (!SkImageDecoder::DecodeStream(&blob_stream,
                                    skia_bitmap_.get(),
                                    skia_config,
                                    SkImageDecoder::kDecodePixels_Mode)) {
    context->SetException(STRING16(L"Couldn't decode blob."));
  }
}

void GearsCanvas::ToBlob(JsCallContext *context) {
  std::string16 mime_type;
  JsObject attributes;
  JsArgument args[] = {
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &mime_type },
    { JSPARAM_OPTIONAL, JSPARAM_OBJECT, &attributes }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  SkImageEncoder::Type type;
  if (mime_type == STRING16(L"") ||
      StringCompareIgnoreCase(mime_type.c_str(), STRING16(L"image/png")) == 0) {
    type = SkImageEncoder::kPNG_Type;
  } else if (StringCompareIgnoreCase(mime_type.c_str(),
      STRING16(L"image/jpeg")) == 0) {
    type = SkImageEncoder::kJPEG_Type;
  } else {
    // TODO(kart): support BMP. create it manually?
    // Skia doesn't support BMP encoding.
    context->SetException(STRING16(L"Unsupported MIME type."));
    return;
  }

  BlobBackedSkiaOutputStream output_stream;
  scoped_ptr<SkImageEncoder> encoder(SkImageEncoder::Create(type));
  if (!encoder->encodeStream(&output_stream, *skia_bitmap_)) {
    // TODO(kart): Support quality attribute.
    context->SetException(STRING16(L"Could not encode image."));
    return;
  }
  scoped_refptr<BlobInterface> blob;
  output_stream.CreateBlob(&blob);
  scoped_refptr<GearsBlob> gears_blob;
  CreateModule<GearsBlob>(GetJsRunner(), &gears_blob);
  if (!gears_blob->InitBaseFromSibling(this)) {
    context->SetException(STRING16(L"Initializing base class failed."));
    return;
  }
  gears_blob->Reset(blob.get());
  context->SetReturnValue(JSPARAM_DISPATCHER_MODULE, gears_blob.get());
}

void GearsCanvas::Clone(JsCallContext *context) {
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvas::GetOriginalPalette(JsCallContext *context) {
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvas::Crop(JsCallContext *context) {
  int x, y, width, height;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &x },
    { JSPARAM_REQUIRED, JSPARAM_INT, &y },
    { JSPARAM_REQUIRED, JSPARAM_INT, &width },
    { JSPARAM_REQUIRED, JSPARAM_INT, &height }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvas::Resize(JsCallContext *context) {
  int width, height;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &width },
    { JSPARAM_REQUIRED, JSPARAM_INT, &height }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

int GearsCanvas::Width() {
  return skia_bitmap_->width();
}

SkScalar GearsCanvas::Height() {
  return skia_bitmap_->height();
}

void GearsCanvas::GetWidth(JsCallContext *context) {
  int width = Width();
  context->SetReturnValue(JSPARAM_INT, &width);
}

void GearsCanvas::GetHeight(JsCallContext *context) {
  int height = Height();
  context->SetReturnValue(JSPARAM_INT, &height);
}

void GearsCanvas::SetWidth(JsCallContext *context) {
  int width;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &width }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  skia_bitmap_->setConfig(skia_config, width, Height());
}

void GearsCanvas::SetHeight(JsCallContext *context) {
  int height;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &height }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  skia_bitmap_->setConfig(skia_config, Width(), height);
}

void GearsCanvas::GetContext(JsCallContext *context) {
  std::string16 context_id;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &context_id },
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  if (context_id != STRING16(L"gears-2d")) {
    context->SetReturnValue(JSPARAM_NULL, 0);
    // As per the HTML5 canvas spec.
    return;
  }
  if (rendering_context_.get() == NULL) {
    CreateModule<GearsCanvasRenderingContext2D>(GetJsRunner(),
        &rendering_context_);
    if (!rendering_context_->InitBaseFromSibling(this)) {
      context->SetException(STRING16(L"Initializing base class failed."));
      // Don't return a broken object the next time this function is called.
      rendering_context_.reset(NULL);
      return;
    }
    rendering_context_->InitCanvasField(this);
  }
  context->SetReturnValue(JSPARAM_DISPATCHER_MODULE, rendering_context_.get());
}

SkBitmap *GearsCanvas::SkiaBitmap() {
  return skia_bitmap_.get();
}

SkCanvas *GearsCanvas::SkiaCanvas() {
  return skia_canvas_.get();
}  
