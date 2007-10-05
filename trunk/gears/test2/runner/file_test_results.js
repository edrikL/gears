// Copyright 2007, Google Inc.
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

/**
 * This class runs a test file and displays the results.
 * @constructor
 */
function FileTestResults() {
  bindMethods(this);
}

/**
 * The number of tests that have been started.
 */
FileTestResults.prototype.testsStarted = 0;

/**
 * The number of tests that are complete.
 */
FileTestResults.prototype.testsComplete = 0;

/**
 * Callback for when all tests have completed.
 */
FileTestResults.prototype.onAllTestsComplete = function() {};

/**
 * Starts the tests.
 * @param div The element to populate with the results.
 * @param testData The test file to run.
 */
FileTestResults.prototype.start = function(div, testData) {
  this.div_ = div;
  this.testData_ = testData;
  this.rowLookup_ = {};
  this.pendingLookup_ = {};

  var heading = document.createElement('h3');
  heading.appendChild(document.createTextNode(testData.relativePath));
  this.div_.appendChild(heading);

  var table = document.createElement('table');
  this.tbody_ = document.createElement('tbody');
  table.appendChild(this.tbody_);
  this.div_.appendChild(table);

  if (this.testData_.config.useIFrame) {
    this.startIFrameTests_();
  } else if (this.testData_.config.useWorker) {
    this.startWorkerTests_();
  }
}

/**
 * Helper to start the iframe tests.
 */
FileTestResults.prototype.startIFrameTests_ = function() {
  var iframeHost = new IFrameHost();
  iframeHost.onTestsLoaded = partial(this.handleTestsLoaded, false);
  iframeHost.onTestComplete = partial(this.handleTestComplete, false);
  iframeHost.onAsyncTestStart = partial(this.handleAsyncTestStart, false);
  iframeHost.onAllTestsComplete = partial(this.handleAllTestsComplete, false);
  iframeHost.load(this.testData_.relativePath);
};

/**
 * Helper to start the worker tests.
 */
FileTestResults.prototype.startWorkerTests_ = function() {
  var workerHost = new WorkerHost();
  workerHost.onTestsLoaded = partial(this.handleTestsLoaded, true);
  workerHost.onTestComplete = partial(this.handleTestComplete, true);
  workerHost.onAsyncTestStart = partial(this.handleAsyncTestStart, true);
  workerHost.onAllTestsComplete = partial(this.handleAllTestsComplete, true);
  workerHost.load(this.testData_.relativePath);
};

/**
 * Called when all tests have been loaded into one of the hosts.
 * @param isWorker Whether the tests were loaded into a worker.
 * @param success Whether the load succeeded.
 * @param errorMessage If success is false, the error message from the failure.
 */
FileTestResults.prototype.handleTestsLoaded = function(isWorker, success,
                                                       errorMessage) {
  if (!success) {
    var elm = document.createElement('div');
    elm.className = 'load-error';
    elm.appendChild(document.createTextNode(errorMessage));
    this.div_.insertBefore(elm, this.div_.firstChild);
  }
};

/**
 * Called when an individual test has completed.
 * @param isWorker Whether the test ran in a worker.
 * @param name The name of the test that is complete.
 * @param success Whether the test passed.
 * @param errorMessage If success is false, the error message from the failure.
 */
FileTestResults.prototype.handleTestComplete = function(isWorker, name, success,
                                                        errorMessage) {
  var pendingLookupKey = name + '_' + isWorker;
  var pendingRow = this.pendingLookup_[pendingLookupKey];

  if (!pendingRow) {
    this.addRow_(name, this.createResultRow_(isWorker, name, success,
                                             errorMessage));
    this.testsStarted++;
  } else {
    this.updateRow_(pendingRow, success, errorMessage);
  }

  this.testsComplete++;
};

/**
 * Called when an async test has started.
 * @param isWorker Whether the test is running in a worker.
 * @param name The name of the test that was started.
 */
FileTestResults.prototype.handleAsyncTestStart = function(isWorker, name) {
  this.testsStarted++;
  var pendingLookupKey = name + '_' + isWorker;
  var pendingRow = this.pendingLookup_[pendingLookupKey];
  if (!pendingRow) {
    var row = this.createResultRow_(isWorker, name);
    this.pendingLookup_[pendingLookupKey] = row;
    this.addRow_(name, row);
  }
};

/**
 * Called when all synchronous tests have been completed.
 * @param isWorker Whether the tests were run in a worker.
 */
FileTestResults.prototype.handleAllTestsComplete = function(isWorker) {
  if (!isWorker && this.testData_.config.useWorker) {
    this.startWorkerTests_();
  } else {
    this.onAllTestsComplete();
  }
};

/**
 * Adds a result to the UI.
 * @param name The name of the test to add a row for.
 * @param row The TR element to add.
 */
FileTestResults.prototype.addRow_ = function(name, row) {
  var previous = this.rowLookup_[name];

  if (!previous) {
    this.tbody_.appendChild(row);
    this.rowLookup_[name] = row;
  } else {
    this.tbody_.insertBefore(row, previous.nextSibling);
  }
};

/**
 * Creates a result row for the UI. If the last two parameters are not passed,
 * The row is displayed in 'pending' status.
 * @param isWorker Whether the row is for a worker test.
 * @param name The name of test to create a row for.
 * @param opt_success If the test is complete, whether it was successful.
 * @param opt_errorMessage If the test is complete and was a failure, the error
 * message to display.
 */
FileTestResults.prototype.createResultRow_ = function(isWorker, name,
                                                      opt_success,
                                                      opt_errorMessage) {
  var row = document.createElement('tr');
  var nameCell = document.createElement('td');
  var resultCell = document.createElement('td');

  if (isWorker) {
    name += ' (worker)';
  }

  nameCell.appendChild(document.createTextNode(name));

  if (opt_success == null) {
    resultCell.className = 'pending';
    resultCell.appendChild(document.createTextNode('pending...'));
  } else {
    this.updateResultCell_(resultCell, opt_success, opt_errorMessage);
  }

  row.appendChild(nameCell);
  row.appendChild(resultCell);

  return row;
};

/**
 * Updates a pending row in the UI to give it a final status.
 * @param row The TR element representing the test to update.
 * @param success Whether the test was successful.
 * @param errorMessage If the test failed, the error message to display.
 */
FileTestResults.prototype.updateRow_ = function(row, success, errorMessage) {
  var resultCell = row.childNodes[1];
  resultCell.removeChild(resultCell.firstChild);
  this.updateResultCell_(resultCell, success, errorMessage);
};

/**
 * Updates the result cell, which displays the 'OK' or error message.
 * @param cell The TD element containing the result.
 * @param success Whether the test passed.
 * @param errorMessage If the test failed, th error message to display.
 */
FileTestResults.prototype.updateResultCell_ = function(cell, success,
                                                       errorMessage) {
  if (success) {
    cell.className = 'success';
    cell.appendChild(document.createTextNode('OK'));
  } else {
    cell.className = 'failure';
    cell.appendChild(document.createTextNode(errorMessage));
  }
};
