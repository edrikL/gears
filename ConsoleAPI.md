# Introduction #

This proposal supercedes the LoggingModule, however it is still worth reading for an overview of the proposed logging functionality.

The Console API will be implemented on top of the existing MessageObserver system.

# Proposed API #

```
class GearsConsole {
  // Default _types_ are 'debug', 'info', 'warn', 'error', however any arbitrary
  // string can used e.g. by developers extending the GearsConsole.
  // _args_ are interpolated into _message_, replacing occurrences of _%s_.
  log(String type, String message, Object[] args);
}
```

The benefit of consolidating individual logging methods such as Debug(), Error(), etc. into a single Log() method is that the log type can be specified arbitrarily which leaves the option open for developers to extend and build upon the GearsConsole using custom event types.

Console objects can be created via the factory whenever logging capabilities are needed (including inside WorkerPool processes).

```
var console = google.gears.factory.create('beta.console');
console.log('error', 'An error occurred! Some variables: %s, %s.', [1, 'Hello World']);

// In log viewer tool
 => (Error): 'An error occurred! Some variables: 1, Hello World.'
```

# Log Events #

Individual log events can be encapsulated by a LogEvent class, which will record a variety of information about the event.

```
class LogEvent {
  String message;    // Already interpolated with _args_
  String type;       // 'debug', 'info', 'warn', 'error'
  String sourceUrl;  // URL of page/script the log message came from
  Date date;         // Time/date of log item
}
```

# Logging Backends #

Since the Gears Console will use the existing MessageObserver system to notify listeners, "Logging Backends" can be created and registered with the observer topic to receive log events as they are created. The MessageObserver topic used by the Gears Console is:

_console:logstream-

<security\_origin>_

where _security\_origin_ is the scheme/host/port of the current page, as returned by EnvPageSecurityOrigin().url().

Each console object provides access to a single logging backend by default - a JSCallbackLoggingBackend - via the console's onlog property. This backend allows a JavaScript callback to be set, which will be called when new log events arrive in the log stream.

```
class GearsConsole {
  callback onlog(LogEvent log_event);
}

// JavaScript
console.onlog = function(log_event) { alert(log_event.message); };
```

Other logging backends can easily be created in future, for a variety of purposes including:
  * log persistence e.g. writing the log stream to a text file, database
  * remote or deployed application error reporting, e.g. send log stream to a remote server

# Log Viewer Tool #

The Log Viewer is a HTML/CSS/JS-based developer tool that displays log events for the current security origin in real-time.

It would ideally be packaged with Gears so that all developers would have access to it easily, but as with other developer tools it could be disabled by default (perhaps with a checkbox "Enable developer tools" in the Google Gears Settings dialog). The log viewer could be accessed by either browser chrome (a "Gears Developer Tools" sub-menu that appears in the browser's Tools menu when developer tools are enabled) or via a special URL (e.g. http://localhost/__gears__/log_viewer.html).

# Security #

The Gears Console follows a same-origin security policy.

The observer topic for each log stream has the security origin (scheme/host/port) embedded in it, and each console object is restricted to logging to the stream (and hence security origin) that it originated from.

Whilst there is nothing stopping a Logging Backend from registering to receive events from any arbitrary log stream, the fact that Logging Backends are compiled C++ shipped with Gears somewhat negates the problem.

The MessageObserver system does not yet expose an interface to JavaScript, so it is not currently possible to arbitrarily register observers with console topics, however if/when this interface _is_ implemented, it should ensure that the same-origin policy for observers is maintained.