# Introduction #
We extend a subset of HTML5's Canvas for photo manipulation, by defining a Canvas class with the additional functionality. We may not support every function the standard Canvas does -- only the ones that make sense for our use cases. But whatever functionality is common with canvas has the same API.

The Gears Canvas is offscreen only. It can be exported to a URL and from there inserted into the DOM as an image element, or drawn onto a HTML canvas (if the browser supports it), used as a background, etc. Here's how you'd do this:
  1. Export to blob.
  1. Capture the blob at a localserver url.
  1. Create a HTMLImageElement, set the src to the blob's url, and insert the image element into the DOM. Or draw the image element onto a html canvas (if supported). You should wait for the load to complete (onload event) before you draw, though that shouldn't take long.

In the future, if this rendering path is too slow, we'll add support for direct rendering through a browser plugin (like Flash). For now we render through localserver since we anyway need to support exporting to a blob for upload, so the flow comes for free.

We currently have no plans to GPU-accelerate this API.
# Requirements #
  1. V1 features (Basic):
    * Basic operations (crop, resize, scale, rotate)
    * Primitives (affine transforms, color transforms, convolution matrices, lookup tables)
    * Draw image onto canvas from local file / remote URL
    * Save image (after manipulation) onto local file / post it to a remote URL (We get this functionality by virtue of using the Blob abstraction)
    * Integration with Gears Blob and URL capture
    * Support for BMP, PNG and JPEG formats
    * Annotation (support for basic text)
  1. Beyond V1 (Advanced):
    * Rendering on screen (real-time as opposed to via the BMP encoded capture URL through HTTP)
    * Clippings of arbitrary shape
    * Drawing primitives
    * Support color lookup tables
    * Cool Photo stuff (Red-eye removal, smart scissors, magic wand, etc...)
    * Support other color depths (other than 32 bpp)
    * Better support for non-destructive edits

