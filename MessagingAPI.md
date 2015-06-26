# Introduction #

Gears WorkerPool has two pieces, the part about running a bit of JS asynchronously, and the part about trading messages with that JS.  This API may be composable from more basic bits.  The messaging bit could be used in other contexts, such as implementing something like WhatWG's postMessage().

# Details #

I propose a new API by example.  Creating an endpoint to receive
messages:

```
var port = google.gears.factory.create('beta.messageport', '1.0');
port.onmessage = function(port, msg, sender) {
  alert("message: " + msg);
};
port.listen("name");   // Omit for anonymous listener.
```

To send a message from elsewhere:

```
var port = google.gears.factory.create('beta.messageport', '1.0');
port.open("name");
port.sendMessage("hello there");
```

Setting up a sink worker (input-only) becomes:

```
// Startup the worker.
var workerPool = google.gears.factory.create('beta.workerpool', '1.0');
var workerPort = workerPool.create("worker.js");
workerPort.sendMessage("hello there");
```

A worker which just spools data to the database:

```
var db = ...;
var port = google.gears.workerPool.port;

while (1) {
  var a = port.getMessage();
  var msg = a[0];         // "hello there"
  var fromPort = a[1];    // undefined

  db.do('INSERT INTO t VALUES (?)', msg);
}
```

An obvious in/out worker is pretty easy:

```
var port = google.gears.workerPool.port;

while (1) {
  var a = port.getMessage();
  var msg = a[0];
  var fromPort = a[1];
  fromPort.sendMessage(msg, port);
}
```

This all enables certain flexibility:

  * One could have a pool of workers listening on a named port, pulling off work as available.
  * Shared workers don't require a specific parent.
  * Workers not associated with a particular window can listen on a named port.
  * This could be extended to allow cross-domain messages across iframes.
  * This could extend to allow in-domain messages across tabs or windows.
  * Synchronous calls can be implemented.

That last wants an example.  An synchronous call would look like:

```
function doit(port, msg) {
  var responsePort = google.gears.factory.create('beta.messageport', '1.0');
  responsePort.listen();

  port.sendMessage(msg, responsePort);
  var a = responsePort.getMessage()
  port.close();
  return a[0];
}
```

Cross-domain messages will need slight API mods both at the sender:

```
var port = google.gears.factory.create('beta.messageport', '1.0');
port.open("name", "my.domain.com");
port.sendMessage("hello there");
```

and at the receiver:

```
var port = google.gears.workerPool.port;
port.allowCrossOrigin();

while (1) {
  var a = port.getMessage();
  var msg = a[0];
  var fromPort = a[1];
  var fromOrigin = a[2];
  if (fromOrigin != "www.evil.com") {
    fromPort.sendMessage(msg, port);
  }
}
```

Alternately, perhaps it could be:

```
var port = google.gears.workerPool.port;
port.allowCrossOrigin(["www.good.com", "www.angelic.com"]);

while (1) {
  var a = port.getMessage();
  var msg = a[0];
  var fromPort = a[1];
  fromPort.sendMessage(msg, port);
}
```

---

Weird idea for use in WorkerPool:

```
var port = google.gears.factory.create('beta.messageport', '1.0');
port.onmessage = function(port, msg, sender) {
  console.log("message: " + msg);
};
port.listen();

var ts = google.gears.factory.create('beta.messageport', '1.0');
ts.open("timerservice");
ts.sendMessage(10000, port);  // Send port a message every 10 seconds.
```

Weirder:

```
function sleep(ms) {
  var port = google.gears.factory.create('beta.messageport', '1.0');
  port.listen();

  var ts = google.gears.factory.create('beta.messageport', '1.0');
  ts.open("timerservice");
  ts.sendMessage(ms, port);

  var a = port.getMessage();
  port.close();
  ts.close();
}
```

Another weird idea (synchronous version left to reader):

```
var port = google.gears.factory.create('beta.messageport', '1.0');
port.onmessage = function(port, msg, sender) {
  console.log("message: " + msg);
};
port.listen();

var ts = google.gears.factory.create('beta.messageport', '1.0');
ts.open("fetch");
ts.sendMessage("http://www.google.com/", port);
```