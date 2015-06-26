# Introduction #

I'm proposing a refinement of the WorkerPool api that I think will make it a little easier to use by making it object-oriented. It also thinks ahead to shared, possibly out-of-process workers.

I believe that this makes the easy things slightly easier (by not having to track the worker ids) while still allowing the hard things.


# Details #

The existing API works like this:

```
// main code
var wp = factory.create("beta.workerpool", "1.0");
var workerId = wp.createWorker("workercodegoeshere");
wp.onmessage = function(message, senderId) {
  alert(message + " from " + senderId);
};
wp.sendMessage("hello", workerId);

// worker code
google.gears.workerPool.onmessage = function(message, senderId) {
  google.gears.workerPool.sendMessage("goodbye", senderId);
};
```


A more object-oriented version of this API would work like this:

```
// main code
var worker = factory.create("beta.worker", "1.0");
worker.create("workercodegoeshere");
worker.onmessage = function(message) {
  alert(message);
};
worker.sendMessage("hello");

// worker code
google.gears.currentWorker.onmessage = function(message) {
  google.gears.currentWorker.sendMessage("goodbye");
};
```

The main functionality that is lost is that workers cannot communicate amongst themselves, only back up to the parent. I personally don't think that's a big deal, it just sounds like rope to me :-). But you can get that for free while thinking ahead to shared, possibly out-of-process workers:

```
// main code
var worker1 = factory.create("beta.worker", "1.0");
// If a worker named "worker1" already exists, it is returned. Otherwise, a new worker
// is created.
worker1.openOrCreate("worker1", "worker1codegoeshere");

var worker2 = factory.create("beta.worker", "1.0");
worker2.openOrCreate("worker2", "worker2codegoeshere");

worker1.sendMessage("hello");

// worker1 code
google.gears.workerParent.onmessage = function(message) {
  var worker2 = google.gears.factory.create("worker", "1.0");
  worker2.open("worker2");
  worker2.sendMessage("hello worker2!");
};
```