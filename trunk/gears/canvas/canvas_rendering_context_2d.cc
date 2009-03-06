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

#include "gears/canvas/canvas_rendering_context_2d.h"

#include "gears/base/common/js_runner.h"
#include "third_party/skia/include/core/SkPorterDuff.h"
#include "third_party/skia/include/utils/SkParse.h"

#ifndef M_PI
// MS Visual Studio doesn't seem to #define M_PI, so we do it here.
#define M_PI 3.14159265358979323846
#endif

// TODO(nigeltao): For any JavaScript-visible method, we should do sanity
// checking on the args (e.g. ensure that we're not scaling by 1e100), and
// also check the return value for overflow (if applicable) and handle that
// gracefully. I think that WebKit's Skia-backed canvas already does this.

DECLARE_DISPATCHER(GearsCanvasRenderingContext2D);
const std::string
    GearsCanvasRenderingContext2D::kModuleName("GearsCanvasRenderingContext2D");

using canvas::skia_config;

GearsCanvasRenderingContext2D::GearsCanvasRenderingContext2D()
    : ModuleImplBaseClass(kModuleName),
      fill_style_as_string_(STRING16(L"#000000")),
      stroke_style_as_string_(STRING16(L"#000000")) {
  clear_style_as_paint_.setAntiAlias(true);
  clear_style_as_paint_.setPorterDuffXfermode(SkPorterDuff::kClear_Mode);
  fill_style_as_paint_.setAntiAlias(true);
  stroke_style_as_paint_.setAntiAlias(true);
  stroke_style_as_paint_.setStyle(SkPaint::kStroke_Style);
}

void GearsCanvasRenderingContext2D::SetCanvas(
    GearsCanvas *canvas,
    SkBitmap *bitmap) {
  assert(gears_canvas_ == NULL || gears_canvas_ == canvas);
  gears_canvas_ = canvas;
  skia_canvas_.reset(new SkCanvas(*bitmap));
  if (unload_monitor_.get() == NULL) {
    unload_monitor_.reset(
        new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD, this));
  }
}

void GearsCanvasRenderingContext2D::ClearReferenceFromGearsCanvas() {
  // The canvas pointer will be null if this object didn't initalize in the
  // first place (that is, if InitBaseFromSibling failed), or after a page
  // unload event.
  if (gears_canvas_)
    gears_canvas_->ClearRenderingContextReference();
}

void GearsCanvasRenderingContext2D::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
  // On page unload, we must destroy the scoped_refptr to GearsCanvas.
  // But to do that, we must first clear the pointer from GearsCanvas, so that
  // it doesn't become a dangling pointer. If we don't clear it now, we can't
  // clear it in the destructor since at that time we no longer have a pointer
  // to Canvas.
  ClearReferenceFromGearsCanvas();
  gears_canvas_.reset();
}

GearsCanvasRenderingContext2D::~GearsCanvasRenderingContext2D() {
  ClearReferenceFromGearsCanvas();
}

