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

function testFailBlob() {
  if (!isDebug) {
    // FailBlob only exists in debug builds
    return;
  }

  var blob = google.gears.factory.create('beta.failblob', '1.0', 500);
  assertNotNull(blob, 'Could not create a failblob');
  assert(blob.length == 500);
}

function testBlobBuilder() {
  var builder1 = google.gears.factory.create('beta.blobbuilder');
  assertNotNull(builder1, 'Could not create a blob builder');
  builder1.append('Hello');
  builder1.append(' ');
  builder1.append('World');
  var blob1 = builder1.getAsBlob();
  assertEqual(blob1.length, 11);

  var builder2 = google.gears.factory.create('beta.blobbuilder');
  builder2.append('.');
  var blob2 = builder2.getAsBlob();
  assertEqual(blob2.length, 1);

  var builder3 = google.gears.factory.create('beta.blobbuilder');
  var blob3 = builder3.getAsBlob();
  assertEqual(blob3.length, 0);

  var builder4 = google.gears.factory.create('beta.blobbuilder');
  builder4.append(blob1);
  builder4.append(blob2);
  builder4.append(blob3);
  var blob4 = builder4.getAsBlob();
  assertEqual(blob4.length, blob1.length + blob2.length + blob3.length);

  if (isDebug) {
    var builder5 = google.gears.factory.create('beta.blobbuilder');
    builder5.append('Hello World.');
    assert(builder5.getAsBlob().hasSameContentsAs(blob4));
  }

  // TODO(michaeln): a builder.reset() method may be nice?
  var builder6 = google.gears.factory.create('beta.blobbuilder');
  builder6.append('\uCF8F');  // 3 bytes in utf8
  builder6.append('\u00A2');  // 2 bytes in utf8
  var blob6 = builder6.getAsBlob();
  assertEqual(3 + 2, blob6.length);

  // \uCF8F is 1100 1111 1000 1111 in UTF-16, which in UTF-8 is
  // iiio 1100 io11 1110 io00 1111, where 'i' and 'o' are ones and zeroes
  // introduced by the UTF-8 encoding, and '1' and '0' are the content.
  // Converting these three bytes to hex gives EC BE 8F, or 236 190 143.
  // Similarly, \u00A2 becomes iio0 0010 io10 0010, or C2 A2, or 194 162.
  var bytes6 = blob6.getBytes(0, 5);
  assertEqual(5, bytes6.length);
  assertEqual(236, bytes6[0]);
  assertEqual(190, bytes6[1]);
  assertEqual(143, bytes6[2]);
  assertEqual(194, bytes6[3]);
  assertEqual(162, bytes6[4]);

  assertError(function() {
    blob6.getBytes(-1, 5);
  }, 'Offset must be a non-negative integer.');
  assertError(function() {
    blob6.getBytes(1, 999999);
  }, 'Length must be at most 1024.');
  assertError(function() {
    blob6.getBytes(1, 999);
  }, 'Read error during getBytes.');
  assertError(function() {
    blob6.getBytes(999, 1);
  }, 'Read error during getBytes.');

  var bytes6a = blob6.slice(2).getBytes();
  var bytes6b = blob6.getBytes(2);
  assertEqual(3, bytes6a.length);
  assertEqual(3, bytes6b.length);
  for (var i = 0; i < bytes6a.length; i++) {
    assert(bytes6a[i] == bytes6b[i]);
  }
}

function testBlobBuilderAppendIntStringBlobArray() {
  // The integer values below map to ASCII characters like so:
  // A=65, B=66, C=67, D=68, E=69, F=70, G=71, H=72, I=73, J=74, ...,
  // W=87, X=88, Y=89, Z=90. Note that the second blob ends with 250,
  // which is outside the range of ASCII (i.e. it is not a valid UTF-8 string
  // and therefore not equivalent to any builder.append(string) call).
  var builderAbcd = google.gears.factory.create('beta.blobbuilder');
  builderAbcd.append('AB');
  builderAbcd.append(67);
  builderAbcd.append(68 + 2560);
  var blobAbcd = builderAbcd.getAsBlob();
  var bytesAbcd = blobAbcd.getBytes();
  assertEqual(4, blobAbcd.length);
  assertEqual(65, bytesAbcd[0]);
  assertEqual(66, bytesAbcd[1]);
  assertEqual(67, bytesAbcd[2]);
  assertEqual(68, bytesAbcd[3]);

  var builder = google.gears.factory.create('beta.blobbuilder');
  builder.append('W');
  builder.append('X');
  builder.append([
    89,
    'Z',
    blobAbcd,
    [bytesAbcd, 69, 'FG', 72]
  ]);
  builder.append('I');
  builder.append(250);
  var blob = builder.getAsBlob();
  var bytes = blob.getBytes();
  assertEqual(18, blob.length);

  assertEqual(87, bytes[0]);
  assertEqual(88, bytes[1]);
  assertEqual(89, bytes[2]);
  assertEqual(90, bytes[3]);
  assertEqual(65, bytes[4]);
  assertEqual(66, bytes[5]);
  assertEqual(67, bytes[6]);
  assertEqual(68, bytes[7]);
  assertEqual(65, bytes[8]);
  assertEqual(66, bytes[9]);
  assertEqual(67, bytes[10]);
  assertEqual(68, bytes[11]);
  assertEqual(69, bytes[12]);
  assertEqual(70, bytes[13]);
  assertEqual(71, bytes[14]);
  assertEqual(72, bytes[15]);
  assertEqual(73, bytes[16]);
  assertEqual(250, bytes[17]);
}

function testBlobBuilderAppendRecursiveArray() {
  var a = [1, 2];
  var b = [3, 4, a];
  a.push(b);
  var builder = google.gears.factory.create('beta.blobbuilder');
  assertError(function() {
    // The next line should not infinite-loop.
    builder.append(a);
  });
}

function testBlobBuilderAppendInvalidArrayIsAtomic() {
  var a = [1, 2, true];
  var b = [3, 4, a];
  var builder = google.gears.factory.create('beta.blobbuilder');
  builder.append(101);
  assertError(function() {
    // We expect an exception because booleans aren't appendable.
    builder.append(b);
  });
  builder.append(102);

  var blob = builder.getAsBlob();
  assertEqual(2, blob.length);
  assertEqual(101, blob.getBytes()[0]);
  assertEqual(102, blob.getBytes()[1]);
}

function testBlobBuilderCanCallGetAsBlobTwice() {
  var builder = google.gears.factory.create('beta.blobbuilder');
  builder.append(101);

  var blob1 = builder.getAsBlob();
  assertEqual(1, blob1.length);
  assertEqual(101, blob1.getBytes()[0]);

  builder.append(102);

  var blob2 = builder.getAsBlob();
  assertEqual(2, blob2.length);
  assertEqual(101, blob2.getBytes()[0]);
  assertEqual(102, blob2.getBytes()[1]);
}
