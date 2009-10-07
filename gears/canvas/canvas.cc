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

// Highest-quality minification involves allocating an intermediate buffer.
// Since it involves summing a number of pixel values, it is 32 bits per
// channel (i.e. 16 bytes per ARGB pixel), even when ARGB is 8 bits per
// channel (and 4 bytes per pixel).
// To avoid trying to allocate too large an intermediate buffer, we cap the
// dimension of the minified bitmap to 1024 pixels per side, which is 1048576
// pixels overall in the worst case, or 16MB.
// A more sophisticated algorithm might have lower memory requirements (e.g.
// allocating a Wx2 buffer rather than a WxH buffer), but such optimization
// is left for the future.
static const int kMaxSideLengthForHighestQualityMinification(1024);

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
  RegisterMethod("rotateCW", &GearsCanvas::RotateCW);
  RegisterMethod("rotate180", &GearsCanvas::Rotate180);
  RegisterMethod("rotateCCW", &GearsCanvas::RotateCCW);
  RegisterMethod("flipHorizontal", &GearsCanvas::FlipHorizontal);
  RegisterMethod("flipVertical", &GearsCanvas::FlipVertical);
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

static inline void accumulate(uint32 *buffer, int x_index, int y_index,
    int columns, int channels[4], int weight) {
  if (weight == 0) {
    return;
  }
  uint32 *b = buffer + 4 * ((y_index * columns) + x_index);
  *b++ += channels[0] * weight;
  *b++ += channels[1] * weight;
  *b++ += channels[2] * weight;
  *b++ += channels[3] * weight;
}

