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


// Test that properties have the correct initial values, and that they can be
// read and written.
function testProperties() {
  var canvas = google.gears.factory.create('beta.canvas');
  var ctx = canvas.getContext('gears-2d');
  assertEqual(300, canvas.width);
  assertEqual(150, canvas.height);
  assertEqual(1.0, ctx.globalAlpha);
  assertEqual('source-over', ctx.globalCompositeOperation);
  assertEqual('#000000', ctx.fillStyle);

  var width = 50, height = 40;
  canvas.width = width;
  canvas.height = height;
  assertEqual(width, canvas.width);
  assertEqual(height, canvas.height);
  // TODO(kart): set to bad values and test?

  var alpha = 0.4;
  ctx.globalAlpha = alpha;
  assertEqual(alpha, ctx.globalAlpha);
  ctx.globalAlpha = 9.4; // should be a no-op.
  assertEqual(alpha, ctx.globalAlpha);

  var compositeOperation = 'copy';
  ctx.globalCompositeOperation = compositeOperation;
  assertEqual(compositeOperation, ctx.globalCompositeOperation);
  ctx.globalCompositeOperation = 'foobar'; // should be a no-op.
  assertEqual(compositeOperation, ctx.globalCompositeOperation);

  var fillStyle = '#34ab56';
  ctx.fillStyle = fillStyle ;
  assertEqual(fillStyle , ctx.fillStyle);
  // ctx.fillStyle = 'foobar';  // should be a no-op.
  // ctx.fillStyle = 42;  // should be a no-op.
  assertEqual(fillStyle , ctx.fillStyle);
}

function testGetContext() {
  var canvas = google.gears.factory.create('beta.canvas');
  var ctx = canvas.getContext('gears-2d');
  assertEqual(canvas, ctx.canvas);
  var ctx2 = canvas.getContext('gears-2d');
  assertEqual(ctx2, ctx);
  assertEqual(null, canvas.getContext('foobar'));
 }

function assertBlobsEqual(blob1, blob2) {
  if (isDebug)
    assert(blob1.hasSameContentsAs(blob2));
  else
    assertEqual(blob1.length, blob2.length);
}

function loadBlob(filename, callback) {
  var request = google.gears.factory.create('beta.httprequest');
  request.open('GET', '/testcases/data/' + filename);
  request.onreadystatechange = function(){
    if (request.readyState == 4) {
      callback(filename, request.responseBlob);
    }
  };
  request.send();
}

// Loads a bunch of blobs and when all of them are loaded, calls the callback
// passing a map from filename to blob.
function loadBlobs(filenames, callback) {
  var numLoaded = 0;
  var blobs = {};

  for (var i = 0; i < filenames.length; ++i) {
    loadBlob(filenames[i], function(filename, blob) {
      blobs[filename] = blob;
      if (++numLoaded == filenames.length) {
        callback(blobs);
      }
    });
  }
}

function runLoadAndExportTest(blobs) {
  var formats = ['png', 'jpeg'];
  for (var i = 0; i < formats.length; ++i) {
    var format = formats[i];
    var canvas = google.gears.factory.create('beta.canvas');  
    canvas.load(blobs['sample-original.' + format]);
    
    var pngBlob = canvas.toBlob('image/png');
    // The default format is png.
    assertBlobsEqual(canvas.toBlob(), pngBlob);
    var jpegBlob = canvas.toBlob('image/jpeg');
    var jpegLowBlob = canvas.toBlob('image/jpeg', { quality: 0.02 });
    // Now compare the exported versions with golden files.
    // The golden files have been manually checked for format and size.
    assertBlobsEqual(pngBlob, 
        blobs['sample-' + format + '-exported-as-png.png']);
    assertBlobsEqual(jpegBlob, 
        blobs['sample-' + format + '-exported-as-jpeg.jpeg']);
    assertBlobsEqual(jpegLowBlob,
        blobs['sample-' + format + '-exported-as-jpeg-with-quality-0.02.jpeg']);
  }
  completeAsync();
}

