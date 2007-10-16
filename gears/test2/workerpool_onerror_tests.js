var globalErrorHandler = new GlobalErrorHandler();

// Class used to test that the onerror handler bubbles correctly.
function WorkerPoolBubbleTest(name, onerrorContent, shouldBubble) {
  this.fullName = 'WorkerPoolBubbleTest: ' + name;
  this.onerrorContent = onerrorContent;
  this.shouldBubble = shouldBubble;
  this.start();

  scheduleCallback(bind(this.checkResult, this), 50);
}

WorkerPoolBubbleTest.prototype.start = function() {
  this.wp = google.gears.factory.create('beta.workerpool', '1.1');
  var str = [];

  if (this.shouldBubble) {
    globalErrorHandler.expectError(this.fullName);
  }

  if (this.onerrorContent) {
    str.push('google.gears.workerPool.onerror = function() {');
    str.push(this.onerrorContent);
    str.push('}');
  }

  str.push('throw new Error("' + this.fullName + '")');
  this.wp.createWorker(str.join('\n'));
};

WorkerPoolBubbleTest.prototype.checkResult = function() {
  if (this.shouldBubble) {
    assert(
      globalErrorHandler.wasErrorReceived(this.fullName),
      'Expected global error for test "%s" was not thrown'.subs(this.fullName));
  } // else, the error was unexpected and was allowed to bubble up to the
    // browser error UI.
};


// Tests begin here

function testNoBubble() {
  new WorkerPoolBubbleTest('no bubble', 'return true;', false);
}

function testBubble1() {
  new WorkerPoolBubbleTest('bubble 1', 'return false;', true);
}

// TODO(aa): When coercion is implemented these next two should *not* bubble
// because they would get coerced to <true>.
function testBubble2() {
  new WorkerPoolBubbleTest('bubble 2', 'return 42;', true);
}

function testBubble3() {
  new WorkerPoolBubbleTest('bubble 3', 'return {};', true);
}

function testBubble4() {
  new WorkerPoolBubbleTest('bubble 4', 'return;', true);
}

function testBubble5() {
  new WorkerPoolBubbleTest('bubble 5', null, true);
}

function testNestedOuterError() {
  new WorkerPoolBubbleTest('nested error outer',
                           'throw new Error("nested error inner")', true);

  // This test generates one extra error.
  globalErrorHandler.expectError('nested error inner');
}

function testWorkerOnError() {
  // Test that onerror inside a worker gets called.
  var onerrorCalled = false;

  var wp = google.gears.factory.create('beta.workerpool', '1.1');
  wp.onmessage = function() {
    onerrorCalled = true;
  };

  var childId = wp.createWorker(
      ['var parentId;',
       'google.gears.workerPool.onmessage = function(m, senderId) {',
       'parentId = senderId;',
       'throw new Error("hello");',
       '}',
       'google.gears.workerPool.onerror = function() {',
       'google.gears.workerPool.sendMessage("", parentId);',
       'return true;',
       '}'].join('\n'));
  wp.sendMessage('hello', childId);

  scheduleCallback(function() {
    assert(onerrorCalled, 'Should have received onerror in child worker');
  }, 50);
}
