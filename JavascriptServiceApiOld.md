summary Means of exporting an API between origins.

_DRAFT(shess) I'm still thinking about what I want, here.  For now, please send comments directly to me, once I'm further along I'll post to the google-gears-eng discussion list, hopefully this week._

# Introduction #

There are many use-cases, such as mash-ups or groups of related apps, where one app needs to expose functionality to other apps.  Here I propose a way to handle this in a secure fashion which can be managed by the service provider.

# Details #

Provide a means by which an origin can vend a service to clients.  The service should manifest as a JavaScript object which can be called in a natural fashion with methods defined by the service.  The code for the service executes synchronously in the client's thread, but in a distinct JavaScript context within the service's origin.

```
var service = google.gears.factory.create('beta.service', '1.0');
service.bind('www.todo.com');
var taskRef = service.create('tasklist', '1.0');
var tasks = taskRef.tasks();
taskRef.addTask('Take out the garbage');
```

'beta.service' is a new Gears API which creates an object representing a service.  bind() binds the service object to a particular service vendor.  The service origin is passed as a parameter - this is not the local code's origin.  Perhaps the default could be the current origin, so that you use the same service for your UI as other users use when using your service?

create() creates an object which acts as a handle to an API exported by the service.  In this case the 'tasklist' API.  Versioning is just like Gears, there could be multiple versions of an API supported by the service.  Clearly there could be reflect methods on the service object to enumerate the set of APIs that are exported.

The taskRef returned from create() is a synthetic object with a certain structure.  Calls to that object's methods are synchronous, just like regular JavaScript calls.

---

Under the covers, this creates a very minimal JavaScript context similar to the context created for WorkerPool.

The service provides the code to run in that context, either using API calls to set things up, or using something like a manifest.  Importantly, the client cannot provide this code, though it may require a means to get the service installed.

The service provider publishes an interface description (TBD), which the client software includes in its code to call things.  The interface should not be vended by the service directly, that would be a security issue.  Instead, the interface could be vended by the service-provider as source code on their website, meaning that it will be open to inspection.  I envision having an interface definition language (perhaps a JavaScript subset) which can be included in the client app's manifest, and included as part of the service's create() call.  Perhaps the client app could even have a "services" file which defines the set of services it uses, along with sugar to make it easy to snag objects corrosponding to given (service, API, version) tuples.

For security reasons, the core data transfer must be strings, so perhaps the core API could be simpler, with a JavaScript library to sit on top and make it pretty.

In operation, this looks like:

  * Client calls method on the API instance.
  * The method's implementation marshalls arguments, and calls a service API call to send a string message into the sandbox.
  * The handler in the sandbox receives the message and unmarshalls arguments.
  * The implementation of the method is called with those arguments.
  * The handler marshalls the results and returns them as a string.
  * The outer implementation unmarshalls the results, and returns them to the calling client.
  * The client moves along.

Basically, this is very similar to WorkerPool, except that the JavaScript context is in-thread, not in a separate thread.  There's no need to have parent-side message handlers, because the messages are entirely synchronous - the service API's implementation of "call()" would just return results.  There is only one possible message within the sandbox, because it has no independent existence - it cannot receive events asynchronously, because it is not executing except when the caller asks it to do something.

---

# Comparisons to some other suggestions around this problem. #

It has been suggested that we could allow an origin to expose its databases to other origins in controlled fashion.  That would make schema updates potentially painful, because all customers of the service exposed would have to agree on how to access the database.  This could be somewhat addressed by vending a JavaScript file encapsulating database access functions, which is likely to be somewhat ad-hoc.  Also, LocalServer cannot capture across origins, so an app needing this requires the service provider to capture itself.  Additionally, origins may wish to expose things other than data, for instance a photo site may expose API to capture thumbnails from your image archive for later display.  My opinion is that sharing databases is not a scalable solution.  In my proposal, we could perhaps provide a default set of APIs to proxy certain Gears APIs through the sandbox (so you could export a service exactly like the Database/ResultSet API if you like).

It has been suggested that we could allow an independent WorkerPool worker to provide such services.  In practice, that would be very similar to what I've described.  One failing is that it requires something which is executing independent of a page.  If this is created and terminated when pages are entered and left, then it's not much different than what I describe, except that everything is asynchronous, so some APIs may be a challenge to export.  If the service is controlling the WorkerPool independent of the page, then we need ways to manage that.  If needed, my suggestion could spin up WorkerPools to do background processing, with the existing termination semantics.

---

# Problems remaining #

XMLHttpRequest support.  In this type of system, we cannot have the client app fetch on behalf of the app in the sandbox, because of the same-origin policy.  We can't do the request in the sandbox because we can't receive async events.  The WorkerPool variant on this idea has that advantage, because it has an entire execution context which _could_ handle the async events.  A possible solution would be to surface API to let the client capture and forward events, though this obviously needs to take care to not expose any actual data directly to the client, except insofar as the service API exposes it.

Injecting code into the service provider and client needs to be nailed down.  Clearly the client and service have to manage the API code entirely in their own origin, but they _do_ have to be in agreement as to what they're doing.

---

# Additional Noodling #

This may be a bit like COM.  For instance, I imagine you wouldn't change an existing API, instead you'd just expose a new API or version of an API.  You could add new methods, but not change existing methods.  This is sort of like an in-process COM object.

It would be interesting to make this consistent with WorkerPool.  If we have an IDL to generate stubs, then it might make sense to have @sync and @async type calls.  WorkerPool calls are @async, service calls are @sync, but the marshalling and perhaps other glue is all the same.

That, in turn, could be extended to a general call-back-to-the-server API.  It can't really be synchronous (well, it could if it were in a WorkerPool!).

It would be interesting to have a concept of named WorkerPools.  Unnamed WorkerPools would be anonymous and per-tab.  Named WorkerPools could be shared between tabs.  Messages could be sent to workers in the pool, or to the parents (foreground threads) which asked for handles to the named pool.  This would allow an app to have a single synchronization worker for the database, shared between multiple UI instances.  In turn, the services API could then use a named WorkerPool to handle work for all clients, rather than having new pools for each client.  An unnamed pool would be anonymous, and different for each create.  Pools would be refcounted so that closing the last parent terminates the pool.

It may make sense to provide a sense of passing objects through to the sandbox, and having them be wrapped with a proxy to pass calls back out.  This can make the interfaces much richer.  In some RPC libraries, if you have an interface on a remote object server, you can pass it a local object conforming to a particular interface.  The implementation on the remote machine receives a remote object which speaks back to the original object passed.  An object passed through an RPC then back will be unwrapped and the local machine again talks to local objects.  An RPC handle passed from one remote service to another will be rewrapped for the new service (rather than wrapping a remote wrapper).  All of these probably have application, here.