// TODO(kart): Add negative tests. Example: load an unsupported blob,
// export to an unsupported format. What else?
function testLoadAndExport() {
  startAsync();
  var filenames = [
    'sample-original.jpeg',
    'sample-original.png',
    'sample-jpeg-exported-as-jpeg-with-quality-0.02.jpeg',
    'sample-jpeg-exported-as-jpeg.jpeg',
    'sample-jpeg-exported-as-png.png',
    'sample-png-exported-as-jpeg-with-quality-0.02.jpeg',
    'sample-png-exported-as-jpeg.jpeg',
    'sample-png-exported-as-png.png'];
  loadBlobs(filenames, runLoadAndExportTest);
}

function testResize() {
  var originalFilename = 'sample-original.jpeg';
  var resizedFilename = 'sample-jpeg-resized-to-400x40.png';
  startAsync();
  loadBlobs([originalFilename , resizedFilename], function(blobs) {
    var canvas = google.gears.factory.create('beta.canvas');
    canvas.load(blobs[originalFilename]);
    
    var newWidth = 400;
    var newHeight = 40;
    canvas.resize(newWidth, newHeight);
    assertEqual(newWidth, canvas.width);
    assertEqual(newHeight, canvas.height);
    assertBlobsEqual(blobs[resizedFilename], canvas.toBlob());
    completeAsync();
  });
}

function testCanvasInitiallyBlank() {
  var canvas = google.gears.factory.create('beta.canvas');
  var blob = canvas.toBlob();
  startAsync();
  loadBlob('blank-300x150.png', function(filename, blankBlob) {
    assertBlobsEqual(blankBlob, blob);
    completeAsync();
  });
}

// Tests that changing width or height
// resets the pixels to all transparent black.
function testCanvasBlankAfterChangingDimensions() {
  var originalFilename  = 'sample-original.jpeg';
  var filenames = [originalFilename,
                   'blank-313x120.png',
                   'blank-240x234.png'];
  startAsync();
  loadBlobs(filenames, function(blobs) {
    var canvas = google.gears.factory.create('beta.canvas');
    var originalBlob = blobs[originalFilename ];
    canvas.load(originalBlob);
    assertEqual(313, canvas.width);
    assertEqual(234, canvas.height);

    // Changing the height must reset the canvas:
    canvas.height = 120;
    assertBlobsEqual(canvas.toBlob(), blobs['blank-313x120.png']);
    
    // Changing the width must reset the canvas:
    canvas.load(originalBlob);
    canvas.width = 240;
    assertBlobsEqual(canvas.toBlob(), blobs['blank-240x234.png']);
    
    completeAsync();
  });
}

// Tests that assigning the width or height its present value
// resets the pixels to all transparent black,
// as the HTML5 canvas spec requires.
function testCanvasBlankAfterDimensionsSelfAssignment() {
  var originalFilename  = 'sample-original.jpeg';
  var filenames = [originalFilename,
                   'blank-313x234.png'];
  startAsync();
  loadBlobs(filenames, function(blobs) {
    var canvas = google.gears.factory.create('beta.canvas');
    var originalBlob = blobs[originalFilename ];
    canvas.load(originalBlob);
    assertEqual(313, canvas.width);
    assertEqual(234, canvas.height);
    // The canvas is not blank.
    assertNotEqual(canvas.toBlob().length, blobs['blank-313x234.png'].length);
    
    // Setting width to current width must blank the canvas:
    canvas.width = canvas.width;
    assertBlobsEqual(canvas.toBlob(), blobs['blank-313x234.png']);
    
    // Test the same for height:
    canvas.load(originalBlob);
    canvas.height = canvas.height;
    assertBlobsEqual(canvas.toBlob(), blobs['blank-313x234.png']);
    
    completeAsync();
  });
}


/*
function testDrawImage() {
  if(isOfficial)
    return;
  var canvas = google.gears.factory.create('beta.canvas');
  var ctx = canvas.getContext('gears-2d');
  var canvas2 = google.gears.factory.create('beta.canvas');
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
