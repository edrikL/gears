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

var desktop = google.gears.factory.create('beta.desktop');

function testRegisterDropTargetDoesntStripExistingAttributes() {
  // This test cannot run in a worker.
  if (typeof document == 'undefined') {
    return;
  }

  // First, make sure we start with a clean slate, i.e. there is no DIV with
  // id 'dropTargetId'.
  assertNull(document.getElementById('dropTargetId'));

  // Now, we create such a DIV, and assert that we can see it. This section
  // isn't testing the Drag and Drop API per se, and is more a sanity check
  // that regular DOM manipulations work, but it does build up to the third
  // section of this test, which ensures that the Drag and Drop API does not
  // interfere with regular DOM manipulations, as was the case on IE prior to
  // r2929 (OCL=8653382).
  var div = document.createElement('div');
  div.id = 'dropTargetId';
  div.appendChild(document.createTextNode('Lorem ipsum.')); 
  document.body.appendChild(div); 
  assertEqual('dropTargetId', div.id);
  assertNotEqual(null, document.getElementById('dropTargetId'));

  // Finally, we registerDropTarget that DIV, and assert that we can still
  // see that DIV.
  desktop.registerDropTarget(div, {
    'ondragenter': function(context) {},
    'ondragover': function(context) {},
    'ondragleave': function(context) {},
    'ondrop': function(context) {}
  });
  assertEqual('dropTargetId', div.id);
  assertNotEqual(null, document.getElementById('dropTargetId'));
}

function testRegisterDropTargetThrowsInAWorker() {
  // This test should only run in a worker.
  if (typeof document != 'undefined') {
    return;
  }

  assertError(function() {
    var fakeDiv = {};
    desktop.registerDropTarget(fakeDiv, {
      'ondragenter': function(context) {},
      'ondragover': function(context) {},
      'ondragleave': function(context) {},
      'ondrop': function(context) {}
    });
  }, 'registerDropTarget is not supported in workers.');
}
