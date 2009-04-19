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

# SQLITE_OUTDIR and THIRD_PARTY_OUTDIR are separate from
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
IEMOBILE_OUTDIR            = $(OUTDIR)/$(OS)-$(ARCH)/iemobile
NONE_OUTDIR                = $(OUTDIR)/$(OS)-$(ARCH)/none
NPAPI_OUTDIR               = $(OUTDIR)/$(OS)-$(ARCH)/npapi
OPERA_OUTDIR               = $(OUTDIR)/$(OS)-$(ARCH)/opera
SF_OUTDIR                  = $(OUTDIR)/$(OS)-$(ARCH)/safari

OSX_LAUNCHURL_OUTDIR       = $(OUTDIR)/$(OS)-$(ARCH)/launch_url_with_browser
RUN_GEARS_DLL_OUTDIR       = $(OUTDIR)/$(OS)-$(ARCH)/run_gears_dll
SF_INSTALLER_PLUGIN_OUTDIR = $(OUTDIR)/$(OS)-$(ARCH)/installer_plugin
VISTA_BROKER_OUTDIR        = $(OUTDIR)/$(OS)-$(ARCH)/vista_broker

BREAKPAD_OUTDIR            = $(COMMON_OUTDIR)/breakpad
SQLITE_OUTDIR              = $(COMMON_OUTDIR)/sqlite
MOZJS_OUTDIR               = $(COMMON_OUTDIR)/spidermonkey
THIRD_PARTY_OUTDIR         = $(COMMON_OUTDIR)/third_party


# TODO(cprince): unify the Firefox directory name across the output dirs
# (where it is 'ff') and the source dirs (where it is 'firefox').  Changing
# the output dirs would require changing #includes that reference genfiles.

# This is the base directory used for I18N files.  Files used under it
# will keep their relative sub-directory.
I18N_INPUTS_BASEDIR = ui/generated

# Macro to substitute OBJ_SUFFIX for sourcefile suffix.
# Usage: $(call SUBSTITUTE_OBJ_SUFFIX, out_dir, source_file_list)
# Example: $(call SUBSTITUTE_OBJ_SUFFIX, out_dir, a.cc foo.m) yields
#  out_dir/a.o out_dir/foo.o
# In the macro's body, $1 is the output directory and $2 is the list of source
# files.
SOURCECODE_SUFFIXES = c cc cpp m mm s
SUBSTITUTE_OBJ_SUFFIX = $(foreach SUFFIX,$(SOURCECODE_SUFFIXES), \
                           $(patsubst %.$(SUFFIX),$1/%$(OBJ_SUFFIX), \
                             $(filter %.$(SUFFIX), $2) \
                           ) \
                        )
# TODO(playmobil): unify CPPSRCS & CSRCS into CXXSRCS.
BREAKPAD_OBJS            = $(call SUBSTITUTE_OBJ_SUFFIX, $(BREAKPAD_OUTDIR), $(BREAKPAD_CPPSRCS) $(BREAKPAD_CSRCS))
COMMON_OBJS              = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(COMMON_CPPSRCS) $(COMMON_CSRCS) $(COMMON_GEN_CPPSRCS))
$(BROWSER)_OBJS          = $(call SUBSTITUTE_OBJ_SUFFIX, $($(BROWSER)_OUTDIR), $($(BROWSER)_CPPSRCS) $($(BROWSER)_CSRCS))
CRASH_SENDER_OBJS        = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(CRASH_SENDER_CPPSRCS))
OSX_CRASH_INSPECTOR_OBJS = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(OSX_CRASH_INSPECTOR_CPPSRCS))
OSX_LAUNCHURL_OBJS       = $(call SUBSTITUTE_OBJ_SUFFIX, $(OSX_LAUNCHURL_OUTDIR), $(OSX_LAUNCHURL_CPPSRCS))
SF_INPUTMANAGER_OBJS     = $(call SUBSTITUTE_OBJ_SUFFIX, $(SF_OUTDIR), $(SF_INPUTMANAGER_CPPSRCS))
SF_INSTALLER_PLUGIN_OBJS = $(call SUBSTITUTE_OBJ_SUFFIX, $(SF_INSTALLER_PLUGIN_OUTDIR), $(SF_INSTALLER_PLUGIN_CPPSRCS))
SF_PROXY_DLL_OBJS        = $(call SUBSTITUTE_OBJ_SUFFIX, $(SF_OUTDIR), $(SF_PROXY_DLL_CPPSRCS))
MOZJS_OBJS               = $(call SUBSTITUTE_OBJ_SUFFIX, $(MOZJS_OUTDIR), $(MOZJS_CSRCS))
SQLITE_OBJS              = $(call SUBSTITUTE_OBJ_SUFFIX, $(SQLITE_OUTDIR), $(SQLITE_CSRCS))
PERF_TOOL_OBJS           = $(call SUBSTITUTE_OBJ_SUFFIX, $(COMMON_OUTDIR), $(PERF_TOOL_CPPSRCS))
IEMOBILE_WINCESETUP_OBJS = $(call SUBSTITUTE_OBJ_SUFFIX, $(IEMOBILE_OUTDIR), $(IEMOBILE_WINCESETUP_CPPSRCS))
OPERA_WINCESETUP_OBJS    = $(call SUBSTITUTE_OBJ_SUFFIX, $(OPERA_OUTDIR), $(OPERA_WINCESETUP_CPPSRCS))
RUN_GEARS_DLL_OBJS       = $(call SUBSTITUTE_OBJ_SUFFIX, $(RUN_GEARS_DLL_OUTDIR), $(RUN_GEARS_DLL_CPPSRCS) $(RUN_GEARS_DLL_CSRCS))
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
	ui/common/location_data.png \
	$(NULL)

FF3_RESOURCES = \
	$(FF3_OUTDIR)/genfiles/browser-overlay.js \
	$(FF3_OUTDIR)/genfiles/browser-overlay.xul \
	$(FF3_OUTDIR)/genfiles/permissions_dialog.html \
	$(FF3_OUTDIR)/genfiles/settings_dialog.html \
	$(FF3_OUTDIR)/genfiles/shortcuts_dialog.html \
	$(NULL)
# End: resource lists that MUST be kept in sync with "win32_msi.wxs.m4"

OPERA_RESOURCES = \
	$(OPERA_OUTDIR)/genfiles/permissions_dialog.html \
	$(OPERA_OUTDIR)/genfiles/settings_dialog.html \
	$(OPERA_OUTDIR)/genfiles/shortcuts_dialog.html \
	$(OPERA_OUTDIR)/genfiles/blank.gif \
	$(OPERA_OUTDIR)/genfiles/icon_32x32.png \
	$(OPERA_OUTDIR)/genfiles/local_data.png \
	$(OPERA_OUTDIR)/genfiles/location_data.png \
	$(NULL)