# JS interface #
The Gears Canvas is based on HTML5 [Canvas](http://www.whatwg.org/specs/web-apps/current-work/#canvas); you should be familiar with the latter before you read this doc.

**_Canvas class:_**
```
Canvas class
{  
  // Throws away the existing contents of the canvas.
  // Sets the canvas's dimensions to those of the image in the blob.
  void load(Blob blob);

  
  // Exports this canvas as a blob. This is a snapshot; updates to the canvas are not reflected in the blob.
  // Supported formats: JPEG, PNG, BMP
  // 'attributes' is a map of key-value pairs. 
  // These are the available keys, all optional:
  //      indexed -- boolean. defaults to false unless palette is specified.
  //      palette -- an array of colors (as integer values). Explicitly specifying indexed = false while passing in a palette is an error.
  //      compressionQuality -- float between 0.0 (small size) and 1.0 (good quality). only for JPEG.
  //      interlaced -- boolean.
  Blob toBlob(optional String mimeType, optional Map attributes);

  // Returns a new Canvas object with the same pixels.
  Canvas clone();

  // The palette of the image, if the canvas was load()ed from an indexed mode image, and null otherwise.
  // Note that on load, the image is converted to RGB; the palette is preserved in case you want
  // to export the image back to indexed mode, but the canvas itself doesn't have an indexed mode.
  // The palette is an array of color values expressed as integers.
  readonly attribute array originalPalette;

  // The functions below change the pixels of the canvas in place, as opposed to scale, rotate, etc 
  // which only set the current transformation matrix that affects future drawing operations.

  // This method ignores the state of the 2d context (see below).
  void crop(int x, int y, int width, int height);
 
  // This method ignores the state of the 2d context.
  // This method creates new pixel values using interpolation, as opposed to creating blank pixels or cropping.
  void resize(int width, int height);


  // Properties. Assigning to any of these resets all pixels to transparent black.
  attribute unsigned long width;
  attribute unsigned long height;

  // Returns a context.
  CanvasRenderingContext2D getContext(string contextId);
};
```

**_CanvasRenderingContext2D class:_**

As in HTML Canvas, our context has a state, which is a subset of the state in HTML Canvas's context. Our state consists of:
  * The current transformation matrix
  * globalAlpha
  * globalCompositeOperation
  * fillStyle

HTML Canvas supports a richer state with more attributes pertaining to drawing that we don't support. See the spec for details.

The 'transform' function just sets the current transformation matrix and does not apply it on the underlying image on the canvas. Just as with HTML Canvas, the drawing functions (drawImage, clearRect, fillRect, strokeRect below) respect the state, and apply the necessary transforms on the clipping region. However color transforms, convolution transforms, etc are not part of the state, and invocations of these functions apply the changes on the underlying image directly. Hence, functions such as 'crop', 'resize', etc. apply the changes right then, whereas 'scale', 'rotate', etc just update the state (the transformation matrix to be precise) and the operations are applied only when a draw function is called.

```
CanvasRenderingContext2D class
{
  // ----- THE FOLLOWING ARE FROM HTML CANVAS -------
  // The arguments, return values and semantics are described at http://www.whatwg.org/specs/web-apps/current-work/#canvas

  // back-reference to the canvas:
  readonly attribute Canvas canvas;

  // state
  void save();  // Push state on state stack
  void restore();  // Pop state stack and restore state
 
  // affine transformations (default transform is the identity matrix)
  void scale(float x, optional float y);  // 'y' scale factor defaults to the 'x' scale factor.
  void rotate(float angle); // in radians.
  void translate(float x, float y);
  void transform(float m11, float m12, float m21, float m22, float dx, float dy);
  void setTransform(float m11, float m12, float m21, float m22, float dx, float dy);

  // compositing
  attribute float globalAlpha;  // default: 1.0
  attribute string globalCompositeOperation;   // we only support 'source-over' and 'copy'. Default: 'source-over'.
  attribute string fillStyle;  // Note: must be a color, not a gradient or pattern; we don't support those.
  
  // drawing rectangles
  void clearRect(int x, int y, int w, int h);
  void fillRect(int x, int y, int w, int h);  // see fillStyle.
  
  // text
  attribute string font;  // default: 10px sans-serif
  // One of "left", "center", "right". We do not support RTL fonts, so we don't need "start" and "end". Default: "left".
  attribute string textAlign;
  // (x, y) specifies a point on the baseline -- the left end, middle point or right end depending on the textAlign property.
  // 'wrap' wraps to the maxWidth, if supplied, otherwise to the canvas's right edge.
  void fillText(string text, float x, float y, optional int maxWidth, optional boolean wrap);
  TextMetrics measureText(string text);

  // drawing images
  // If the source rectangle stretches beyond the bounds of its canvas or has zero width or height, an error is generated, as per the HTML canvas spec. We also generate an error if the destination rectangle is invalid (i.e., stretches beyond the bounds of its canvas or has zero width or height).
  void drawImage(Canvas image, int dx, int dy);
  void drawImage(Canvas image, int dx, int dy, int dw, int dh);
  void drawImage(Canvas image, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
 
  // pixel manipulation
  // An ImageData object created with this method should be substitutable for one created from a HTML Canvas.
  ImageData createImageData(int sw, int sh);
  ImageData getImageData(int sx, int sy, int sw, int sh);
  void putImageData(ImageData imagedata, int dx, int dy);
  void putImageData(ImageData imagedata, int dx, int dy, int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight);




  // -------- THE FOLLOWING ARE ADDITIONAL ABSTRACTIONS WE SUPPORT ------------

  // 4x5 matrix for RGBA images and 3x4 for RGB.
  // Referring to the color matrix as 'a', here's how this function works (for RGBA):
  // redResult      = a[0] * srcR    + a[1] * srcG   + a[2] * srcB   + a[3] * srcA    + a[4]
  // greenResult  = a[5] * srcR    + a[6] * srcG   + a[7] * srcB   + a[8] * srcA    + a[9]
  // blueResult     = a[10] * srcR + a[11] * srcG + a[12] * srcB + a[13] * srcA + a[14]
  // alphaResult   = a[15] * srcR + a[16] * srcG + a[17] * srcB + a[18] * srcA + a[19]
  // 
  // Result components are clipped to the range 0.0 to 1.0.
  // 
  // All color channel values (both source and result) are expressed as floats between 0.0 and 1.0 so that they are independent of color depth.
  // 
  // The identity matrix has a[0] = a[6] = a[12] = a[18] = 1 with other elements being 0.
  void colorTransform(float[] colorMatrix);
 
  // Computes the color of each pixel by taking a weighted mean of its surrounding pixels' colors, and then adding the bias, if it's supplied.
  // The matrix specifies the weights, with the center element of the matrix representing the source pixel's weight and 
  // surrounding elements in the matrix representing the surrounding pixels' weights.
  // Unlike a color matrix, a convolution matrix's size isn't fixed, but it must be a square matrix of odd dimension (otherwise there's no center element).
  // 
  // All color channel values (both source and result) are expressed as floats between 0.0 and 1.0 so that they are independent of color depth.
  // For pixels that are off the source image, the input image is extended along each of its borders infinitely by duplicating the color values at the given edge of the input image.
  // If applyToAlpha is true, the convolution is applied to the alpha channel in addition to the R, G and B channels. The default is false.
  void convolutionTransform(float[] convolutionMatrix, optional float bias, optional boolean applyToAlpha);
 
  // Allows fractional radii.
  // Takes the median of all pixels within the specified distance from the target pixel.
  // Works component-wise, of course. First premultiplies alpha, and then takes median
  // of R, G and B components separately.
  // After the filter finishes, all pixels have alpha 1.0.
  void medianFilter(float radius);




  // ----- CONVENIENCE FUNCTIONS FOR COMMON OPERATIONS --------
  // These build on top of the primitives above.


  // This function adds a specified delta to each color channel (expressed as a float between 0.0 and 1.0).
  // delta = -1.0 => fully black image, 0.0 => original image, 1.0 => fully white image.
  // 
  // Equivalent to a call to colorTransform with a[4] = a[9] = a[14] = delta, and other
  // elements having values from the identity matrix. 
  void adjustBrightness(float delta);

  // Equivalent to a call to colorTransform with the matrix
  //     x, 0, 0, 0, 0.5*(1-x)
  //     0, x, 0, 0, 0.5*(1-x)
  //     0, 0, x, 0, 0.5*(1-x)
  //     0, 0, 0, 1, 0
  // where x is the contrast.
  void adjustContrast(float contrast);

  // This function adjusts saturation while preserving luminance.
  // saturation = -1.0 => invert colors, 0.0 => grayscale, 1.0 => original image.
  //
  // Equivalent a call to colorTransform() with the matrix
  //     lumR*(1-x)+x, lumG*(1-x),      lumB*(1-x),      0, 0
  //     lumR*(1-x),      lumG*(1-x)+x, lumB*(1-x),      0, 0
  //     lumR*(1-x),      lumG*(1-x),      lumB*(1-x)+x, 0, 0
  //     0,                        0,                        0,                        1, 0
  //     0,                        0,                        0,                        0, 1
  // where x is the saturation value (the argument)
  // and (lumR, lumG, lumB) = (0.3086, 0.6094, 0.0820)  [the standard luminance vector]
  void adjustSaturation(float saturation);
 
  // Rotates colors on the color wheel while preserving luminance.
  void adjustHue(float angle); // in radians.

  // Equivalent to a call to convolutionTransform with all pixels within the specified radius having a weight w
  // and other pixels having weight zero. The weight w is chosen so that the sum of all weights is 1.
  void blur(float factor, int radius);

  // Equivalent to a call to convolutionTransform with the source pixel having a weight w,
  // all other pixels within the specified radius having a weight -1, and pixels outside the radius having a weight 0.
  // The weight w is chosen so that the sum of all weights in the matrix is 1.
  void sharpen(float factor, int radius);

  // Sets the transformation matrix to the identity matrix.
  void resetTransform(); 
};
```

**_ImageData class:_**
```
ImageData class {
  readonly attribute long int width;
  readonly attribute long int height;
  readonly attribute int[] data;
};
```

**_TextMetrics class:_**
```
TextMetrics class {
  readonly attribute float width;
};
```

# Differences from HTML Canvas #
Unlike the HTML5 canvas, which can be onscreen (part of the DOM) or not (instantiated using Javascript), we are purely an offscreen canvas as of v1, with export to a URL for display as an image. Also, the Gears canvas can be used from worker threads. Here are the other changes:
  * Added primitives for color transformation, convolution and lookup tables.
  * Drawing of arbitrary paths not supported yet.
  * No support for clipping regions, gradients, patterns, shadows, strokes and line styles.
  * We export to blobs, which can be uploaded to http servers or exported as a URL on the LocalServer; we do not export to data URIs. This is because exporting to a data URI involves base64-encoding the image, which makes it bigger.
  * We do not support the drawImage functions that take in either a HTMLImageElement or a HTMLCanvasElement as a parameter.

# Sample Code #
**_Example 1: Downloads an image from a server, crops and resizes it, displays the result and uploads it_**:
```
var request = google.gears.factory.create('httprequest');
var serverUrl = 'http://picasaweb.google.com/photo/23432094834';
request.open('GET', serverUrl);
request.onreadystatechange = function() {
  if (request.readyState == 4) {
    process(request.responseBlob);
  }
};
request.send();

function process(var srcBlob) {
  var canvas = google.gears.factory.create('canvas');
  canvas.load(srcBlob);

  var context = canvas.getContext('gears-2d');
  // crop out 10 pixels on each side.
  context.crop(10, 10, canvas.width - 20, canvas.height - 20);
  context.resize(1024, 768);

  var destBlob = canvas.toBlob();

  // update on screen:
  var localServer = google.gears.factory.create('localserver');
  var store = localServer.createStore(STORE_NAME);
  store.captureBlob(destBlob, IMAGE_URL);
  var img = document.getElementById('myImage');
  img.src = IMAGE_URL;

  // now upload to server:
  request = google.gears.factory.create('httprequest');
  request.open('PUT', serverUrl);
  request.onreadystatechange = function() {
    if (request.readyState == 4) {
      alert('Upload finished!');
    }
  }
  request.sendBlob(destBlob);
};
```

**_Example 2: Performs some more image operations and draws text_**:
```
// load the image as above.

function process(var srcBlob) {
  var canvas = google.gears.factory.create('canvas');
  canvas.load(srcBlob);
  
  var context = canvas.getContext('gears-2d');
  context.crop(10, 10, canvas.width - 20, canvas.height - 20); 
  context.resize(1024, 768);

  // rotate the image:
  var destCanvas = google.gears.factory.create('canvas');
  var destContext = destCanvas.getContext('gears-2d');

  // NOTE: does not rotate the image in place;
  // instead sets the current transformation matrix for future draw operations.
  // This is how Canvas works.
  destContext.rotate(0.2);

  destContext.draw(canvas);
  destContext.resetTransform();
  destContext.adjustBrightness(0.2);
  destContext.drawText(950, 100, 'The quick brown fox jumped over the lazy dog', 'Times New Roman', 14, {
      'bold': true,
      'italics': true,
      'wrap': true });

  var destBlob = destCanvas.toBlob("image/jpeg", {
      'interlaced': true,
      'compressionQuality':0.7});

  // display and upload the output blob as in the previous example.
}
```


# Beyond V1 #
We may add these in a future version:
  1. Arbitrary color lookup tables
  1. Read and write metadata (EXIF)
  1. High color depths: There are three reasons why we want to support more than 8 bits per channel color depth:
    1. To support camera RAW files.
    1. To prevent rounding errors from accumulating after every operation.
    1. For HDR imaging.
> > However, we're not sure how much of a need there is for the above, and how high color depths work on mobiles, so we'll implement this in future based on need.
  1. Gaussian Blur?
  1. Some fun filters: cinemascope, night vision, neon, vignette (from picnic), blueprint, cartoonize, fresco (from fotoflexer).
  1. Distort: you give a set of source and destination "control points" and the api distorts the image based on that, or the same can be done with "control lines".
  1. Apply image mask, generate image mask from an existing image. Clip drawing and photo manipulation operations to an image mask.
  1. Detect and remove timestamps embedded in images.
  1. Red-eye correction
  1. Auto color correction
  1. Magic Wand (as in Photoshop)
  1. Clip Regions. Here's the API:
**_CanvasRenderingContext2D:_**
```
  // TODO: add EXIF stuff here.

  // ----- NEW ABSTRACTION ------------
  // TODO: need to add an abstraction to support color lookup tables.

  // ------ CLIP REGION SUPPORT (FROM HTML5) ---------
  // assign undefined to set the clip region to the whole canvas.
  // may be arbitrarily shaped, in general, not necessarily rectangular.
  // once clipRegion is added, it will be part of the state, and drawing operations and various image operations
  // (color matrix, median filter, ...) will respect the clip region.
  attribute ClipRegion clipRegion; 

  // returns the new clip region; does not set it as the current clip region.
  ClipRegion defineRectangularClipRegion(int x, int y, int width, int height);

  // copies the pixels selected in this clip region to a new canvas and returns it.
  // The returned canvas is the smallest rectangle that can hold all selected pixels.
  // Since the clip region may be non-rectangular, unselected pixels may be present in the
  // returned canvas, but with alpha = 0.
  Canvas getPixels(ClipRegion clipRegion);
};
```

**_Clip Region:_**
```
// ----- FROM HTML5 --------
class ClipRegion {
  void add(ClipRegion other);
  void subtract(ClipRegion other);
  void invert();

  ClipRegion clone();
};
```

Not Sure If We Need These:
  1. "[Smart scissors](http://fotoflexer.com/demos/smartScissorsDemoHiRes.php)" basically allows you to select an object in a photo by drawing roughly on its boundary.
  1. "[Smart resize](http://fotoflexer.com/demos/smartResizeDemoHiRes.php)" enables you to change only the width (or the height) of an image without cropping it and without changing aspect ratio. It selectively deletes some rows or columns of pixels to preserve the look of the photo. Similarly it can automatically generate rows or columns of pixels. Watch the demo; it's pretty cool.
  1. Text rendering -- sub-pixel antialiasing, control over character spacing, small caps, superscript and subscript, bidi text...
  1. Color space conversions, gamma correction.



## Thoughts on Undo: ##

Right now the way an undo can be implemented using the above API is to either clone the canvas after each operation or keep track of the initial state and the list of operations applied on it, so that one can undo to any point by applying some of the operations.

If needed in future, here is one possible way in which we can implement undo in the API itself:

Many operations normally modify the canvas in place, but take an optional switch that makes them return a new canvas instead. The advantage with this approach is flexibility -- if you're interested in the previous state of the canvas (for undo), you generate the output in a new canvas without having to do a potentially expensive clone operation. On the other hand, if don't care about undo, you can overwrite the source in place without paying the memory cost of a second buffer (for many operations).

The problem with this approach is that for consistency the draw functions should also have an option to return a new canvas, and such a thing may be tougher to merge into the HTML standard. So right now, we don't support this. If you want to preserve the old state, you have to either clone the canvas after each operation or keep track of the initial state and the list of operations applied on it, so that you can undo to any point by applying some of the operations.

We can re-examine the problem for v2 if one of our clients wants to use it.