template<>
void Dispatcher<GearsCanvasRenderingContext2D>::Init() {
  // The section headings here (e.g., "state", "transformations") are copied
  // from the HTML5 canvas spec: http://www.whatwg.org/specs/web-apps/
  //     current-work/multipage/the-canvas-element.html

  // back-reference to the canvas
  RegisterProperty("canvas", &GearsCanvasRenderingContext2D::GetCanvas, NULL);

  // state
  RegisterMethod("save", &GearsCanvasRenderingContext2D::Save);
  RegisterMethod("restore", &GearsCanvasRenderingContext2D::Restore);

  // transformations
  RegisterMethod("scale", &GearsCanvasRenderingContext2D::Scale);
  RegisterMethod("rotate", &GearsCanvasRenderingContext2D::Rotate);
  RegisterMethod("translate", &GearsCanvasRenderingContext2D::Translate);
  RegisterMethod("transform", &GearsCanvasRenderingContext2D::Transform);
  RegisterMethod("setTransform", &GearsCanvasRenderingContext2D::SetTransform);

  // compositing
  RegisterProperty("globalAlpha",
      &GearsCanvasRenderingContext2D::GetGlobalAlpha,
      &GearsCanvasRenderingContext2D::SetGlobalAlpha);
  RegisterProperty("globalCompositeOperation",
      &GearsCanvasRenderingContext2D::GetGlobalCompositeOperation,
      &GearsCanvasRenderingContext2D::SetGlobalCompositeOperation);

  // colors and styles
  RegisterProperty("strokeStyle",
      &GearsCanvasRenderingContext2D::GetStrokeStyle,
      &GearsCanvasRenderingContext2D::SetStrokeStyle);
  RegisterProperty("fillStyle",
      &GearsCanvasRenderingContext2D::GetFillStyle,
      &GearsCanvasRenderingContext2D::SetFillStyle);
  // Missing wrt HTML5: createLinearGradient.
  // Missing wrt HTML5: createRadialGradient.
  // Missing wrt HTML5: createPattern.

  // line caps/joins
  RegisterProperty("lineWidth",
      &GearsCanvasRenderingContext2D::GetLineWidth,
      &GearsCanvasRenderingContext2D::SetLineWidth);
  RegisterProperty("lineCap",
      &GearsCanvasRenderingContext2D::GetLineCap,
      &GearsCanvasRenderingContext2D::SetLineCap);
  RegisterProperty("lineJoin",
      &GearsCanvasRenderingContext2D::GetLineJoin,
      &GearsCanvasRenderingContext2D::SetLineJoin);
  RegisterProperty("miterLimit",
      &GearsCanvasRenderingContext2D::GetMiterLimit,
      &GearsCanvasRenderingContext2D::SetMiterLimit);

  // shadows
  // Missing wrt HTML5: shadowOffsetX.
  // Missing wrt HTML5: shadowOffsetY.
  // Missing wrt HTML5: shadowBlur.
  // Missing wrt HTML5: shadowColor.

  // rects
  RegisterMethod("clearRect", &GearsCanvasRenderingContext2D::ClearRect);
  RegisterMethod("fillRect", &GearsCanvasRenderingContext2D::FillRect);
  RegisterMethod("strokeRect", &GearsCanvasRenderingContext2D::StrokeRect);

  // path API
  // Missing wrt HTML5: beginPath.
  // Missing wrt HTML5: closePath.
  // Missing wrt HTML5: moveTo.
  // Missing wrt HTML5: lineTo.
  // Missing wrt HTML5: quadraticCurveTo.
  // Missing wrt HTML5: bezierCurveTo.
  // Missing wrt HTML5: arcTo.
  // Missing wrt HTML5: rect.
  // Missing wrt HTML5: arc.
  // Missing wrt HTML5: fill.
  // Missing wrt HTML5: stroke.
  // Missing wrt HTML5: clip.
  // Missing wrt HTML5: isPointInPath.

  // text
  // Missing wrt HTML5: font.
  // Missing wrt HTML5: textAlign.
  // Missing wrt HTML5: textBaseline.
  // Missing wrt HTML5: fillText.
  // Missing wrt HTML5: strokeText.
  // Missing wrt HTML5: measureText.

  // drawing images
  RegisterMethod("drawImage", &GearsCanvasRenderingContext2D::DrawImage);

  // pixel manipulation
  RegisterMethod("createImageData",
      &GearsCanvasRenderingContext2D::CreateImageData);
  RegisterMethod("getImageData", &GearsCanvasRenderingContext2D::GetImageData);
  RegisterMethod("putImageData", &GearsCanvasRenderingContext2D::PutImageData);
}

// TODO(nigeltao): Unless otherwise stated, for the 2D context interface, any
// method call with a numeric argument whose value is infinite or
// a NaN value must be ignored.

void GearsCanvasRenderingContext2D::GetCanvas(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_MODULE, gears_canvas_.get());
}

void GearsCanvasRenderingContext2D::Save(JsCallContext *context) {
  skia_canvas_->save();
}