// As of revision 89, Skia does not implement tri-linear filtering, so we have
// a custom implementation of high-quality minification. A canvas minification
// is presumably a one-off operation, and so we do not go through the overhead
// of building a traditional mipmap pyramid. Instead, the algorithm to minify
// an ow*oh sized bitmap to a nw*nh sized bitmap goes like this:
// (1) a nw*nh*4 contiguous buffer of uint32s are allocated, one per channel
//     per pixel in the new bitmap. This buffer is initialized to all-zero,
//     and eventually, the buffer will have the weighted sums of the old per-
//     channel per-pixel values.
// (2) the old bitmap is iterated over, and each old per-channel per-pixel
//     value from an old pixel contributes to one or more buffer entries (more
//     on the actual weighting below).
// (3) the new bitmap is set, pixel by pixel. Each pixel is constructed from
//     its four channels, with each channel value being the buffer's value
//     scaled by the sum of its constituent weights.
static bool HighestQualityMinify(SkBitmap &old_bitmap, SkBitmap &new_bitmap) {
  int old_width = old_bitmap.width();
  int old_height = old_bitmap.height();
  int old_size = old_width * old_height;
  int new_width = new_bitmap.width();
  int new_height = new_bitmap.height();
  int new_size = new_width * new_height;
  // This algorithm only applies during a minifying resize, not a magnifying
  // resize, or a mixed resize (e.g. minify along the horizontal axis, and
  // magnify along the vertical axis).
  // We also cap the size of the intermediate buffer, via capping the size of
  // the resultant minified bitmap.
  if (new_width >= old_width || new_height >= old_height ||
      new_width > kMaxSideLengthForHighestQualityMinification ||
      new_height > kMaxSideLengthForHighestQualityMinification) {
    return false;
  }

  // The implementation of step (2) below has so far only been done for the two
  // most common configurations: 8-bit indexed color and 32-bit true-color.
  if (old_bitmap.config() != SkBitmap::kIndex8_Config &&
      old_bitmap.config() != SkBitmap::kARGB_8888_Config) {
    return false;
  }
  SkColorTable *color_table =
      (old_bitmap.config() == SkBitmap::kARGB_8888_Config)
          ? NULL : old_bitmap.getColorTable();

  assert(new_bitmap.config() == SkBitmap::kARGB_8888_Config);
  assert(old_bitmap.readyToDraw());
  assert(new_bitmap.readyToDraw());
  SkAutoLockPixels old_bitmap_lock(old_bitmap);
  SkAutoLockPixels new_bitmap_lock(new_bitmap);

  // This is step (1). As a reminder, buffer is 32-bit per channel, not
  // 32-bit per pixel. A pixel has four channels: typically (A,R,G,B), but
  // it could be configured to be (A,B,G,R). To avoid confusion under different
  // SK_R32_SHIFT / SK_B32_SHIFT values, we shall just call these channels
  // c0, c1, c2 and c3.
  uint32 *buffer = new uint32[new_size * 4];
  if (buffer == NULL) {
    return false;
  }
  memset(buffer, 0, new_size * 4 * sizeof(uint32));

  // This is step (2). The weighting will be illustrated with an example of
  // minifying a 4*3 image to become a 3*2 image.
  // Conceptually, consider a 12*6 grid. This grid has a width (12) equal to
  // the product of the old and new widths (4 and 3). Similarly, this grid has
  // a height (6) equal to the product of the old and new heights (3 and 2).
  // Each of the 12 old pixels (labelled a-l) is worth 6 (= new_size) cells
  // each. Each of the 6 new pixels (delimited by whitespace) are worth 12
  // (= old_size) cells each. The weights that each old pixel contributes to
  // each new pixel is:
  //
  //      aaab bbcc cddd
  //      aaab bbcc cddd
  //      eeef ffgg ghhh
  //
  //      eeef ffgg ghhh
  //      iiij jjkk klll
  //      iiij jjkk klll
  //
  // For example, the middle-bottom pixel in the new bitmap will be 17% f,
  // 17% g, 33% j, and 33% k. The buffer value for that pixel (for a specific
  // channel) would be the sum of 2 * f's value, 2 * g's value, 4 * j's value
  // and 4 * k's value, and at step (3) that buffer value would be divided by
  // the number of cells per new pixel (i.e. 12 = old_size).
  //
  // Note that, since we have restricted the new bitmap to be neither wider nor
  // higher than the old one, then each old pixel can contribute to at most
  // four new pixels. Let's call these pixels top-left, top-right, bottom-left
  // and bottom-right. If an old pixel does not straddle a new pixel boundary
  // horizontally, then lets say there is only a left and not a right.
  // Similarly, we favor top over bottom.
  //
  // Let qt, qb, ql and qr be such that the number of cells that the old pixel
  // contributes to the new top-left pixel be (qt * ql), to the new top-right
  // pixel be (qt * qr), and so on. Note that the two invariants (qt + qr ==
  // new_height) and (ql + qr == new_width) must always hold.
  // For example, the "f" old pixel has qt=1, qb=1, ql=1, qr=2.
  // Similarly, the "j" old pixel has qt=2, qb=0, ql=1, qr=2.
  //
  // As a final point (for step (2)), there are old_size grid cells per new
  // pixel, and each cell can have a value from 0-255 inclusive. To avoid
  // 32-bit (signed) integer overflow during the accumulation, if old_size is
  // larger than 2^23 then we scale the weights by a positive weight_shift.
  // The magic number 2^23 was chosen because it is (slightly) less than
  // ((2^31) - 1) / 255), and ((2^31) - 1) is the maximum value of an int32.
  int weight_shift = 0;
  int max_weight_divided_by_255 = old_size;
  while (max_weight_divided_by_255 > (1 << 23)) {
    max_weight_divided_by_255 >>= 1;
    weight_shift++;
  }

  for (int y = 0; y < old_height; y++) {
    uint32_t* old_row32 = NULL;
    uint8_t* old_row8 = NULL;
    if (old_bitmap.config() == SkBitmap::kARGB_8888_Config) {
      old_row32 = old_bitmap.getAddr32(0, y);
    } else {
      old_row8 = old_bitmap.getAddr8(0, y);
    }

    int new_y = (y * new_height) / old_height;
    int qt = ((new_y + 1) * old_height) - (y * new_height);
    if (qt > new_height) {
      qt = new_height;
    }
    int qb = new_height - qt;
    assert((y != old_height - 1) || (qb == 0));

    for (int x = 0; x < old_width; x++) {
      int new_x = (x * new_width) / old_width;
      int ql = ((new_x + 1) * old_width) - (x * new_width);
      if (ql > new_width) {
        ql = new_width;
      }
      int qr = new_width - ql;
      assert((x != old_width - 1) || (qr == 0));

      SkPMColor pm_color = (old_bitmap.config() == SkBitmap::kARGB_8888_Config)
          ? old_row32[x] : (*color_table)[old_row8[x]];
      int channels[4];
      channels[0] = (pm_color >> 24) & 0xFF;
      channels[1] = (pm_color >> 16) & 0xFF;
      channels[2] = (pm_color >>  8) & 0xFF;
      channels[3] = (pm_color >>  0) & 0xFF;

      // Now accumulate the old pixels over the (up to four) new pixels. Note
      // that, although (new_x + 1), (new_y + 1) or both might be out of bounds
      // of the buffer array, in those cases (i.e. new_x == new_width - 1),
      // (new_y == new_height - 1) or both, the accumulate call will be a no-op
      // because (qr == 0), (qb == 0) or both.
      accumulate(buffer, new_x + 0, new_y + 0, new_width, channels,
          (qt * ql) >> weight_shift);
      accumulate(buffer, new_x + 1, new_y + 0, new_width, channels,
          (qt * qr) >> weight_shift);
      accumulate(buffer, new_x + 0, new_y + 1, new_width, channels,
          (qb * ql) >> weight_shift);
      accumulate(buffer, new_x + 1, new_y + 1, new_width, channels,
          (qb * qr) >> weight_shift);
    }
  }

  // This is step (3).
  uint32 *b = buffer;
  int half = max_weight_divided_by_255 / 2;
  for (int y = 0; y < new_height; y++) {
    uint32_t* new_row = new_bitmap.getAddr32(0, y);
    for (int x = 0; x < new_width; x++) {
      // We round to nearest, not round down, so we first add "half".
      int c0 = ((*b++) + half) / max_weight_divided_by_255;
      int c1 = ((*b++) + half) / max_weight_divided_by_255;
      int c2 = ((*b++) + half) / max_weight_divided_by_255;
      int c3 = ((*b++) + half) / max_weight_divided_by_255;
      new_row[x] = (((((c0 << 8) | c1) << 8) | c2) << 8) | c3;
    }
  }

  delete buffer;
  return true;
}

