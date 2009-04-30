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

#include "gears/canvas/canvas.h"

#include "gears/base/common/string_utils.h"
#include "gears/blob/blob.h"
#include "gears/canvas/blob_backed_skia_input_stream.h"
#include "gears/canvas/blob_backed_skia_output_stream.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/images/SkImageDecoder.h"
#include "third_party/skia/include/images/SkImageEncoder.h"

#if defined(OFFICIAL_BUILD)
// Canvas 2D graphics (getContext) and on-screen rendering (getRenderingElement,
// invalidateRenderingElement) methods are not yet part of the official build.
#else
#include "gears/canvas/canvas_rendering_context_2d.h"
#if BROWSER_IE
#include "gears/canvas/canvas_rendering_element_ie.h"
#endif
#endif  // defined(OFFICIAL_BUILD)

namespace canvas {
const SkBitmap::Config skia_config = SkBitmap::kARGB_8888_Config;
}

using canvas::skia_config;

DECLARE_DISPATCHER(GearsCanvas);
const std::string GearsCanvas::kModuleName("GearsCanvas");

// 16384 pixels wide * 16384 pixels high * 4 bytes per pixel requires
// 1GB of memory. Asking for that much will probably just cause an out-of-
// memory condition, but also 1GB (which is 0x40000000 bytes) is less than
// the number of bytes that causes overflow for 32-bit signed ints. On the
// other hand, 16384 pixels is large enough (by about 20%) to accomodate the
// height of A0-sized paper (1189mm, or 46.8 inches) at 300 dpi.
const int GearsCanvas::kMaxSideLength(16384);

const int GearsCanvas::kDefaultWidth(300);
const int GearsCanvas::kDefaultHeight(150);

static bool ValidateWidthAndHeight(int w, int h, JsCallContext *context) {
  if (w <= 0) {
    context->SetException(STRING16(L"Invalid (non-positive) width."));
    return false;
  }
  if (w > GearsCanvas::kMaxSideLength) {
    context->SetException(STRING16(L"Invalid width (it is too large)."));
    return false;
  }
  if (h <= 0) {
    context->SetException(STRING16(L"Invalid (non-positive) height."));
    return false;
  }
  if (h > GearsCanvas::kMaxSideLength) {
    context->SetException(STRING16(L"Invalid height (it is too large)."));
    return false;
  }
  return true;
}

#if defined(OFFICIAL_BUILD)
// Canvas 2D graphics (rendering_context_) and on-screen rendering
// (rendering_element_) fields are not yet part of the official build.
GearsCanvas::GearsCanvas()
    : ModuleImplBaseClass(kModuleName) {
  // Initial dimensions as per the HTML5 canvas spec.
  ResetCanvas(kDefaultWidth, kDefaultHeight);
}

GearsCanvas::~GearsCanvas() {
}
#else
GearsCanvas::GearsCanvas()
    : ModuleImplBaseClass(kModuleName),
      rendering_context_(NULL) {
#if BROWSER_IE
  rendering_element_ = NULL;
#endif
  // Initial dimensions as per the HTML5 canvas spec.
  ResetCanvas(kDefaultWidth, kDefaultHeight);
}

GearsCanvas::~GearsCanvas() {
  // The rendering context is destroyed first, since it has a scoped_refptr to
  // this object. See comment near bottom of canvas.h.
  assert(rendering_context_ == NULL);
#if BROWSER_IE
  if (rendering_element_ != NULL) {
    rendering_element_->OnGearsCanvasDestroyed();
    // This Release is balanced by an AddRef in GetRenderingElement.
    rendering_element_->Release();
    rendering_element_ = NULL;
  }
#endif
}

void GearsCanvas::ClearRenderingContextReference() {
  rendering_context_ = NULL;
}
#endif  // defined(OFFICIAL_BUILD)

