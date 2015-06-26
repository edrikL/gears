# Introduction #

If you're new to Gears development, getting your head around a new codebase can be pretty daunting. It helps if you have something specific that you want to implement, but without much Gears experience, it can be difficult to estimate, a priori, how hard a project will be. You might end up knee-deep in your debugger without really knowing what you're looking for.

Here are some relatively easy-to-implement ideas that might be good starter projects for new Gears engineers. Hopefully they're small enough to be completed quickly, big enough to give you a taste of how various pieces inside Gears fit together, useful enough to be worth rolling into the main codebase, and fun enough to want to spend your time on.


# Projects #

## Parsing JSON ##

Proposed on July 2008.

[JSON](http://en.wikipedia.org/wiki/JSON) is a popular format for server responses that are to be consumed by JavaScript. For responses from trusted servers, you can `eval()` the string to get a JavaScript object, or you can use the JSONP trick to evaluate the response inside a `<script>` tag. But, for untrusted servers, neither approach is secure, since it allows foreign code to run in your page (and then, for example, issue requests with your cookies).

json.org provides an all-JavaScript library to parse untrusted JSON strings, but a C++ implementation (with JS bindings) would presumably be faster, and speed might matter when parsing the JSON that represents your mail inbox.

Gears could provide an API to parse a JSON-formatted string and return the appropriate JavaScript. For example:
```
factory.parseJson('3');  // returns the number 3
factory.parseJson('[true, {\"foo\": 3, \"bar\": \"\"}]');  // returns an array with two elements
factory.parseJson('function() {alert(\"Not JSON!\");}');  // throws an Exception
```

The key question is whether or not the performance gain is big enough that we should care. Benchmark evidence will be as important as actual working code.

WebKit's Bugzilla has [some working code](https://bugs.webkit.org/show_bug.cgi?id=20031) that we could possibly re-use. There's also a [WHAT-WG mail thread](http://lists.whatwg.org/pipermail/whatwg-whatwg.org/2008-June/015078.html) that might be worth reading.


# Other Sources for Project Ideas #

openajax.org has a [wiki page](http://www.openajax.org/runtime/wiki/Feature_Requests_Summary_Page) tallying oft-requested features for improving AJAX. Most of these are too large to be considered a starter project, but there is certainly plenty of inspiration to be found there.