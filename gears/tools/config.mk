# Copyright 2005, Google Inc.
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice, 
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  3. Neither the name of Google Inc. nor the names of its contributors may be
#     used to endorse or promote products derived from this software without
#     specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Store value of unmodified command line parameters.
ifdef MODE
  CMD_LINE_MODE = $MODE
endif

ifdef BROWSER
  CMD_LINE_BROWSER = $BROWSER
endif

# Discover the OS
ifeq ($(shell uname),Linux)
OS = linux
else
ifeq ($(shell uname),Darwin)
OS = osx
else
  IS_WIN32_OR_WINCE = 1
  ifneq ($(OS),wince)
  OS = win32
  endif
endif
endif

# Set default build mode
#   dbg = debug build
#   opt = release build
MODE = dbg

# Set default OS architecture
#   OSX builds will override this value (see rules.mk).
#   For other platforms we set just one value.
ifeq ($(OS),wince)
  ARCH = arm
else
  ARCH = i386
endif

# $(shell ...) statements need to be different on Windows (%% vs %).
ifdef IS_WIN32_OR_WINCE
PCT=%%
else
PCT=%
endif

MAKEFLAGS = --no-print-directory

CPPFLAGS = -I.. -I$(OUTDIR)/$(OS)-$(ARCH)

LIBPNG_CFLAGS = -DPNG_USER_CONFIG -Ithird_party/zlib
ZLIB_CFLAGS = -DNO_GZIP -DNO_GZCOMPRESS
ifeq ($(OS),wince)
ZLIB_CFLAGS += -DNO_ERRNO_H
endif
CFLAGS += $(LIBPNG_CFLAGS) $(ZLIB_CFLAGS)
CPPFLAGS += $(LIBPNG_CFLAGS) $(ZLIB_CFLAGS)

ifdef IS_WIN32_OR_WINCE
# Breakpad assumes it is in the include path
CPPFLAGS += -Ithird_party/breakpad/src
endif

# We include several different base paths of GECKO_SDK because different sets
# of files include SDK/internal files differently.
FF_CPPFLAGS = -DBROWSER_FF=1 -I$(GECKO_SDK) -I$(GECKO_SDK)/gecko_sdk/include -Ithird_party/gecko_1.8 -DMOZILLA_STRICT_API
IE_CPPFLAGS = -DBROWSER_IE=1
IEMOBILE_CPPFLAGS = -DBROWSER_IE=1
NPAPI_CPPFLAGS = -DBROWSER_NPAPI=1 -I$(GECKO_SDK) -I$(GECKO_SDK)/gecko_sdk/include

# When adding or removing SQLITE_OMIT_* options, also update and
# re-run ../third_party/sqlite_google/google_generate_preprocessed.sh.
SQLITE_CFLAGS += -DSQLITE_CORE -DSQLITE_ENABLE_FTS1 -DSQLITE_ENABLE_FTS2 \
  -DTHREADSAFE=1 -DSQLITE_DEFAULT_FILE_PERMISSIONS=0600 \
  -DSQLITE_OMIT_ATTACH=1 \
  -DSQLITE_OMIT_LOAD_EXTENSION=1 \
  -DSQLITE_OMIT_VACUUM=1 \
  -DSQLITE_TRANSACTION_DEFAULT_IMMEDIATE=1 \
  -Ithird_party/sqlite_google/src -Ithird_party/sqlite_google/preprocessed

LIBGD_CFLAGS += -Ithird_party/libjpeg -Ithird_party/libpng -DHAVE_CONFIG_H

# libGD assumes it is in the include path
CPPFLAGS += -Ithird_party/libgd

######################################################################
# OS == linux
######################################################################
ifeq ($(OS),linux)
CC = gcc
CXX = g++
OBJ_SUFFIX = .o
MKDEP = gcc -M -MF $(@D)/$*.pp -MT $@ $(CPPFLAGS) $(FF_CPPFLAGS) $<

CPPFLAGS += -DLINUX
LIBGD_CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-label
SQLITE_CFLAGS += -Wno-uninitialized -DHAVE_USLEEP=1
# for libjpeg:
THIRD_PARTY_CFLAGS = -Wno-main

# all the GTK headers using includes relative to this directory
GTK_CFLAGS = -Ithird_party/gtk/include/gtk-2.0 -Ithird_party/gtk/include/atk-1.0 -Ithird_party/gtk/include/glib-2.0 -Ithird_party/gtk/include/pango-1.0 -Ithird_party/gtk/include/cairo -Ithird_party/gtk/lib/gtk-2.0/include -Ithird_party/gtk/lib/glib-2.0/include 
CPPFLAGS += $(GTK_CFLAGS)

