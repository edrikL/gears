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

#ifndef OFFICIAL_BUILD

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/canvas/canvas_rendering_context_2d.h"

DECLARE_GEARS_WRAPPER(GearsCanvasRenderingContext2D);
const std::string
    GearsCanvasRenderingContext2D::kModuleName("GearsCanvasRenderingContext2D");
const std::string16 GearsCanvasRenderingContext2D::kCompositeOpSourceOver(
    STRING16(L"source-over"));
const std::string16 GearsCanvasRenderingContext2D::kCompositeOpCopy(
    STRING16(L"copy"));  // the right values as per the canvas spec.
const std::string16 GearsCanvasRenderingContext2D::kTextAlignLeft(
    STRING16(L"left"));
const std::string16 GearsCanvasRenderingContext2D::kTextAlignCenter(
    STRING16(L"center"));
const std::string16 GearsCanvasRenderingContext2D::kTextAlignRight(
    STRING16(L"right"));

GearsCanvasRenderingContext2D::GearsCanvasRenderingContext2D()
    : ModuleImplBaseClassVirtual(kModuleName),
    canvas_(NULL),
    alpha_(1.0),
    composite_op_(kCompositeOpSourceOver),
    fill_style_(STRING16(L"#000000")),
    font_(STRING16(L"10px sans-serif")),
    text_align_(STRING16(L"left")) {
}

template<>
void Dispatcher<GearsCanvasRenderingContext2D>::Init() {
  RegisterProperty("canvas", &GearsCanvasRenderingContext2D::GetCanvas, NULL);
  RegisterMethod("save", &GearsCanvasRenderingContext2D::Save);
  RegisterMethod("restore", &GearsCanvasRenderingContext2D::Restore);
  RegisterMethod("scale", &GearsCanvasRenderingContext2D::Scale);
  RegisterMethod("rotate", &GearsCanvasRenderingContext2D::Rotate);
  RegisterMethod("translate", &GearsCanvasRenderingContext2D::Translate);
  RegisterMethod("transform", &GearsCanvasRenderingContext2D::Transform);
  RegisterMethod("setTransform", &GearsCanvasRenderingContext2D::SetTransform);
  RegisterProperty("globalAlpha",
      &GearsCanvasRenderingContext2D::GetGlobalAlpha,
      &GearsCanvasRenderingContext2D::SetGlobalAlpha);
  RegisterProperty("globalCompositeOperation",
      &GearsCanvasRenderingContext2D::GetGlobalCompositeOperation,
      &GearsCanvasRenderingContext2D::SetGlobalCompositeOperation);
  RegisterProperty("fillStyle",
      &GearsCanvasRenderingContext2D::GetFillStyle,
      &GearsCanvasRenderingContext2D::SetFillStyle);
  RegisterMethod("clearRect", &GearsCanvasRenderingContext2D::ClearRect);
  RegisterMethod("fillRect", &GearsCanvasRenderingContext2D::FillRect);
  RegisterMethod("strokeRect", &GearsCanvasRenderingContext2D::StrokeRect);
  RegisterProperty("font", &GearsCanvasRenderingContext2D::GetFont,
      &GearsCanvasRenderingContext2D::SetFont);
  RegisterProperty("textAlign", &GearsCanvasRenderingContext2D::GetTextAlign,
      &GearsCanvasRenderingContext2D::SetTextAlign);
  RegisterMethod("fillText", &GearsCanvasRenderingContext2D::FillText);
  RegisterMethod("measureText", &GearsCanvasRenderingContext2D::MeasureText);
  RegisterMethod("drawImage", &GearsCanvasRenderingContext2D::DrawImage);
  RegisterMethod("createImageData",
      &GearsCanvasRenderingContext2D::CreateImageData);
  RegisterMethod("getImageData", &GearsCanvasRenderingContext2D::GetImageData);
  RegisterMethod("putImageData", &GearsCanvasRenderingContext2D::PutImageData);
  RegisterMethod("colorTransform",
      &GearsCanvasRenderingContext2D::ColorTransform);
  RegisterMethod("convolutionTransform",
      &GearsCanvasRenderingContext2D::ConvolutionTransform);
  RegisterMethod("medianFilter", &GearsCanvasRenderingContext2D::MedianFilter);
  RegisterMethod("drawImageData",
      &GearsCanvasRenderingContext2D::DrawImageData);
  RegisterMethod("adjustBrightness",
      &GearsCanvasRenderingContext2D::AdjustBrightness);
  RegisterMethod("adjustContrast",
      &GearsCanvasRenderingContext2D::AdjustContrast);
  RegisterMethod("adjustSaturation",
      &GearsCanvasRenderingContext2D::AdjustSaturation);
  RegisterMethod("adjustHue",
      &GearsCanvasRenderingContext2D::AdjustHue);
  RegisterMethod("blur", &GearsCanvasRenderingContext2D::Blur);
  RegisterMethod("sharpen", &GearsCanvasRenderingContext2D::Sharpen);
  RegisterMethod("resetTransform",
      &GearsCanvasRenderingContext2D::ResetTransform);
}