template<>
void Dispatcher<GearsCanvas>::Init() {
  RegisterMethod("crop", &GearsCanvas::Crop);
  RegisterMethod("decode", &GearsCanvas::Decode);
  RegisterMethod("encode", &GearsCanvas::Encode);
  RegisterMethod("resize", &GearsCanvas::Resize);
  RegisterProperty("height", &GearsCanvas::GetHeight, &GearsCanvas::SetHeight);
  RegisterProperty("width", &GearsCanvas::GetWidth, &GearsCanvas::SetWidth);
#if defined(OFFICIAL_BUILD)
// Canvas 2D graphics (getContext) and on-screen rendering (getRenderingElement,
// invalidateRenderingElement) methods are not yet part of the official build.
#else
  RegisterMethod("getContext", &GearsCanvas::GetContext);
  // TODO(nigeltao): We need to decide which one of the (off-screen)
  // GearsCanvas and the (on-screen) rendering DOM element is the definitive
  // one, in terms of width and height. Does modifying the WxH of one
  // necessitate keeping the other one in sync?
  RegisterMethod("getRenderingElement", &GearsCanvas::GetRenderingElement);
  // TODO(nigeltao): Should we even have this method? Should invalidation
  // instead be automatic (like the Gecko and WebKit canvas implementations,
  // which are invalidated after every drawing operation) rather than explicit?
  // The answer probably depends on whether we can make rendering sufficiently
  // fast on all Browser x OS combinations.
  RegisterMethod("invalidateRenderingElement",
      &GearsCanvas::InvalidateRenderingElement);
#endif  // defined(OFFICIAL_BUILD)
}

void GearsCanvas::EnsureBitmapPixelsAreAllocated() {
  if (!skia_bitmap_->getPixels()) {
    skia_bitmap_->allocPixels();
    skia_bitmap_->eraseARGB(0, 0, 0, 0);
  }
}

void GearsCanvas::Decode(JsCallContext *context) {
  ModuleImplBaseClass *other_module;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_MODULE, &other_module },
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
                                    SkImageDecoder::kDecodeBounds_Mode)) {
    context->SetException(STRING16(L"Could not decode the Blob as an image."));
  }
  if (context->is_exception_set() ||
      !ValidateWidthAndHeight(
          skia_bitmap_->width(), skia_bitmap_->height(), context)) {
    ResetCanvas(kDefaultWidth, kDefaultHeight);
    return;
  }
  blob_stream.rewind();
  if (!SkImageDecoder::DecodeStream(&blob_stream,
                                    skia_bitmap_.get(),
                                    skia_config,
                                    SkImageDecoder::kDecodePixels_Mode)) {
    context->SetException(STRING16(L"Could not decode the Blob as an image."));
  }
}