DEPS = \
	$($(BROWSER)_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(BREAKPAD_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(COMMON_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(CRASH_SENDER_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(OSX_CRASH_INSPECTOR_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(OSX_LAUNCHURL_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(SF_INSTALLER_PLUGIN_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(PERF_TOOL_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(SF_INPUTMANAGER_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(VISTA_BROKER_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(MOZJS_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(RUN_GEARS_DLL_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(SQLITE_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(THIRD_PARTY_OBJS:$(OBJ_SUFFIX)=.pp)

RGS_FILES = \
  $($(BROWSER)_OUTDIR)/genfiles/bho.rgs \
  $($(BROWSER)_OUTDIR)/genfiles/factory_ie.rgs \
  $($(BROWSER)_OUTDIR)/genfiles/html_dialog_bridge_iemobile.rgs \
  $($(BROWSER)_OUTDIR)/genfiles/module.rgs \
  $($(BROWSER)_OUTDIR)/genfiles/tools_menu_item.rgs \
  $(NULL)

$(BROWSER)_GEN_HEADERS = \
	$(patsubst %.idl,$($(BROWSER)_OUTDIR)/genfiles/%.h,$($(BROWSER)_IDLSRCS))

FF3_GEN_TYPELIBS = \
	$(patsubst %.idl,$(FF3_OUTDIR)/genfiles/%.xpt,$(FF3_IDLSRCS))

IE_OBJS += \
	$(patsubst %.idl,$(IE_OUTDIR)/%_i$(OBJ_SUFFIX),$(IE_IDLSRCS))

IEMOBILE_OBJS += \
	$(patsubst %.idl,$(IEMOBILE_OUTDIR)/%_i$(OBJ_SUFFIX),$(IEMOBILE_IDLSRCS))

NPAPI_GEN_TYPELIBS = \
	$(patsubst %.idl,$(NPAPI_OUTDIR)/genfiles/%.xpt,$(NPAPI_IDLSRCS))

$(BROWSER)_M4FILES = \
	$(patsubst %.m4,$($(BROWSER)_OUTDIR)/genfiles/%,$($(BROWSER)_M4SRCS))
COMMON_M4FILES = \
	$(patsubst %.m4,$(COMMON_OUTDIR)/genfiles/%,$(COMMON_M4SRCS))
# .html m4 files are separate because they have different build rules.
$(BROWSER)_HTML_M4FILES = \
	$(patsubst %_m4,$($(BROWSER)_OUTDIR)/genfiles/%,$($(BROWSER)_HTML_M4SRCS))

$(BROWSER)_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $($(BROWSER)_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$($(BROWSER)_M4SRCS_I18N))))
COMMON_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $(COMMON_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$(COMMON_M4SRCS_I18N))))

$(BROWSER)_BASE64FILES = \
	$(patsubst %.base64,$($(BROWSER)_OUTDIR)/genfiles/%.base64,$($(BROWSER)_BASE64SRCS))
$(BROWSER)_STABFILES = \
	$(patsubst %.stab,$($(BROWSER)_OUTDIR)/genfiles/%,$($(BROWSER)_STABSRCS))

$(BROWSER)_VPATH += $($(BROWSER)_OUTDIR)/genfiles
COMMON_VPATH += $(COMMON_OUTDIR)/genfiles
IE_VPATH += $(IE_OUTDIR)
IEMOBILE_VPATH += $(IEMOBILE_OUTDIR)
IE_VPATH += $(VISTA_BROKER_OUTDIR)
IE_VPATH += $(RUN_GEARS_DLL_OUTDIR)
SF_VPATH += $(SF_OUTDIR)
ifeq ($(OS),osx)
$(BROWSER)_VPATH += $(OSX_LAUNCHURL_OUTDIR) $(SF_INSTALLER_PLUGIN_OUTDIR)
endif

# Make VPATH search our paths before third-party paths.
VPATH += $($(BROWSER)_VPATH) $(COMMON_VPATH) $(BREAKPAD_VPATH) $(THIRD_PARTY_VPATH)

#-----------------------------------------------------------------------------
# OUTPUT FILENAMES

ifeq ($(OS),linux)
# On Linux, we distinguish between the 64-bit and default (32-bit) installers.
ifeq ($(ARCH),x86_64)
INSTALLER_BASE_NAME = $(MODULE)-$(OS)-$(ARCH)-$(MODE)-$(VERSION)
else
INSTALLER_BASE_NAME = $(MODULE)-$(OS)-$(MODE)-$(VERSION)
endif
else
# On OSes other than Linux, there is no ARCH in INSTALLER_BASE_NAME because
# we created merged installers.
INSTALLER_BASE_NAME = $(MODULE)-$(OS)-$(MODE)-$(VERSION)
endif

# Rules for per-OS installers need to reference MODULE variables, but BROWSER
#   is not defined.  So explicitly define all module vars, instead of using:
#   $(BROWSER)_MODULE_DLL = $($(BROWSER)_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
FF2_MODULE_DLL      = $(FF2_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
FF3_MODULE_DLL      = $(FF3_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
IE_MODULE_DLL       = $(IE_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
IEMOBILE_MODULE_DLL = $(IEMOBILE_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
NPAPI_MODULE_DLL    = $(NPAPI_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
# We have to use different names for the IE Mobile and Opera Mobile DLLs on
# WinCE to prevent clashes.
OPERA_MODULE_DLL    = $(OPERA_OUTDIR)/$(DLL_PREFIX)$(MODULE)op$(DLL_SUFFIX)
SF_MODULE_DLL       = $(SF_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)

FF3_MODULE_TYPELIB      = $(FF3_OUTDIR)/$(MODULE).xpt
IEMOBILE_WINCESETUP_DLL = $(IEMOBILE_OUTDIR)/$(DLL_PREFIX)setup$(DLL_SUFFIX)
OPERA_WINCESETUP_DLL    = $(OPERA_OUTDIR)/$(DLL_PREFIX)setup$(DLL_SUFFIX)
SF_INPUTMANAGER_EXE     = $(SF_OUTDIR)/$(EXE_PREFIX)GearsEnabler$(EXE_SUFFIX)

# Note: crash_sender.exe name needs to stay in sync with name used in
# exception_handler_win32.cc and exception_handler_osx/google_breakpad.mm.
CRASH_SENDER_EXE        = $(COMMON_OUTDIR)/$(EXE_PREFIX)crash_sender$(EXE_SUFFIX)
# Note: crash_inspector name needs to stay in sync with name used in
# exception_handler_osx/google_breakpad.mm.
OSX_CRASH_INSPECTOR_EXE = $(COMMON_OUTDIR)/$(EXE_PREFIX)crash_inspector$(EXE_SUFFIX)
SF_PROXY_DLL            = $(COMMON_OUTDIR)/$(DLL_PREFIX)gears_proxy$(DLL_SUFFIX)
OSX_LAUNCHURL_EXE       = $(COMMON_OUTDIR)/$(EXE_PREFIX)launch_url_with_browser$(EXE_SUFFIX)
SF_INSTALLER_PLUGIN_EXE = $(COMMON_OUTDIR)/$(EXE_PREFIX)stats_pane$(EXE_SUFFIX)
PERF_TOOL_EXE           = $(COMMON_OUTDIR)/$(EXE_PREFIX)perf_tool$(EXE_SUFFIX)

# Note: We use IE_OUTDIR because run_gears_dll.exe and gears.dll must reside
# in the same directory to function.
# Note: run_gears_dll.exe name needs to stay in sync with name used in
# ipc_message_queue_test_win32.cc
RUN_GEARS_DLL_EXE = $(IE_OUTDIR)/$(EXE_PREFIX)run_gears_dll$(EXE_SUFFIX)

# Note: We use IE_OUTDIR so that relative path from gears.dll is same in
# development environment as deployment environment.
# Note: vista_broker.exe needs to stay in sync with name used in
# desktop_win32.cc.
# TODO(aa): This can move to common_outdir like crash_sender.exe
VISTA_BROKER_EXE = $(IE_OUTDIR)/$(EXE_PREFIX)vista_broker$(EXE_SUFFIX)


SF_INSTALLER_PLUGIN_BUNDLE = $(INSTALLERS_OUTDIR)/Safari/StatsPane.bundle
SF_PLUGIN_BUNDLE           = $(INSTALLERS_OUTDIR)/Safari/Gears.bundle
SF_PLUGIN_PROXY_BUNDLE     = $(INSTALLERS_OUTDIR)/Safari/Gears.plugin
SF_INPUTMANAGER_BUNDLE     = $(INSTALLERS_OUTDIR)/Safari/GearsEnabler

SF_INSTALLER_PKG       = $(INSTALLERS_OUTDIR)/Safari/Gears.pkg
FFMERGED_INSTALLER_XPI = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).xpi

NPAPI_INSTALLER_MSI    = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)-chrome.msi

WIN32_INSTALLER_MSI    = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).msi

# TODO(steveblock): Should update CAB name for IE Mobile on WinCE.
IEMOBILE_WINCE_INSTALLER_CAB = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).cab
OPERA_WINCE_INSTALLER_CAB    = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)-opera.cab

INFSRC_BASE_NAME = wince_cab
IEMOBILE_INFSRC = $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME)_iemobile.inf
OPERA_INFSRC    = $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME)_op.inf

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

	$(MAKE) prereqs    BROWSER=NONE
	$(MAKE) genheaders BROWSER=NONE
	$(MAKE) modules    BROWSER=NONE

  ifeq ($(ARCH),i386)
	# We don't build for 64-bit Firefox2 (but we do build for 32-bit Firefox2
	# and 64-bit Firefox3).
	$(MAKE) prereqs    BROWSER=FF2
	$(MAKE) genheaders BROWSER=FF2
	$(MAKE) modules    BROWSER=FF2
  endif

	$(MAKE) prereqs    BROWSER=FF3
	$(MAKE) genheaders BROWSER=FF3
	$(MAKE) modules    BROWSER=FF3

	$(MAKE) installers

  else
  ifeq ($(OS),win32)

	$(MAKE) prereqs    BROWSER=NONE
	$(MAKE) genheaders BROWSER=NONE
	$(MAKE) modules    BROWSER=NONE

	$(MAKE) prereqs    BROWSER=FF2
	$(MAKE) genheaders BROWSER=FF2
	$(MAKE) modules    BROWSER=FF2

	$(MAKE) prereqs    BROWSER=FF3
	$(MAKE) genheaders BROWSER=FF3
	$(MAKE) modules    BROWSER=FF3

	$(MAKE) prereqs    BROWSER=IE
	$(MAKE) genheaders BROWSER=IE
	$(MAKE) modules    BROWSER=IE

	$(MAKE) prereqs    BROWSER=NPAPI
	$(MAKE) genheaders BROWSER=NPAPI
	$(MAKE) modules    BROWSER=NPAPI

	$(MAKE) installers

  else
  ifeq ($(OS),wince)

	$(MAKE) prereqs    BROWSER=NONE
	$(MAKE) genheaders BROWSER=NONE
	$(MAKE) modules    BROWSER=NONE

	$(MAKE) prereqs    BROWSER=IEMOBILE
	$(MAKE) genheaders BROWSER=IEMOBILE
	$(MAKE) modules    BROWSER=IEMOBILE

	$(MAKE) prereqs    BROWSER=OPERA
	$(MAKE) genheaders BROWSER=OPERA
	$(MAKE) modules    BROWSER=OPERA

	$(MAKE) installers

  else
  ifeq ($(OS),osx)
        # For osx, build the non-installer targets for multiple architectures.
	$(MAKE) prereqs    BROWSER=NONE
	$(MAKE) genheaders BROWSER=NONE
	$(MAKE) modules    BROWSER=NONE

	$(MAKE) prereqs    BROWSER=FF2
	$(MAKE) genheaders BROWSER=FF2
	$(MAKE) modules    BROWSER=FF2

	$(MAKE) prereqs    BROWSER=FF3
	$(MAKE) genheaders BROWSER=FF3
	$(MAKE) modules    BROWSER=FF3

	$(MAKE) prereqs    BROWSER=SF
	$(MAKE) genheaders BROWSER=SF
	$(MAKE) modules    BROWSER=SF

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
  else
  ifeq ($(OS),android)
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
prereqs:: $($(BROWSER)_OUTDIR) $($(BROWSER)_OUTDIR)/genfiles $($(BROWSER)_OUTDIRS_I18N) $($(BROWSER)_M4FILES) $($(BROWSER)_M4FILES_I18N) $($(BROWSER)_HTML_M4FILES) $($(BROWSER)_STABFILES)
prereqs::     $(COMMON_OUTDIR)     $(COMMON_OUTDIR)/genfiles     $(COMMON_OUTDIRS_I18N)     $(COMMON_M4FILES)     $(COMMON_M4FILES_I18N)                                                    $(patsubst %,$(COMMON_OUTDIR)/genfiles/%,$(COMMON_GEN_CPPSRCS))
prereqs:: $(BREAKPAD_OUTDIR) $(INSTALLERS_OUTDIR) $(SQLITE_OUTDIR) $(THIRD_PARTY_OUTDIR)
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
ifeq ($(USING_CCTESTS),1)
prereqs:: $(RUN_GEARS_DLL_OUTDIR)
modules:: $(RUN_GEARS_DLL_EXE)
endif
endif
endif

ifeq ($(BROWSER),IEMOBILE)
modules:: $(IEMOBILE_MODULE_DLL)
modules:: $(IEMOBILE_WINCESETUP_DLL)
endif

ifeq ($(BROWSER),NPAPI)
modules:: $(NPAPI_MODULE_DLL)
endif

ifeq ($(BROWSER),OPERA)
ifeq ($(OS),wince)
# Add dependency on OPERA_MODULE_DLL for Win32 once we have Gears for Opera on
# desktop.
modules:: $(OPERA_MODULE_DLL)
modules:: $(OPERA_WINCESETUP_DLL)
endif
endif

ifeq ($(USING_MOZJS),1)
prereqs:: $(MOZJS_OUTDIR)
endif
ifeq ($(BROWSER),SF)
modules:: $(SF_MODULE_DLL) $(SF_INPUTMANAGER_EXE) $(SF_PLUGIN_PROXY_BUNDLE) $(SF_INPUTMANAGER_BUNDLE)
endif

# OS-specific targets.
# Note that the 'prereqs' and 'modules' targets should only be built
# when BROWSER is 'NONE'. 'installers' targets are built without any
# BROWSER value set.
ifeq ($(BROWSER), NONE)
# If one of these OS names matches, the 'findstring' result is not empty.
ifeq ($(OS),win32)
# TODO(aa): Should this run on wince too?
# TODO(aa): Implement crash senders for more platforms
modules:: $(CRASH_SENDER_EXE)
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
prereqs:: $(OSX_LAUNCHURL_OUTDIR) $(SF_INSTALLER_PLUGIN_OUTDIR)
modules:: $(OSX_LAUNCHURL_EXE) $(SF_INSTALLER_PLUGIN_EXE)
installers:: $(SF_INSTALLER_PKG) $(FFMERGED_INSTALLER_XPI)
else
ifeq ($(OS),win32)
installers:: $(FFMERGED_INSTALLER_XPI) $(WIN32_INSTALLER_MSI) $(NPAPI_INSTALLER_MSI)
else
ifeq ($(OS),wince)
installers:: $(IEMOBILE_WINCE_INSTALLER_CAB) $(OPERA_WINCE_INSTALLER_CAB)
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

ifeq ($(OS),android)
# Rule to ensure classes_to_load.inc is generated and class files are
# compiled before module.inc.
$($(BROWSER)_OUTDIR)/module$(OBJ_SUFFIX): \
		$($(BROWSER)_OUTDIR)/genfiles/classes_to_load.inc

# Generate a symlink from asm -> kernel/include/asm-arm. This is
# required because the Linux headers refer to "asm", and the SDK does
# not provide the link.
$($(BROWSER)_OUTDIR)/symlinks:
	mkdir -p $@

$($(BROWSER)_OUTDIR)/symlinks/asm: $($(BROWSER)_OUTDIR)/symlinks
	ln -sf $(ANDROID_BUILD_TOP)/kernel/include/asm-arm $@

prereqs:: $($(BROWSER)_OUTDIR)/symlinks/asm

# Compress the HTML files
modules:: $(patsubst %.html,$($(BROWSER)_OUTDIR)/genfiles/%.html.compress,$($(BROWSER)_HTML_COMPRESSED_FILES))

$($(BROWSER)_OUTDIR)/genfiles/%.html.compress: \
		$($(BROWSER)_OUTDIR)/genfiles/%.html
	$(JS_COMPRESSION_TOOL) $< $(HTML_LOCALE) $($(BROWSER)_OUTDIR)/genfiles

# Installer (build the zip)
installers:: $(ANDROID_INSTALLER_ZIP_PACKAGE)

# Rule to invoke the install.sh script with the detected environment
# variables and MODE set. This install Gears directly into a live
# emulator.
.PHONY:	adb-install
adb-install:
	ANDROID_BUILD_TOP=$(ANDROID_BUILD_TOP) \
		ANDROID_PRODUCT_OUT=$(ANDROID_PRODUCT_OUT) \
		ANDROID_TOOLCHAIN=$(ANDROID_TOOLCHAIN) \
		$(ADB_INSTALL) $(MODE)
endif # android

clean::
ifdef CMD_LINE_MODE  # If MODE is specified on command line.
	rm -rf $(OUTDIR)
else
	rm -rf bin-dbg
	rm -rf bin-opt
endif

help::
	$(ECHO) "Usage: make [MODE=dbg|opt] [BROWSER=FF|IE|IEMOBILE|NPAPI|OPERA] [OS=wince]"
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
$(BREAKPAD_OUTDIR):
	"mkdir" -p $@
$(COMMON_OUTDIR):
	"mkdir" -p $@
$(COMMON_OUTDIR)/genfiles:
	"mkdir" -p $@
$(COMMON_OUTDIRS_I18N):
	"mkdir" -p $@
$(INSTALLERS_OUTDIR):
	"mkdir" -p $@
$(MOZJS_OUTDIR):
	"mkdir" -p $@
$(OSX_LAUNCHURL_OUTDIR):
	"mkdir" -p $@
$(RUN_GEARS_DLL_OUTDIR):
	"mkdir" -p $@
$(SF_INSTALLER_PLUGIN_OUTDIR):
	"mkdir" -p $@
$(SQLITE_OUTDIR):
	"mkdir" -p $@
$(THIRD_PARTY_OUTDIR):
	"mkdir" -p $@
$(VISTA_BROKER_OUTDIR):
	"mkdir" -p $@

# Base64 targets
$($(BROWSER)_OUTDIR)/genfiles/%.png.base64: ui/common/%.png
	uuencode -m $< gears | sed '1d;$$d' > $@

# M4 (GENERIC PREPROCESSOR) TARGETS

# HTML files depend on their string table.
$($(BROWSER)_OUTDIR)/genfiles/%.html: %.html_m4 $($(BROWSER)_OUTDIR)/genfiles/%.js $($(BROWSER)_BASE64FILES)
	m4 $(M4FLAGS) $< > $@

$($(BROWSER)_OUTDIR)/genfiles/%: %.m4 $($(BROWSER)_BASE64FILES)
	m4 $(M4FLAGS) $< > $@
$(COMMON_OUTDIR)/genfiles/%: %.m4
	m4 $(M4FLAGS) $< > $@

# I18N M4 (GENERIC PREPROCESSOR) TARGETS

$($(BROWSER)_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@
$(COMMON_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@

# STAB (String Table) TARGETS

$($(BROWSER)_OUTDIR)/genfiles/%: %.stab
	python tools/parse_stab.py $(M4FLAGS) $@ $< $(I18N_INPUTS_BASEDIR)

# GENERATED CPPSRCS FROM BINARIES (.from_bin.cc)

$(COMMON_OUTDIR)/genfiles/%.from_bin.cc: %
	python tools/bin2cpp.py $< > $@

# IDL TARGETS

# TODO(cprince): see whether we can remove the extra include paths after
# the 1.9 inclusion is complete.
$(FF2_OUTDIR)/genfiles/%.h: %.idl
	$(GECKO_BIN)/xpidl -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m header -o $(FF2_OUTDIR)/genfiles/$* $<
$(FF2_OUTDIR)/genfiles/%.xpt: %.idl
	$(GECKO_BIN)/xpidl -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m typelib -o $(FF2_OUTDIR)/genfiles/$* $<

$(FF3_OUTDIR)/genfiles/%.h: %.idl
	$(GECKO_BIN)/xpidl -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m header -o $(FF3_OUTDIR)/genfiles/$* $<
$(FF3_OUTDIR)/genfiles/%.xpt: %.idl
	$(GECKO_BIN)/xpidl -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m typelib -o $(FF3_OUTDIR)/genfiles/$* $<

$(IE_OUTDIR)/genfiles/%.h: %.idl
	midl $(CPPFLAGS) -env win32 -Oicf -tlb "$(@D)/$*.tlb" -h "$(@D)/$*.h" -iid "$(IE_OUTDIR)/$*_i.c" -proxy "$(IE_OUTDIR)/$*_p.c" -dlldata "$(IE_OUTDIR)/$*_d.c" $<

$(IEMOBILE_OUTDIR)/genfiles/%.h: %.idl
	midl $(CPPFLAGS) -env win32 -Oicf -tlb "$(@D)/$*.tlb" -h "$(@D)/$*.h" -iid "$(IEMOBILE_OUTDIR)/$*_i.c" -proxy "$(IEMOBILE_OUTDIR)/$*_p.c" -dlldata "$(IEMOBILE_OUTDIR)/$*_d.c" $<

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

$(BREAKPAD_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BREAKPAD_CPPFLAGS) $(BREAKPAD_CFLAGS) $<
$(BREAKPAD_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(BREAKPAD_CPPFLAGS) $(BREAKPAD_CXXFLAGS) $<

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

$(OSX_LAUNCHURL_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CXXFLAGS) $<

$(RUN_GEARS_DLL_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CFLAGS) $<
$(RUN_GEARS_DLL_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CXXFLAGS) $<

$(SF_INSTALLER_PLUGIN_OUTDIR)/%$(OBJ_SUFFIX): %.m
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CFLAGS) $<

$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CFLAGS) $<
$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CXXFLAGS) $<
$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.cpp
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CXXFLAGS) $<

$(VISTA_BROKER_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CFLAGS) $<
$(VISTA_BROKER_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CXXFLAGS) $<


# Omit @$(MKDEP) for mozjs because they include files which
# aren't in the same directory, but don't use explicit paths.
$(MOZJS_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MOZJS_CFLAGS) $<
$(MOZJS_OUTDIR)/%$(OBJ_SUFFIX): %.s
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MOZJS_CFLAGS) $<


# Java TARGETS (Android only)
ifeq ($(OS),android)
ifeq ($($(BROWSER)_JAVASRCS),)
$($(BROWSER)_OUTDIR)/genfiles/classes_to_load.inc:
	@echo " No Java sources, creating stub class list."
	@rm -f $@
	@echo "static const char *s_classes_to_load[0];" >$@
else
$($(BROWSER)_OUTDIR)/$(JAVA_PACKAGE_NAME)/%.java: %.aidl
	@echo " Compiling AIDL sources $(notdir $^)"
	@mkdir -p $($(BROWSER)_OUTDIR)/$(JAVA_PACKAGE_NAME)
	@cp $< $($(BROWSER)_OUTDIR)/$(JAVA_PACKAGE_NAME)/
	@(cd $($(BROWSER)_OUTDIR) && \
	  aidl $(JAVA_PACKAGE_NAME)/$(notdir $<))
	@sed -i 's/DESCRIPTOR = "$(JAVA_PACKAGE_NAME_DOTS)/DESCRIPTOR = "$(BROWSER_JAVA_PACKAGE_NAME_DOTS)/' $@

$($(BROWSER)_JAR): $(patsubst %.aidl,$($(BROWSER)_OUTDIR)/$(JAVA_PACKAGE_NAME)/%.java,$($(BROWSER)_JAVASRCS))
	@echo " Compiling Java sources $(notdir $^)"
	@rm -f $($(BROWSER)_OUTDIR)/$(JAVA_PACKAGE_NAME)/*.class
	@$(JAVAC) $(JAVAFLAGS) -d $($(BROWSER)_OUTDIR) $(filter %.java, $^)
	@(cd $($(BROWSER)_OUTDIR) && \
          $(DEX) --dex --output=classes.dex $(JAVA_PACKAGE_NAME)/*.class)
	@rm -f $@
	@zip -qj $@ $($(BROWSER)_OUTDIR)/classes.dex

$($(BROWSER)_OUTDIR)/genfiles/classes_to_load.inc: $($(BROWSER)_JAR)
	@echo "static const char *s_classes_to_load[] = {" >$@
	@dexdump $($(BROWSER)_OUTDIR)/classes.dex | grep 'Class descriptor' | sed 's/.*'\''L\(.*\);.*/  "\1",/' >>$@
	@echo "};" >>$@
endif # $(BROWSER)_JAVASRCS
endif # android

# RESOURCE TARGETS

# Strictly, only module.res depends on the .rgs files.
$(IE_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES) $(RGS_FILES)
	$(RC) $(RCFLAGS) /DBROWSER_IE=1 $<

# Strictly, only module.res depends on the .rgs files.
$(IEMOBILE_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES) $(RGS_FILES)
	$(RC) $(RCFLAGS) /DBROWSER_IEMOBILE=1 $<

$(FF2_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_FF2=1 $<

$(FF3_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_FF3=1 $<

$(NPAPI_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_NPAPI=1 $<

$(OPERA_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_OPERA=1 $<

# For Opera, also copy all the image files used in the HTML dialogs to the
# genfiles directory.
$(OPERA_OUTDIR)/genfiles/%.gif: ui/common/%.gif
	cp $< $@
	chmod 600 $(OPERA_OUTDIR)/genfiles/*.gif
$(OPERA_OUTDIR)/genfiles/%.png: ui/common/%.png
	cp $< $@
	chmod 600 $(OPERA_OUTDIR)/genfiles/*.png

$(VISTA_BROKER_OUTDIR)/%.res: %.rc
	$(RC) $(RCFLAGS) /DVISTA_BROKER=1 $<

$(SF_OUTDIR)/%.res: $(COMMON_RESOURCES) $(SF_M4FILES_I18N)
# On Safari, the .res file is actually an object file. But we still use
# the .res extension, which lets us run special build steps for resources.

# Bundle ui files into the executable itself by first generating .webarchive files, and then
# including those in the dylib by converting them into .h files with xxd.
# TODO(playmobil): Handle localization correctly.
	tools/osx/webarchiver/webarchiver $(SF_OUTDIR)/permissions_dialog.webarchive $(SF_OUTDIR)/genfiles/permissions_dialog.html $(COMMON_RESOURCES)
	tools/osx/webarchiver/webarchiver $(SF_OUTDIR)/settings_dialog.webarchive $(SF_OUTDIR)/genfiles/settings_dialog.html $(COMMON_RESOURCES)
	tools/osx/webarchiver/webarchiver $(SF_OUTDIR)/shortcuts_dialog.webarchive $(SF_OUTDIR)/genfiles/shortcuts_dialog.html $(COMMON_RESOURCES)
	xxd -i "$(SF_OUTDIR)/settings_dialog.webarchive" > "$($(BROWSER)_OUTDIR)/genfiles/settings_dialog.h"
	xxd -i "$(SF_OUTDIR)/permissions_dialog.webarchive" > "$($(BROWSER)_OUTDIR)/genfiles/permissions_dialog.h"
	xxd -i "$(SF_OUTDIR)/shortcuts_dialog.webarchive" > "$($(BROWSER)_OUTDIR)/genfiles/shortcuts_dialog.h"
# Resources for native dialogs
	xxd -i "ui/common/location_data.png" > "$($(BROWSER)_OUTDIR)/genfiles/location_data.h"
	xxd -i "ui/common/local_data.png" > "$($(BROWSER)_OUTDIR)/genfiles/local_data.h"

	tools/osx/gen_resource_list.py "$($(BROWSER)_OUTDIR)/genfiles/resource_list.h" "$($(BROWSER)_OUTDIR)/genfiles/settings_dialog.h" "$($(BROWSER)_OUTDIR)/genfiles/permissions_dialog.h" "$($(BROWSER)_OUTDIR)/genfiles/shortcuts_dialog.h" "$($(BROWSER)_OUTDIR)/genfiles/location_data.h" "$($(BROWSER)_OUTDIR)/genfiles/local_data.h"

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
$(BROWSER)_OBJS1 = $(wordlist   1, 100, $($(BROWSER)_OBJS))
$(BROWSER)_OBJS2 = $(wordlist 101, 999, $($(BROWSER)_OBJS))

THIRD_PARTY_OBJS1 = $(wordlist 1, 100, $(THIRD_PARTY_OBJS))
THIRD_PARTY_OBJS2 = $(wordlist 101, 999, $(THIRD_PARTY_OBJS))

# WARNING: Must keep the following two rules (FF2|FF3_MODULE_DLL) in sync!
# The only difference should be the rule name.
$(FF2_MODULE_DLL): $(BREAKPAD_OBJS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $($(BROWSER)_OBJS) $($(BROWSER)_LINK_EXTRAS)
  ifeq ($(OS),linux)
        # TODO(playmobil): Find equivalent of "@args_file" for ld on Linux.
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_OBJS) $(BREAKPAD_OBJS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS)
  else
	$(ECHO) $($(BROWSER)_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $($(BROWSER)_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(BREAKPAD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp
  endif
$(FF3_MODULE_DLL): $(BREAKPAD_OBJS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $($(BROWSER)_OBJS) $($(BROWSER)_LINK_EXTRAS)
  ifeq ($(OS),linux)
        # TODO(playmobil): Find equivalent of "@args_file" for ld on Linux.
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_OBJS) $(BREAKPAD_OBJS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS)
  else
	$(ECHO) $($(BROWSER)_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $($(BROWSER)_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(BREAKPAD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp
  endif

$(FF3_MODULE_TYPELIB): $(FF3_GEN_TYPELIBS)
	$(GECKO_BIN)/xpt_link $@ $^

# Split the list of OBJS to avoid "input line is too long" errors.
IE_OBJS1 = $(wordlist 1, 100, $(IE_OBJS))
IE_OBJS2 = $(wordlist 101, 999, $(IE_OBJS))

$(IE_MODULE_DLL): $(BREAKPAD_OBJS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(IE_OBJS) $(IE_LINK_EXTRAS)
	$(ECHO) $(IE_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $(IE_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(BREAKPAD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp

# Split the list of OBJS to avoid "input line is too long" errors.
IEMOBILE_OBJS1 = $(wordlist 1, 100, $(IEMOBILE_OBJS))
IEMOBILE_OBJS2 = $(wordlist 101, 999, $(IEMOBILE_OBJS))

$(IEMOBILE_MODULE_DLL): $(BREAKPAD_OBJS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(IEMOBILE_OBJS) $(IEMOBILE_LINK_EXTRAS)
	$(ECHO) $(IEMOBILE_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $(IEMOBILE_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(BREAKPAD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp

# Note the use of DLLFLAGS_NOPDB instead of DLLFLAGS here.
$(IEMOBILE_WINCESETUP_DLL): $(IEMOBILE_WINCESETUP_OBJS) $(IEMOBILE_WINCESETUP_LINK_EXTRAS)
	$(MKDLL) $(DLLFLAGS_NOPDB) $(IEMOBILE_WINCESETUP_LINK_EXTRAS) $(IEMOBILE_LIBS) $(IEMOBILE_WINCESETUP_OBJS)

ifeq ($(OS),android)
$(NPAPI_MODULE_DLL): $(COMMON_OBJS) $(SQLITE_OBJS) $(MOZJS_OBJS) $(THIRD_PARTY_OBJS) $(NPAPI_OBJS) $(NPAPI_LINK_EXTRAS)
	@echo " Linking $(notdir $@)"
	@$(MKDLL) $(DLLFLAGS) $(NPAPI_DLLFLAGS) $(COMMON_OBJS) \
		$(SQLITE_OBJS) \
		$(MOZJS_OBJS) \
		$(THIRD_PARTY_OBJS) \
		$(NPAPI_LINK_EXTRAS) \
		$(NPAPI_LIBS) $(NPAPI_OBJS) -nostdlib
	@ls -l $@
	@$(CROSS_PREFIX)size $@
	@$(LIST_SYMBOLS) \
		$(CROSS_PREFIX) \
		defined \
		$(wildcard $(ANDROID_PRODUCT_OUT)/system/lib/*.so) \
		> $(NPAPI_OUTDIR)/android_symbols
	@$(LIST_SYMBOLS) \
		$(CROSS_PREFIX) \
		undefined \
		$@ \
		> $@.extern
	@diff -u $(NPAPI_OUTDIR)/android_symbols $@.extern \
		| grep '^\+[^+]' \
		| sed 's/^\+//' \
		| $(CROSS_PREFIX)c++filt \
		> $@.unresolved
	@(if [ -s $@.unresolved ]; \
	  then \
		echo "****************************************"; \
		echo "*** Unresolved symbols not found in  ***"; \
		echo "*** system libraries. List follows.  ***"; \
		echo "****************************************"; \
		cat $@.unresolved; \
		rm -f $@; \
		false; \
	  fi)
else # !android
ifneq ($(OS),symbian)
# Split the list of OBJS to avoid "input line is too long" errors.
NPAPI_OBJS1 = $(wordlist 1, 100, $(NPAPI_OBJS))
NPAPI_OBJS2 = $(wordlist 101, 999, $(NPAPI_OBJS))
$(NPAPI_MODULE_DLL): $(BREAKPAD_OBJS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(NPAPI_OBJS) $(NPAPI_LINK_EXTRAS)
	$(ECHO) $(NPAPI_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $(NPAPI_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(BREAKPAD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp
endif # !symbian
endif # !android

OPERA_OBJS1 = $(wordlist 1, 100, $(OPERA_OBJS))
OPERA_OBJS2 = $(wordlist 101, 999, $(OPERA_OBJS))
$(OPERA_MODULE_DLL): $(BREAKPAD_OBJS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(OPERA_OBJS) $(OPERA_LINK_EXTRAS)
	$(ECHO) $(OPERA_CPPSRCS) > tmp
	$(ECHO) $(OPERA_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $(OPERA_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(BREAKPAD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp

# Note the use of DLLFLAGS_NOPDB instead of DLLFLAGS here.
$(OPERA_WINCESETUP_DLL): $(OPERA_WINCESETUP_OBJS) $(OPERA_WINCESETUP_LINK_EXTRAS)
	$(MKDLL) $(DLLFLAGS_NOPDB) $(OPERA_WINCESETUP_LINK_EXTRAS) $(OPERA_LIBS) $(OPERA_WINCESETUP_OBJS)

$(SF_MODULE_DLL): $(BREAKPAD_OBJS) $(COMMON_OBJS) $(MOZJS_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $($(BROWSER)_OBJS) $($(BROWSER)_LINK_EXTRAS)
	$(ECHO) $($(BROWSER)_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(ECHO) $($(BROWSER)_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(BREAKPAD_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(COMMON_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(MOZJS_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(SQLITE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS1) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(ECHO) $(THIRD_PARTY_OBJS2) | $(TRANSLATE_LINKER_FILE_LIST) >> $(OUTDIR)/obj_list.temp
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
# Dump the symbols and strip the executable
	../third_party/breakpad_osx/src/tools/mac/dump_syms/dump_syms -a ppc $@ > $@_ppc.symbols
	../third_party/breakpad_osx/src/tools/mac/dump_syms/dump_syms -a i386 $@ > $@_i386.symbols
	$(STRIP_EXECUTABLE)
	rm $(OUTDIR)/obj_list.temp

$(SF_PROXY_DLL) : $(SF_PROXY_DLL_OBJS)
	$(MKDLL) $(DLLFLAGS) $($(BROWSER)_DLLFLAGS) $(SF_PROXY_DLL_OBJS)
	$(STRIP_EXECUTABLE)

# Crash inspector is launched by the crashed process from it's exception handler
# and is what actually communicates with the crashed process to extract the
# minidump.  It then launches crash_sender in order to actually send the
# minidump over the wire.
$(OSX_CRASH_INSPECTOR_EXE): $(OSX_CRASH_INSPECTOR_OBJS)
	$(MKEXE) $(EXEFLAGS) $(OSX_CRASH_INSPECTOR_OBJS) -framework Carbon
	$(STRIP_EXECUTABLE)

ifeq ($(OS),win32)
$(CRASH_SENDER_EXE): $(CRASH_SENDER_OBJS)
	$(MKEXE) $(EXEFLAGS) $(CRASH_SENDER_OBJS) advapi32.lib shell32.lib wininet.lib
endif
ifeq ($(OS),osx)
$(CRASH_SENDER_EXE): $(CRASH_SENDER_OBJS)
	$(MKEXE) $(EXEFLAGS) $(CRASH_SENDER_OBJS) -framework Carbon -framework Cocoa -framework Foundation -framework IOKit -framework SystemConfiguration -lstdc++
	$(STRIP_EXECUTABLE)
endif

$(OSX_LAUNCHURL_EXE): $(OSX_LAUNCHURL_OBJS)
	 $(MKEXE) $(EXEFLAGS) -framework CoreFoundation -framework ApplicationServices -lstdc++ $(OSX_LAUNCHURL_OBJS)
	 $(STRIP_EXECUTABLE)

ifeq ($(USING_CCTESTS),1)
$(RUN_GEARS_DLL_EXE): $(RUN_GEARS_DLL_OBJS) $(RUN_GEARS_DLL_LINK_EXTRAS)
	$(ECHO) $(RUN_GEARS_DLL_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(MKEXE) $(EXEFLAGS) $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp
else
.PHONY: $(RUN_GEARS_DLL_EXE)
$(RUN_GEARS_DLL_EXE):
endif

$(SF_INSTALLER_PLUGIN_EXE): $(SF_INSTALLER_PLUGIN_OBJS)
	 $(MKDLL) $(DLLFLAGS) $($(BROWSER)_LINK_EXTRAS) -framework Cocoa -framework InstallerPlugins $(SF_INSTALLER_PLUGIN_OBJS)
	 $(STRIP_EXECUTABLE)

$(SF_INPUTMANAGER_EXE): $(SF_INPUTMANAGER_OBJS)
	 $(MKEXE) $(EXEFLAGS) -framework Foundation -framework AppKit -bundle $(SF_INPUTMANAGER_OBJS)
	$(STRIP_EXECUTABLE)

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
$(SF_INSTALLER_PKG):
	$(warning To create a Safari installer for Gears, you must install Iceberg \
  from http://s.sudre.free.fr/Software/Iceberg.html.  You can install the \
  Safari version manually by running tools/osx/install_gears.sh script)
endif
endif

ifeq ($(OS),android)
# Installer which packages up relevant Android Gears files into a .zip
$(ANDROID_INSTALLER_DLL): $(NPAPI_MODULE_DLL)
	@echo "Copy $<"
	@mkdir -p $(ANDROID_INSTALLER_OUTDIR)
	@cp $< $@
	@echo "Strip $<"
	@$(CROSS_PREFIX)strip $@

# TODO(jripley): The compressed html files are not working. Use the
# uncompressed ones for now.
#$(ANDROID_INSTALLER_OUTDIR)/%.html: $(NPAPI_OUTDIR)/genfiles/%.html.compress
$(ANDROID_INSTALLER_OUTDIR)/%.html: $(NPAPI_OUTDIR)/genfiles/%.html
	@echo "Copy `basename $@`"
	@mkdir -p $(ANDROID_INSTALLER_OUTDIR)
	@cp $< $@

$(ANDROID_INSTALLER_ZIP_PACKAGE): $(patsubst %.html,$(ANDROID_INSTALLER_OUTDIR)/%.html,$(NPAPI_HTML_COMPRESSED_FILES)) $(ANDROID_INSTALLER_DLL)
	@echo "Build Android package"
	@mkdir -p $(ANDROID_INSTALLER_OUTDIR)
	@-rm -f $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).zip
	@(cd $(INSTALLERS_OUTDIR) && zip -r $(INSTALLER_BASE_NAME).zip `basename $(ANDROID_INSTALLER_OUTDIR)`)
	@echo "Clean files"
	@rm -rf $(ANDROID_INSTALLER_OUTDIR)
	@ls -l $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).zip
endif # android

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
	cp $(FF3_MODULE_TYPELIB) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	cp $(FF3_MODULE_DLL) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
ifeq ($(ARCH),i386)
	cp $(FF2_MODULE_DLL) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(DLL_PREFIX)$(MODULE)_ff2$(DLL_SUFFIX)
endif
ifeq ($(OS),osx)
	cp $(OSX_LAUNCHURL_EXE) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/
else # not OSX
ifeq ($(OS),linux)
else # not LINUX (and not OSX)
ifeq ($(MODE),dbg)
ifdef IS_WIN32_OR_WINCE
	cp $(FF3_OUTDIR)/$(MODULE).pdb $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(MODULE).pdb
	cp $(FF2_OUTDIR)/$(MODULE).pdb $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(MODULE)_ff2.pdb
endif
endif
endif # not LINUX
endif # not OSX

    # Mark files writeable to allow .xpi rebuilds
	chmod -R 777 $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/*
	(cd $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME) && zip -r ../$(INSTALLER_BASE_NAME).xpi .)

$(SF_INSTALLER_PLUGIN_BUNDLE): $(SF_INSTALLER_PLUGIN_EXE)
	rm -rf "$@"
	mkdir -p "$@/Contents/Resources/"
	mkdir -p "$@/Contents/Resources/AdvancedStatsSheet.nib"
	mkdir -p "$@/Contents/MacOS"
# Copy Info.plist
	cp "base/safari/advanced_stats_sheet.plist" "$@/Contents/Info.plist"
# Copy binary
	cp "$(SF_INSTALLER_PLUGIN_EXE)" "$@/Contents/MacOS/InstallerPlugin"
# Copy nib file
	cp base/safari/advanced_stats_sheet.nib/* $@/Contents/Resources/AdvancedStatsSheet.nib/

$(SF_PLUGIN_PROXY_BUNDLE): $(SF_PLUGIN_BUNDLE) $(SF_PROXY_DLL)
# --- Gears.plugin ---
# Create fresh copies of the Gears.plugin directories.
	rm -rf $@
	mkdir -p $@/Contents/Resources/
	mkdir -p $@/Contents/MacOS
# Copy Info.plist
	cp $($(BROWSER)_OUTDIR)/genfiles/Info.plist $@/Contents/
# Copy proxy DLL
	cp $(SF_PROXY_DLL) $@/Contents/MacOS/libgears.dylib
# Copy Gears.bundle
	cp -R $(SF_PLUGIN_BUNDLE) $@/Contents/Resources/
# Copy uninstaller
	cp "tools/osx/uninstall.command" "$@/Contents/Resources/"
	/usr/bin/touch -c $@

$(SF_PLUGIN_BUNDLE): $(CRASH_SENDER_EXE) $(OSX_CRASH_INSPECTOR_EXE) $(OSX_LAUNCHURL_EXE) $(SF_MODULE_DLL) $(SF_M4FILES) $(SF_M4FILES_I18N)
# --- Gears.bundle ---
# Create fresh copies of the Gears.bundle directories.
	rm -rf $@
	mkdir -p $@/Contents/Resources/English.lproj
	mkdir -p $@/Contents/MacOS
# Add Info.plist file & localized strings.
	cp $($(BROWSER)_OUTDIR)/genfiles/Info.plist $@/Contents/
	cp tools/osx/English.lproj/InfoPlist.strings $@/Contents/Resources/English.lproj/InfoPlist.strings
# Copy Native dialog resources
	cp -R ui/safari/*.nib $@/Contents/Resources/
# Copy breakpad exes.
	cp -r $(CRASH_SENDER_EXE) $@/Contents/Resources/
	cp -r $(OSX_CRASH_INSPECTOR_EXE) $@/Contents/Resources/
# Copy the actual plugin.
	cp  "$(SF_MODULE_DLL)" "$@/Contents/MacOS/"
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

$(IEMOBILE_WINCE_INSTALLER_CAB): $(IEMOBILE_INFSRC) $(IEMOBILE_MODULE_DLL) $(IEMOBILE_WINCESETUP_DLL)
	cabwiz.exe $(IEMOBILE_INFSRC) /compress /err $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME).log
	mv -f $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME)_iemobile.cab $@

$(OPERA_WINCE_INSTALLER_CAB): $(OPERA_INFSRC) $(OPERA_MODULE_DLL) $(OPERA_WINCESETUP_DLL) $(OPERA_RESOURCES)
	cabwiz.exe $(OPERA_INFSRC) /compress /err $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME).log
	mv -f $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME)_op.cab $@

NPAPI_INSTALLER_WIXOBJ = $(COMMON_OUTDIR)/npapi_msi.wxiobj
# We must disable certain WiX integrity check errors ("ICE") to successfully
# create a per-user installer.
$(NPAPI_INSTALLER_MSI): $(NPAPI_INSTALLER_WIXOBJ) $(NPAPI_MODULE_DLL)
	light.exe -out $@ $(NPAPI_INSTALLER_WIXOBJ) -sice:ICE39 -sice:ICE64 -sice:ICE91

# We generate dependency information for each source file as it is compiled.
# Here, we include the generated dependency information, which silently fails
# if the files do not exist.
-include $(DEPS)
