var wp = google.gears.workerPool;
wp.onmessage = function(text, sender, msg) {
  wp.sendMessage({
    echo: msg.body,
    masterOrigin: msg.origin,
    workerOrigin: wp.location.protocol + '//' + wp.location.host
  }, sender);
};
