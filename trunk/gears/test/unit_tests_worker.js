var wp = google.gears.workerPool;

wp.onmessage = function(body, sender) {
  wp.sendMessage('RE: ' + body, sender);  // echo
}
