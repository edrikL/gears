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

#if !defined(OFFICIAL_BUILD)

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/canvas/canvas.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace canvas {
extern const SkBitmap::Config skia_config;
}

// 2D Context to manipulate the canvas.
// See http://code.google.com/p/google-gears/wiki/CanvasAPI for more detailed
// documentation.
class GearsCanvasRenderingContext2D
    : public ModuleImplBaseClass,
      public JsEventHandlerInterface {
 public:
  static const std::string kModuleName;

  // {Create,Get,Put}ImageData won't return or accept any ImageData that is
  // wider than kMaxImageDataSize, or taller than kMaxImageDataSize.
  static const int kMaxImageDataSize;

  GearsCanvasRenderingContext2D();
  virtual ~GearsCanvasRenderingContext2D();

  // Returns a backreference to the canvas this context was created from.
  // IN: -
  // OUT: Canvas
  void GetCanvas(JsCallContext *context);

  // Pushes the current state on the state stack.
  // IN: -
  // OUT: -
  void Save(JsCallContext *context);

  // Restores the current state by popping the state stack.
  // IN: -
  // OUT: -
  void Restore(JsCallContext *context);

  // Sets the geometry transformation matrix to scale future drawing operations.
  // If only one scale argument is supplied, we use it for both dimensions.
  // IN: float x, float y
  // OUT: -
  void Scale(JsCallContext *context);

  // Sets the geometry transformation matrix to rotate future drawing
  // operations by the given angle, in radians.
  // IN: float angle
  // OUT: -
  void Rotate(JsCallContext *context);

  // Sets the geometry transformation matrix to translate future drawing
  // operations by the specified amount.
  // IN: int x, int y
  // OUT: -
  void Translate(JsCallContext *context);

  // Multiplies the current transformation matrix by the given matrix.
  // IN: float m11, float m12, float m21, float m22, float dx, float dy
  // OUT: -
  void Transform(JsCallContext *context);

  // Sets the current transformation matrix to the given matrix.
  // IN: float m11, float m12, float m21, float m22, float dx, float dy
  // OUT: -
  void SetTransform(JsCallContext *context);

  // Returns the current global alpha, which is used for future drawing
  // operations.
  // IN: -
  // OUT: float
  void GetGlobalAlpha(JsCallContext *context);

  // Sets the global alpha, which must be between 0.0 and 1.0.
  // IN: float
  // OUT: -
  void SetGlobalAlpha(JsCallContext *context);

  // Returns the current global composite operation, which defines how future
  // drawing operations are composited onto the canvas.
  // IN: -
  // OUT: int
  void GetGlobalCompositeOperation(JsCallContext *context);

  // Sets the global composite operation, which must be one of 'source-over'
  // and 'copy'.
  // IN: int
  // OUT: -
  void SetGlobalCompositeOperation(JsCallContext *context);

  // Basic getters / setters.
  void GetStrokeStyle(JsCallContext *context);
  void SetStrokeStyle(JsCallContext *context);
  void GetFillStyle(JsCallContext *context);
  void SetFillStyle(JsCallContext *context);
  void GetLineWidth(JsCallContext *context);
  void SetLineWidth(JsCallContext *context);
  void GetLineCap(JsCallContext *context);
  void SetLineCap(JsCallContext *context);
  void GetLineJoin(JsCallContext *context);
  void SetLineJoin(JsCallContext *context);
  void GetMiterLimit(JsCallContext *context);
  void SetMiterLimit(JsCallContext *context);

  // Rectangle operations.
  // IN: float x, float y, float width, float height
  // OUT: -
  // Clears the rectangle (sets it to transparent black).
  void ClearRect(JsCallContext *context);
  // Fills the rectangle with the current fill style.
  void FillRect(JsCallContext *context);
  // Strokes the rectangle with the current stroke style.
  void StrokeRect(JsCallContext *context);

  // Path operations.
  void BeginPath(JsCallContext *context);
  void ClosePath(JsCallContext *context);
  void MoveTo(JsCallContext *context);
  void LineTo(JsCallContext *context);
  void QuadraticCurveTo(JsCallContext *context);
  void BezierCurveTo(JsCallContext *context);
  void ArcTo(JsCallContext *context);
  void Rect(JsCallContext *context);
  void Arc(JsCallContext *context);
  void Fill(JsCallContext *context);
  void Stroke(JsCallContext *context);

  // Draws a canvas onto this canvas. sx, sy, sw, and sh identify a rectangular
  // subset of the source canvas, and dx, dy, dw, dh identify the target area
  // in the destination canvas (this canvas) into which pixels from the source
  // rectangle are drawn, with resizing if necessary.
  // IN: Canvas image, int sx, int sy, int sw, int sh, int dx, int dy, int dw,
  //     int dh
  // OUT: -
  void DrawImage(JsCallContext *context);

  // Creates an ImageData object, used for pixel manipulation.
  // IN: int sw, int sh
  // OUT: ImageData
  void CreateImageData(JsCallContext *context);

  // Reads pixels from the specified rectangular portion of the canvas, copies
  // them into a new ImageData object and returns it.
  // IN: int sx, int sy, int sw, int sh
  // OUT: ImageData
  void GetImageData(JsCallContext *context);

  // Copies pixels from the given ImageData object into the canvas at a
  // specified (x, y) offset. If the 'dirty' arguments are supplied, they
  // identify a rectangular subset of the given ImageData; only those pixels
  // are copied into the canvas.
  // Overloaded:
  // IN: ImageData imagedata, int dx, int dy
  // IN: ImageData imagedata, int dx, int dy, int dirtyX, int dirtyY,
  //     int dirtyWidth, int dirtyHeight
  // OUT: -
  void PutImageData(JsCallContext *context);

  // This function is not exposed to Javascript and exists only for internal
  // use.
  void SetCanvas(GearsCanvas *canvas, SkBitmap *bitmap);

 private:
  struct Style {
    SkColor color_;  // This color is before premultiplication.
    std::string16 string_;
    SkPaint paint_;

    Style();
  };

  // Calls ClearRenderingContextReference() on canvas_, if canvas_ is not NULL.
  void ClearReferenceFromGearsCanvas();

  // Callback used to handle the 'JSEVENT_UNLOAD' event.
  virtual void HandleEvent(JsEventType event_type);

  // Implements ClearRect, FillRect and StrokeRect.
  void PaintRect(JsCallContext *context, SkPaint *paint);

  // Sets a Style's paint_ to be its color_ multiplied by the global_alpha.
  void PremultiplyColor(Style *style);

  // Implements SetFillStyle and SetStrokeStyle.
  void SetStyle(JsCallContext *context, Style *style);

  // Implements Transform and SetTransform.
  void Transform(JsCallContext *context, bool reset_matrix);

  scoped_refptr<GearsCanvas> gears_canvas_;
  scoped_ptr<SkCanvas> skia_canvas_;

  // TODO(nigeltao): Introduce a class to capture the {SkPaint, SkColor,
  // std::string16} for both the fill and the stroke.
  SkPaint clear_style_as_paint_;
  Style fill_style_;
  Style stroke_style_;
  double global_alpha_as_double_;
  int global_alpha_as_int_;
  std::string16 global_composite_operation_as_string_;
  SkPath path_;

  scoped_ptr<JsEventMonitor> unload_monitor_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsCanvasRenderingContext2D);
};

#endif  // !defined(OFFICIAL_BUILD)
#endif  // GEARS_CANVAS_CANVAS_RENDERING_CONTEXT_2D_H__
