/**
 * This class tracks the FileTestResults objects publishes all results
 * to the server when testing is done.
 * @constructor
 */
function ResultsPublisher() {
  bindMethods(this);
}

/**
 * All of the FileTestResults created during testing.
 */
ResultsPublisher.prototype.fileTestResults = [];

/**
 * Called if testing has timed out.
 */
ResultsPublisher.prototype.postbackTestResultsOnTimeout = function() {
	this.allAvailableTestsStarted = false;
	this.postbackTestResults();	
};

/**
 * Publish given parameters by posting them to a url.  Returns false
 * on failure to create a request.
 * @param url
 * @param parameters post data
 */
ResultsPublisher.prototype.publish = function(url, parameters) {
  var http_request = google.gears.factory.create('beta.httprequest', '1.0');
  if (!http_request) {
    alert('Cannot create Gears HTTP Request Obj');
    return false;
  }
  
  http_request.open('POST', url);
  http_request.setRequestHeader("Content-type", 
                                "application/x-www-form-urlencoded");
  http_request.send(parameters);
  return true;
};
   
/**
 * Encode all test results using json, then post them back to
 * the test server.
 * TODO: modify gui.html to add additional parameter to tell whether
 * to post back results to server
 */
ResultsPublisher.prototype.postbackTestResults = function() {
  testResults = [];  
  for (var i = 0, fileTest; i < this.fileTestResults.length; i++) {
    fileTest = this.fileTestResults[i];
    testResults.push(fileTest.toJson());
  }
  
  var postbackHash = { 
      gears_info: google.gears.factory.getBuildInfo(),
      browser_info: navigator.userAgent,
      url: location.href,
      results: testResults};
  this.publish(location.href, JSON.stringify(postbackHash));
};

/**
 * Add a new FileTestResults to local set, and set callback to notify
 * when tests are complete.
 */
ResultsPublisher.prototype.addFileTestResults = function(fileTestResults) {
  this.fileTestResults.push(fileTestResults);
  fileTestResults.onAllTestsComplete = this.handleAllTestsComplete;
};
