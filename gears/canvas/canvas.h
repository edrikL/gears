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

#ifndef OFFICIAL_BUILD
// The Image API has not been finalized for official builds.

#include "gears/base/common/common.h"
#include "gears/base/common/scoped_refptr.h"

class GearsCanvasRenderingContext2D;

// Extension of a subset of HTML5 Canvas for photo manipulation.
// See http://code.google.com/p/google-gears/wiki/ImagingAPI for more detailed
// documentation.
class GearsCanvas : public ModuleImplBaseClassVirtual {
 public:
  static const std::string kModuleName;

  GearsCanvas();

  // Loads an image from a supplied blob.
  // IN: Blob blob.
  // OUT: -
  void Load(JsCallContext *context);

  // Exports the contents of this canvas into a blob. This is a one-time
  // operation; updates to the canvas don't reflect in the blob.
  // IN: optional String mimeType, optional Object attributes
  // OUT: Blob
  void ToBlob(JsCallContext *context);

  // Returns a new Canvas object with the
  // same state (width, height, color mode, pixels).
  // IN: -
  // OUT: Canvas
  void Clone(JsCallContext *context);

  // Returns the palette of the image this canvas was loaded from. Note that the
  // canvas itself doesn't support an indexed mode.
  // IN: -
  // OUT: Array of colors (as integers)
  void GetOriginalPalette(JsCallContext *context);

  // Crops the canvas to the specified rectangle, in-place.
  // IN: int x, int y, int width, int height
  // OUT: -
  void Crop(JsCallContext *context);

  // Resizes the canvas to the supplied dimensions. In-place.
  // IN: int width, int height
  // OUT: -
  void Resize(JsCallContext *context);

  // Accessors for the state of the canvas. Setting any of these causes the
  // canvas to be reset to transparent black pixels and its context to be
  // reset (to alpha=1, geometry transformation matrix=identity matrix, etc).
  void GetWidth(JsCallContext *context);
  void GetHeight(JsCallContext *context);
  void SetWidth(JsCallContext *context);
  void SetHeight(JsCallContext *context);
  // One of the strings "rgba_32" and "rgb_24":
  void GetColorMode(JsCallContext *context);
  void SetColorMode(JsCallContext *context);

  // Returns a context object to draw onto the canvas.
  // IN: String contextId
  // OUT: CanvasRenderingContext2D
  void GetContext(JsCallContext *context);


 private:
  static const std::string16 kColorModeRgba32;
  static const std::string16 kColorModeRgb24;
  std::string16 color_mode_;
  int width_;
  int height_;
  scoped_refptr<GearsCanvasRenderingContext2D> rendering_context_;

  // Resets the canvas to all transparent black pixels at the given dimensions.
  void ResetToBlankPixels();

  DISALLOW_EVIL_CONSTRUCTORS(GearsCanvas);
};

#endif  // OFFICIAL_BUILD
#endif  // GEARS_CANVAS_CANVAS_H__
