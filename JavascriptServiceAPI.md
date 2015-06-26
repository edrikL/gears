_DRAFT(shess) I'm still thinking about what I want, here.  For now, please send comments directly to me, once I'm further along I'll post to the google-gears-eng discussion list, hopefully this week._

_See also JavascriptServiceApiOld, my original stream-of-conciousness on this._

# Introduction #

There are many use-cases, such as mash-ups or groups of related apps, where one app needs to expose functionality to other apps.  Here I propose a way to handle this in a secure fashion which can be managed by the service provider.

# Details #

The basic idea is to provide a container for a JS security context, along with a means of passing requests into the container and receiving results from the container.  An example usage might look like:

```
var service google.gears.factory.create('beta.service', '1.0');
service.open('http://www.todo.com/tasklist.js);

var tasks = service.tasks();
var task;

while (task = tasks.shift()) {
  var title = service.getTitle(task);
  console.log("title: " + title);
}

service.addTask("Take out the garbage");
```

The service is known by the manifest which it uses to describe itself.  Multiple versions of the service can be surfaced using multiple manifests, which may include many of the same files.  The service in `tasklist.js` might be implemented like:

```
// Setup global db.
var db = google.gears.factory.create('beta.database', '1.0');
db.open('taskdb');

google.gears.service.register("tasks", function() {
  return db.selectColumn('SELECT taskid FROM tasks');
});

google.gears.service.register("getTitle", function(task) {
  return db.selectField('SELECT title FROM tasks WHERE taskid = ?', [task]);
});

google.gears.service.register("addTask", function(title) {
  db.do('INSERT INTO tasks (taskid, title) VALUES (null, ?)', [title]);
  return db.lastInsertRowId;
});

google.gears.service.register("capture", function() {
  var localServer = google.gears.factory.create('beta.localserver', '1.0');

  var store = localServer.createManagedStore('tasklist');
  store.manifestUrl = 'tasklist.manifest';
  store.checkForUpdate();
});
```

All of that runs within the `www.todo.com` origin, so the database and managed store access happen entirely within that domain.  The service could naturally spin up a WorkerPool instance to do background synchronization work.

---

The main reason I've phrased the proposal in this way is because I think it's important to allow services to export synchronous APIs.  For some services, this will of course be impossible.  But for those which can be synchronous, the ease of use is very greatly improved (async APIs are generally hard!).  Just as WorkerPool can be used to make a synchronous API like Database async, it could be used to make service async (either the client could manage the service from a worker, or the service could manage a background worker).

In the limit, with synchronous support, one could even export APIs identical to Database and ResultSet.

There are problems, though:

  * The capture() function above is hand-wavey, it really needs a means of registering async callbacks.  This kind of wart begins to argue for doing this in terms of a WorkerPool worker rather than something new.  MessagingAPI would make this easy.  _Alternately, ManagedResourceStore could have a synchronous API._

  * Does service code get injected into the client?  One model would be to have service.js be the service-side implementation, and a parallel service\_api.js be the client-side implementation.  The client would include service\_api.js as a script directly or by copying to their own site, so the client could be confident that the service wasn't doing anything bad in the client security context.