COMPILE_FLAGS_dbg = -g -O0
COMPILE_FLAGS_opt = -O2
COMPILE_FLAGS = -c -o $@ -fPIC -fmessage-length=0 -Wall -Werror $(COMPILE_FLAGS_$(MODE))
# NS_LITERAL_STRING does not work properly without this compiler option
COMPILE_FLAGS += -fshort-wchar

CFLAGS = $(COMPILE_FLAGS)
CXXFLAGS = $(COMPILE_FLAGS) -fno-exceptions -fno-rtti -Wno-non-virtual-dtor -Wno-ctor-dtor-privacy -funsigned-char

DLL_PREFIX = lib
DLL_SUFFIX = .so
MKSHLIB = g++
SHLIBFLAGS = -o $@ -shared -fPIC -Bsymbolic -Wl,--version-script -Wl,tools/xpcom-ld-script

# These aren't used on Linux because ld doesn't support "@args_file".
#TRANSLATE_LINKER_FILE_LIST = cat -
#EXT_LINKER_CMD_FLAG = -Xlinker @

GECKO_SDK = third_party/gecko_1.8/linux

FF_LIBS = -L $(GECKO_SDK)/gecko_sdk/bin -L $(GECKO_SDK)/gecko_sdk/lib -lxpcom -lxpcomglue_s -lnspr4
endif

######################################################################
# OS == osx
######################################################################
ifeq ($(OS),osx)
CC = gcc -arch $(ARCH)
CXX = g++ -arch $(ARCH)
OBJ_SUFFIX = .o
MKDEP = gcc -M -MF $(@D)/$*.pp -MT $@ $(CPPFLAGS) $(FF_CPPFLAGS) $<

CPPFLAGS += -DLINUX -DOS_MACOSX
LIBGD_CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-label
SQLITE_CFLAGS += -Wno-uninitialized -Wno-pointer-sign -isysroot $(OSX_SDK_ROOT)
SQLITE_CFLAGS += -DHAVE_USLEEP=1
# for libjpeg:
THIRD_PARTY_CFLAGS = -Wno-main

COMPILE_FLAGS_dbg = -g -O0
COMPILE_FLAGS_opt = -O2
COMMON_COMPILE_FLAGS = -fmessage-length=0 -Wall -Werror $(COMPILE_FLAGS_$(MODE)) -isysroot $(OSX_SDK_ROOT)
COMPILE_FLAGS = -c -o $@ -fPIC $(COMMON_COMPILE_FLAGS)
# NS_LITERAL_STRING does not work properly without this compiler option
COMPILE_FLAGS += -fshort-wchar

CFLAGS = $(COMPILE_FLAGS)
CXXFLAGS = $(COMPILE_FLAGS) -fno-exceptions -fno-rtti -Wno-non-virtual-dtor -Wno-ctor-dtor-privacy -funsigned-char

DLL_PREFIX = lib
DLL_SUFFIX = .dylib
MKSHLIB = g++ -framework CoreServices -framework Carbon -arch $(ARCH) -isysroot $(OSX_SDK_ROOT)
SHLIBFLAGS = -o $@ -bundle -Wl,-dead_strip -Wl,-exported_symbols_list -Wl,tools/xpcom-ld-script.darwin

# ld on OSX requires filenames to be separated by a newline, rather than spaces
# used on most platforms. So TRANSLATE_LINKER_FILE_LIST changes ' ' to '\n'.
TRANSLATE_LINKER_FILE_LIST = tr " " "\n"
EXT_LINKER_CMD_FLAG = -Xlinker -filelist -Xlinker 

GECKO_SDK = third_party/gecko_1.8/osx
OSX_SDK_ROOT = /Developer/SDKs/MacOSX10.4u.sdk

FF_LIBS = -L$(GECKO_SDK)/gecko_sdk/bin -L$(GECKO_SDK)/gecko_sdk/lib -lxpcom -lmozjs -lnspr4 -lplds4 -lplc4 -lxpcom_core
endif

######################################################################
# OS == win32 OR wince
######################################################################
ifdef IS_WIN32_OR_WINCE
CC = cl
CXX = cl
OBJ_SUFFIX = .obj
MKDEP = python tools/mkdepend.py $< $@ > $(@D)/$*.pp

# Most Windows headers use the cross-platform NDEBUG and DEBUG #defines
# (handled later).  But a few Windows files look at _DEBUG instead.
CPPFLAGS_dbg = /D_DEBUG=1
CPPFLAGS_opt =
CPPFLAGS += /nologo /DSTRICT /D_UNICODE /DUNICODE /D_USRDLL /DWIN32 /D_WINDLL \
            /D_CRT_SECURE_NO_DEPRECATE