// TODO(kart): Unless otherwise stated, for the 2D context interface, any
// method call with a numeric argument whose value is infinite or
// a NaN value must be ignored.

void GearsCanvasRenderingContext2D::InitCanvasField(GearsCanvas *new_canvas) {
  assert(canvas_ == NULL);
  assert(new_canvas != NULL);
  canvas_ = new_canvas;
}

void GearsCanvasRenderingContext2D::GetCanvas(JsCallContext *context) {
  assert(canvas_ != NULL);
  context->SetReturnValue(JSPARAM_DISPATCHER_MODULE, canvas_);
}

void GearsCanvasRenderingContext2D::Save(JsCallContext *context) {
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::Restore(JsCallContext *context) {
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::Scale(JsCallContext *context) {
  double x, y;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &x},
    {JSPARAM_OPTIONAL, JSPARAM_DOUBLE, &y},
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  if (context->GetArgumentType(1) == JSPARAM_UNDEFINED) {
    y = x;
    // TODO(kart): test this.
  }
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::Rotate(JsCallContext *context) {
  double angle;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &angle},
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::Translate(JsCallContext *context) {
  int x, y;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &x},
    {JSPARAM_REQUIRED, JSPARAM_INT, &y},
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::Transform(JsCallContext *context) {
  double m11, m12, m21, m22, dx, dy;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m11},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m12},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m21},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m22},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &dx},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &dy},
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::SetTransform(JsCallContext *context) {
  double m11, m12, m21, m22, dx, dy;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m11},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m12},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m21},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &m22},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &dx},
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &dy},
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::GetGlobalAlpha(
    JsCallContext *context) {
  context->SetReturnValue(JSPARAM_DOUBLE, &alpha_);
}

void GearsCanvasRenderingContext2D::SetGlobalAlpha(
    JsCallContext *context) {
  double new_alpha;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &new_alpha}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  if (new_alpha < 0.0 || new_alpha > 1.0) {
    return;  // as per canvas spec.
  }
  alpha_ = new_alpha;
}

void GearsCanvasRenderingContext2D::GetGlobalCompositeOperation(
    JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16, &composite_op_);
}

void GearsCanvasRenderingContext2D::SetGlobalCompositeOperation(
    JsCallContext *context) {
  std::string16 new_composite_op;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_STRING16, &new_composite_op}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  if (new_composite_op != kCompositeOpCopy &&
      new_composite_op != kCompositeOpCopy)
    return;  // as per canvas spec.

  // TODO(kart): if we're given a composite mode that Canvas supports but we
  // don't, raise an Unsupported exception.
  composite_op_ = new_composite_op;
}

void GearsCanvasRenderingContext2D::GetFillStyle(
    JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16, &fill_style_);
}

void GearsCanvasRenderingContext2D::SetFillStyle(
    JsCallContext *context) {
  std::string16 new_fill_style;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_STRING16, &new_fill_style}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  // TODO(kart): do not generate error on type mismatch.
  if (context->is_exception_set())
    return;

  // TODO(kart):
  // if (new_fill_style is not a valid CSS color)
  //  return; // as per canvas spec.
  fill_style_ = new_fill_style;
}

void GearsCanvasRenderingContext2D::ClearRect(JsCallContext *context) {
  int x, y, width, height;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &x},
    {JSPARAM_REQUIRED, JSPARAM_INT, &y},
    {JSPARAM_REQUIRED, JSPARAM_INT, &width},
    {JSPARAM_REQUIRED, JSPARAM_INT, &height}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::FillRect(JsCallContext *context) {
  int x, y, width, height;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &x},
    {JSPARAM_REQUIRED, JSPARAM_INT, &y},
    {JSPARAM_REQUIRED, JSPARAM_INT, &width},
    {JSPARAM_REQUIRED, JSPARAM_INT, &height}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::StrokeRect(JsCallContext *context) {
  int x, y, width, height;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &x},
    {JSPARAM_REQUIRED, JSPARAM_INT, &y},
    {JSPARAM_REQUIRED, JSPARAM_INT, &width},
    {JSPARAM_REQUIRED, JSPARAM_INT, &height}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::GetFont(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16, &font_);
}

