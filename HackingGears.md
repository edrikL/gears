This page provides some tips for hacking the Gears codebase using VS 2005 under Windows, some of it is probably applicable to other platforms and environments.

If you simply want to build Gears from source, you can find instructions for that [here](BuildingGearsForWindows.md).

# Make Targets #

`>> make BROWSER=[IE|FF] MODE=[dbg|opt] OS=[win32|wince]`

e.g.

`>> make BROWSER=FF MODE=dbg`

Typing _make_ with no parameters in Windows will default to debug mode and will build installers for both Firefox & IE.

When _MODE_ is omitted from the command line, the default is _dbg_.

Build output is placed in **gears\bin-dbg** or **gears\bin-opt**.

# Setting Up Visual Studio #

A script to automatically generate a VS 2005 project file from the Gears source tree can be found at **gears\tools\gen\_vs\_project.py**.

C++ code in Gears is wrapped to 80 columns, You can use the tip [here](http://blogs.msdn.com/saraford/archive/2004/05/05/257953.aspx) to add a vertical rule to VS to ease conformance.

# Running Gears Directly From The Build Directory #

When developing new code, it's useful to be able to run the Gears dll directly from the build directory, rather than having to uninstall and install it manually after each build.

This allows you to test your changes quickly.

## In IE: ##

The general idea is to register the dll you're building directly, rather than IE loading the dll from the installer.

There are 2 ways to go about this:

### Method 1: ###
Paste the following into a batch file and run it from the gears/ directory after building:

```
regsvr32 /u /s bin-dbg/.../gears.dll
regsvr32 /i /s bin-dbg/.../gears.dll
start iexplore
```

(replace "..." with the actual path to gears.dll)

### Method 2: ###
  * Add a right click "Register DLL" menu by saving the following in a .reg file and double clicking it:

```
Windows Registry Editor Version 5.00

[HKEY_CLASSES_ROOT\dllfile\shell\Register DLL\command]
@=hex(2):25,00,57,00,49,00,4e,00,44,00,49,00,52,00,25,00,5c,00,73,00,79,00,\
  73,00,74,00,65,00,6d,00,33,00,32,00,5c,00,72,00,65,00,67,00,73,00,76,00,72,\
  00,33,00,32,00,2e,00,65,00,78,00,65,00,20,00,25,00,31,00,00,00

[HKEY_CLASSES_ROOT\dllfile\shell\Unregister DLL\command]
@=hex(2):25,00,57,00,49,00,4e,00,44,00,49,00,52,00,25,00,5c,00,73,00,79,00,\
  73,00,74,00,65,00,6d,00,33,00,32,00,5c,00,72,00,65,00,67,00,73,00,76,00,72,\
  00,33,00,32,00,2e,00,65,00,78,00,65,00,20,00,2f,00,75,00,20,00,25,00,31,00,\
  00,00
```

  * Right click on **gears\bin-{MODE}\win32-i386\ie\gears.dll** and select the Register DLL item you just installed.

## In FF: ##

  * Uninstall the Gears Firefox addon, restart Firefox and make sure that Gears no longer appears in the add-ons window.

  * As detailed [here](http://developer.mozilla.org/en/docs/Adding_Extensions_using_the_Windows_Registry), run the following command in a dos prompt:

```
>> reg add HKLM\Software\Mozilla\Firefox\Extensions /f /v "{000a9d1c-beef-4f90-9363-039d445309b8}" /d "{PATH_TO_GEARS_SVN}\gears\bin-dbg\installers\gears-win32-dbg-{GEARS_VERSION}"
```
where {PATH\_TO\_GEARS\_SVN} is the full path to your local copy of the gears source tree and {GEARS\_VERSION} is replaced by the current version string e.g. 0.3.0.0.

  * Reopen Firefox and make sure Gears appears in the Add-ons window.  If it doesn't check that the path you specified at the end of the previous command exists and contains the build output.

## Running Unit Tests ##
For detailed information on running the unit tests, refer to the **gears/test/README.txt** file.

## Debugging ##
To turn on debug logging in FF, you need to set the following environmental variables:
```
NSPR_LOG_MODULES = gears:5
NSPR_LOG_FILE = <filepath where you want the logfile written>
```

On Windows, you can debug javascript exceptions using Visual Studio.  To turn this on in IE:

  * `Tools -> Internet Options -> Advanced -> Browsing -> Disable script debugging (uncheck)`

You can also attach to the process.  In Visual Studio:

  * `Tools -> Attach to Process... -> iexplore.exe -> Script Debugging`