ifeq ($(OS),win32)
# We require APPVER=5.0 for things like HWND_MESSAGE.
# When APPVER=5.0, win32.mak in the Platform SDK sets:
#   C defines:  WINVER=0x0500
#               _WIN32_WINNT=0x0500
#               _WIN32_IE=0x0500
#               _RICHEDIT_VER=0x0010
#   RC defines: WINVER=0x0500
#   MIDL flags: /target NT50
# Note: _WIN32_WINDOWS was replaced by _WIN32_WINNT for post-Win95 builds.
# Note: XP_WIN is only used by Firefox headers
CPPFLAGS += /D_WINDOWS \
            /DWINVER=0x0500 \
            /D_WIN32_WINNT=0x0500 \
            /D_WIN32_IE=0x0500 \
            /D_RICHEDIT_VER=0x0010 \
            /D_MERGE_PROXYSTUB \
            /DBREAKPAD_AVOID_STREAMS \
            /DXP_WIN \
            $(CPPFLAGS_$(MODE))
else
# For Windows Mobile we need:
#   C defines:  _WIN32_WCE=0x0501
#               _UNDER_CE=0x0501
CPPFLAGS += /D_WIN32_WCE=0x501 \
	    /DWINVER=_WIN32_WCE \
	    /DUNDER_CE=0x501 \
	    /DWINCE \
	    /DWIN32_PLATFORM_PSPC \
	    /DARM \
	    /D_ARM_ \
	    /DPOCKETPC2003_UI_MODEL \
	    /D_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA \
	    $(CPPFLAGS_$(MODE))
endif

# disable some warnings when building SQLite on Windows, so we can enable /WX
# warning C4244: conversion from 'type1' to 'type2', possible loss of data
SQLITE_CFLAGS += /wd4018 /wd4244
ifeq ($(OS),wince)
SQLITE_CFLAGS += /wd4146
endif

LIBGD_CFLAGS += /DBGDWIN32 /wd4244 /wd4996 /wd4005 /wd4142 /wd4018 /wd4133 /wd4102

COMPILE_FLAGS_dbg = /MTd /Zi
COMPILE_FLAGS_opt = /MT  /Zi /Ox
COMPILE_FLAGS = /c /Fo"$@" /Fd"$(@D)/$*.pdb" /W3 /WX /GR- $(COMPILE_FLAGS_$(MODE))
# In VC8, the way to disable exceptions is to remove all /EH* flags, and to
# define _HAS_EXCEPTIONS=0 (for C++ headers) and _ATL_NO_EXCEPTIONS (for ATL).
COMPILE_FLAGS += /D_HAS_EXCEPTIONS=0 /D_ATL_NO_EXCEPTIONS

CFLAGS = $(COMPILE_FLAGS)
CXXFLAGS = $(COMPILE_FLAGS) /TP /J

# disable some warnings when building third party code on Windows, so we can enable /WX
# warning C4018: signed/unsigned mismatch in comparison
# warning C4003: not enough actual parameters for macro
THIRD_PARTY_CPPFLAGS = /wd4018 /wd4003


ifeq ($(BROWSER),NPAPI)
DLL_PREFIX = np
else
DLL_PREFIX =
endif
DLL_SUFFIX = .dll
MKSHLIB	= link
MKEXE = link
# /RELEASE adds a checksum to the PE header to aid symbol loading.
# /DEBUG causes PDB files to be produced.
# We want both these flags in all build modes, despite their names.
LINKFLAGS_dbg =
LINKFLAGS_opt = /INCREMENTAL:NO /OPT:REF /OPT:ICF
LINKFLAGS = /NOLOGO /OUT:$@ /DEBUG /RELEASE
ifeq ($(OS),win32)
LINKFLAGS += /SUBSYSTEM:WINDOWS \
              $(LINKFLAGS_$(MODE))
else
LINKFLAGS += /SUBSYSTEM:WINDOWSCE,5.01 \
              /NODEFAULTLIB:secchk.lib \
              /MACHINE:THUMB \
              $(LINKFLAGS_$(MODE))
endif
# We need SHLIBFLAGS_NOPDB for generating other targets than gears.dll
# (e.g. setup.dll for Windows Mobile)
SHLIBFLAGS_NOPDB = $(LINKFLAGS) /DLL
SHLIBFLAGS = $(SHLIBFLAGS_NOPDB) /PDB:"$(@D)/$(MODULE).pdb"

ifeq ($(OS),win32)
FF_SHLIBFLAGS_dbg = /NODEFAULTLIB:MSVCRT
FF_SHLIBFLAGS_opt = /NODEFAULTLIB:MSVCRT
FF_SHLIBFLAGS = $(FF_SHLIBFLAGS_$(MODE))

IE_SHLIBFLAGS_dbg =
IE_SHLIBFLAGS_opt =
IE_SHLIBFLAGS = $(IE_SHLIBFLAGS_$(MODE)) /DEF:tools/mscom.def