void GearsCanvasRenderingContext2D::SetFont(JsCallContext *context) {
  std::string16 new_font;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_STRING16, &new_font}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  // TODO(kart):
  // if (new_font is not a valid CSS font specification) {
  //   context->SetException("Bad font specification");
  //   return;
  // }
  font_ = new_font;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::GetTextAlign(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16, &text_align_);
}

void GearsCanvasRenderingContext2D::SetTextAlign(JsCallContext *context) {
  std::string16 new_align;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_STRING16, &new_align}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  if (new_align != kTextAlignLeft && new_align != kTextAlignCenter
      && new_align != kTextAlignRight) {
    context->SetException(STRING16(L"Unrecognized text alignment"));
    return;
  }
  text_align_ = new_align;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::FillText(JsCallContext *context) {
  std::string16 text;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_STRING16, &text}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::MeasureText(JsCallContext *context) {
  std::string16 text;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_STRING16, &text}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::DrawImage(JsCallContext *context) {
  JsObject image;
  int sx, sy, sw, sh, dx, dy, dw, dh;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_OBJECT, &image},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &sx},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &sy},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &sw},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &sh},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &dx},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &dy},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &dw},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &dh}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  // TODO(kart): make sure that if only 2 or 4 args are given,
  // they are the dest args.
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::CreateImageData(JsCallContext *context) {
  int width, height;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &width},
    {JSPARAM_REQUIRED, JSPARAM_INT, &height}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::GetImageData(JsCallContext *context) {
  int sx, sy, sw, sh;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &sx},
    {JSPARAM_REQUIRED, JSPARAM_INT, &sy},
    {JSPARAM_REQUIRED, JSPARAM_INT, &sw},
    {JSPARAM_REQUIRED, JSPARAM_INT, &sh},
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::PutImageData(JsCallContext *context) {
  JsObject imageData;
  int dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_OBJECT, &imageData},
    {JSPARAM_REQUIRED, JSPARAM_INT, &dx},
    {JSPARAM_REQUIRED, JSPARAM_INT, &dy},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &dirtyX},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &dirtyY},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &dirtyWidth},
    {JSPARAM_OPTIONAL, JSPARAM_INT, &dirtyHeight}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::ColorTransform(JsCallContext *context) {
  JsObject matrix;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_OBJECT, &matrix}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::ConvolutionTransform(
    JsCallContext *context) {
  JsObject matrix;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_OBJECT, &matrix}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::MedianFilter(JsCallContext *context) {
  double radius;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &radius}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::DrawImageData(JsCallContext *context) {
  JsObject image;
  int sx, sy, sw, sh, dx, dy;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_OBJECT, &image},
    {JSPARAM_REQUIRED, JSPARAM_INT, &sx},
    {JSPARAM_REQUIRED, JSPARAM_INT, &sy},
    {JSPARAM_REQUIRED, JSPARAM_INT, &sw},
    {JSPARAM_REQUIRED, JSPARAM_INT, &sh},
    {JSPARAM_REQUIRED, JSPARAM_INT, &dx},
    {JSPARAM_REQUIRED, JSPARAM_INT, &dy}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  // TODO(kart): make sure that if only 2 or 4 args are given,
  // they are the dest args.
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::AdjustBrightness(JsCallContext *context) {
  double delta;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &delta}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::AdjustContrast(JsCallContext *context) {
  double amount;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &amount}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::AdjustSaturation(JsCallContext *context) {
  double amount;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &amount}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::AdjustHue(JsCallContext *context) {
  double angle;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &angle}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::Blur(JsCallContext *context) {
  double factor;
  int radius;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &factor},
    {JSPARAM_REQUIRED, JSPARAM_INT, &radius}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::Sharpen(JsCallContext *context) {
  double factor;
  int radius;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_DOUBLE, &factor},
    {JSPARAM_REQUIRED, JSPARAM_INT, &radius}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvasRenderingContext2D::ResetTransform(JsCallContext *context) {
  context->SetException(STRING16(L"Unimplemented"));
}

#endif  // OFFICIAL_BUILD