void GearsCanvas::Encode(JsCallContext *context) {
  std::string16 mime_type;
  scoped_ptr<JsObject> attributes;
  JsArgument args[] = {
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &mime_type },
    { JSPARAM_OPTIONAL, JSPARAM_OBJECT, as_out_parameter(attributes) }
  };
  if (!context->GetArguments(ARRAYSIZE(args), args)) {
    assert(context->is_exception_set());
    return;
  }
  SkImageEncoder::Type type;
  if (mime_type == STRING16(L"") ||
      StringCompareIgnoreCase(mime_type.c_str(), STRING16(L"image/png")) == 0) {
    type = SkImageEncoder::kPNG_Type;
  } else if (StringCompareIgnoreCase(mime_type.c_str(),
      STRING16(L"image/jpeg")) == 0) {
    type = SkImageEncoder::kJPEG_Type;
  } else {
    // TODO(nigeltao): Should we support BMP?
    context->SetException(STRING16(L"Unsupported MIME type."));
    return;
  }

  EnsureBitmapPixelsAreAllocated();

  // SkBitmap's isOpaque flag tells whether the bitmap has any transparent
  // pixels. If this flag is set, the encoder creates a file without alpha
  // channel. We don't want this to happen, for several reasons:
  //   1. Consider:
  //        canvas.decode(jpegBlob);
  //        canvas.crop(0, 0, canvas.width, canvas.height);
  //      The original SkBitmap has isOpaque set to true (jpegs have no
  //      transparent pixels), but the new SkBitmap created during crop() has
  //      the bit clear. So the crop will not be a noop.
  //   2. A similar situation occurs with clone()...
  //   3. ... and with drawImage().
  //   4. Consider:
  //        canvas.decode(blob);
  //        canvas.width = 100;
  //        do some drawing on the canvas
  //        var outputBlob = canvas.encode();
  //
  //      When the image is loaded, the isOpaque flag is updated, and is not
  //      modified again anywhere. As a result, exported blobs will have an
  //      alpha channel or not, depending on whether the image that was loaded
  //      *before* resetting the dimensions has an alpha channel.
  //
  // All the above two cases except (3) can be dealt with by making Crop() and
  // Clone() propagate the flag from the source SkBitmap to the newly created
  // one, and by having SetWidth()/SetHeight create a fresh SkBitmap to prevent
  // picking up the isOpaque flag from the previous state.
  //
  // But for drawImage, if the source canvas is opaque, we can't set the target
  // opaque, since there may be other pixels outside the drawing rectangle that
  // may or may not be transparent. Futher, these other pixels can be cropped
  // away in the future. The only clean way is to set IsOpaque false and not
  // have the encoder strip away the alpha channel while exporting to a blob.
  skia_bitmap_->setIsOpaque(false);

  BlobBackedSkiaOutputStream output_stream;
  scoped_ptr<SkImageEncoder> encoder(SkImageEncoder::Create(type));
  bool encode_succeeded;
  double quality;
  if (args[0].was_specified &&  // Only use attributes if mime_types was also
      args[1].was_specified &&  // specified.
      attributes->GetPropertyAsDouble(STRING16(L"quality"), &quality)) {
    if (quality < 0.0 || quality > 1.0) {
      context->SetException(STRING16(L"quality must be between 0.0 and 1.0"));
      return;
    }
    encode_succeeded = encoder->encodeStream(&output_stream, *skia_bitmap_,
        static_cast<int> (quality * 100));
  } else {
    encode_succeeded = encoder->encodeStream(&output_stream, *skia_bitmap_,
        SkImageEncoder::kDefaultQuality);
  }
  if (!encode_succeeded) {
    context->SetException(STRING16(L"Could not encode image."));
    return;
  }

  scoped_refptr<BlobInterface> blob;
  output_stream.CreateBlob(&blob);
  scoped_refptr<GearsBlob> gears_blob;
  if (!CreateModule<GearsBlob>(module_environment_.get(),
                               context, &gears_blob)) {
    return;
  }
  gears_blob->Reset(blob.get());
  context->SetReturnValue(JSPARAM_MODULE, gears_blob.get());
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
  if (context->is_exception_set() ||
      !ValidateWidthAndHeight(width, height, context))
    return;
  
  SkIRect src_rect = { x, y, x + width, y + height };
  if (!IsRectValid(src_rect)) {
    context->SetException(STRING16(L"Rectangle to crop stretches beyond the "
        L"bounds of the bitmap or has negative dimensions"));
    return;
  }
  EnsureBitmapPixelsAreAllocated();
  SkBitmap new_bitmap;
  new_bitmap.setConfig(skia_config, width, height);
  new_bitmap.allocPixels();
  SkCanvas new_canvas(new_bitmap);
  SkRect dest_rect = { SkIntToScalar(0),
                       SkIntToScalar(0),
                       SkIntToScalar(width),
                       SkIntToScalar(height) };
  new_canvas.drawBitmapRect(*skia_bitmap_, &src_rect, dest_rect);
  new_bitmap.swap(*skia_bitmap_);
}

