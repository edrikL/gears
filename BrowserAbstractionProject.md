# PROBLEM #

Since the launch of Gears, only Internet Explorer and Firefox have always had the latest features.  Safari support has lagged behind.  Other browsers, like Windows Mobile, have not been supported at all.

# SOLUTION #

We can fix the problem by doing two things:
  1. Formalize the browser abstractions that exist today.  This will make it easier to support new browsers.
  1. Have only one (shared) implementation of each Gears module.  This will ensure all browsers get the latest features.

The `/base` directory will contain all browser-specific underpinnings.  Each abstraction will consist of a shared interface, and some combination of shared code and browser-specific code.  For example:

```
   /base/file.h      // single interface
   /base/file.cc     // code that is shared
   /base/file_ie.cc  // code that is IE-specific
   /base/file_ff.cc  // code that is Firefox-specific
```

Each Gears module will live in its own directory.  Modules will have a single implementation, based on browser-neutral types (`int`, `bool`, `string16`).  There will also be a browser-specific interface definition file.

```
   /database/database.h       // single interface
   /database/database.cc      // single implementation
   /database/database_ie.idl  // interface definition for IE
   /database/database_ff.idl  // interface definition for Firefox
```

Code for each module should depend only on `/base` and not on other modules.

# DETAILS #

Some initial work items come to mind:

  1. Look through `/base/common` to understand what abstractions exist today.  Understand whether these abstractions can be implemented on additional browsers.
  1. Move some abstractions into `/base`.  These examples come to mind:
    * http\_request
    * http\_constants
  1. Move non-abstractions out of `/base`.  These examples come to mind:
    * factory
  1. Make any changes required to share a single module (perhaps `Database`?) across Firefox and IE.  Known work items:
    * implement JsParamFetcher for IE
    * switch to a consistent LOG() framework