// Helper used to test that the onerror handler bubbles correctly.
function workerPoolBubbleTest(name, onerrorContent, shouldBubble, firstError) {
  var fullName = 'WorkerPoolBubbleTest: ' + name;

  var wp = google.gears.factory.create('beta.workerpool');
  // Have to create a reference to the wp so that it doesn't get gc'd early.
  workerPoolBubbleTest.wp = wp;

  if (shouldBubble) {
    var errors = [fullName];
    if (firstError) {
      errors.unshift(firstError);
    }
    waitForGlobalErrors(errors);
  }

  var str = [];

  if (onerrorContent) {
    str.push('google.gears.workerPool.onerror = function() {');
    str.push(onerrorContent);
    str.push('}');
  }

  str.push('throw new Error("' + fullName + '")');
  wp.createWorker(str.join('\n'));
};

// Tests begin here

function testNoBubble() {
  workerPoolBubbleTest('no bubble', 'return true;', false, null);
}

function testBubble1() {
  workerPoolBubbleTest('bubble 1', 'return false;', true, null);
}

// These next two should *not* bubble because they get coerced to <true>.
function testBubble2() {
  workerPoolBubbleTest('bubble 2', 'return 42;', false, null);
}

function testBubble3() {
  workerPoolBubbleTest('bubble 3', 'return {};', false, null);
}

function testBubble4() {
  workerPoolBubbleTest('bubble 4', 'return;', true, null);
}

function testBubble5() {
  workerPoolBubbleTest('bubble 5', null, true, null);
}

function testNestedOuterError() {
  workerPoolBubbleTest('nested error outer',
                       'throw new Error("nested error inner")', true,
                       'nested error inner');
}

function testWorkerOnError() {
  // Test that onerror inside a worker gets called.
  // NOTE: There was a bug in IE where onerror could get called multiple times
  // when the error occurred inside an eval() call. This test detects that as
  // well as whether onerror is working at all.
  startAsync();

  var wp = google.gears.factory.create('beta.workerpool');
  var onErrorCount = 0;

  wp.onmessage = function() {
    onErrorCount++;
  };

  var childId = wp.createWorker(
      ['var parentId;',
       'google.gears.workerPool.onmessage = function(m, senderId) {',
       'parentId = senderId;',
       'eval(\'throw new Error("hello");\')',
       '}',
       'google.gears.workerPool.onerror = function() {',
       'google.gears.workerPool.sendMessage("", parentId);',
       'return true;',
       '}'].join('\n'));
  wp.sendMessage('hello', childId);

  timer.setTimeout(function() {
    assertEqual(1, onErrorCount, 'onerror called wrong number of times');
    completeAsync();
  }, 100);
}
