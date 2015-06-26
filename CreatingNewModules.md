If you are considering contributing a new module to Gears, this document is for you.

## Research ##

Propose the idea for a module on the [gears-eng](http://groups.google.com/group/gears-eng) list and gauge interest in the proposal. Identify any prior work or related Web standards that correlate with the proposed module. It may also be useful to look at the trunk and latest check-ins to see if there is any new development that is along the lines of your idea. Facilitate discussion to refine your idea.

## Design ##

Given strong interest, develop a design document, which consists of project goals and general overview of module functionality, JavaScript and C++ (if necessary) APIs, as well as the detailed technical design.

Technical design should outline key classes behind the module and their interaction. You can use [Database2 API design document](Database2API.md) as a guide.

## Develop ##

Once the design document has been reviewed by peers, start development. One of your hardest task will be making sure that your code runs and interacts properly across all supported browsers and platforms. This is why the Gears team created an abstraction around the browsers-specific implementation details, allowing you to concentrate on the actual purpose of your module.

At the core of the abstraction is the Dispatcher class template, which provides a way to specify methods and properties that are part of the JavaScript interface and wire them to the methods in your module. As the methods or property accessors are invoked, the dispatcher in turn invokes (dispatches the invocations to) the methods in your module. A consistent argument retrieval and return scheme is also provided. With this model, it is fairly easy to create a new module in Gears. The following is a simple set of instructions to get you going.

### Set Up Development Environment ###

First, you'll need to get the code and get it to compile. Follow the instructions [posted on this wiki](BuildingGearsForWindows.md), if you are on Windows, or just go to the root of your working copy and run `make` on Mac or Linux.

### Create Module Files ###

  1. In the gears project, there is a blank module template, which can be used as a starting point for any new module. This template is located under [gears/dummy](http://code.google.com/p/gears/source/browse/trunk/gears/dummy/) directory.
  1. Create a new directory for your module at the root of your working copy, copy the template files there and rename them accordingly. Make sure that the filenames are unique across all .cc files in the project.
  1. Change the name of the module to reflect its purpose. Don't forget to also change the `kModuleName` member.
  1. Modify the body of `Dispatcher<T>::Init` method to register JavaScript-facing properties and methods for your module, following the same convention as show in the sample.
  1. Write module code :)

### Modify the Makefile ###

Append to module directory to `$(BROWSER)_VPATH` and module filename `$(BROWSER)_CPPSRCS` macros, respectively:

```
#-----------------------------------------------------------------------------
# your_module_dir

$(BROWSER)_VPATH += \
		your_module_dir \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		module_file.cc \
		$(NULL)
```

### Add New Module To The Factory ###

Modify `GearsFactory::CreateDispatcherModule` in `factory\ie\factory.cc` and `factory\firefox\factory.cc` and `GearsFactory::Create` in `factory\npapi\factory.cc` to add module instantiation to the conditional chain:

```
  ...
  } else if (object_name == STRING16(L"beta.yourmodulename")) {
    CreateModule<YourModuleName>(GetJsRunner(), &object);
  } else {
  ...
```

### Add Unit Tests ###

Finally, make sure to add unit tests for your new code. The project comes with its own test framework. See the [test/README.txt](http://code.google.com/p/gears/source/browse/trunk/gears/test/README.txt) for more details. You can use the [dummy module test suite](http://code.google.com/p/gears/source/browse/trunk/gears/test/testcases/dummy_tests.js) as a starter.