void GearsCanvasRenderingContext2D::Restore(JsCallContext *context) {
  // SkCanvas silently ignores any restore() calls over and above the number of
  // save() calls.
  skia_canvas_->restore();
}

void GearsCanvasRenderingContext2D::Scale(JsCallContext *context) {
  double sx, sy;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &sx },
    { JSPARAM_OPTIONAL, JSPARAM_DOUBLE, &sy }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;

  if (context->GetArgumentCount() == 1) {
    // If only one scale argument is supplied, use it for both dimensions.
    sy = sx;
  }
  skia_canvas_->scale(SkDoubleToScalar(sx), SkDoubleToScalar(sy));
}

void GearsCanvasRenderingContext2D::Rotate(JsCallContext *context) {
  double angle;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &angle },
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;

  skia_canvas_->rotate(SkDoubleToScalar(angle * 180.0 / M_PI));
}

void GearsCanvasRenderingContext2D::Translate(JsCallContext *context) {
  double tx, ty;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &tx },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &ty }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;

  skia_canvas_->translate(SkDoubleToScalar(tx), SkDoubleToScalar(ty));
}

void GearsCanvasRenderingContext2D::Transform(
    JsCallContext *context,
    bool reset_matrix) {
  double m11, m12, m21, m22, dx, dy;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m11 },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m12 },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m21 },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m22 },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &dx },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &dy }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;

  SkMatrix m;
  m.reset();
  m.set(0, SkDoubleToScalar(m11));
  m.set(1, SkDoubleToScalar(m21));
  m.set(2, SkDoubleToScalar(dx));
  m.set(3, SkDoubleToScalar(m12));
  m.set(4, SkDoubleToScalar(m22));
  m.set(5, SkDoubleToScalar(dy));
  if (reset_matrix) {
    skia_canvas_->setMatrix(m);
  } else {
    skia_canvas_->concat(m);
  }
}

void GearsCanvasRenderingContext2D::Transform(JsCallContext *context) {
  Transform(context, false);
}

void GearsCanvasRenderingContext2D::SetTransform(JsCallContext *context) {
  Transform(context, true);
}

void GearsCanvasRenderingContext2D::GetGlobalAlpha(
    JsCallContext *context) {
  double alpha = gears_canvas_->alpha();
  context->SetReturnValue(JSPARAM_DOUBLE, &alpha);
}

void GearsCanvasRenderingContext2D::SetGlobalAlpha(
    JsCallContext *context) {
  double new_alpha;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &new_alpha }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  gears_canvas_->set_alpha(new_alpha);
}

void GearsCanvasRenderingContext2D::GetGlobalCompositeOperation(
    JsCallContext *context) {
  std::string16 op = gears_canvas_->composite_operation();
  context->SetReturnValue(JSPARAM_STRING16, &op);
}

void GearsCanvasRenderingContext2D::SetGlobalCompositeOperation(
    JsCallContext *context) {
  std::string16 new_composite_op;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &new_composite_op }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  if (!gears_canvas_->set_composite_operation(new_composite_op)) {
    // HTML5 canvas-only composite operation.
    context->SetException(
        STRING16(L"This composite operation is implemented only in HTML5 canvas"
                 L" and not in Gears' canvas."));
    return;
  }
}

void GearsCanvasRenderingContext2D::SetStyle(
    JsCallContext *context,
    std::string16 *style_as_string,
    SkPaint *style_as_paint) {
  std::string16 new_style_as_string;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &new_style_as_string }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  // TODO(nigeltao): Do not generate error on type mismatch; fail silently
  // (as per HTML5 canvas spec).
  if (context->is_exception_set())
    return;

  std::string new_style_as_utf8;
  if (!String16ToUTF8(new_style_as_string, &new_style_as_utf8)) {
    return;
  }

  // SkParse::FindColor re-uses color's existing alpha value, if unspecified
  // by the string, so we initialize color with alpha=255.
  SkColor color = 0xFFFFFFFF;
  if (SkParse::FindColor(new_style_as_utf8.c_str(), &color)) {
    style_as_paint->setColor(color);
    *style_as_string = new_style_as_string;
  }
}

