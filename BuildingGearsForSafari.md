# Current Status #
Gears for Safari compiles and runs in the current repository, there are still bugs and we currently working on stabilizing the port. If you are not a developer, interested in contributing to the Safari port, it's recommended that you wait for an official release.

Gears for Safari requires OS X >=10.4 (Tiger or later) and Safari version 3.1.1 or better.  The Windows version of Safari is not supported at this time (see [relevant entry in issue tracker](http://code.google.com/p/gears/issues/detail?id=433)).

# Introduction #

The Safari port is implemented as an NPAPI plugin.  It currently has feature parity with other platform with the exception of the FileSubmitter class.

# Components #

Gears for Safari is made up of two components:
  1. GearsEnabler - An Inputmanager installed in `/Library/InputManagers/` serves to load Gears at browser startup in order to activate HTTP Interception and install the 'Gears Settings...' menu item.
  1. Gears.plugin - The actual NPAPI plugin, resides in `/Library/Internet Plugins/`.

# Building #
  1. In a terminal window, navigate to the  `gears/` directory.
  1. Type `make`.

# Installing #

If you have [Iceberg](http://s.sudre.free.fr/Software/Iceberg.html) installed you will get a Gears.pkg package that you can double click to install.  Otherwise you can run the script under `tools/osx/install_gears.sh` which will install Gears for you.

Another option is to install the components manually:
  1. Copy `gears/bin-[dbg|opt]/Installers/Safari/Gears.plugin` to the `/Library/Internet Plug-Ins` folder.
  1. Copy ```gears/bin-[dbg|opt]/Installers/Safari/GearsEnabler`` to the `/Library/InputManagers` folder.  In order to get this to work correctly on OS X 10.5, you will need to set the permissions of GearsEnabler correctly, the `tools/osx/install_inputmanager.sh` script will do this for you.

# Testing #
After installing, you should restart Safari.  Go to the [Gears Homepage](http://gears.google.com), this will indicated whether Gears is installed and what version is running.  In addition you will see the 'Gears Settings...' menu item under the Safari Application menu.  If you've compiled a debug build, Gears will print diagnostic messages to the console as it loads, you can use these to diagnose problems you may have with the Gears installation.

# Useful Scripts #
You can find some useful scripts related to Safari compilation in the `tools/osx/` directory.  Most of these can be run without arguments to print a usage message:
  * install\_gears.sh - as noted above, this will install the plugin and InputManager into the correct locations.
  * clean\_gears.sh - Will uninstall Gears, note that the Gears Data located in `~/Library/Application Support/Google/` is **not** removed by this script.
  * m42iceberg.py & iceberg2m4.py - Scripts used for converting the .packproj.m4 files used to create installer packages to and from files that can be edited with the iceberg GUI, see `installer/safari/readme.txt` for more details.
  * install\_inputmanager.sh - Installs the Gears InputManager and sets it's permissions correctly.

# XCode Project #
You can find an XCode project for compiling Gears at `tools/osx/gears.xcodeproject`.  This is included mainly for convenience for Mac Developers, the "official" releases of Gears are compiled via the Makefile.

# Debugging with XCode #
You can debug Gears plugin using GDB and attaching to the Safari process. However, it's more convinient to use XCode. Here is how to setup a special 'debugging' project for XCode for that. Note if you use XCode project file to build Gears, you don't need this - but if you build with 'make' you do.
  1. Open Xcode, and create a new Project with File > New Project. In the list, choose "Empty Project" as the project type, click Next, name the project and choose where to save the project, then click Finish.
  1. Now you need to add the executable. Select Project > New Custom Executable and type any name, then click the Choose button to locate the Safari.app from Applications folder.  Click Finish.
  1. In order for Xcode to know what sources you want to debug, you need to tell it where to find them. Go to Project > Edit Active Executable, click the Debugging tab, and finally drag 'gears' top-level folder to the panel "Additional folders to find source files in".
  1. Go to Xcode > Preferences. Find the Debugging preferences pane, and make sure "Load symbols lazily" is unchecked.
You now can start Safari and debug Gears using regular XCode methods.

# Breakpad #
Breakpad is only enabled for official builds, it will not work correctly if the file in `/Library/Internet Plugins` is an alias to the Gears.plugin file in your build directory rather than a copy.  Also, by default, Breakpad turns itself off when running Gears under a debugger.