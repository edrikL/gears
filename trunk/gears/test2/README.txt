How to write tests:

Gears unit tests are contained in the files listed in the test2/testcases/ 
directory. If you add a new file, you need to update test2/testcases/config.js 
to point to it.

The unit test runner can automatically run your tests in workers as well as
in the main HTML document. You should take advantage of this where possible.
To do so, just set the useWorker flag to true for your test file in config.js.

Unit tests have access to a variety of utility functions in test2/tester/lang.js 
and test2/tester/assert.js.

The test runner also has support for asynchronous tests. To use it, set up your
test as usual in a test* function. After you start the asynchronous bit, call
scheduleCallback(fn, delayMs) with a callback function as the first argument
and a length of time to wait as the second. You can then check the results of
the asynchronous operation in the callback. See timer_tests.js for an example
of this.


How to run tests:

Run python test2/runner/testwebserver.py
Then go to http://localhost:8001/runner/gui.html.