void GearsCanvasRenderingContext2D::GetStrokeStyle(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16, &stroke_style_as_string_);
}

void GearsCanvasRenderingContext2D::SetStrokeStyle(JsCallContext *context) {
  SetStyle(context, &stroke_style_as_string_, &stroke_style_as_paint_);
}

void GearsCanvasRenderingContext2D::GetFillStyle(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16, &fill_style_as_string_);
}

void GearsCanvasRenderingContext2D::SetFillStyle(JsCallContext *context) {
  SetStyle(context, &fill_style_as_string_, &fill_style_as_paint_);
}

void GearsCanvasRenderingContext2D::GetLineWidth(JsCallContext *context) {
  double line_width = SkScalarToDouble(
      stroke_style_as_paint_.getStrokeWidth());
  context->SetReturnValue(JSPARAM_DOUBLE, &line_width);
}

void GearsCanvasRenderingContext2D::SetLineWidth(JsCallContext *context) {
  double width;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &width }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;

  stroke_style_as_paint_.setStrokeWidth(SkDoubleToScalar(width));
}

void GearsCanvasRenderingContext2D::GetLineCap(JsCallContext *context) {
  switch (stroke_style_as_paint_.getStrokeCap()) {
    case SkPaint::kButt_Cap: {
      std::string16 result(STRING16(L"butt"));
      context->SetReturnValue(JSPARAM_STRING16, &result);
      break;
    }
    case SkPaint::kRound_Cap: {
      std::string16 result(STRING16(L"round"));
      context->SetReturnValue(JSPARAM_STRING16, &result);
      break;
    }
    case SkPaint::kSquare_Cap: {
      std::string16 result(STRING16(L"square"));
      context->SetReturnValue(JSPARAM_STRING16, &result);
      break;
    }
    default:
      // Do nothing.
      break;
  }
}

void GearsCanvasRenderingContext2D::SetLineCap(JsCallContext *context) {
  std::string16 s;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &s }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;

  if (s == STRING16(L"butt")) {
    stroke_style_as_paint_.setStrokeCap(SkPaint::kButt_Cap);
  } else if (s == STRING16(L"round")) {
    stroke_style_as_paint_.setStrokeCap(SkPaint::kRound_Cap);
  } else if (s == STRING16(L"square")) {
    stroke_style_as_paint_.setStrokeCap(SkPaint::kSquare_Cap);
  }
}

void GearsCanvasRenderingContext2D::GetLineJoin(JsCallContext *context) {
  switch (stroke_style_as_paint_.getStrokeJoin()) {
    case SkPaint::kMiter_Join: {
      std::string16 result(STRING16(L"miter"));
      context->SetReturnValue(JSPARAM_STRING16, &result);
      break;
    }
    case SkPaint::kRound_Join: {
      std::string16 result(STRING16(L"round"));
      context->SetReturnValue(JSPARAM_STRING16, &result);
      break;
    }
    case SkPaint::kBevel_Join: {
      std::string16 result(STRING16(L"bevel"));
      context->SetReturnValue(JSPARAM_STRING16, &result);
      break;
    }
    default:
      // Do nothing.
      break;
  }
}

void GearsCanvasRenderingContext2D::SetLineJoin(JsCallContext *context) {
  std::string16 s;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_STRING16, &s }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;

  if (s == STRING16(L"miter")) {
    stroke_style_as_paint_.setStrokeJoin(SkPaint::kMiter_Join);
  } else if (s == STRING16(L"round")) {
    stroke_style_as_paint_.setStrokeJoin(SkPaint::kRound_Join);
  } else if (s == STRING16(L"bevel")) {
    stroke_style_as_paint_.setStrokeJoin(SkPaint::kBevel_Join);
  }
}

