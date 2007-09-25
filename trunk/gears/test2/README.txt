How to write tests:

Gears unit tests are contained in the files listed in this directory. If you
add a new file, you need to update config.js to point to it.

The unit test runner can automatically run your tests in workers as well as
in the main HTML document. You should take advantage of this where possible.
To do so, just set the useWorker flag to true for your test file in config.js.

Unit tests have access to a variety of utility functions in runner/lang.js and
runner/assert.js.

The test runner also has support for asynchronous tests. To use it, set up your
test as usual in a test* function. After you start the asynchronous bit, call
scheduleCallback(fn, delayMs) with a callback function as the first argument
and a length of time to wait as the second. You can then check the results of
the asynchronous operation in the callback. See timer_tests.js for an example
of this.


How to run tests:

There is a GUI for unit testing in runner/gui.html. It must be run from an
http:// URL (not file://). If you don't have an easy way to do this on your
system, you can use the mini Python-based HTTP server in
tools\localhost_web_server.py.
