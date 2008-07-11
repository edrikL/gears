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

# NOTES:
# - Quotes around "mkdir" are required so Windows cmd.exe uses mkdir.exe
#     instead of built-in mkdir command.  (Running mkdir.exe without
#     quotes creates a directory named '.exe'!!)

OUTDIR = bin-$(MODE)

# LIBGD_OUTDIR, SQLITE_OUTDIR and THIRD_PARTY_OUTDIR are separate from
# COMMON_OUTDIR because we want different build flags for them, and flags are
# set per output directory.
#
# INSTALLERS_OUTDIR doesn't include $(ARCH) because OSes that support
# multiple CPU architectures (namely, OSX) have merged install packages.
COMMON_OUTDIR       = $(OUTDIR)/$(OS)-$(ARCH)/common
# As of 2008/04/03, our code relies on lowercase names for the generated-file
#   dirs, so we must explicitly list all browser OUTDIRs, instead of defining:
#   $(BROWSER)_OUTDIR   = $(OUTDIR)/$(OS)-$(ARCH)/$(BROWSER)
$(BROWSER)_OUTDIRS_I18N    = $(foreach lang,$(I18N_LANGS),$($(BROWSER)_OUTDIR)/genfiles/i18n/$(lang))
COMMON_OUTDIRS_I18N = $(foreach lang,$(I18N_LANGS),$(COMMON_OUTDIR)/genfiles/i18n/$(lang))
INSTALLERS_OUTDIR          = $(OUTDIR)/installers

FF2_OUTDIR                 = $(OUTDIR)/$(OS)-$(ARCH)/ff2
FF3_OUTDIR                 = $(OUTDIR)/$(OS)-$(ARCH)/ff3
IE_OUTDIR                  = $(OUTDIR)/$(OS)-$(ARCH)/ie
NONE_OUTDIR                = $(OUTDIR)/$(OS)-$(ARCH)/none
NPAPI_OUTDIR               = $(OUTDIR)/$(OS)-$(ARCH)/npapi
SF_OUTDIR                  = $(OUTDIR)/$(OS)-$(ARCH)/safari

OSX_LAUNCHURL_OUTDIR       = $(OUTDIR)/$(OS)-$(ARCH)/launch_url_with_browser
VISTA_BROKER_OUTDIR        = $(OUTDIR)/$(OS)-$(ARCH)/vista_broker

LIBGD_OUTDIR               = $(COMMON_OUTDIR)/gd
SQLITE_OUTDIR              = $(COMMON_OUTDIR)/sqlite
MOZJS_OUTDIR               = $(COMMON_OUTDIR)/spidermonkey
PORTAUDIO_OUTDIR           = $(COMMON_OUTDIR)/portaudio
LIBSPEEX_OUTDIR            = $(COMMON_OUTDIR)/libspeex
LIBTREMOR_OUTDIR           = $(COMMON_OUTDIR)/tremor
THIRD_PARTY_OUTDIR         = $(COMMON_OUTDIR)/third_party


# TODO(cprince): unify the Firefox directory name across the output dirs
# (where it is 'ff') and the source dirs (where it is 'firefox').  Changing
# the output dirs would require changing #includes that reference genfiles.

# This is the base directory used for I18N files.  Files used under it
# will keep their relative sub-directory.
I18N_INPUTS_BASEDIR = ui/generated

# Macro to substitute .o for sourcefile suffix.
# Usage: $(call SUBSTITUTE_OBJ_SUFFIX, out_dir, source_file_list)
# Example: $(call SUBSTITUTE_OBJ_SUFFIX, out_dir, a.cc foo.m) yields
#  out_dir/a.o out_dir/foo.o
# In the macro's body, $1 is the output directory and $2 is the list of source
# files.
SOURCECODE_SUFFIXES = c cc m mm s
SUBSTITUTE_OBJ_SUFFIX = $(foreach SUFFIX,$(SOURCECODE_SUFFIXES), \
                           $(patsubst %.$(SUFFIX),$1/%$(OBJ_SUFFIX), \
                             $(filter %.$(SUFFIX), $2) \
                           ) \
                        )

# TODO(playmobil): unify CPPSRCS & CSRCS into CXX_SRCS.
COMMON_OBJS              = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(COMMON_CPPSRCS) $(COMMON_CSRCS))
$(BROWSER)_OBJS          = $(call SUBSTITUTE_OBJ_SUFFIX, $($(BROWSER)_OUTDIR), $($(BROWSER)_CPPSRCS) $($(BROWSER)_CSRCS))
CRASH_SENDER_OBJS        = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(CRASH_SENDER_CPPSRCS))
NOTIFIER_OBJS            = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(NOTIFIER_CPPSRCS) $(NOTIFIER_CSRCS))
NOTIFIER_SHELL_OBJS      = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(NOTIFIER_SHELL_CPPSRCS))
NOTIFIER_TEST_OBJS       = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(NOTIFIER_TEST_CPPSRCS)  $(NOTIFIER_TEST_CSRCS))
OSX_CRASH_INSPECTOR_OBJS = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(OSX_CRASH_INSPECTOR_CPPSRCS))
OSX_LAUNCHURL_OBJS       = $(call SUBSTITUTE_OBJ_SUFFIX, $(OSX_LAUNCHURL_OUTDIR), $(OSX_LAUNCHURL_CPPSRCS))
SF_INPUTMANAGER_OBJS     = $(call SUBSTITUTE_OBJ_SUFFIX, $(SF_OUTDIR), $(SF_INPUTMANAGER_CPPSRCS))
LIBGD_OBJS               = $(call SUBSTITUTE_OBJ_SUFFIX, $(LIBGD_OUTDIR), $(LIBGD_CSRCS))
MOZJS_OBJS               = $(call SUBSTITUTE_OBJ_SUFFIX, $(MOZJS_OUTDIR), $(MOZJS_CSRCS))
SQLITE_OBJS              = $(call SUBSTITUTE_OBJ_SUFFIX, $(SQLITE_OUTDIR), $(SQLITE_CSRCS))
PORTAUDIO_OBJS           = $(call SUBSTITUTE_OBJ_SUFFIX, $(PORTAUDIO_OUTDIR), $(PORTAUDIO_CSRCS))
LIBSPEEX_OBJS            = $(call SUBSTITUTE_OBJ_SUFFIX, $(LIBSPEEX_OUTDIR), $(LIBSPEEX_CSRCS))
LIBTREMOR_OBJS           = $(call SUBSTITUTE_OBJ_SUFFIX, $(LIBTREMOR_OUTDIR), $(LIBTREMOR_CSRCS))
PERF_TOOL_OBJS           = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(PERF_TOOL_CPPSRCS))
IE_WINCESETUP_OBJS       = $(call SUBSTITUTE_OBJ_SUFFIX, $(IE_OUTDIR), $(IE_WINCESETUP_CPPSRCS))
THIRD_PARTY_OBJS         = $(call SUBSTITUTE_OBJ_SUFFIX, $(THIRD_PARTY_OUTDIR), $(THIRD_PARTY_CPPSRCS) $(THIRD_PARTY_CSRCS))
VISTA_BROKER_OBJS        = $(call SUBSTITUTE_OBJ_SUFFIX, $(VISTA_BROKER_OUTDIR), $(VISTA_BROKER_CPPSRCS) $(VISTA_BROKER_CSRCS))