void GearsCanvasRenderingContext2D::GetMiterLimit(JsCallContext *context) {
  double miter_limit = SkScalarToDouble(
      stroke_style_as_paint_.getStrokeMiter());
  context->SetReturnValue(JSPARAM_DOUBLE, &miter_limit);
}

void GearsCanvasRenderingContext2D::SetMiterLimit(JsCallContext *context) {
  double miter_limit;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &miter_limit }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;

  stroke_style_as_paint_.setStrokeMiter(SkDoubleToScalar(miter_limit));
}

void GearsCanvasRenderingContext2D::PaintRect(
    JsCallContext *context,
    SkPaint *paint) {
  double x, y, w, h;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &x },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &y },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &w },
    { JSPARAM_REQUIRED, JSPARAM_DOUBLE, &h }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;

  SkRect rect;
  rect.set(SkDoubleToScalar(x), SkDoubleToScalar(y),
      SkDoubleToScalar(x + w), SkDoubleToScalar(y + h));
  skia_canvas_->drawRect(rect, *paint);
}

void GearsCanvasRenderingContext2D::ClearRect(JsCallContext *context) {
  PaintRect(context, &clear_style_as_paint_);
}

void GearsCanvasRenderingContext2D::FillRect(JsCallContext *context) {
  PaintRect(context, &fill_style_as_paint_);
}

void GearsCanvasRenderingContext2D::StrokeRect(JsCallContext *context) {
  PaintRect(context, &stroke_style_as_paint_);
}

