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

#ifndef GEARS_CANVAS_CANVAS_H__
#define GEARS_CANVAS_CANVAS_H__

#include "gears/base/common/base_class.h"
#include "gears/base/common/common.h"
#include "gears/base/common/scoped_refptr.h"
#include "third_party/scoped_ptr/scoped_ptr.h"
// Can't include any Skia header here, or factory.cc will fail to compile due
// to unused inline functions defined in the Skia header.
class SkBitmap;
class SkCanvas;
struct SkIRect;

#if !defined(OFFICIAL_BUILD)
// The Canvas rendering API (i.e. 2D context, on-screen DOM element) is not
// yet enabled in official builds.
#if BROWSER_IE
class CanvasRenderingElementIE;
#endif
class GearsCanvasRenderingContext2D;
#endif  // !defined(OFFICIAL_BUILD)

class GearsCanvas : public ModuleImplBaseClass {
 public:
  static const std::string kModuleName;

  // The maximum width, and maximum height, for a valid GearsCanvas.
  static const int kMaxSideLength;

  // Default canvas dimensions, as per the HTML5 specification.
  static const int kDefaultWidth;
  static const int kDefaultHeight;

  enum ResizeFilter {
    FILTER_FASTEST,
    FILTER_NEAREST,
    FILTER_BILINEAR,
    FILTER_NICEST
  };

  GearsCanvas();
  virtual ~GearsCanvas();
  
  // Loads an image (given as a blob) into this canvas, overwriting any
  // existing canvas contents. The canvas' width and height will be set to
  // the natural width and height of the image.
  // IN: Blob blob
  // OUT: -
  void Decode(JsCallContext *context);

  // Exports the contents of this canvas to a blob. This is a one-time
  // operation; updates to the canvas don't reflect in the blob.
  // IN: optional String mimeType, optional Object attributes
  // OUT: Blob
  void Encode(JsCallContext *context);

  // Crops the canvas, in-place.
  // IN: int x, int y, int width, int height
  // OUT: -
  void Crop(JsCallContext *context);

  // Resizes the canvas (scaling its contents), in-place. The filter (either
  // "bilinear" or "nearest") defaults to "bilinear" if unspecified.
  // IN: int width, int height, optional String filter
  // OUT: -
  void Resize(JsCallContext *context);

  // Accessors for the state of the canvas. Setting any of these causes the
  // canvas to be reset to transparent black, and invalidates any contexts.
  void GetWidth(JsCallContext *context);
  void GetHeight(JsCallContext *context);
  void SetWidth(JsCallContext *context);
  void SetHeight(JsCallContext *context);

#if !defined(OFFICIAL_BUILD)
  // Returns a context object to draw onto the canvas.
  // IN: String contextId
  // OUT: CanvasRenderingContext2D
  void GetContext(JsCallContext *context);

  // Returns a DOM element peer that renders this Gears canvas onto the screen.
  // IN: -
  // OUT: DomElement
  void GetRenderingElement(JsCallContext *context);

  // Invalidates the DOM element peer, and therefore repaints the Gears canvas.
  // IN: -
  // OUT: -
  void InvalidateRenderingElement(JsCallContext *context);
#endif  // !defined(OFFICIAL_BUILD)


  // The following are not exported to Javascript.

#if !defined(OFFICIAL_BUILD)
  // Clears the plain pointer to GearsCanvasRenderingContext2D, to prevent a
  // dangling pointer. The rendering context will be recreated when needed.
  void ClearRenderingContextReference();
#endif  // !defined(OFFICIAL_BUILD)

  // Returns true if the rectangle is contained completely within the bounds
  // of this bitmap, and has non-negative width and height.
  bool IsRectValid(const SkIRect &rect);

  int GetWidth() const;
  int GetHeight() const;
  const SkBitmap &GetSkBitmap();

 private:
  // Resets the Canvas to the specified dimensions and fills it with transparent
  // black pixels. All Context state is also destroyed. The HTML5 canvas spec
  // requires this when the user sets the canvas's width or height.
  void ResetCanvas(int width, int height);

  // We lazily allocate the SkBitmap's pixels. This method ensures that those
  // pixels have been allocated.
  void EnsureBitmapPixelsAreAllocated();

  // Cannot embed objects directly due to compilation issues; see comment
  // at top of file.
  scoped_ptr<SkBitmap> skia_bitmap_;

#if !defined(OFFICIAL_BUILD)
  // Can't use a scoped_refptr since that will create a reference cycle.
  // Instead, use a plain pointer and clear it when the target is destroyed.
  // Recreate this pointer when accessed again. For this to work, we make
  // the Context stateless.
  GearsCanvasRenderingContext2D *rendering_context_;

#if BROWSER_IE
  // Ideally, this should be a CComPtr<CanvasRenderingElementIE>, but doing
  // that causes inscrutable C++ template errors. Instead, we manually AddRef
  // and Release this COM object.
  CanvasRenderingElementIE *rendering_element_;
#endif
#endif  // !defined(OFFICIAL_BUILD)

  DISALLOW_EVIL_CONSTRUCTORS(GearsCanvas);
};

#endif  // GEARS_CANVAS_CANVAS_H__
