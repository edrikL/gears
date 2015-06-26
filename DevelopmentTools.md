# Problem #

It's hard to build Gears-enabled applications because we don't provide good development tools. Here are some of the most necessary tools:

  * Database
    * List databases per origin
    * Create new
    * Delete
    * Interactive DB command line (can just use existing /sdk/tools/dbquery.html)
  * LocalServer
    * List ResourceStores (and ManagedResourceStores) per origin
    * ResourceStore and ManagedResourceStore status (last error, update history, etc)
    * command line (like db command line, but pointed at localserver DBs)
  * WorkerPool
    * Show running workers
    * Interactive JS prompt to run JS inside a worker
    * Interactive prompt to send messages to a worker
  * Logging (requires LoggingModule)
    * Show logging in real time as it happens
    * Show historical logging
    * Sort/filter by origin/page of source page

In order to be really useful these tools should be available to every application automatically without any extra work, similar to how the JavaScript console and DOM inspector are available to every application.

Also, it's likely that we will want to add to this tool suite over time, so the solution should be extensible.


# Solution #

We can implement Gears tools as HTML/JavaScript applications that use the Gears APIs themselves. In order to make the tools available to every application, we should package them with Gears and serve them automatically from a special set of URLs of the form: `<origin>/__gears__/<tool_name>`. For example:

```
http://www.rememberthemilk.com/__gears__/databases.html
http://www.rememberthemilk.com/__gears__/show_database.html?mydb
... etc ...
```

Serving the Gears tools from within the origin they are manipulating means that we don't have to add any special debug override to the Gears security model. The tools can manipulate Gears resources because they are being run inside that origin.

There is a minor chance of the `__gears__` keyword conflicting with an existing application. We can make this configurable if people think this is a big deal.

We should reuse the existing Gears APIs to implement these tools. In the cases where there is no Gears API to provide a feature, it would be preferable to add the API. For example, there is currently no API to list the databases for a given origin. But there is an outstanding feature request for such an API. We should implement that API it instead of adding something special just for these tools.


# Security #

One risk is that we push a buggy tool which introduces an XSS into other people's applications. To protect against this, users should have to specifically enable the Gears toolset via some sort of preference. This will protect the vast majority of users (all non-developers) from being affected by any problems in them.