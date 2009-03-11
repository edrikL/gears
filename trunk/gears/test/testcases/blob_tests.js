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
  assert(builder6.getAsBlob().length, 3 + 2);
}