void GearsCanvas::Resize(JsCallContext *context) {
  int new_width, new_height;
  std::string16 filter;
  bool filter_is_nearest = false;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &new_width },
    { JSPARAM_REQUIRED, JSPARAM_INT, &new_height },
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &filter }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set() ||
      !ValidateWidthAndHeight(new_width, new_height, context))
    return;
  if (context->GetArgumentCount() == 3) {
    if (filter == STRING16(L"nearest")) {
      filter_is_nearest = true;
    } else if (filter != STRING16(L"bilinear")) {
      context->SetException(
          STRING16(L"Filter must be 'bilinear' or 'nearest'."));
      return;
    }
  }

  EnsureBitmapPixelsAreAllocated();
  SkBitmap new_bitmap;
  new_bitmap.setConfig(skia_config, new_width, new_height);
  new_bitmap.allocPixels();
  new_bitmap.eraseARGB(0, 0, 0, 0);

  int old_width = GetWidth();
  int old_height = GetHeight();
  if (old_width != 0 && old_height != 0) {
    SkCanvas new_canvas(new_bitmap);
    SkScalar x_scale = SkDoubleToScalar(
        static_cast<double>(new_width) / old_width);
    SkScalar y_scale = SkDoubleToScalar(
        static_cast<double>(new_height) / old_height);
    if (!new_canvas.scale(x_scale, y_scale)) {
      context->SetException(STRING16(L"Could not resize the image."));
      return;
    }
    if (filter_is_nearest) {
      new_canvas.drawBitmap(*skia_bitmap_, SkIntToScalar(0), SkIntToScalar(0));
    } else {
      SkPaint paint;
      paint.setFilterBitmap(true);
      new_canvas.drawBitmap(
          *skia_bitmap_, SkIntToScalar(0), SkIntToScalar(0), &paint);
    }
  }
  new_bitmap.swap(*skia_bitmap_);
}

void GearsCanvas::GetWidth(JsCallContext *context) {
  int its_width = GetWidth();
  context->SetReturnValue(JSPARAM_INT, &its_width);
}

void GearsCanvas::GetHeight(JsCallContext *context) {
  int its_height = GetHeight();
  context->SetReturnValue(JSPARAM_INT, &its_height);
}

void GearsCanvas::SetWidth(JsCallContext *context) {
  int new_width;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &new_width }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set() ||
      !ValidateWidthAndHeight(new_width, GetHeight(), context))
    return;
  ResetCanvas(new_width, GetHeight());
}

void GearsCanvas::SetHeight(JsCallContext *context) {
  int new_height;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &new_height }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set() ||
      !ValidateWidthAndHeight(GetWidth(), new_height, context))
    return;
  ResetCanvas(GetWidth(), new_height);
}

#if defined(OFFICIAL_BUILD)
// Canvas 2D graphics (getContext) and on-screen rendering (getRenderingElement,
// invalidateRenderingElement) methods are not yet part of the official build.
#else
void GearsCanvas::GetContext(JsCallContext *context) {
  std::string16 context_id;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &context_id },
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  if (context_id != STRING16(L"gears-2d")) {
    context->SetReturnValue(JSPARAM_NULL, NULL);
    // As per the HTML5 canvas spec.
    return;
  }
  // Make sure the rendering context is not destroyed before SetReturnValue().
  scoped_refptr<GearsCanvasRenderingContext2D> rendering_context_scoped_ptr;
  if (rendering_context_ == NULL) {
    if (!CreateModule<GearsCanvasRenderingContext2D>(
        module_environment_.get(), context, &rendering_context_scoped_ptr)) {
      context->SetException(STRING16(L"Unable to create context"));
      return;
    }
    EnsureBitmapPixelsAreAllocated();
    rendering_context_ = rendering_context_scoped_ptr.get();
    rendering_context_->SetCanvas(this, skia_bitmap_.get());
  }
  context->SetReturnValue(JSPARAM_MODULE, rendering_context_);
}

void GearsCanvas::GetRenderingElement(JsCallContext *context) {
  if (EnvIsWorker()) {
    context->SetException(
        STRING16(L"getRenderingElement is not supported in workers."));
    return;
  }

#if BROWSER_FF
  JsToken retval = JSVAL_NULL;
  // In the script below, g is the Gears canvas.
  module_environment_->js_runner_->EvalFuncWithModuleArg(
      STRING16(L"function(g){"
          L"if(!g.renderingElement)"
            L"g.renderingElement=document.createElement('canvas');"
          L"return g.renderingElement;}"),
      this,
      &retval);
  if (retval == JSVAL_NULL) {
    context->SetException(STRING16(L"Error in getRenderingElement."));
    return;
  }
  context->SetReturnValue(JSPARAM_TOKEN, &retval);
#elif BROWSER_IE
  if (!rendering_element_) {
    CComObject<CanvasRenderingElementIE> *rendering_element;
    if (FAILED(CComObject<CanvasRenderingElementIE>::CreateInstance(
            &rendering_element))) {
      context->SetException(GET_INTERNAL_ERROR_MESSAGE());
      return;
    }
    rendering_element_ = rendering_element;
    if (!rendering_element_) {
      context->SetException(GET_INTERNAL_ERROR_MESSAGE());
      return;
    }
    // This AddRef is balanced by a Release in the GearsCanvas destructor.
    rendering_element_->AddRef();
    rendering_element_->SetGearsCanvas(this);
  }
  IHTMLElement *element = rendering_element_->GetHtmlElement();
  if (!element) {
    context->SetException(GET_INTERNAL_ERROR_MESSAGE());
    return;
  }
  CComVariant result(element);
  context->SetReturnValue(JSPARAM_TOKEN, &result);
#else
  context->SetException(STRING16(L"Unimplemented"));
#endif
}