# IMPORTANT: If you change these lists, you need to change the corresponding
# files in win32_msi.wxs.m4 as well.
# TODO(aa): We should somehow generate win32_msi.wxs because it is lame to
# repeat the file list.
#
# Begin: resource lists that MUST be kept in sync with "win32_msi.wxs.m4"
COMMON_RESOURCES = \
	ui/common/blank.gif \
	ui/common/button_bg.gif \
	ui/common/button_corner_black.gif \
	ui/common/button_corner_blue.gif \
	ui/common/button_corner_grey.gif \
	ui/common/icon_32x32.png \
	ui/common/local_data.png \
	ui/common/location_data.png

FF3_RESOURCES = \
	$(FF3_OUTDIR)/genfiles/browser-overlay.js \
	$(FF3_OUTDIR)/genfiles/browser-overlay.xul
# End: resource lists that MUST be kept in sync with "win32_msi.wxs.m4"

DEPS = \
	$($(BROWSER)_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(COMMON_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(CRASH_SENDER_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(NOTIFIER_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(NOTIFIER_SHELL_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(NOTIFIER_TEST_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(OSX_CRASH_INSPECTOR_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(OSX_LAUNCHURL_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(PERF_TOOL_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(SF_INPUTMANAGER_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(VISTA_BROKER_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(LIBGD_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(MOZJS_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(SQLITE_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(PORTAUDIO_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(LIBSPEEX_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(LIBTREMOR_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(THIRD_PARTY_OBJS:$(OBJ_SUFFIX)=.pp)

$(BROWSER)_GEN_HEADERS = \
	$(patsubst %.idl,$($(BROWSER)_OUTDIR)/genfiles/%.h,$($(BROWSER)_IDLSRCS))

FF3_GEN_TYPELIBS = \
	$(patsubst %.idl,$(FF3_OUTDIR)/genfiles/%.xpt,$(FF3_IDLSRCS))

IE_OBJS += \
	$(patsubst %.idl,$(IE_OUTDIR)/%_i$(OBJ_SUFFIX),$(IE_IDLSRCS))

NPAPI_GEN_TYPELIBS = \
	$(patsubst %.idl,$(NPAPI_OUTDIR)/genfiles/%.xpt,$(NPAPI_IDLSRCS))

$(BROWSER)_M4FILES = \
	$(patsubst %.m4,$($(BROWSER)_OUTDIR)/genfiles/%,$($(BROWSER)_M4SRCS))
COMMON_M4FILES = \
	$(patsubst %.m4,$(COMMON_OUTDIR)/genfiles/%,$(COMMON_M4SRCS))

$(BROWSER)_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $($(BROWSER)_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$($(BROWSER)_M4SRCS_I18N))))
COMMON_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $(COMMON_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$(COMMON_M4SRCS_I18N))))

$(BROWSER)_VPATH += $($(BROWSER)_OUTDIR)/genfiles
COMMON_VPATH += $(COMMON_OUTDIR)/genfiles
IE_VPATH += $(IE_OUTDIR)
IE_VPATH += $(VISTA_BROKER_OUTDIR)
SF_VPATH += $(SF_OUTDIR)
ifeq ($(OS),osx)
$(BROWSER)_VPATH += $(OSX_LAUNCHURL_OUTDIR)
endif

# Make VPATH search our paths before third-party paths.
VPATH += $($(BROWSER)_VPATH) $(COMMON_VPATH) $(THIRD_PARTY_VPATH)

#-----------------------------------------------------------------------------
# OUTPUT FILENAMES

# no ARCH in INSTALLER_BASE_NAME because we created merged installers
INSTALLER_BASE_NAME = $(MODULE)-$(OS)-$(MODE)-$(VERSION)

# Rules for per-OS installers need to reference MODULE variables, but BROWSER
#   is not defined.  So explicitly define all module vars, instead of using:
#   $(BROWSER)_MODULE_DLL = $($(BROWSER)_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
FF2_MODULE_DLL    = $(FF2_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
FF3_MODULE_DLL    = $(FF3_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
IE_MODULE_DLL     = $(IE_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
NPAPI_MODULE_DLL  = $(NPAPI_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
SF_MODULE_DLL     = $(SF_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)

FF3_MODULE_TYPELIB  = $(FF3_OUTDIR)/$(MODULE).xpt
IE_WINCESETUP_DLL   = $(IE_OUTDIR)/$(DLL_PREFIX)setup$(DLL_SUFFIX)
SF_INPUTMANAGER_EXE = $(SF_OUTDIR)/$(EXE_PREFIX)GearsEnabler$(EXE_SUFFIX)

# Note: crash_sender.exe name needs to stay in sync with name used in
# exception_handler_win32.cc and exception_handler_osx/google_breakpad.mm.
CRASH_SENDER_EXE        = $(COMMON_OUTDIR)/$(EXE_PREFIX)crash_sender$(EXE_SUFFIX)
NOTIFIER_DLL            = $(COMMON_OUTDIR)/$(EXE_PREFIX)notifier$(DLL_SUFFIX)
NOTIFIER_EXE            = $(COMMON_OUTDIR)/$(EXE_PREFIX)notifier$(EXE_SUFFIX)
NOTIFIER_TEST_EXE       = $(COMMON_OUTDIR)/$(EXE_PREFIX)notifier_test$(EXE_SUFFIX)
# Note: crash_inspector name needs to stay in sync with name used in
# exception_handler_osx/google_breakpad.mm.
OSX_CRASH_INSPECTOR_EXE = $(COMMON_OUTDIR)/$(EXE_PREFIX)crash_inspector$(EXE_SUFFIX)
OSX_LAUNCHURL_EXE       = $(COMMON_OUTDIR)/$(EXE_PREFIX)launch_url_with_browser$(EXE_SUFFIX)
PERF_TOOL_EXE           = $(COMMON_OUTDIR)/$(EXE_PREFIX)perf_tool$(EXE_SUFFIX)

# Note: We use IE_OUTDIR so that relative path from gears.dll is same in
# development environment as deployment environment.
# Note: vista_broker.exe needs to stay in sync with name used in
# desktop_win32.cc.
# TODO(aa): This can move to common_outdir like crash_sender.exe
VISTA_BROKER_EXE = $(IE_OUTDIR)/$(EXE_PREFIX)vista_broker$(EXE_SUFFIX)

SF_PLUGIN_BUNDLE        = $(INSTALLERS_OUTDIR)/Safari/Gears.plugin
SF_INPUTMANAGER_BUNDLE  = $(INSTALLERS_OUTDIR)/Safari/GearsEnabler

SF_INSTALLER_PKG       = $(INSTALLERS_OUTDIR)/Safari/Gears.pkg
FFMERGED_INSTALLER_XPI = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).xpi

WIN32_INSTALLER_MSI    = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).msi

WINCE_INSTALLER_CAB    = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).cab
INFSRC_BASE_NAME = wince_cab
INFSRC = $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME).inf

SYMBIAN_INSTALLER_SISX = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).sisx

# BUILD TARGETS

default::
ifneq "$(BROWSER)" ""
  # Build for just the selected browser.
  # Note that only the modules get built, not the final installers.
	$(MAKE) prereqs    BROWSER=$(BROWSER)
	$(MAKE) genheaders BROWSER=$(BROWSER)
	$(MAKE) modules    BROWSER=$(BROWSER)
else
  # build for all browsers valid on this OS
  ifeq ($(OS),linux)
	$(MAKE) prereqs    BROWSER=FF2
	$(MAKE) genheaders BROWSER=FF2
	$(MAKE) modules    BROWSER=FF2

	$(MAKE) prereqs    BROWSER=FF3
	$(MAKE) genheaders BROWSER=FF3
	$(MAKE) modules    BROWSER=FF3

	$(MAKE) prereqs    BROWSER=NONE
	$(MAKE) genheaders BROWSER=NONE
	$(MAKE) modules    BROWSER=NONE

	$(MAKE) installers

  else
  ifeq ($(OS),win32)
	$(MAKE) prereqs    BROWSER=FF2
	$(MAKE) genheaders BROWSER=FF2
	$(MAKE) modules    BROWSER=FF2

	$(MAKE) prereqs    BROWSER=FF3
	$(MAKE) genheaders BROWSER=FF3
	$(MAKE) modules    BROWSER=FF3

	$(MAKE) prereqs    BROWSER=IE
	$(MAKE) genheaders BROWSER=IE
	$(MAKE) modules    BROWSER=IE

	$(MAKE) prereqs    BROWSER=NONE
	$(MAKE) genheaders BROWSER=NONE
	$(MAKE) modules    BROWSER=NONE

	$(MAKE) installers

  else
  ifeq ($(OS),wince)
	$(MAKE) prereqs    BROWSER=IE
	$(MAKE) genheaders BROWSER=IE
	$(MAKE) modules    BROWSER=IE

	$(MAKE) prereqs    BROWSER=NONE
	$(MAKE) genheaders BROWSER=NONE
	$(MAKE) modules    BROWSER=NONE

	$(MAKE) installers
  else
  ifeq ($(OS),osx)
        # For osx, build the non-installer targets for multiple architectures.
	$(MAKE) prereqs    BROWSER=FF2
	$(MAKE) genheaders BROWSER=FF2
	$(MAKE) modules    BROWSER=FF2

	$(MAKE) prereqs    BROWSER=FF3
	$(MAKE) genheaders BROWSER=FF3
	$(MAKE) modules    BROWSER=FF3

	$(MAKE) prereqs    BROWSER=SF
	$(MAKE) genheaders BROWSER=SF
	$(MAKE) modules    BROWSER=SF

	$(MAKE) prereqs    BROWSER=NONE
	$(MAKE) genheaders BROWSER=NONE
	$(MAKE) modules    BROWSER=NONE

	$(MAKE) installers
  else
  ifeq ($(OS),symbian)
  ifeq ($(ARCH),)
	$(MAKE) prereqs    BROWSER=NPAPI ARCH=i386
	$(MAKE) prereqs    BROWSER=NPAPI ARCH=arm
	$(MAKE) genheaders BROWSER=NPAPI ARCH=i386
	$(MAKE) genheaders BROWSER=NPAPI ARCH=arm
	$(MAKE) modules    BROWSER=NPAPI ARCH=i386
	$(MAKE) modules    BROWSER=NPAPI ARCH=arm
	$(MAKE) installers ARCH=i386
	$(MAKE) installers ARCH=arm
  else
	$(MAKE) prereqs    BROWSER=NPAPI
	$(MAKE) genheaders BROWSER=NPAPI
	$(MAKE) modules    BROWSER=NPAPI
	$(MAKE) installers 
  endif
  endif
  endif
  endif
  endif
  endif
endif


# Cross-browser targets.
prereqs:: $($(BROWSER)_OUTDIR) $($(BROWSER)_OUTDIR)/genfiles $($(BROWSER)_OUTDIRS_I18N) $($(BROWSER)_M4FILES) $($(BROWSER)_M4FILES_I18N)
prereqs::     $(COMMON_OUTDIR)     $(COMMON_OUTDIR)/genfiles     $(COMMON_OUTDIRS_I18N)     $(COMMON_M4FILES)     $(COMMON_M4FILES_I18N)
prereqs:: $(INSTALLERS_OUTDIR) $(LIBGD_OUTDIR) $(SQLITE_OUTDIR) $(PORTAUDIO_OUTDIR) $(LIBSPEEX_OUTDIR) $(LIBTREMOR_OUTDIR) $(THIRD_PARTY_OUTDIR)
modules::
genheaders:: $($(BROWSER)_GEN_HEADERS)

# Browser-specific targets.
ifeq ($(BROWSER),FF2)
modules:: $(FF2_MODULE_DLL)
endif

ifeq ($(BROWSER),FF3)
modules:: $(FF3_MODULE_DLL) $(FF3_MODULE_TYPELIB)
endif

ifeq ($(BROWSER),IE)
modules:: $(IE_MODULE_DLL)

ifeq ($(OS),win32)
prereqs:: $(VISTA_BROKER_OUTDIR)
modules:: $(VISTA_BROKER_EXE)
else # wince
modules:: $(IE_WINCESETUP_DLL)
endif
endif # $(BROWSER),IE

ifeq ($(BROWSER),NPAPI)
modules:: $(NPAPI_MODULE_DLL)
endif

ifeq ($(USING_MOZJS),1)
prereqs:: $(MOZJS_OUTDIR)
endif
ifeq ($(BROWSER),SF)
modules:: $(SF_MODULE_DLL) $(SF_INPUTMANAGER_EXE) $(SF_PLUGIN_BUNDLE) $(SF_INPUTMANAGER_BUNDLE)
endif

# OS-specific targets.
# Note that the 'prereqs' and 'modules' targets should only be built
# when BROWSER is 'NONE'. 'installers' targets are built without any
# BROWSER value set.
ifeq ($(BROWSER), NONE)
# If one of these OS names matches, the 'findstring' result is not empty.
ifneq ($(findstring $(OS),linux|osx|win32),)
modules:: $(NOTIFIER_TEST_EXE) $(NOTIFIER_EXE)
endif
ifeq ($(OS),win32)
# TODO(aa): Should this run on wince too?
# TODO(aa): Implement crash senders for more platforms
modules:: $(CRASH_SENDER_EXE) $(NOTIFIER_DLL)
endif
ifneq ($(OS),wince)
ifneq ($(OS),android)
# TODO(cprince): Get tools to link on WinCE.
modules:: $(PERF_TOOL_EXE)
endif
endif
endif

ifeq ($(OS),linux)
installers:: $(FFMERGED_INSTALLER_XPI)
else
ifeq ($(OS),osx)
prereqs:: $(OSX_LAUNCHURL_OUTDIR)
modules:: $(OSX_LAUNCHURL_EXE)
installers:: $(SF_INSTALLER) $(FFMERGED_INSTALLER_XPI)
else
ifeq ($(OS),win32)
installers:: $(FFMERGED_INSTALLER_XPI) $(WIN32_INSTALLER_MSI)
else
ifeq ($(OS),wince)
installers:: $(WINCE_INSTALLER_CAB)
else
ifeq ($(OS),symbian)
ifeq ($(ARCH),arm)
installers:: $(SYMBIAN_INSTALLER_SISX)
else
installers:: $(SYMBIAN_EMULATOR_INSTALLER)
endif
endif
endif
endif
endif
endif

clean::
ifdef CMD_LINE_MODE  # If MODE is specified on command line.
	rm -rf $(OUTDIR)
else
	rm -rf bin-dbg
	rm -rf bin-opt
endif

help::
	$(ECHO) "Usage: make [MODE=dbg|opt] [BROWSER=FF|IE|NPAPI] [OS=wince]"
	$(ECHO)
	$(ECHO) "  If you omit MODE, the default is dbg."
	$(ECHO) "  If you omit BROWSER, all browsers available on the current OS are built."

.PHONY: prereqs genheaders modules clean help

$($(BROWSER)_OUTDIR):
	"mkdir" -p $@
$($(BROWSER)_OUTDIR)/genfiles:
	"mkdir" -p $@
$($(BROWSER)_OUTDIRS_I18N):
	"mkdir" -p $@
$(COMMON_OUTDIR):
	"mkdir" -p $@
$(COMMON_OUTDIR)/genfiles:
	"mkdir" -p $@
$(COMMON_OUTDIRS_I18N):
	"mkdir" -p $@
$(INSTALLERS_OUTDIR):
	"mkdir" -p $@
$(LIBGD_OUTDIR):
	"mkdir" -p $@
$(MOZJS_OUTDIR):
	"mkdir" -p $@
$(OSX_LAUNCHURL_OUTDIR):
	"mkdir" -p $@
$(SQLITE_OUTDIR):
	"mkdir" -p $@
$(PORTAUDIO_OUTDIR):
	"mkdir" -p $@
$(LIBSPEEX_OUTDIR):
	"mkdir" -p $@
$(LIBTREMOR_OUTDIR):
	"mkdir" -p $@
$(THIRD_PARTY_OUTDIR):
	"mkdir" -p $@
$(VISTA_BROKER_OUTDIR):
	"mkdir" -p $@

# M4 (GENERIC PREPROCESSOR) TARGETS

$($(BROWSER)_OUTDIR)/genfiles/%: %.m4
	m4 $(M4FLAGS) $< > $@
$(COMMON_OUTDIR)/genfiles/%: %.m4
	m4 $(M4FLAGS) $< > $@

# I18N M4 (GENERIC PREPROCESSOR) TARGETS

$($(BROWSER)_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@
$(COMMON_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@

# IDL TARGETS

# Need /base/common in the include path to derive from GearsBaseClassInterface
# (xpidl doesn't like slashes in #include "base_interface_ff.idl")
#
# TODO(cprince): see whether we can remove the extra include paths after
# the 1.9 inclusion is complete.
$(FF2_OUTDIR)/genfiles/%.h: %.idl
	$(GECKO_BIN)/xpidl -I base/common -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m header -o $(FF2_OUTDIR)/genfiles/$* $<
$(FF2_OUTDIR)/genfiles/%.xpt: %.idl
	$(GECKO_BIN)/xpidl -I base/common -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m typelib -o $(FF2_OUTDIR)/genfiles/$* $<

$(FF3_OUTDIR)/genfiles/%.h: %.idl
	$(GECKO_BIN)/xpidl -I base/common -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m header -o $(FF3_OUTDIR)/genfiles/$* $<
$(FF3_OUTDIR)/genfiles/%.xpt: %.idl
	$(GECKO_BIN)/xpidl -I base/common -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m typelib -o $(FF3_OUTDIR)/genfiles/$* $<

$(IE_OUTDIR)/genfiles/%.h: %.idl
	midl $(CPPFLAGS) -env win32 -Oicf -tlb "$(@D)/$*.tlb" -h "$(@D)/$*.h" -iid "$(IE_OUTDIR)/$*_i.c" -proxy "$(IE_OUTDIR)/$*_p.c" -dlldata "$(IE_OUTDIR)/$*_d.c" $<

# Yacc UNTARGET, so we don't try to build sqlite's parse.c from parse.y.

%.c: %.y

# C/C++ TARGETS

# Omit @$(MKDEP) for sqlite because they include files which
# aren't in the same directory, but don't use explicit paths.  All necessary -I
# flags are in SQLITE_CFLAGS.
# Moved this rule before the COMMON_OUTDIR one as the make system could use
# the latter for files in $(COMMON_OUTDIR)/sqlite using sqlite/*.o as stem.
$(SQLITE_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SQLITE_CPPFLAGS) $(SQLITE_CFLAGS) $<
$(SQLITE_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SQLITE_CPPFLAGS) $(SQLITE_CFLAGS) $<

$($(BROWSER)_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CFLAGS) $<
$($(BROWSER)_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CXXFLAGS) $<
$($(BROWSER)_OUTDIR)/%$(OBJ_SUFFIX): %.m
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CFLAGS) $<
$($(BROWSER)_OUTDIR)/%$(OBJ_SUFFIX): %.mm
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CXXFLAGS) $<

$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CFLAGS) $<
$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CXXFLAGS) $<
$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.m
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CFLAGS) $<
$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.mm
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CXXFLAGS) $<

$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CFLAGS) $<
$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CXXFLAGS) $<

$(OSX_LAUNCHURL_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CXXFLAGS) $<

$(VISTA_BROKER_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CFLAGS) $<
$(VISTA_BROKER_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CXXFLAGS) $<


# Omit @$(MKDEP) for mozjs and libgd because they include files which
# aren't in the same directory, but don't use explicit paths.  All necessary -I
# flags are in LIBGD_CFLAGS.
$(MOZJS_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MOZJS_CFLAGS) $<
$(MOZJS_OUTDIR)/%$(OBJ_SUFFIX): %.s
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MOZJS_CFLAGS) $<

$(LIBGD_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIBGD_CFLAGS) $<


$(PORTAUDIO_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PORTAUDIO_CFLAGS) $<

$(LIBSPEEX_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(LIBSPEEX_CFLAGS) $(LIBOGG_CFLAGS) $(CPPFLAGS) $(CFLAGS) $<

$(LIBTREMOR_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(LIBTREMOR_CFLAGS) $(LIBOGG_CFLAGS) $(CPPFLAGS) $(CFLAGS) $<

# RESOURCE TARGETS

$(IE_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_IE=1 $<

$(FF2_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_FF2=1 $<

$(FF3_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_FF3=1 $<

$(NPAPI_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_NPAPI=1 $<

$(COMMON_OUTDIR)/%.res: %.rc
	$(RC) $(RCFLAGS) $<

$(VISTA_BROKER_OUTDIR)/%.res: %.rc
	$(RC) $(RCFLAGS) /DVISTA_BROKER=1 $<
	
$(SF_OUTDIR)/%.res: $(COMMON_RESOURCES) $(SF_M4FILES_I18N)
# On Safari, the .res file is actually an object file. But we still use
# the .res extension, which lets us run special build steps for resources.

# Bundle ui files into the executable itself by first generating .webarchive files, and then
# including those in the dylib by converting them into .h files with xxd. 
# TODO(playmobil): Handle localization correctly.
	tools/osx/webarchiver/webarchiver $(SF_OUTDIR)/permissions_dialog.webarchive $(SF_OUTDIR)/genfiles/i18n/en-US/permissions_dialog.html $(COMMON_RESOURCES)
	tools/osx/webarchiver/webarchiver $(SF_OUTDIR)/settings_dialog.webarchive $(SF_OUTDIR)/genfiles/i18n/en-US/settings_dialog.html $(COMMON_RESOURCES)
	tools/osx/webarchiver/webarchiver $(SF_OUTDIR)/shortcuts_dialog.webarchive $(SF_OUTDIR)/genfiles/i18n/en-US/shortcuts_dialog.html $(COMMON_RESOURCES)
	xxd -i "$(SF_OUTDIR)/settings_dialog.webarchive" > "$($(BROWSER)_OUTDIR)/genfiles/settings_dialog.h"
	xxd -i "$(SF_OUTDIR)/permissions_dialog.webarchive" > "$($(BROWSER)_OUTDIR)/genfiles/permissions_dialog.h"
	xxd -i "$(SF_OUTDIR)/shortcuts_dialog.webarchive" > "$($(BROWSER)_OUTDIR)/genfiles/shortcuts_dialog.h"
	
	tools/osx/gen_resource_list.py "$($(BROWSER)_OUTDIR)/genfiles/resource_list.h" "$($(BROWSER)_OUTDIR)/genfiles/settings_dialog.h" "$($(BROWSER)_OUTDIR)/genfiles/permissions_dialog.h" "$($(BROWSER)_OUTDIR)/genfiles/shortcuts_dialog.h"
	
	$(CC) $(SF_CPPFLAGS) $(SF_CXXFLAGS) $(CPPFLAGS) -include base/safari/prefix_header.h -fshort-wchar -c base/safari/resource_archive.cc -o $@
	

# INSTALLER-RELATED INTERMEDIATE TARGETS

ifeq ($(OS),win32)
NAMESPACE_GUID = 36F65206-5D4E-4752-9D52-27708E10DA79

# You can change the names of PRODUCT_ID vars, but NEVER change their values!
OUR_WIN32_PRODUCT_ID = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_PRODUCT_ID-$(VERSION))
OUR_COMPONENT_GUID_FF_COMPONENTS_DIR_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_COMPONENTS_DIR_FILES-$(VERSION))
OUR_COMPONENT_GUID_FF_CONTENT_DIR_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_CONTENT_DIR_FILES-$(VERSION))
OUR_COMPONENT_GUID_FF_DIR_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_DIR_FILES-$(VERSION))
OUR_COMPONENT_GUID_FF_LIB_DIR_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_LIB_DIR_FILES-$(VERSION))
OUR_COMPONENT_GUID_FF_REGISTRY = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_REGISTRY-$(VERSION))
OUR_COMPONENT_GUID_IE_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_IE_FILES-$(VERSION))
OUR_COMPONENT_GUID_IE_REGISTRY = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_IE_REGISTRY-$(VERSION))
OUR_COMPONENT_GUID_SHARED_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_SHARED_FILES-$(VERSION))
OUR_COMPONENT_GUID_SHARED_VERSIONED_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_SHARED_VERSIONED_FILES-$(VERSION))  
OUR_COMPONENT_GUID_SHARED_REGISTRY = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_SHARED_REGISTRY-$(VERSION))