NPAPI_SHLIBFLAGS_dbg = /NODEFAULTLIB:MSVCRT
NPAPI_SHLIBFLAGS_opt = /NODEFAULTLIB:MSVCRT
NPAPI_SHLIBFLAGS = $(NPAPI_SHLIBFLAGS_$(MODE)) /DEF:base/npapi/npgears.def
else
IEMOBILE_SHLIBFLAGS_dbg =
IEMOBILE_SHLIBFLAGS_opt =
IEMOBILE_SHLIBFLAGS = $(IE_SHLIBFLAGS_$(MODE)) /DEF:tools/mscom.def
endif

TRANSLATE_LINKER_FILE_LIST = cat -
EXT_LINKER_CMD_FLAG = @

GECKO_SDK = third_party/gecko_1.8/win32

FF_LIBS = $(GECKO_SDK)/gecko_sdk/lib/xpcom.lib $(GECKO_SDK)/gecko_sdk/lib/xpcomglue_s.lib $(GECKO_SDK)/gecko_sdk/lib/nspr4.lib $(GECKO_SDK)/gecko_sdk/lib/js3250.lib ole32.lib shell32.lib shlwapi.lib advapi32.lib wininet.lib comdlg32.lib
IE_LIBS = kernel32.lib user32.lib gdi32.lib uuid.lib sensapi.lib shlwapi.lib shell32.lib advapi32.lib wininet.lib comdlg32.lib
IEMOBILE_LIBS = wininet.lib ceshell.lib coredll.lib corelibc.lib ole32.lib oleaut32.lib uuid.lib commctrl.lib atlosapis.lib piedocvw.lib cellcore.lib htmlview.lib imaging.lib toolhelp.lib aygshell.lib
NPAPI_LIBS = sensapi.lib ole32.lib shell32.lib advapi32.lib wininet.lib

# Other tools specific to win32/wince builds.
MIDL = midl
MIDLFLAGS = $(CPPFLAGS) -env win32 -Oicf -tlb "$(@D)/$*.tlb" -h "$(@D)/$*.h" -iid "$(IE_OUTDIR)/$*_i.c" -proxy "$(IE_OUTDIR)/$*_p.c" -dlldata "$(IE_OUTDIR)/$*_d.c"

RC = rc
RCFLAGS_dbg = /DDEBUG=1
RCFLAGS_opt = /DNDEBUG=1
RCFLAGS = $(RCFLAGS_$(MODE)) /d "_UNICODE" /d "UNICODE" /i $(OUTDIR)/$(OS)-$(ARCH) /l 0x409 /fo"$(@D)/$*.res"
ifeq ($(OS),wince)
RCFLAGS += /d "WINCE" /d "_WIN32" /d "_WIN32_WCE" /d "UNDER_CE" /n /i ../
endif

GGUIDGEN = tools/gguidgen.exe
endif

######################################################################
# set the following for all OS values
######################################################################

CPPFLAGS += $(INCLUDES)
M4FLAGS  += --prefix-builtins

ifeq ($(MODE),dbg)
CPPFLAGS += -DDEBUG=1
M4FLAGS  += -DDEBUG=1
else
CPPFLAGS += -DNDEBUG=1
M4FLAGS  += -DNDEBUG=1
endif

ifeq ($(OFFICIAL_BUILD),1)
CPPFLAGS += -DOFFICIAL_BUILD=1
M4FLAGS  += -DOFFICIAL_BUILD=1
endif

# Additional values needed for M4 preprocessing

M4FLAGS  += -DPRODUCT_VERSION=$(VERSION)
M4FLAGS  += -DPRODUCT_VERSION_MAJOR=$(MAJOR)
M4FLAGS  += -DPRODUCT_VERSION_MINOR=$(MINOR)
M4FLAGS  += -DPRODUCT_VERSION_BUILD=$(BUILD)
M4FLAGS  += -DPRODUCT_VERSION_PATCH=$(PATCH)

M4FLAGS  += -DPRODUCT_OS=$(OS)

# These three macros are suggested by the GNU make documentation for creating
# a comma-separated list.
COMMA    := ,
EMPTY    :=
SPACE    := $(EMPTY) $(EMPTY)
M4FLAGS  += -DI18N_LANGUAGES="($(subst $(SPACE),$(COMMA),$(strip $(I18N_LANGS))))"

# The friendly name can include spaces.
# The short name should be lowercase_with_underscores.
# "UQ" means "un-quoted".
# IMPORTANT: these values affect most visible names, but there are exceptions.
# If you change a name below, also search the code for "[naming]"
FRIENDLY_NAME=Google Gears
SHORT_NAME=gears
M4FLAGS  += -DPRODUCT_FRIENDLY_NAME_UQ="$(FRIENDLY_NAME)"
M4FLAGS  += -DPRODUCT_SHORT_NAME_UQ="$(SHORT_NAME)"
