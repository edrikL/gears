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

// Tests that all functions and properties of the canvas and context are
// defined.
/* function testStub() {
  // Create a new canvas object in each test to not have a dependency on the
  // state of the canvas or its context and therefore a dependency on the order
  // of execution of tests.
  if(isOfficial)
    return;
  var canvas = google.gears.factory.create("beta.canvas","1.0");
  var ctx = canvas.getContext('gears-2d');
  canvas.load({});
  canvas.toBlob("image/jpeg", {});

  canvas.clone();

  assertEqual(canvas.originalPalette, undefined);

  canvas.crop(10, 10, 100, 100);

  ctx.save();
  ctx.restore();

  ctx.clearRect(200, 200, 30, 30);
  ctx.fillRect(200, 200, 30, 30);
  ctx.strokeRect(200, 200, 30, 30);

  ctx.scale(0.8, 0.7);
  ctx.scale(0.8);
  ctx.rotate(0.34);
  ctx.translate(30, -20);
  ctx.transform(1, 2, 3, 4, 5, 6);
  ctx.setTransform(1, 2, 3, 4, 5, 6);

  ctx.createImageData(1024, 768);
  ctx.getImageData(0, 0, 500, 400);
  
  var dummyImageData = {};
  ctx.putImageData(dummyImageData, 100, 100);
  ctx.putImageData(dummyImageData, 100, 100, 10, 20, 30, 40);

  var dummyMatrix = {};
  ctx.colorTransform(dummyMatrix);
  ctx.convolutionTransform(dummyMatrix);
  ctx.medianFilter(4.3);

  ctx.drawText('Hello World', 'Times New Roman', 12, 0, 0, 100, 40, {});
  ctx.drawText('Hello World', 'Times New Roman', 12, 0, 0, 100, 40);
  ctx.drawText('Hello World', 'Times New Roman', 12, 0, 0);

  var dummyImage = {};
  ctx.drawImage(dummyImage, 100, 90);
  ctx.drawImage(dummyImage, 100, 90, 1024, 768);
  ctx.drawImage(dummyImage, 10, 10, 50, 50, 100, 90, 1024, 768);

  var dummyImageData = {};
  ctx.drawImageData(dummyImageData, 10, 10, 400, 300, 50, 40);

  ctx.adjustBrightness(0.5);
  ctx.adjustContrast(0.5);
  ctx.adjustSaturation(0.5);
  ctx.adjustHue(0.5);

  ctx.blur(0.1, 8);
  ctx.sharpen(0.2, 6);

  ctx.resetTransform();
} */

// Test that properties have the correct initial values, and that they can be 
// read and written.
function testProperties() {
  if(isOfficial)
    return;
  var canvas = google.gears.factory.create("beta.canvas","1.0");
  var ctx = canvas.getContext('gears-2d');
  assertEqual(300, canvas.width);
  assertEqual(150, canvas.height);
  assertEqual(1.0, ctx.globalAlpha);
  assertEqual('source-over', ctx.globalCompositeOperation);
  assertEqual('#000000', ctx.fillStyle);

/*   var WIDTH = 50, HEIGHT = 40;
  canvas.width = WIDTH;
  canvas.height = HEIGHT;
  assertEqual(canvas.width, WIDTH);
  assertEqual(canvas.height, HEIGHT);
  // TODO(kart): set to bad values and test?
 */
 
  var ALPHA = 0.4;
  ctx.globalAlpha = ALPHA;
  assertEqual(ALPHA, ctx.globalAlpha);
  ctx.globalAlpha = 9.4; // should be a no-op.
  assertEqual(ALPHA, ctx.globalAlpha);
  
  var COMPOSITE_OPERATION = 'copy';
  ctx.globalCompositeOperation = COMPOSITE_OPERATION;
  assertEqual(COMPOSITE_OPERATION, ctx.globalCompositeOperation);
  ctx.globalCompositeOperation = 'foobar'; // should be a no-op.
  assertEqual(COMPOSITE_OPERATION, ctx.globalCompositeOperation);
  
  var FILL_STYLE = '#34ab56';
  ctx.fillStyle = FILL_STYLE;
  assertEqual(FILL_STYLE, ctx.fillStyle);
  // ctx.fillStyle = 'foobar';  // should be a no-op.
  // ctx.fillStyle = 42;  // should be a no-op.
  assertEqual(FILL_STYLE, ctx.fillStyle);
}

function testGetContext() {
  if(isOfficial)
    return;
  var canvas = google.gears.factory.create("beta.canvas","1.0");
  var ctx = canvas.getContext('gears-2d');
  assertEqual(canvas, ctx.canvas);
  var ctx2 = canvas.getContext('gears-2d');
  assertEqual(ctx2, ctx);
  assertEqual(null, canvas.getContext('foobar'));
 }

/*
function testDrawImage() {
  if(isOfficial)
    return;
  var canvas = google.gears.factory.create("beta.canvas","1.0");
  var ctx = canvas.getContext('gears-2d');
  var canvas2 = google.gears.factory.create("beta.canvas","1.0");
  // assertError(ctx.drawImage(canvas2, 1000, 1000, 30, 20, 0, 0, 10, 10));
    // TODO(kart): check for INDEX_SIZE_ERR
  // assertError(ctx.drawImage(42));
    // TODO(kart): check for TYPE_MISMATCH_ERR
}
*/

// TODO(kart): restore should do nothing if the stack is
// empty (i.e, blank canvas)
 
// TODO(kart): clearRect and fillRect have no effect is width or height is zero.
// strokeRect has no effect if both width and height are zero, and draws a line
// if exactly one of width and height is zero.
 
// TODO(kart): If any of the arguments to createImageData() or getImageData()
// are infinite or NaN, or if either the sw or sh arguments are zero, the method
// must instead raise an INDEX_SIZE_ERR exception.
 
// TODO(kart): The values of the data array may be changed
// (the length of the array,
//  and the other attributes in ImageData objects, are all read-only). 
// On setting, JS undefined values must be converted to zero. Other values must
// first be converted to numbers using JavaScript's ToNumber algorithm, and if
// the result is a NaN value, a TYPE_MISMATCH_ERR exception must be raised. If
// the result is less than 0, it must be clamped to zero. If the result is more
// than 255, it must be clamped to 255. If the number is not an integer, it must
//  be rounded to the nearest integer using the IEEE 754r roundTiesToEven 
// rounding mode.

// TODO(kart): putImageData: If the first argment to the method is null or
//  not an
// ImageData object that was returned by createImageData() or getImageData()
// then the putImageData() method must raise a TYPE_MISMATCH_ERR exception. If
// any of the arguments to the method are infinite or NaN, the method must raise
// an INDEX_SIZE_ERR exception.

// TODO(kart): make sure 
// context.putImageData(context.getImageData(x, y, w, h), x, y);
// is a noop.