OUR_NPAPI_PRODUCT_ID = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_2ND_PRODUCT_ID-$(VERSION))
OUR_COMPONENT_GUID_NPAPI_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_NPAPI_FILES-$(VERSION))
OUR_COMPONENT_GUID_NPAPI_REGISTRY = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_NPAPI_REGISTRY-$(VERSION))

WIX_DEFINES_I18N = $(foreach lang,$(subst -,_,$(I18N_LANGS)),-dOurComponentGUID_FFLang$(lang)DirFiles=$(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_$(lang)_DIR_FILES-$(VERSION)))

# MSI version numbers must have the form <major>.<minor>.<build>. To meet this,
# we combine our build and patch version numbers like so:
# MSI_VERSION = <major>.<minor>.<BUILD * 100 + PATCH>.
# Note: This assumes that the BUILD and PATCH variables adhere to the range
# requirements in version.mk. See comments in version.mk for more details.
MSI_BUILD = $(shell dc -e "$(BUILD) 100 * $(PATCH) + p")
MSI_VERSION = $(MAJOR).$(MINOR).$(MSI_BUILD)

$(COMMON_OUTDIR)/%.wxiobj: %.wxs
	candle.exe -out $@ $< \
	  -dOurWin32ProductId=$(OUR_WIN32_PRODUCT_ID) \
	  -dOurCommonPath=$(OUTDIR)/$(OS)-$(ARCH)/common \
	  -dOurIEPath=$(OUTDIR)/$(OS)-$(ARCH)/ie \
	  -dOurFFPath=$(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME) \
	  -dOurComponentGUID_FFComponentsDirFiles=$(OUR_COMPONENT_GUID_FF_COMPONENTS_DIR_FILES) \
	  -dOurComponentGUID_FFContentDirFiles=$(OUR_COMPONENT_GUID_FF_CONTENT_DIR_FILES) \
	  -dOurComponentGUID_FFDirFiles=$(OUR_COMPONENT_GUID_FF_DIR_FILES) \
	  -dOurComponentGUID_FFLibDirFiles=$(OUR_COMPONENT_GUID_FF_LIB_DIR_FILES) \
	  -dOurComponentGUID_FFRegistry=$(OUR_COMPONENT_GUID_FF_REGISTRY) \
	  -dOurComponentGUID_IEFiles=$(OUR_COMPONENT_GUID_IE_FILES) \
	  -dOurComponentGUID_IERegistry=$(OUR_COMPONENT_GUID_IE_REGISTRY) \
	  -dOurComponentGUID_SharedFiles=$(OUR_COMPONENT_GUID_SHARED_FILES) \
	  -dOurComponentGUID_SharedVersionedFiles=$(OUR_COMPONENT_GUID_SHARED_VERSIONED_FILES) \
	  -dOurComponentGUID_SharedRegistry=$(OUR_COMPONENT_GUID_SHARED_REGISTRY) \
	  -dOurNpapiProductId=$(OUR_NPAPI_PRODUCT_ID) \
	  -dOurNpapiPath=$(OUTDIR)/$(OS)-$(ARCH)/npapi \
	  -dOurComponentGUID_NpapiFiles=$(OUR_COMPONENT_GUID_NPAPI_FILES) \
	  -dOurComponentGUID_NpapiRegistry=$(OUR_COMPONENT_GUID_NPAPI_REGISTRY) \
	  -dOurMsiVersion=$(MSI_VERSION) \
	  $(WIX_DEFINES_I18N)
