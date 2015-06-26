**OWNER:** Aaron Boodman

# Introduction #

WorkerPool has several issues today that make it harder to get started with compared to the other Gears APIs.

  * Errors do not get reported anywhere by default, making it hard to get working code at first.
  * The code to run in the worker has to get passed in as a string - most AJAX applications do not have access to their code as a string, and this also makes it hard to have the worker have dependencies.
  * There's no debugger or logging support in most browsers.
  * You need to set google.gears.workerPool.onmessage or you get an error. This isn't intuitive. Most people's first attempt seems to be to put "hello?" or something simple in the worker code.

This proposal addresses these issues by recommending the following changes:

  * Add WorkerPool.load();
  * Add WorkerPool.onerror;
  * Support for the defacto standard Console object
  * Remove the requirement to set google.gears.workerPool.onmessage inside workers

Note that there are many other features (!XHR, setTimeout, !RPC-style  messages, support for passing rich datatypes, etc, etc, etc) that would be very useful to add to WorkerPool. I think the features listed in this proposal are the highest priority because they enable other features to be implemented in JavaScript with minimum pain.

For example, if you can load JS libraries into Workers, it's not hard to add your own JSON encoding and decoding to either side of WorkerPool to enable passing rich datatypes through `sendMessage`. And improving the bootstrap and debugging experience will generally makes building more complex mechanisms on top of WorkerPool easier.

# WorkerPool.load() #

You should be able to load the contents of a JavaScript file to start a Worker. This is easier for most web developers since it is similar to the way that windows are loaded. Most javascript programs are split into many separate files, so there also needs to be a way for workers to include additional javascript files synchronously.

```
// parent code
var wp = google.gears.factory.create("beta.workerpool", "1.1");
var workerId = wp.load(["foo.js", "bar.js", "baz.js"]);
// load() is async. sendMessage's that happen before the worker is loaded are queued
// and send when load is complete.
wp.sendMessage("muah!", workerId);

// worker code
google.gears.workerContext.include("library1.js");
```


# WorkerPool.onerror #

WorkerPool should expose an optional onerror callback property.

If this function is not set, all errors that happen inside a worker should be re-thrown globally into the window containing the worker (the same thing that happens when you do `window.setTimeout(function() { throw new Error('oops') }, 0)`.

If the function is set, then all errors that happen inside the worker are routed to the function, which can be used for logging, etc.

```
var wp = google.gears.factory.create("beta.workerpool", "1.1");
// Optional property. If not set, error is thrown globally.
wp.onerror = function(e) {
  myLogger.logWorkerError(e);
};
var workerId = wp.load(["buggy.js"]);
```


# Support the defacto standard Console object in Workers #

When [Firebug](http://www.getfirebug.com/) is installed, it adds a very useful object called [console](http://www.getfirebug.com/console.html) to the global javascript scope. This object is quickly becoming a defacto standard for logging and debugging in various libraries and toolkits and we should support it too!

So you could do things like:

```
// parent code
var wp = google.gears.factory.create("beta.workerpool", "1.1");
wp.load(["test.js"]);
wp.sendMessage("hello!");

// test.js
google.gears.workerPool.onmessage = function(message, senderId) {
  console.log("worker got message %s from sender %s", message, sender);
```

An easy way for Gears to implement this API is to just delegate to whatever is defined as `console` in the containing window's global scope.

That way we would just magically work with whatever is implementing the interface, be it Firebug, or some JS library.


# Remove the requirement to set onmessage in workers #

Even though a worker that doesn't receive messages has limited functionality, it's still userful. You can imagine one-off workers that already know what they need to do. They don't require any messages.

Together with `console` support, above, this reduces the minimum observable worker example to:

```
// parent code
var wp = google.gears.factory.create("beta.workerpool", "1.1");
wp.load(["test.js"]);

// test.js
console.log("hello!");
```

Developers can then add-in message handling as necessary.