void GearsCanvas::InvalidateRenderingElement(JsCallContext *context) {
  if (EnvIsWorker()) {
    context->SetException(
        STRING16(L"invalidateRenderingElement is not supported in workers."));
    return;
  }
  // Currently, this method takes no args, although it could conceivably take
  // an (optional) x,y,w,h rectangle to invalidate.

#if BROWSER_FF
  JsToken retval = JSVAL_NULL;
  module_environment_->js_runner_->EvalFuncWithModuleArg(
      // In the script below, g is the Gears canvas, gc is the Gears canvas'
      // 2d-context, and nc is the native canvas' 2d-context. The 256 is
      // just a number that is not more than the getImageData upper bound,
      // GearsCanvasRenderingContext2D::kMaxImageDataSize.
      // This script uses getImageData/putImageData to move the pixels from
      // the off-screen Gears canvas to the on-screen native canvas, in sets
      // of 256 x 256 slices. This probably isn't the fastest way to paint a
      // Gears canvas onto the screen (as opposed to some low-level XPCOM
      // incantations), and we're not even caching the eval'ed function, but
      // it is simple and it works, and we can optimize the implementation
      // later, if necessary.
      STRING16(L"function(g){"
          L"if(!g.renderingElement)"
            L"return;"
          L"var gc=g.getContext('gears-2d');"
          L"var nc=g.renderingElement.getContext('2d');"
          L"for(var y=0;y<g.height;y+=256){"
            L"for(var x=0;x<g.width;x+=256){"
              L"var w=Math.min(256,g.width-x);"
              L"var h=Math.min(256,g.height-y);"
              L"nc.putImageData(gc.getImageData(x,y,w,h),x,y);"
          L"}}}"),
      this,
      &retval);
  if (retval == JSVAL_NULL) {
    context->SetException(STRING16(L"Error in invalidateRenderingElement."));
    return;
  }
#elif BROWSER_IE
  if (rendering_element_) {
    rendering_element_->InvalidateRenderingElement();
  }
#else
  context->SetException(STRING16(L"Unimplemented"));
#endif
}
#endif  // defined(OFFICIAL_BUILD)

int GearsCanvas::GetWidth() const {
  return skia_bitmap_->width();
}

int GearsCanvas::GetHeight() const {
  return skia_bitmap_->height();
}

const SkBitmap &GearsCanvas::GetSkBitmap() {
  EnsureBitmapPixelsAreAllocated();
  return *(skia_bitmap_.get());
}

void GearsCanvas::ResetCanvas(int width, int height) {
  assert(width > 0 && width <= kMaxSideLength &&
         height > 0 && height <= kMaxSideLength);
  // TODO(nigeltao): poke the gears_canvas_rendering_context_2d to notify it
  // that its underlying bitmap has been deleted.
  skia_bitmap_.reset(new SkBitmap);
  skia_bitmap_->setConfig(skia_config, width, height);
}

bool GearsCanvas::IsRectValid(const SkIRect &rect) {
  return rect.fLeft <= rect.fRight && rect.fTop <= rect.fBottom &&
      rect.fLeft >= 0 && rect.fTop >= 0 &&
      rect.fRight <= GetWidth() && rect.fBottom <= GetHeight();
}
