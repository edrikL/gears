# Introduction #

Proposal to add Gears-like workers to HTML5. Gears workers have two main benefits:

  * They simplify the programming model of complex applications by allowing computation and synchronous IO to be performed on a background thread.
  * They allow controlled cross-origin access, similar to PostMessage, but with higher performance due to the lack of a full rendering context.

This document proposes bringing these benefits toHTML5, with a few minor changes from the Gears API based on our experience.


# Proposed API #

```
// Allows the creation, and maybe someday enumeration and destruction of workers.
// Implemented by Window as well as by WorkerContext (for creation of nested workers).
interface WorkerManager {
  Worker createWorker(string url);
  // TODO: listWorkers()?
};

// The external interface to a worker.
// See: http://www.w3.org/TR/DOM-Level-2-Events/events.html#Events-EventTarget
interface Worker : WorkerPort, EventTarget {
};

// Represents a port to send messages into or out of a worker.
interface WorkerPort {
  // Sends a message. Can send any of the 'JSON types' as arguments: boolean,
  // number, string, array, and object.
  void sendMessage(object arguments);
};


// Exposed as a singleton inside workers.
interface WorkerContext : WorkerManager, EventTarget {
  // Used to send messages to the creator of the worker.
  readonly attribute WorkerPort parent;

  // Synchronously fetch and include a JavaScript file.
  void include(string url);

  // Allows this worker to receive messages from other origins. This is
  // necessary so that workers which don't intend to be used across origins can
  // be written naively without having to worry about every message.
  void allowCrossOrigin();
};


// The argument type for worker-message events.
interface WorkerEvent : Event {
  readonly attribute object data;
  readonly attribute string domain;
};
```

# Example #

`example.html:`
```
var worker = window.createWorker("worker.js");
worker.addEventListener("worker-message", function(e) {
  e.arguments.results.forEach(function(obj) {
    console.log("weather in " + obj.city + ": " + obj.weather);
  });
}, false);

worker.sendMessage({
  operation: "lookupWeather",
  cities: [
    "New York, NY",
    "Paris, France",
    "Tokyo, Japan"
  ]
});
```

`worker.js:`
```
workerContext.addEventListener("worker-message", function(e) {
  if (e.arguments.operation == "lookupWeather") {
    var results = [];

    e.arguments.cities.forEach(function(cityName) {
      var req = new XMLHttpRequest();
      req.open("GET", "lookupWeather?city=" + cityName,
               false); // synchronous
      req.send(null);

      results.push({
        city: cityName,
        weather: req.responseText
      });
    });

    workerContext.parent.sendMessage(results);
  }
}, false);
```


# Appendix A: APIs available inside workers #

The following existing APIs from the DOM Window and Document interfaces
should also be available inside workers.

  * XMLHttpRequest
  * Database
  * document.cookie
  * window.location (doesn't necessarily need to be writable though)
  * window.navigator
  * ApplicationCache // TODO: make sure this makes sense inside a worker
  * WindowTimers (setTimeout, setInterval, etc)

Additionally, the proprietary Console object that many browsers make
available should be standardized and made available inside workers.

_TODO: What object should these APIs go on? I can buy WorkerContext for most of them, except XMLHttpRequest and Console for some reason._


# Appendix B: Worker Lifetime #

Workers must live at least as long as code inside them is executing, including any waiting asynchronous callbacks. Workers must be destroyed when unloading their owning document, even if code is executing.


# Appendix C: Worker Objects instead of Worker IDs #

The current Gears WorkerPool deals in _Worker IDs_, not _Worker_ objects. The benefit of this design is that any worker can talk to any other worker in the same pool knowing only its ID. The downside is that the common case of a child worker becomes more complex:
  * The parent must maintain two variables (one for the ID and for the pool)
  * Responding to messages from the child worker requires extra work to save the parent ID from the first received message
  * Children workers cannot send the first message because they don't know their parent worker's ID until the first received message

This proposal shifts this balance a little bit. Performing direct communication between child workers is now impossible, but you _can_ arrange simulations with trees of workers if necessary. On the other hand, the common case of creating a child worker requires only one variables -- the reference to the worker. Sending messages from child to parent is trivial and can be done without waiting for a message from the parent. As a bonus, the API also becomes more idiomatic JavaScript.

Finally, having separate Worker objects has another benefit: it defines a lifetime for the worker threads. Worker threads can be shut down when they are done running code and when there are no longer any references to them. You can do the same thing with WorkerPool objects in the current Gears design, but one long-running worker prevents the entire pool from getting cleaned up.