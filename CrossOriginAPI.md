**OWNER:** Chris Prince

# PROBLEM #

Google Gears has a strict same-origin security model.  However,
some web applications need to affect Gears resources in other origins.
OriginA may need to:
  * read/write a database in OriginB
  * capture a set of URLs in OriginB
  * access future Gears APIs in OriginB


# SOLUTION #

We can solve the problem by making a few incremental additions
to the WorkerPool class.  Those additions are noted below.

```
int  createWorker(fullScript)
int  createWorkerFromUrl(scriptUrl) // NEW METHOD
void allowCrossOrigin()             // NEW METHOD
void sendMessage(messageString, destWorkerId)
[write] callback onmessage(messageText, senderId, [object:{text,sender,origin}]) // NEW 3RD PARAM
```

Workers also need networking capabilities akin to XmlHttpRequest
if they want to implement certain services like data synchronization.
That API, which is not strictly part of cross-origin API usage,
should be addressed in a separate design document.


# EXAMPLE #

Assume mash-up.com wants to read and write tasks managed by task-list.com.
  1. task-list.com hosts workerpool-service.js, which it expects to be loaded by another origin.  That .js code uses:
    * LocalServer to make itself and any required resources available offline.
    * WorkerPool to respond to sendMessage() commands like "getAllTasks"
  1. mash-up.com calls createWorkerFromUrl() with the workerpool-service.js URL.
  1. The worker can access tast-list.com resources, as it runs in that origin.
  1. When mash-up.com calls sendMessage('getAllTasks'), task-list.com reads the database associated with task-list.com and returns the tasks.
    * Note that because only task-list.com directly accesses the database, the schema can change without requiring changes to mash-up.com.


# DETAILS #

**Security origin associated with each worker**
  * Every worker has a single origin associated with it.
  * That origin determines what Gears resources can be accessed.
  * A worker pool can contain workers with different origins.
  * The origin for each worker is determined as follows:
    * the "root" worker gets its origin from the page's URL
    * createWorker() gets the same origin as its parent
    * createWorkerFromUrl() gets its origin from the given URL

**Description of `createWorkerFromUrl()`**
  * The function returns immediately.
  * Gears asynchronously fetches the given URL and populates a new worker with the JavaScript code that was fetched.
  * Any error triggers the workerpool's 'onerror' callback.
  * Messages sent to the worker before it finishes initializing get queued.

**Description of `allowCrossOrigin()`**
  * A worker must call this method if it expects to be used across origins.
  * If a worker does not call this method:
    1. If the worker was created from a different origin (e.g. possibly by an attacker), all methods on `google.gears.factory` will fail until `allowCrossOrigin()` is called.

**Description of `srcOrigin`**
  * Receivers can examine this to decide whether to respond to each message.
  * The value is a normalized string representing the sender's origin:
> > SCHEME://DOMAIN[:PORT]
  * The port is omitted for standard ports (http port 80, https port 443)


# DESIGN Q&A #

**Q: Why require workers to call `allowCrossOrigin()`?**

For security, our mantra is "make the lazy case secure".
Requiring 'allowCrossOrigin()' helps protect against XSS attacks.
Simple code that doesn't think about cross-origin access cannot be attacked because the worker's Factory will be disabled, and so all Gears resources will be inaccessible.


**Q: Why not allow direct access to databases / etc in other origins?**

Two reasons:
  1. Every current and future Gears API would need to contain parameters for cross-origin access.  This is messy to use, and also messy to design.
  1. Simultaneous access across origins can require tricky synchronization.  Consider cross-origin database access when the schema needs to change.  Our cross-origin API pushes developers toward exposing JavaScript "services" which avoid this problem, by adding a layer of indirection.


**Q: Does this abandon the messaging API proposals?**

Not really.  Those proposals are just as relevant to this design as they
were before.  Gears provides a way to create JS engines (via CreateWorker),
and a way to communicate between them (via SendMessage).  The messaging API
proposals are a way to improve on SendMessage, which is still a possibility.