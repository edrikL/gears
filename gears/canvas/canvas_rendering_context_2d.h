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

#ifndef GEARS_CANVAS_CANVAS_RENDERING_CONTEXT_2D_H__
#define GEARS_CANVAS_CANVAS_RENDERING_CONTEXT_2D_H__

#ifndef OFFICIAL_BUILD

#include "gears/base/common/common.h"
#include "gears/canvas/canvas.h"

class GearsCanvasRenderingContext2D : public ModuleImplBaseClassVirtual {
 public:
  static const std::string kModuleName;

  GearsCanvasRenderingContext2D();

  // Initialize the canvas field. Must be called only once.
  // This is not exposed to Javascript and exists only for internal use.
  void InitCanvasField(GearsCanvas *canvas);

  // IN: -
  // OUT: Canvas
  void GetCanvas(JsCallContext *context);

  // IN: -
  // OUT: -
  void Save(JsCallContext *context);

  // IN: -
  // OUT: -
  void Restore(JsCallContext *context);

  // IN: double x, double y
  // OUT: -
  void Scale(JsCallContext *context);

  // IN: double angle
  // OUT: -
  void Rotate(JsCallContext *context);

  // IN: int x, int y
  // OUT: -
  void Translate(JsCallContext *context);

  // IN: float m11, float m12, float m21, float m22, float dx, float dy
  // OUT: -
  void Transform(JsCallContext *context);

  // IN: float m11, float m12, float m21, float m22, float dx, float dy
  // OUT: -
  void SetTransform(JsCallContext *context);

  // IN: -
  // OUT: double
  void GetGlobalAlpha(JsCallContext *context);

  // IN: double
  // OUT: -
  void SetGlobalAlpha(JsCallContext *context);

  // IN: -
  // OUT: int
  void GetGlobalCompositeOperation(JsCallContext *context);

  // IN: int
  // OUT: -
  void SetGlobalCompositeOperation(JsCallContext *context);

  // IN: -
  // OUT: string
  void GetFillStyle(JsCallContext *context);

  // IN: string
  // OUT: -
  void SetFillStyle(JsCallContext *context);

  // IN: int x, int y, int width, int height
  // OUT: -
  void ClearRect(JsCallContext *context);
  void FillRect(JsCallContext *context);
  void StrokeRect(JsCallContext *context);

  void GetFont(JsCallContext *context);
  void SetFont(JsCallContext *context);
  void GetTextAlign(JsCallContext *context);
  void SetTextAlign(JsCallContext *context);

  // IN: string text, float x, float y, optional float maxWidth
  // OUT: -
  void FillText(JsCallContext *context);

  // IN: string text
  // OUT: TextMetrics
  void MeasureText(JsCallContext *context);

  // IN: Canvas image, int sx, int sy, int sw, int sh, int dx, int dy, int dw,
  //     int dh
  // OUT: -
  void DrawImage(JsCallContext *context);

  // IN: int sw, int sh
  // OUT: ImageData
  void CreateImageData(JsCallContext *context);

  // IN: int sx, int sy, int sw, int sh
  // OUT: ImageData
  void GetImageData(JsCallContext *context);

  // Overloaded:
  // IN: ImageData imagedata, int dx, int dy
  // IN: ImageData imagedata, int dx, int dy, int dirtyX, int dirtyY,
  //     int dirtyWidth, int dirtyHeight
  // OUT: -
  void PutImageData(JsCallContext *context);

  // IN: double[] colorMatrix
  // OUT: -
  void ColorTransform(JsCallContext *context);

  // IN: double[] convolutionMatrix, optional float bias,
  //     optional boolean applyToAlpha
  // OUT: -
  void ConvolutionTransform(JsCallContext *context);

  // IN: double radius
  // OUT: -
  void MedianFilter(JsCallContext *context);

  // IN: ImageData imagedata, int sx, int sy, int sw, int sh, int dx, int dy
  // OUT: -
  void DrawImageData(JsCallContext *context);

  // IN: double delta.
  // OUT: -
  void AdjustBrightness(JsCallContext *context);

  // IN: double contrast.
  // OUT: -
  void AdjustContrast(JsCallContext *context);

  // IN: double saturation
  // OUT: -
  void AdjustSaturation(JsCallContext *context);

  // IN: double angle
  // OUT: -
  void AdjustHue(JsCallContext *context);

  // IN: double factor, int radius.
  // OUT: -
  void Blur(JsCallContext *context);

  // IN: double factor, int radius.
  // OUT: -
  void Sharpen(JsCallContext *context);

  // IN: -
  // OUT: -
  void ResetTransform(JsCallContext *context);

 private:
  // TODO(kart): Probably use scoped_refptr, but what about cycle
  // (canvas -> context -> canvas)?
  GearsCanvas *canvas_;

  double alpha_;
  std::string16 composite_op_;
  static const std::string16 kCompositeOpCopy;
  static const std::string16 kCompositeOpSourceOver;
  std::string16 fill_style_;
  std::string16 font_;
  std::string16 text_align_;
  static const std::string16 kTextAlignLeft;
  static const std::string16 kTextAlignRight;
  static const std::string16 kTextAlignCenter;
  DISALLOW_EVIL_CONSTRUCTORS(GearsCanvasRenderingContext2D);
};

#endif  // OFFICIAL_BUILD
#endif  // GEARS_CANVAS_CANVAS_RENDERING_CONTEXT_2D_H__