void GearsCanvasRenderingContext2D::DrawImage(JsCallContext *context) {
  // TODO(nigeltao): This function has a bug that doesn't make it work after
  // calling resize() on the target canvas. That is, if you resize() a canvas
  // and then draw another canvas on it, the drawImage() is a noop.
  // Also this function doesn't properly respect globalAlpha and
  // globalCompositeOperation.
  // Return an error code to prevent clients from getting bitten by these bugs.
  context->SetException(STRING16(L"Unimplemented"));
  // Some of the functionality of this function can be achieved using
  // crop(), resize() and clone().

  // TODO(nigeltao): Generate a better error message if given a HTMLImageElement
  // or a HTMLCanvasElement.
  
  /*
  ModuleImplBaseClass *other_module;
  int source_x, source_y, source_width, source_height;
  int dest_x, dest_y, dest_width, dest_height;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_MODULE, &other_module },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &source_x },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &source_y },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &source_width },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &source_height },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &dest_x },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &dest_y },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &dest_width },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &dest_height }
  };
  if (!context->GetArguments(ARRAYSIZE(args), args)) {
    assert(context->is_exception_set());
    return;
  }
  assert(other_module);
  if (GearsCanvas::kModuleName != other_module->get_module_name()) {
    context->SetException(STRING16(L"Argument must be a Canvas."));
    return;
  }
  scoped_refptr<GearsCanvas> src = static_cast<GearsCanvas*>(other_module);
  
  bool optional_source_pos_arguments_present = args[1].was_specified &&
                                               args[2].was_specified;
  bool optional_source_size_arguments_present = args[3].was_specified &&
                                                args[4].was_specified;
  bool optional_dest_arguments_present = args[5].was_specified &&
                                         args[6].was_specified &&
                                         args[7].was_specified &&
                                         args[8].was_specified;
  if (!(optional_source_pos_arguments_present &&
        optional_source_size_arguments_present &&
        optional_dest_arguments_present)) {
    // Handle missing arguments.
    if (optional_source_pos_arguments_present &&
        optional_source_size_arguments_present) {
      dest_width = source_width;
      dest_height = source_height;
    } else if (optional_source_pos_arguments_present) {
      dest_width = src->width();
      dest_height = src->height();
    } else {
      context->SetException(STRING16(L"Unsupported number of arguments."));
      return;
    }
    source_width = src->width();
    source_height = src->height();
    dest_x = source_x;
    dest_y = source_y;
    source_x = 0;
    source_y = 0;
  }
  
  SkIRect src_irect = {
      source_x, source_y, source_x + source_width, source_y + source_height };
  SkIRect dest_irect = {
      dest_x, dest_y, dest_x + dest_width, dest_y + dest_height };

  // The HTML5 canvas spec says that an invalid src rect must trigger an
  // exception, but it does not say what to do if the dest rect is invalid.
  // So, if both rects are invalid, it's more spec-compliant to raise an error
  // for the src rect.
  if (!src->IsRectValid(src_irect)) {
    context->SetException(STRING16(
        L"INDEX_SIZE_ERR: Source rectangle stretches beyond the bounds"
        L"of its bitmap or has negative dimensions."));
    return;
  }
  if (src_irect.isEmpty()) {
    context->SetException(STRING16(
        L"INDEX_SIZE_ERR: Source rectangle has zero width or height."));
    return;
  }
  if (!canvas_->IsRectValid(dest_irect)) {
    context->SetException(STRING16(
        L"INDEX_SIZE_ERR: Destination rectangle stretches beyond the bounds"
        L"of its bitmap or has negative dimensions."));
    return;
  }
  if (dest_irect.isEmpty()) {
    context->SetException(STRING16(
        L"INDEX_SIZE_ERR: Destination rectangle has zero width or height."));
    return;
  }
  
  // TODO(nigeltao): First apply geometry xform, then alpha, then compositing,
  // as per HTML5 canvas spec.

  // If the globalAlpha is not 1.0, the source image must be multiplied by
  // globalAlpha *before* compositing.
  SkBitmap *modified_src = src->skia_bitmap();
  scoped_ptr<SkBitmap> bitmap_to_delete;

  if (canvas_->alpha() != 1.0) {
    bitmap_to_delete.reset(new SkBitmap);
    modified_src = bitmap_to_delete.get();
    modified_src->setConfig(skia_config, src->width(), src->height());
    modified_src->allocPixels();
    SkCanvas temp_canvas(*modified_src);
    SkPaint temp_paint;
    temp_paint.setAlpha(static_cast<U8CPU>(canvas_->alpha() * 255));
    temp_canvas.drawBitmap(
        *modified_src, SkIntToScalar(0), SkIntToScalar(0), &temp_paint);
  }

  SkPaint paint;
  SkPorterDuff::Mode porter_duff = static_cast<SkPorterDuff::Mode>(
      canvas_->ParseCompositeOperationString(canvas_->composite_operation()));
  paint.setPorterDuffXfermode(porter_duff);


  SkRect dest_rect;
  dest_rect.set(dest_irect);

  // When drawBitmapRect() is called with a source rectangle, it (also) handles
  // the case where source canvas is same as this canvas.
  // TODO(nigeltao): This function silently fails on errors. Find out what can
  // be done about this.
  canvas_->skia_canvas()->drawBitmapRect(
      *modified_src, &src_irect, dest_rect);
  */
  
  // TODO(nigeltao): Are the colors premultiplied? If so, unpremultiply them.
}

void GearsCanvasRenderingContext2D::CreateImageData(JsCallContext *context) {
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

void GearsCanvasRenderingContext2D::GetImageData(JsCallContext *context) {
  int sx, sy, sw, sh;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_INT, &sx },
    { JSPARAM_REQUIRED, JSPARAM_INT, &sy },
    { JSPARAM_REQUIRED, JSPARAM_INT, &sw },
    { JSPARAM_REQUIRED, JSPARAM_INT, &sh }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::PutImageData(JsCallContext *context) {
  scoped_ptr<JsObject> image_data;
  int dx, dy, dirty_x, dirty_y, dirty_width, dirty_height;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_OBJECT, as_out_parameter(image_data) },
    { JSPARAM_REQUIRED, JSPARAM_INT, &dx },
    { JSPARAM_REQUIRED, JSPARAM_INT, &dy },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &dirty_x },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &dirty_y },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &dirty_width },
    { JSPARAM_OPTIONAL, JSPARAM_INT, &dirty_height }
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}
