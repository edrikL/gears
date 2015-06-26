# Introduction #

It's easier to use an API when it has a consistent style because you can guess how something works without thinking too hard. It's also easier to implement such APIs because  it lets you focus on your feature rather than grammar and stylist minutiae.

Let's grow this document into a set of easy to follow conventions for designing Gears APIs that are easy for developers and easy for us.


# Error Messages #

## All Errors ##

Rules:
  * Start with a capital letter, and end with a period.
  * Use correct grammar.
  * Avoid abbreviations (eg, "param", "proc").

Examples:
  * **Good:** First parameter must be a string.
  * _Bad:_ invalid param

## Specific Error Categories ##

### Internal errors ###

Description:
  * Are unexpected and should almost never happen (OS / hardware failure).
  * Are not things the developer could have avoided.

Rules:
**Use the GET\_INTERNAL\_ERROR\_MESSAGE() macro. It outputs something like "Internal Error. File gears/base/string\_utils.cc Line 325.".** Strive to create an error message in every unique place something could fail.

TODO:
**We should implement something like SetLastError() where the actual system call that failed can specify the error at that moment and callers don't need to worry about it.**

### Developer Errors ###

Description:
  * Are caused by a developer using Gears improperly.
  * Are things the developer could have fixed.

Rules:
  * Provide enough detail to identify problem.
  * Don't repeat things available in documentation.

Examples:
  * **Good:** Invalid database name: really neat name!.
  * _Bad:_ Invalid database name: really neat name!. Invalid character was '!'. Database names cannot contain any of these characters: !~$#@....
  * **Good:** Permission denied to capture resource: http://www.google.com/.
  * _Bad:_ Permission denied.