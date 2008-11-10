// This file is used to test that cross-origin workers served with incorrect
// content types are not loaded by Gears. Since this file's extension is .js,
// it will be served by our test webserver with the content-type
// 'text/javascript', instead of the 'application/x-gears-worker' that is
// required.
//
// This should cause an error before the worker is even parsed, which means
// the contents of this file is not important.
