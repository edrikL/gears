# Problem #

There is no standard logging capability built into today's browsers. This makes working with web applications more difficult during development (developers revert to alert() or have to build their own logging systems) and in production (there is no way to see what went wrong with a deployed application).

The lack of a logging module is especially problematic when using Gears WorkerPools. See NewWorkerPoolFeatures for a previous proposal to add the console object to Gears workers to solve this problem. A Logging module would solve the problems that proposal meant to solve, as well as all the general problems listed above.


# Solution #

Implement a new Gears module, `beta.logger`. This module should have the following features:

  * `printf()`-style formatting.
  * Levels (eg debug, info, warn, error).
  * The logging output should be saved by default for some reasonable amount of time (perhaps 1 day) so that deployed applications can be debugged.
  * Log entries should have the URL of the page or worker they were created from saved with them.
  * A real-time viewer for logging should be available in the DevelopmentTools.

I think it makes sense to log directly to a Gears database. This will make it easier for developers to analyze and search logs. Also, this will provide additional internal stressing of the Gears database module.


# Proposed API #

Note: This API was blatantly stolen from Firebug, which is widely used among web developers in Firefox, and works pretty well. No reason to force people to relearn. There were some things in the Firebug API that didn't seem applicable to Gears. For the full API, see: http://getfirebug.com/console.html

```
class GearsLogger {
  // All these methods interpolate _args_ into _message_, replacing occurrences of _%s_.
  debug(String message, Object[] args);
  info(String message, Object[] args);
  warn(String message, Object[] args);
  error(String message, Object[] args);
}
```


# Questions #

What configuration options should there be for an application developer? For users?