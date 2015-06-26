**Author:** Chris Prince

# PROBLEM #

Gears provides parallel execution of JavaScript through the [worker object](http://code.google.com/apis/gears/api_workerpool.html).  Workers also support [cross-origin communication](CrossOriginAPI.md).

However, two additional features are frequently requested:
  * **Global namespacing** - for singleton workers, to which multiple pages can connect.
  * **Detached execution** - for running a script without any browser windows open.


# SOLUTION #

We can enable global namespacing and detached execution by making small, incremental changes to the WorkerPool module.

The API changes are summarized here.  Design details follow.

```
interface WorkerPool
  // NEW: Adds WorkerOptions.  Return value may now be integer|GearsEndpoint.
  integer|GearsEndpoint createWorker(text, [WorkerOptions])
  integer|GearsEndpoint createWorkerFromUrl(url, [WorkerOptions])

  void sendMessage(msg, dest)  // NEW: 'dest' may now be integer|GearsEndpoint.
  callback onmessage(m)    // NEW: 'm.sender' may now be integer|GearsEndpoint.
  callback onerror(e)

  void allowCrossOrigin()

// NEW INTERFACES:
interface WorkerOptions
  boolean openGlobally
  boolean runDetached
  string  globalName

interface GearsEndpoint
  // Opaque handle, similar to GearsBlob.  No attributes in V1.
```


# EXAMPLES #

Assume an offline-enabled site wants to synchronize data with a server.  If multiple windows are open, the site only wants one sync task running.  Using a global worker solves this problem:

```
wp.createWorkerFromUrl('sync.js', {openGlobally: true});
```

Now assume the site wants the sync process to continue even when all browser windows are closed.  It can do that by changing the call to:

```
wp.createWorkerFromUrl('sync.js', {openGlobally: true, runDetached: true});
```

Assume a different site would like to communicate across all its open tabs.  A global worker enables this as well:

```
var w = wp.createWorkerFromUrl('site-comm.js', {openGlobally: true});
wp.sendMessage(['addListener'], w);  // a site-specific protocol
...
wp.sendMessage(['broadcast', data], w);
...
wp.sendMessage(['removeListener'], w);
...
```


# DETAILS #

Two new concepts are being introduced: worker **scope** (global or not) and worker **lifetime** (detached or not).

These concepts are best considered separately.  A site may use workers that are global but not detached.

## Scope ##

When the `openGlobally` option is true, createWorker() and createWorkerFromUrl() first check if a worker with the given identity exists.  If it does not exist, the worker is created.  The worker is returned as a `GearsEndpoint`.

URL-based global workers are uniquely identified by their `url` parameter, plus the `globalName` option, which may be empty.

Text-based global workers are uniquely identified by the security origin of the calling site, plus the `globalName` option, which may be empty.  (Note that text-based global workers cannot be opened cross-origin.)

## Lifetime ##

When the `runDetached` option is true, a worker may continue running after the creating page is closed.  In fact, it may continue running even after the browser is closed.

When the page that creates a detached worker is closed, Gears shows a notification to alert the user. The notification also lets the user control whether the detached worker continues.

It is an error to request `runDetached` without also specifying `openGlobally`.  (Detached non-global workers are not an interesting use case.  Also, they would be unreachable by other workers.)

Note: global, _non_-detached workers should exit when they (a) have finished processing all messages, and (b) are not referenced by any in-page (non-global) or detached workers.
References from other global, non-detached workers do _not_ count; otherwise unintended cycles can occur.


## Worker Environment ##

Beyond the basic concepts of scope and lifetime, there are interesting questions about the environment in which global and detached workers run.

**Cookies**

Cookies pose an interesting challenge for global workers (detached or not).  A global worker cannot automatically _copy_ the cookie store of its creator.  The "create or open" model for global workers makes copy semantics unpredictable.  And while non-detached workers could _share_ cookies with the browser, detached workers could not; this inconsistency is undesirable.

One solution is to run each global worker in a separate, initially empty cookie environment.  Cookies can be set by servers in response to network requests made global workers.  We may also want to provide the worker equivalent of `document.cookie`.

A nice property of separate cookie environments is that workers can run on behalf of multiple accounts simultaneously.  It becomes easy to have multiple IM accounts listening for chat messages, or multiple email accounts synchronizing user data.

**UI Capabilities**

Global workers might not have an associated HTML page.  But they still may need to interact with the user, for things like authentication.

In these cases, workers should show a notification to get the user's attention.  Notifications can contain links that open HTML pages in the browser.

**Other Capabilities**

All Gears APIs should be available in global workers.

Additionally, global workers should have the equivalent of `document.location.reload()`.  JavaScript files sometimes change while a web application is running, and some web applications need to detect this and force a reload.  (Note: reload() is better than exiting the worker and letting the next caller recreate it, because GearsEndpoint objects do not get invalidated.)

Internally, global workers must be associated with a particular user data directory, so the correct Gears databases and other content can be accessed.  This data directory would not be exposed to applications.


# DESIGN Q&A #

**Q: How does this compare to Ian Hickson's proposal for [workers](http://hixie.ch/specs/dom/workers/0.9) and [endpoints](http://hixie.ch/specs/dom/messages/0.9)?**

Gears has a few goals not addressed by those specs:
  * cross-origin workers
  * detached workers
  * passing non-string values (including Blob and Endpoint) between workers

There are also compatibility goals, including:
  * backward compatibility with text-based workers
  * backward compatibility with existing naming conventions

We should be able to reconcile these differences.  And Gears should implement the unified spec as WorkerPool2.  But until then, compatibility with WorkerPool should be maintained. Breaking compatibility twice is not worth the cost.


**Q: Why doesn't GearsEndpoint have a sendMessage() method?**

It almost certainly will in WorkerPool2.  And it could today as well.  But then the API would be inconsistent, and thus confusing:
```
var w1 = wp.createWorkerFromUrl('foo.js');
var w2 = wp.createWorkerFromUrl('bar.js', {openGlobally: true});
...
wp.sendMessage(message, w1);
...
w2.sendmessage(message);
```

**Q: Why do callers decide whether a worker is global, rather than the worker itself?**

Callers may want to open the same script multiple times, as both a global worker and a non-global worker.  For example:
```
wp.createWorkerFromUrl("http://a.com/dataManager.js");  // manages secret data
wp.createWorkerFromUrl("http://a.com/dataManager.js",
                       {openGlobally: true});           // manages shared data
```


**Q1: Why is the full URL used for worker identity, rather than just the security origin?**

**Q2: Why aren't there simple (or "naked") names in the global namespace?**

It would be a security hole if sites could map scripts to (name, origin) pairs.  Similar logic applies to naked global names.

Consider this scenario:
  * `good.com` loads `helper.com/login.js` as ("helper.com", "login").
  * But if `evil.com` runs first, it can map ("helper.com", "login") to `helper.com/echo.js`.
  * When `good.com` runs, it will be sending sensitive information to the wrong script!

**Q: Why add GearsEndpoint?  Why not continue using integers?**

A global worker can be a member of multiple WorkerPools.  So integers are no longer unique identifiers.  And even if they were unique, they would not be secure against "scanning" attacks from a rogue global worker.

Adding a GearsEndpoint object solves these problems, while still allowing worker handles to be passed between workers.  ("Hi worker1, you can use 

&lt;endpoint&gt;

 for all your background database requests.")

**Q: Where can I find more information on the design discussions?**

The email thread that accompanies this design is at: http://groups.google.com/group/gears-eng/browse_thread/thread/78022ca19d24cf82/