void GearsCanvas::Resize(JsCallContext *context) {
  int new_width, new_height;
  std::string16 filter_as_string;
  ResizeFilter filter = FILTER_NICEST;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &new_width },
    { JSPARAM_REQUIRED, JSPARAM_INT, &new_height },
    { JSPARAM_OPTIONAL, JSPARAM_STRING16, &filter_as_string }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set() ||
      !ValidateWidthAndHeight(new_width, new_height, context))
    return;
  if (context->GetArgumentCount() == 3) {
    if (filter_as_string == STRING16(L"fastest")) {
      filter = FILTER_FASTEST;
    } else if (filter_as_string == STRING16(L"nearest")) {
      filter = FILTER_NEAREST;
    } else if (filter_as_string == STRING16(L"bilinear")) {
      filter = FILTER_BILINEAR;
    } else if (filter_as_string != STRING16(L"nicest")) {
      context->SetException(STRING16(
          L"Filter must be 'nicest', 'bilinear', 'nearest' or 'fastest'."));
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
    if (filter != FILTER_NICEST ||
        !HighestQualityMinify(*skia_bitmap_, new_bitmap)) {
      SkCanvas new_canvas(new_bitmap);
      SkScalar x_scale = SkDoubleToScalar(
          static_cast<double>(new_width) / old_width);
      SkScalar y_scale = SkDoubleToScalar(
          static_cast<double>(new_height) / old_height);
      if (!new_canvas.scale(x_scale, y_scale)) {
        context->SetException(STRING16(L"Could not resize the image."));
        return;
      }
      if (filter == FILTER_NEAREST || filter == FILTER_FASTEST) {
        new_canvas.drawBitmap(
            *skia_bitmap_, SkIntToScalar(0), SkIntToScalar(0));
      } else {
        SkPaint paint;
        paint.setFilterBitmap(true);
        new_canvas.drawBitmap(
            *skia_bitmap_, SkIntToScalar(0), SkIntToScalar(0), &paint);
      }
    }
  }
  new_bitmap.swap(*skia_bitmap_);
}

void GearsCanvas::RotateCW(JsCallContext *context) {
  Rotate(1);
}

void GearsCanvas::Rotate180(JsCallContext *context) {
  Rotate(2);
}

void GearsCanvas::RotateCCW(JsCallContext *context) {
  Rotate(3);
}

void GearsCanvas::Rotate(int clockwiseTurns) {
  int new_width = clockwiseTurns == 2 ? GetWidth() : GetHeight();
  int new_height = clockwiseTurns == 2 ? GetHeight() : GetWidth();

  EnsureBitmapPixelsAreAllocated();
  SkBitmap new_bitmap;
  new_bitmap.setConfig(skia_config, new_width, new_height);
  new_bitmap.allocPixels();

  SkCanvas new_canvas(new_bitmap);
  switch (clockwiseTurns) {
    case 1:
      new_canvas.rotate(SkIntToScalar(90));
      new_canvas.drawBitmap(*skia_bitmap_,
          SkIntToScalar(0), SkIntToScalar(-new_width));
      break;
    case 2:
      new_canvas.rotate(SkIntToScalar(180));
      new_canvas.drawBitmap(*skia_bitmap_,
          SkIntToScalar(-new_width), SkIntToScalar(-new_height));
      break;
    case 3:
      new_canvas.rotate(SkIntToScalar(270));
      new_canvas.drawBitmap(*skia_bitmap_,
          SkIntToScalar(-new_height), SkIntToScalar(0));
      break;
  }
  new_bitmap.swap(*skia_bitmap_);
}

void GearsCanvas::FlipHorizontal(JsCallContext *context) {
  EnsureBitmapPixelsAreAllocated();
  SkBitmap new_bitmap;
  new_bitmap.setConfig(skia_config, GetWidth(), GetHeight());
  new_bitmap.allocPixels();
  SkCanvas new_canvas(new_bitmap);
  new_canvas.scale(SkIntToScalar(-1), SkIntToScalar(1));
  new_canvas.drawBitmap(*skia_bitmap_,
      SkIntToScalar(-GetWidth()), SkIntToScalar(0));
  new_bitmap.swap(*skia_bitmap_);
}

void GearsCanvas::FlipVertical(JsCallContext *context) {
  EnsureBitmapPixelsAreAllocated();
  SkBitmap new_bitmap;
  new_bitmap.setConfig(skia_config, GetWidth(), GetHeight());
  new_bitmap.allocPixels();
  SkCanvas new_canvas(new_bitmap);
  new_canvas.scale(SkIntToScalar(1), SkIntToScalar(-1));
  new_canvas.drawBitmap(*skia_bitmap_,
      SkIntToScalar(0), SkIntToScalar(-GetHeight()));
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
