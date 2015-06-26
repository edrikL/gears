# Introduction #

This document describes how to build Gears for Windows.

# Building #

Install the following to create the build environment:
  * Visual Studio - There are two options:
    * VS 2005 - This can build the full version of Gears.
    * VS Express - This free package can only build the Firefox version of Gears. http://msdn.microsoft.com/vstudio/express/downloads/
  * Microsoft Platform SDK for Windows Server 2003 [R2](https://code.google.com/p/gears/source/detail?r=2) - http://msdn.microsoft.com/vstudio/express/visualc/usingpsdk/
  * Wix 3.0.2925 - http://wix.sourceforge.net/downloadv3.html
  * Python 2.4 - http://www.python.org/ftp/python/2.4.4/python-2.4.4.msi
  * Subversion - http://subversion.tigris.org/downloads/subversion-1.4.4.zip
  * Unxutils - [ftp://garbo.uwasa.fi/win95/unix/UnxUtils.zip](ftp://garbo.uwasa.fi/win95/unix/UnxUtils.zip) and [ftp://garbo.uwasa.fi/win95/unix/UnxUpdates.zip](ftp://garbo.uwasa.fi/win95/unix/UnxUpdates.zip).
    * Or you can get the combined package [here](http://gears.googlecode.com/files/UnxUtils.zip).

Follow the instructions at http://code.google.com/p/gears/source to set up your source tree.

Finally open a command prompt, setup your build environment, and build Gears.  (These instructions assume you put Gears in c:\svn-gears and installed Visual Studio and the Platform SDK to their default locations.)
  * Start -> Run -> cmd.exe
    * cd \svn-gears\gears\gears\
    * "c:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat"
    * "c:\Program Files\Microsoft Platform SDK for Windows Server 2003 [R2](https://code.google.com/p/gears/source/detail?r=2)\SetEnv.cmd"
    * set PATH=%PATH%;<<python path>>;<<wix path>>;<<unxutils path>>\usr\local\wbin
  * Build Gears
    * make BROWSER=[IE|FF] MODE=[dbg|opt] OS=[win32|wince]

# Installing #

To install Gears, go to bin-dbg\win32\installers\ from your Gears directory and run the .msi (IE and Firefox) or open the .xpi (Firefox only).

# Testing #
After installing, you should restart your browser.  Go to the Gears API [page](http://code.google.com/apis/gears/).  At the lower left corner, it should indicate the build of Gears that is active.  You should be able to try out the demos or create your own.  You can also run the unit tests in the source tree (/gears/test/unit\_tests.html).