endif

# LINK TARGETS

# Split the list of OBJS to avoid "input line is too long" errors.
$(BROWSER)_OBJS1 = $(wordlist 1, 100, $($(BROWSER)_OBJS))
$(BROWSER)_OBJS2 = $(wordlist 101, 999, $($(BROWSER)_OBJS))

# WARNING: Must keep the following two rules (FF2|FF3_MODULE_DLL) in sync!
# The only difference should be the rule name.
$(FF2_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(PORTAUDIO_OBJS) $(LIBSPEEX_OBJS) $(LIBTREMOR_OBJS) $(THIRD_PARTY_OBJS) $(SKIA_LIB) $($(BROWSER)_OBJS) $($(BROWSER)_LINK_EXTRAS)
  ifeq ($(OS),linux)
        # TODO(playmobil): Find equivalent of "@args_file" for ld on Linux.
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_OBJS) $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(PORTAUDIO_OBJS) $(LIBSPEEX_OBJS) $(LIBTREMOR_OBJS) $(THIRD_PARTY_OBJS) $(SKIA_LIB) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS)
  else
	$(ECHO) $($(BROWSER)_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $($(BROWSER)_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBGD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(PORTAUDIO_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBSPEEX_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBTREMOR_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(SKIA_LIB) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp
  endif
$(FF3_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(PORTAUDIO_OBJS) $(LIBSPEEX_OBJS) $(LIBTREMOR_OBJS) $(THIRD_PARTY_OBJS) $(SKIA_LIB) $($(BROWSER)_OBJS) $($(BROWSER)_LINK_EXTRAS)
  ifeq ($(OS),linux)
        # TODO(playmobil): Find equivalent of "@args_file" for ld on Linux.
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_OBJS) $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(PORTAUDIO_OBJS) $(LIBSPEEX_OBJS) $(LIBTREMOR_OBJS) $(THIRD_PARTY_OBJS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS)
  else
	$(ECHO) $($(BROWSER)_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $($(BROWSER)_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBGD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(PORTAUDIO_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBSPEEX_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBTREMOR_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(SKIA_LIB) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp
  endif

$(FF3_MODULE_TYPELIB): $(FF3_GEN_TYPELIBS)
	$(GECKO_BIN)/xpt_link $@ $^

# Split the list of OBJS to avoid "input line is too long" errors.
IE_OBJS1 = $(wordlist 1, 100, $(IE_OBJS))
IE_OBJS2 = $(wordlist 101, 999, $(IE_OBJS))

$(IE_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(PORTAUDIO_OBJS) $(LIBSPEEX_OBJS) $(LIBTREMOR_OBJS) $(THIRD_PARTY_OBJS) $(SKIA_LIB) $(IE_OBJS) $(IE_LINK_EXTRAS)
	$(ECHO) $(IE_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $(IE_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBGD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(PORTAUDIO_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBSPEEX_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBTREMOR_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(SKIA_LIB) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp

# Note the use of DLLFLAGS_NOPDB instead of DLLFLAGS here.
$(IE_WINCESETUP_DLL): $(IE_WINCESETUP_OBJS) $(IE_WINCESETUP_LINK_EXTRAS)
	$(MKDLL) $(DLLFLAGS_NOPDB) $(IE_WINCESETUP_LINK_EXTRAS) $(IE_LIBS) $(IE_WINCESETUP_OBJS)

ifneq ($(OS),android)
ifneq ($(OS),symbian)
# Split the list of OBJS to avoid "input line is too long" errors.
NPAPI_OBJS1 = $(wordlist 1, 100, $(NPAPI_OBJS))
NPAPI_OBJS2 = $(wordlist 101, 999, $(NPAPI_OBJS))
$(NPAPI_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(PORTAUDIO_OBJS) $(LIBSPEEX_OBJS) $(LIBTREMOR_OBJS) $(SKIA_LIB) $(THIRD_PARTY_OBJS) $(NPAPI_OBJS) $(NPAPI_LINK_EXTRAS)
	$(ECHO) $(NPAPI_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $(NPAPI_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBGD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(PORTAUDIO_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBSPEEX_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBTREMOR_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(SKIA_LIB) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp
else
# Create library for S60
ifeq ($(ARCH),i386)
$(NPAPI_MODULE_DLL): $(COMMON_OBJS) $(NPAPI_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS)
	@echo Creating Gears library for emulator...
	$(MKDLL) $(DLLFLAGS) $(STAGE1_DLLFLAGS) -g $(LIBS) -implib "$(OUTDIR)\$(SHORT_NAME).lib" -o "$(OUTDIR)\$(SHORT_NAME).dll" -l $($(BROWSER)_OUTDIR) -search $(subst $(NPAPI_OUTDIR)/,,$(COMMON_OBJS) $(NPAPI_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS))

	@rm "$(OUTDIR)\$(SHORT_NAME).dll"
	@$(MKDLL) -S -show only,names,unmangled,verbose -o "$(OUTDIR)\$(SHORT_NAME).inf" "$(OUTDIR)\$(SHORT_NAME).lib" 

	@perl -S "makedef.pl"  -absent __E32Dll -Inffile "$(OUTDIR)\$(SHORT_NAME).inf" -1 ?ImplementationGroupProxy@@YAPBUTImplementationProxy@@AAH@Z "$(OUTDIR)\$(SHORT_NAME).def" 
	@rm "$(OUTDIR)\$(SHORT_NAME).inf"
	@rm "$(OUTDIR)\$(SHORT_NAME).lib"

	@$(MKDLL) $(DLLFLAGS) $(STAGE2_DLLFLAGS) -g $(LIBS) -f "$(OUTDIR)\$(SHORT_NAME).def" -o $(NPAPI_MODULE_DLL) -l $($(BROWSER)_OUTDIR) -search $(subst $(NPAPI_OUTDIR)/,,$(COMMON_OBJS) $(NPAPI_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS))
	@rm "$(OUTDIR)\$(SHORT_NAME).def"
else
$(NPAPI_MODULE_DLL): $(COMMON_OBJS) $(NPAPI_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS)
	@echo Creating Gears library for device...
	@echo $(COMMON_OBJS) $(NPAPI_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) > $(OUTDIR)\$(SHORT_NAME).via
	@$(MKDLL) $(DLLFLAGS) $(STAGE1_DLLFLAGS) \
	-o $(OUTDIR)/$(SHORT_NAME).in \
	--via $(OUTDIR)/$(SHORT_NAME).via
	@rm $(OUTDIR)/$(SHORT_NAME).via

	@perl -S elf2inf.pl -o $(OUTDIR)\$(SYMBIAN_TMP_FILE).inf \
	$(OUTDIR)/$(SHORT_NAME).in

	@perl -S makedef.pl  -absent _E32Dll -Inf $(OUTDIR)\$(SYMBIAN_TMP_FILE).inf \
	-1 _Z24ImplementationGroupProxyRi \
	$(OUTDIR)/$(SYMBIAN_TMP_FILE).def

	@perl -S efreeze.pl  "$(OUTDIR)\$(SHORT_NAME).def" "$(OUTDIR)\$(SYMBIAN_TMP_FILE).def" 

	@perl -S prepdef.pl "$(OUTDIR)\$(SHORT_NAME).def" "$(OUTDIR)\$(SHORT_NAME).prep.def"

	@def2dll.bat --path=$(OUTDIR) \
	--bldpath=$(OUTDIR) \
	--import=$(SYMBIAN_TMP_FILE) \
	--deffile="$(OUTDIR)\$(SHORT_NAME).prep.def" \
	--linkAs=$(SYMBIAN_TMP_FILE)[0xA0008EE4].dll \
	--inter

	@elf2e32 --definput="$(OUTDIR)\$(SHORT_NAME).prep.def" --dso="$(OUTDIR)\$(SYMBIAN_TMP_FILE).dso" --linkas=$(SYMBIAN_TMP_FILE)[0xA0008EE4].dll
	@rm $(OUTDIR)/$(SHORT_NAME).prep.def
	@rm $(OUTDIR)/$(SYMBIAN_TMP_FILE).dso

	@perl -S elf2inf.pl -o $(OUTDIR)\$(SYMBIAN_TMP_FILE).inf \
	$(OUTDIR)/$(SHORT_NAME).in

	@perl -S makedef.pl  -absent _E32Dll -Inf $(OUTDIR)\$(SYMBIAN_TMP_FILE).inf \
	-Frzfile "$(OUTDIR)\$(SHORT_NAME).def" \
	-1 _Z24ImplementationGroupProxyRi \
	$(OUTDIR)/$(SYMBIAN_TMP_FILE).def
	@rm $(OUTDIR)/$(SYMBIAN_TMP_FILE).inf
	@rm $(OUTDIR)/$(SYMBIAN_TMP_FILE).def

	@def2dll.bat  -absent _E32Dll \
	--path=$(OUTDIR) \
	--bldpath=$(OUTDIR) \
	--export=$(SYMBIAN_TMP_FILE) \
	--deffile=$(OUTDIR)/$(SHORT_NAME).def \
	--linkAs=$(SYMBIAN_TMP_FILE)[0xA0008EE4].dll \
	--inter 

	@$(MKDLL) $(DLLFLAGS) $(STAGE2_DLLFLAGS) \
	$(OUTDIR)\$(SYMBIAN_TMP_FILE).exp \
	-o $(OUTDIR)\$(SHORT_NAME)_tmp.dll \
	--list $(OUTDIR)\$(SHORT_NAME).dll.map \
	$(OUTDIR)/$(SHORT_NAME).in $(LIBS)
	@rm $(OUTDIR)/$(SHORT_NAME).in
	@rm $(OUTDIR)/$(SYMBIAN_TMP_FILE).exp

	@elftran $(ELFTRAN_FLAGS) $(OUTDIR)\$(SHORT_NAME)_tmp.dll $(NPAPI_MODULE_DLL)

	@cp $(OUTDIR)\$(SHORT_NAME)_tmp.dll $(NPAPI_OUTDIR)\$(SHORT_NAME).sym
	@rm $(OUTDIR)/$(SHORT_NAME)_tmp.dll

	@rm $(OUTDIR)/$(SHORT_NAME).def
	@rm $(OUTDIR)/$(SHORT_NAME).dll.map
	@rm $(OUTDIR)/$(SYMBIAN_TMP_FILE).lib
endif
endif
endif

$(SF_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(MOZJS_OBJS) $(SQLITE_OBJS) $(PORTAUDIO_OBJS) $(LIBSPEEX_OBJS) $(LIBTREMOR_OBJS) $(THIRD_PARTY_OBJS) $(SKIA_LIB) $($(BROWSER)_OBJS) $($(BROWSER)_LINK_EXTRAS)
	$(ECHO) $($(BROWSER)_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBGD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(MOZJS_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(PORTAUDIO_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBSPEEX_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(LIBTREMOR_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(SKIA_LIB) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
# Dump the symbols and strip the executable
	../third_party/breakpad_osx/src/tools/mac/dump_syms/dump_syms -a ppc $@ > $@_ppc.symbols
	../third_party/breakpad_osx/src/tools/mac/dump_syms/dump_syms -a i386 $@ > $@_i386.symbols
	$(STRIP_EXECUTABLE)
	rm $(OUTDIR)/obj_list.temp

# Crash inspector is launched by the crashed process from it's exception handler
# and is what actually communicates with the crashed process to extract the
# minidump.  It then launches crash_sender in order to actually send the
# minidump over the wire.
$(OSX_CRASH_INSPECTOR_EXE): $(OSX_CRASH_INSPECTOR_OBJS)
	$(MKEXE) $(EXEFLAGS) $(OSX_CRASH_INSPECTOR_OBJS) -framework Carbon

ifeq ($(OS),win32)
$(CRASH_SENDER_EXE): $(CRASH_SENDER_OBJS)
	$(MKEXE) $(EXEFLAGS) $(CRASH_SENDER_OBJS) advapi32.lib shell32.lib wininet.lib
endif
ifeq ($(OS),osx)
$(CRASH_SENDER_EXE): $(CRASH_SENDER_OBJS)
	$(MKEXE) $(EXEFLAGS) $(CRASH_SENDER_OBJS) -framework Carbon -framework Cocoa -framework Foundation -framework IOKit -framework SystemConfiguration -lstdc++
endif

ifeq ($(OS),win32)
$(NOTIFIER_EXE): $(NOTIFIER_SHELL_OBJS) $(NOTIFIER_SHELL_LINK_EXTRAS)
	$(MKEXE) $(EXEFLAGS) $(NOTIFIER_SHELL_OBJS) $(NOTIFIER_SHELL_LINK_EXTRAS) $(NOTIFIER_SHELL_LIBS)
$(NOTIFIER_DLL): $(NOTIFIER_OBJS) $(NOTIFIER_LINK_EXTRAS)
	$(MKDLL) $(DLLFLAGS) $(NOTIFIER_OBJS) $(NOTIFIER_LINK_EXTRAS) $(NOTIFIER_LIBS)
else
$(NOTIFIER_EXE): $(NOTIFIER_OBJS) $(NOTIFIER_LINK_EXTRAS)
	$(MKEXE) $(EXEFLAGS) $(NOTIFIER_OBJS) $(NOTIFIER_LINK_EXTRAS) $(NOTIFIER_LIBS)
endif

$(NOTIFIER_TEST_EXE): $(NOTIFIER_TEST_OBJS)
	$(MKEXE) $(EXEFLAGS) $(NOTIFIER_TEST_OBJS) $(NOTIFIER_LIBS)

$(OSX_LAUNCHURL_EXE): $(OSX_LAUNCHURL_OBJS)
	 $(MKEXE) $(EXEFLAGS) -framework CoreFoundation -framework ApplicationServices -lstdc++ $(OSX_LAUNCHURL_OBJS)

$(SF_INPUTMANAGER_EXE): $(SF_INPUTMANAGER_OBJS)
	 $(MKEXE) $(EXEFLAGS) -framework Foundation -framework AppKit -bundle $(SF_INPUTMANAGER_OBJS)


$(PERF_TOOL_EXE): $(PERF_TOOL_OBJS)
	$(MKEXE) $(EXEFLAGS) $(PERF_TOOL_OBJS)

$(VISTA_BROKER_EXE): $(VISTA_BROKER_OBJS) $(VISTA_BROKER_LINK_EXTRAS) $(VISTA_BROKER_OUTDIR)/vista_broker.res
	$(ECHO) $(VISTA_BROKER_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(MKEXE) $(EXEFLAGS) $(VISTA_BROKER_OUTDIR)/vista_broker.res $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp

# INSTALLER TARGETS

# We can't list the following as dependencies, because no BROWSER is defined
# for this target, therefore our $(BROWSER)_FOO variables and rules don't exist.
# For $(FFMERGED_INSTALLER_XPI):
#   $(FF2_MODULE_DLL) $(FF3_MODULE_DLL) $(FF3_MODULE_TYPELIB) $(FF3_RESOURCES) $(FF3_M4FILES_I18N) $(FF3_OUTDIR)/genfiles/chrome.manifest
# For $(SF_INSTALLER_PKG):
#   $(SF_PLUGIN_BUNDLE) $(SF_INPUTMANAGER_BUNDLE)
# In order to make sure the Installer is always up to date despite these missing
# dependencies, we list it as a phony target, so it's always rebuilt.
.PHONY: $(FFMERGED_INSTALLER_XPI) $(SF_INSTALLER_PKG)

ifeq ($(OS),osx)
ifeq ($(HAVE_ICEBERG),1)
# This rule generates a package installer for the Plugin and InputManager.
$(SF_INSTALLER_PKG):
	$(ICEBERG) -v $(SF_OUTDIR)/genfiles/installer.packproj
else
$(warning To create a Safari installer for Gears, you must install Iceberg \
  from http://s.sudre.free.fr/Software/Iceberg.html.  You can install the \
  Safari version manually by running tools/osx/install_gears.sh script)
endif
endif

ifeq ($(OS),osx)
$(FFMERGED_INSTALLER_XPI): $(COMMON_RESOURCES) $(COMMON_M4FILES_I18N) $(OSX_LAUNCHURL_EXE)
else
$(FFMERGED_INSTALLER_XPI): $(COMMON_RESOURCES) $(COMMON_M4FILES_I18N)
endif
	rm -rf $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/lib
	cp base/firefox/static_files/components/bootstrap.js $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	cp base/firefox/static_files/lib/updater.js $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/lib
	cp $(FF3_OUTDIR)/genfiles/install.rdf $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/install.rdf
	cp $(FF3_OUTDIR)/genfiles/chrome.manifest $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome.manifest
ifneq ($(OS),win32)
    # TODO(playmobil): Inspector should be located in extensions dir on win32.
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/inspector
	cp -R inspector/* $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/inspector
	cp sdk/gears_init.js $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/inspector/common
	cp sdk/samples/sample.js $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/inspector/common
endif
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/content
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/locale
	cp $(FF3_RESOURCES) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/content
	cp $(COMMON_RESOURCES) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/content
	cp -R $(FF3_OUTDIR)/genfiles/i18n/* $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/locale
	cp -R $(COMMON_OUTDIR)/genfiles/i18n/* $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/locale
ifneq ($(OS),osx)
	cp $(FF3_MODULE_TYPELIB) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	cp $(FF3_MODULE_DLL) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
	cp $(FF2_MODULE_DLL) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(DLL_PREFIX)$(MODULE)_ff2$(DLL_SUFFIX)
ifeq ($(MODE),dbg)
ifdef IS_WIN32_OR_WINCE
	cp $(FF3_OUTDIR)/$(MODULE).pdb $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(MODULE).pdb
	cp $(FF2_OUTDIR)/$(MODULE).pdb $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(MODULE)_ff2.pdb
endif
endif
else
	cp $(FF3_MODULE_TYPELIB) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	cp $(FF3_MODULE_DLL) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
	cp $(FF2_MODULE_DLL) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(DLL_PREFIX)$(MODULE)_ff2$(DLL_SUFFIX)
	cp $(OSX_LAUNCHURL_EXE) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/
endif
ifeq ($(OS),osx)
    # notifier_test isn't yet supported on OSX.
else
ifeq ($(USING_CCTESTS),1)
	cp $(NOTIFIER_TEST_EXE) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
endif
endif
    # Mark files writeable to allow .xpi rebuilds
	chmod -R 777 $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/*
	(cd $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME) && zip -r ../$(INSTALLER_BASE_NAME).xpi .)

$(SF_PLUGIN_BUNDLE): $(CRASH_SENDER_EXE) $(OSX_CRASH_INSPECTOR_EXE) $(OSX_LAUNCHURL_EXE) $(SF_MODULE_DLL) $(SF_M4FILES) $(SF_M4FILES_I18N)
# --- Gears.plugin ---
# Create fresh copies of the Gears.plugin directories.
	rm -rf $@
	mkdir -p $@/Contents/Resources/English.lproj
	mkdir -p $@/Contents/MacOS
# Add Info.plist file & localized strings.
	cp $($(BROWSER)_OUTDIR)/genfiles/Info.plist $@/Contents/
	cp tools/osx/English.lproj/InfoPlist.strings $@/Contents/Resources/English.lproj/InfoPlist.strings
# Copy breakpad exes.
	cp -r $(CRASH_SENDER_EXE) $@/Contents/Resources/
	cp -r $(OSX_CRASH_INSPECTOR_EXE) $@/Contents/Resources/
# Copy the actual plugin.
	cp  "$(SF_MODULE_DLL)" "$@/Contents/MacOS/Gears"
# Copy launch_url
	mkdir -p $@/Contents/Resources/
	cp "$(OSX_LAUNCHURL_EXE)" "$@/Contents/Resources/"
	/usr/bin/touch -c $@

$(SF_INPUTMANAGER_BUNDLE): $(SF_INPUTMANAGER_EXE)
# Create fresh copies of the GoogleGearsEnabler directories.
	rm -rf $@
	mkdir -p $@/GearsEnabler.bundle/Contents/Resources/English.lproj/
	mkdir -p $@/GearsEnabler.bundle/Contents/MacOS
# Add Info Info.plist file & localized strings.
	cat tools/osx/Enabler-Info.plist | sed 's/$${EXECUTABLE_NAME}/GearsEnabler/' | sed 's/$${PRODUCT_NAME}/GearsEnabler/' > $@/GearsEnabler.bundle/Contents/Info.plist
	cp tools/osx/Info $@/
	cp tools/osx/English.lproj/InfoPlist.strings $@/GearsEnabler.bundle/Contents/Resources/English.lproj/InfoPlist.strings
# Copy the InputManager.
	cp "$(SF_INPUTMANAGER_EXE)" "$@/GearsEnabler.bundle/Contents/MacOS/"
	/usr/bin/touch -c $@/GearsEnabler.bundle

WIN32_INSTALLER_WIXOBJ = $(COMMON_OUTDIR)/win32_msi.wxiobj
$(WIN32_INSTALLER_MSI): $(WIN32_INSTALLER_WIXOBJ) $(IE_MODULE_DLL) $(FFMERGED_INSTALLER_XPI)
	light.exe -out $@ $(WIN32_INSTALLER_WIXOBJ)

$(WINCE_INSTALLER_CAB): $(INFSRC) $(IE_MODULE_DLL) $(IE_WINCESETUP_DLL)
	cabwiz.exe $(INFSRC) /compress /err $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME).log
	mv -f $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME).cab $@

$(SYMBIAN_SIS_FILE) : $(NPAPI_MODULE_DLL) $(SYMBIAN_RSC_FILE) $(SYMBIAN_PKG_FILE)
	@makesis $(SYMBIAN_PKG_FILE)
  
$(SYMBIAN_INSTALLER_SISX) : $(SYMBIAN_SIS_FILE) $(SYMBIAN_CER_FILE) $(SYMBIAN_KEY_FILE)
	@signsis $(SYMBIAN_SIS_FILE) $@ $(SYMBIAN_CER_FILE) $(SYMBIAN_KEY_FILE)

$(SYMBIAN_RSC_FILE) : $(SYMBIAN_RSS_FILE)
	@perl -S epocrc.pl $(RC_FLAGS) -u $< -o$@

$(SYMBIAN_EMULATOR_INSTALLER) : $(NPAPI_MODULE_DLL) $(SYMBIAN_RSC_FILE)
	@cp $< $@
	@cp $(SYMBIAN_RSC_FILE) "$(EPOCROOT)\release\winscw\udeb\z\resource\plugins"

# We generate dependency information for each source file as it is compiled.
# Here, we include the generated dependency information, which silently fails
# if the files do not exist.
-include $(DEPS)
