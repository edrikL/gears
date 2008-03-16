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
# - For IEMOBILE and IE we want to share the IE_OUTDIR variable so that
#     we avoid #ifdefs in the files that include headers from the OUTDIR. 
#     This means that we also need to share the IE_MOBILE_DLL link target 
#     (otherwise we end up with two identical link targets when 
#     building IE for i386).

OUTDIR = bin-$(MODE)

# LIBGD_OUTDIR, SQLITE_OUTDIR and THIRD_PARTY_OUTDIR are separate from
# COMMON_OUTDIR because we want different build flags for them, and flags are
# set per output directory.
#
# INSTALLERS_OUTDIR doesn't include $(ARCH) because OSes that support
# multiple CPU architectures (namely, OSX) have merged install packages.
COMMON_OUTDIR       = $(OUTDIR)/$(OS)-$(ARCH)/common
FF_OUTDIR           = $(OUTDIR)/$(OS)-$(ARCH)/ff
IE_OUTDIR           = $(OUTDIR)/$(OS)-$(ARCH)/ie
NPAPI_OUTDIR        = $(OUTDIR)/$(OS)-$(ARCH)/npapi
LIBGD_OUTDIR        = $(COMMON_OUTDIR)/gd
SQLITE_OUTDIR       = $(COMMON_OUTDIR)/sqlite
THIRD_PARTY_OUTDIR  = $(COMMON_OUTDIR)/third_party
INSTALLERS_OUTDIR   = $(OUTDIR)/installers
VISTA_BROKER_OUTDIR = $(OUTDIR)/$(OS)-$(ARCH)/vista_broker
COMMON_OUTDIRS_I18N = $(foreach lang,$(I18N_LANGS),$(COMMON_OUTDIR)/genfiles/i18n/$(lang))
FF_OUTDIRS_I18N     = $(foreach lang,$(I18N_LANGS),$(FF_OUTDIR)/genfiles/i18n/$(lang))
IE_OUTDIRS_I18N     = $(foreach lang,$(I18N_LANGS),$(IE_OUTDIR)/genfiles/i18n/$(lang))
NPAPI_OUTDIRS_I18N  = $(foreach lang,$(I18N_LANGS),$(NPAPI_OUTDIR)/genfiles/i18n/$(lang))
# TODO(cprince): unify the Firefox directory name across the output dirs
# (where it is 'ff') and the source dirs (where it is 'firefox').  Changing
# the output dirs would require changing #includes that reference genfiles.

# This is the base directory used for I18N files.  Files used under it
# will keep their relative sub-directory.
I18N_INPUTS_BASEDIR = ui/generated

COMMON_OBJS = \
	$(patsubst %.cc,$(COMMON_OUTDIR)/%$(OBJ_SUFFIX),$(COMMON_CPPSRCS)) \
	$(patsubst %.c,$(COMMON_OUTDIR)/%$(OBJ_SUFFIX),$(COMMON_CSRCS))
FF_OBJS = \
	$(patsubst %.cc,$(FF_OUTDIR)/%$(OBJ_SUFFIX),$(FF_CPPSRCS)) \
	$(patsubst %.c,$(FF_OUTDIR)/%$(OBJ_SUFFIX),$(FF_CSRCS)) \
	$(patsubst %.cc,$(FF_OUTDIR)/%$(OBJ_SUFFIX),$(SHARED_CPPSRCS))
IE_OBJS = \
	$(patsubst %.cc,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(IE_CPPSRCS)) \
	$(patsubst %.c,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(IE_CSRCS)) \
	$(patsubst %.cc,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(SHARED_CPPSRCS))
VISTA_BROKER_OBJS = \
	$(patsubst %.cc,$(VISTA_BROKER_OUTDIR)/%$(OBJ_SUFFIX),$(VISTA_BROKER_CPPSRCS))
IEMOBILE_OBJS = \
	$(patsubst %.cc,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(IEMOBILE_CPPSRCS)) \
	$(patsubst %.c,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(IEMOBILE_CSRCS)) \
	$(patsubst %.cc,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(SHARED_CPPSRCS))
IEMOBILE_SETUP_OBJS = \
	$(patsubst %.cc,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(IEMOBILE_SETUP_CPPSRCS))
NPAPI_OBJS = \
	$(patsubst %.cc,$(NPAPI_OUTDIR)/%$(OBJ_SUFFIX),$(NPAPI_CPPSRCS)) \
	$(patsubst %.c,$(NPAPI_OUTDIR)/%$(OBJ_SUFFIX),$(NPAPI_CSRCS)) \
	$(patsubst %.cc,$(NPAPI_OUTDIR)/%$(OBJ_SUFFIX),$(SHARED_CPPSRCS))
LIBGD_OBJS = \
	$(patsubst %.c,$(LIBGD_OUTDIR)/%$(OBJ_SUFFIX),$(LIBGD_CSRCS))
SQLITE_OBJS = \
	$(patsubst %.c,$(SQLITE_OUTDIR)/%$(OBJ_SUFFIX),$(SQLITE_CSRCS))
THIRD_PARTY_OBJS = \
	$(patsubst %.cc,$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX),$(THIRD_PARTY_CPPSRCS)) \
	$(patsubst %.c,$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX),$(THIRD_PARTY_CSRCS))

# IMPORTANT: If you change these lists, you need to change the corresponding
# files in win32_msi.wxs.m4 as well.
# TODO(aa): We should somehow generate win32_msi.wxs because it is lame to
# repeat the file list.
#
# Begin: resource lists that MUST be kept in sync with "win32_msi.wxs.m4"
COMMON_RESOURCES = \
	ui/common/button.css \
	ui/common/button_bg.gif \
	ui/common/html_dialog.css \
	ui/common/html_dialog.js \
	ui/common/icon_32x32.png \
	third_party/jsonjs/json_noeval.js

FF_RESOURCES = \
	$(FF_OUTDIR)/genfiles/browser-overlay.js \
	$(FF_OUTDIR)/genfiles/browser-overlay.xul
# End: resource lists that MUST be kept in sync with "win32_msi.wxs.m4"

DEPS = \
	$(COMMON_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(FF_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(IE_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(VISTA_BROKER_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(IEMOBILE_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(NPAPI_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(LIBGD_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(SQLITE_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(THIRD_PARTY_OBJS:$(OBJ_SUFFIX)=.pp)

FF_GEN_HEADERS = \
	$(patsubst %.idl,$(FF_OUTDIR)/genfiles/%.h,$(FF_IDLSRCS))
FF_GEN_TYPELIBS = \
	$(patsubst %.idl,$(FF_OUTDIR)/genfiles/%.xpt,$(FF_IDLSRCS))

IE_GEN_HEADERS = \
	$(patsubst %.idl,$(IE_OUTDIR)/genfiles/%.h,$(IE_IDLSRCS))
IE_OBJS += \
	$(patsubst %.idl,$(IE_OUTDIR)/%_i$(OBJ_SUFFIX),$(IE_IDLSRCS))

IEMOBILE_GEN_HEADERS = \
	$(patsubst %.idl,$(IE_OUTDIR)/genfiles/%.h,$(IEMOBILE_IDLSRCS))
IEMOBILE_OBJS += \
	$(patsubst %.idl,$(IE_OUTDIR)/%_i$(OBJ_SUFFIX),$(IEMOBILE_IDLSRCS))

NPAPI_GEN_HEADERS = \
	$(patsubst %.idl,$(NPAPI_OUTDIR)/genfiles/%.h,$(NPAPI_IDLSRCS))
NPAPI_GEN_TYPELIBS = \
	$(patsubst %.idl,$(NPAPI_OUTDIR)/genfiles/%.xpt,$(NPAPI_IDLSRCS))

COMMON_M4FILES = \
	$(patsubst %.m4,$(COMMON_OUTDIR)/genfiles/%,$(COMMON_M4SRCS))
FF_M4FILES = \
	$(patsubst %.m4,$(FF_OUTDIR)/genfiles/%,$(FF_M4SRCS))
IE_M4FILES = \
	$(patsubst %.m4,$(IE_OUTDIR)/genfiles/%,$(IE_M4SRCS))
IEMOBILE_M4FILES = \
	$(patsubst %.m4,$(IE_OUTDIR)/genfiles/%,$(IEMOBILE_M4SRCS))
NPAPI_M4FILES = \
	$(patsubst %.m4,$(NPAPI_OUTDIR)/genfiles/%,$(NPAPI_M4SRCS))

COMMON_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $(COMMON_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$(COMMON_M4SRCS_I18N))))
FF_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $(FF_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$(FF_M4SRCS_I18N))))
IE_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $(IE_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$(IE_M4SRCS_I18N))))
IEMOBILE_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $(IE_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$(IEMOBILE_M4SRCS_I18N))))
NPAPI_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $(NPAPI_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$(NPAPI_M4SRCS_I18N))))

FF_VPATH += $(FF_OUTDIR)/genfiles

IE_VPATH += $(IE_OUTDIR)/genfiles
IE_VPATH += $(IE_OUTDIR)
IE_VPATH += $(VISTA_BROKER_OUTDIR)

# The use of IE_OUTDIR for IEMOBILE is intentional. The ARCH variable makes
# IE_OUTDIR different for IE (ARCH=i386) and IEMOBILE (ARCH=arm).
IEMOBILE_VPATH += $(IE_OUTDIR)/genfiles
IEMOBILE_VPATH += $(IE_OUTDIR)

NPAPI_VPATH += $(NPAPI_OUTDIR)/genfiles

# Make VPATH search our paths before third-party paths.
VPATH += $(COMMON_VPATH) $($(BROWSER)_VPATH) $(SHARED_VPATH) $(THIRD_PARTY_VPATH)

#-----------------------------------------------------------------------------
# OUTPUT FILENAMES

# no ARCH in TARGET_BASE_NAME because we created merged installers
INSTALLER_BASE_NAME = $(MODULE)-$(OS)-$(MODE)-$(VERSION)

FF_MODULE_DLL = $(FF_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
FF_MODULE_TYPELIB = $(FF_OUTDIR)/$(MODULE).xpt
FF_INSTALLER_XPI = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).xpi

IE_MODULE_DLL = $(IE_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
# Note: We use IE_OUTDIR so that relative path from gears.dll is same in
# development environment as deployment environment.
VISTA_BROKER_EXE = $(IE_OUTDIR)/vista_broker.exe
WIN32_INSTALLER_MSI = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).msi
WIXOBJ = $(COMMON_OUTDIR)/win32_msi.wxiobj
WIXSRC = $(COMMON_OUTDIR)/genfiles/win32_msi.wxs

NPAPI_MODULE_DLL  = $(NPAPI_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)

IEMOBILE_SETUP_DLL = $(IE_OUTDIR)/setup$(DLL_SUFFIX)
IEMOBILE_INSTALLER_CAB = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).cab
INFSRC_BASE_NAME = wince_cab
INFSRC = $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME).inf

# Launch URL binary for OS X.
LAUNCHURL = launch_url_with_browser

# BUILD TARGETS

default::
ifneq "$(BROWSER)" ""
  # build for just the selected browser
  ifeq ($(OS),osx)
        # For osx, build the non-installer targets for multiple architectures.
	$(MAKE) prereqs    BROWSER=$(BROWSER) ARCH=i386
	$(MAKE) prereqs    BROWSER=$(BROWSER) ARCH=ppc
	$(MAKE) genheaders BROWSER=$(BROWSER) ARCH=i386
	$(MAKE) genheaders BROWSER=$(BROWSER) ARCH=ppc
	$(MAKE) modules    BROWSER=$(BROWSER) ARCH=i386
	$(MAKE) modules    BROWSER=$(BROWSER) ARCH=ppc
	$(MAKE) installer  BROWSER=FF
  else
	$(MAKE) prereqs    BROWSER=$(BROWSER)
	$(MAKE) genheaders BROWSER=$(BROWSER)
	$(MAKE) modules    BROWSER=$(BROWSER)
	$(MAKE) installer  BROWSER=$(BROWSER)
  endif
else
  # build for all browsers valid on this OS
  ifeq ($(OS),linux)
	$(MAKE) prereqs    BROWSER=FF
	$(MAKE) genheaders BROWSER=FF
	$(MAKE) modules    BROWSER=FF
	$(MAKE) installer  BROWSER=FF

  else
  ifeq ($(OS),win32)
	$(MAKE) prereqs    BROWSER=FF
	$(MAKE) genheaders BROWSER=FF
	$(MAKE) modules    BROWSER=FF
	$(MAKE) installer  BROWSER=FF

	$(MAKE) prereqs    BROWSER=IE
	$(MAKE) genheaders BROWSER=IE
	$(MAKE) modules    BROWSER=IE
	$(MAKE) installer  BROWSER=IE

	$(MAKE) prereqs    BROWSER=NPAPI
	$(MAKE) genheaders BROWSER=NPAPI
	$(MAKE) modules    BROWSER=NPAPI
	$(MAKE) installer  BROWSER=NPAPI

        # For win32, also build a cross-browser MSI.
	$(MAKE) win32installer

  else
  ifeq ($(OS),wince)
	$(MAKE) prereqs    BROWSER=IEMOBILE
	$(MAKE) genheaders BROWSER=IEMOBILE
	$(MAKE) modules    BROWSER=IEMOBILE
	$(MAKE) installer  BROWSER=IEMOBILE

  else
  ifeq ($(OS),osx)
        # For osx, build the non-installer targets for multiple architectures.
	$(MAKE) prereqs    BROWSER=FF ARCH=i386
	$(MAKE) prereqs    BROWSER=FF ARCH=ppc
	$(MAKE) genheaders BROWSER=FF ARCH=i386
	$(MAKE) genheaders BROWSER=FF ARCH=ppc
	$(MAKE) modules    BROWSER=FF ARCH=i386
	$(MAKE) modules    BROWSER=FF ARCH=ppc
	$(MAKE) installer  BROWSER=FF
  endif
  endif
  endif
  endif
endif


win32installer:: $(WIN32_INSTALLER_MSI)

prereqs:: $(COMMON_OUTDIR) $(LIBGD_OUTDIR) $(SQLITE_OUTDIR) $(THIRD_PARTY_OUTDIR) $(COMMON_OUTDIR)/genfiles $(COMMON_OUTDIRS_I18N) $(INSTALLERS_OUTDIR)

genheaders::

ifeq ($(BROWSER),FF)
prereqs:: $(FF_OUTDIR)/genfiles $(FF_OUTDIRS_I18N) $(COMMON_M4FILES) $(COMMON_M4FILES_I18N) $(FF_M4FILES) $(FF_M4FILES_I18N)
genheaders:: $(FF_GEN_HEADERS)
installer:: $(FF_INSTALLER_XPI)

ifeq ($(OS),osx)
launchurlhelper:: $(FF_OUTDIR)/genfiles/$(LAUNCHURL) 

$(FF_OUTDIR)/genfiles/$(LAUNCHURL):: $(FF_OUTDIR)/genfiles $(LAUNCHURLHELPER_CPPSRC)
	 g++ $(COMMON_COMPILE_FLAGS) $(CPPFLAGS) -x c++ -mmacosx-version-min=10.2 -arch ppc -arch i386 -framework CoreFoundation -framework ApplicationServices -lstdc++ $(LAUNCHURLHELPER_CPPSRC) -o $(FF_OUTDIR)/genfiles/$(LAUNCHURL)

modules:: $(FF_MODULE_DLL) $(FF_MODULE_TYPELIB) launchurlhelper
else
modules:: $(FF_MODULE_DLL) $(FF_MODULE_TYPELIB)
endif

endif

ifeq ($(BROWSER),IE)
prereqs:: $(IE_OUTDIR)/genfiles $(VISTA_BROKER_OUTDIR) $(IE_OUTDIRS_I18N) $(COMMON_M4FILES) $(COMMON_M4FILES_I18N) $(IE_M4FILES) $(IE_M4FILES_I18N)
genheaders:: $(IE_GEN_HEADERS)
modules:: $(IE_MODULE_DLL) $(VISTA_BROKER_EXE)
endif

ifeq ($(BROWSER),IEMOBILE)
prereqs:: $(IE_OUTDIR)/genfiles $(IE_OUTDIRS_I18N) $(COMMON_M4FILES) $(COMMON_M4FILES_I18N) $(IEMOBILE_M4FILES) $(IEMOBILE_M4FILES_I18N)
genheaders:: $(IEMOBILE_GEN_HEADERS)
modules:: $(IE_MODULE_DLL) $(IEMOBILE_SETUP_DLL)
installer:: $(IEMOBILE_INSTALLER_CAB)
endif

ifeq ($(BROWSER),NPAPI)
prereqs:: $(NPAPI_OUTDIR)/genfiles $(NPAPI_OUTDIRS_I18N) $(COMMON_M4FILES) $(COMMON_M4FILES_I18N) $(NPAPI_M4FILES) $(NPAPI_M4FILES_I18N)
genheaders:: $(NPAPI_GEN_HEADERS)
modules:: $(NPAPI_MODULE_DLL)
endif

clean::
ifdef CMD_LINE_MODE  # If MODE is specified on command line.
	rm -rf $(OUTDIR)
else
	rm -rf bin-dbg
	rm -rf bin-opt
endif

help::
	@echo "Usage: make [MODE=dbg|opt] [BROWSER=FF|IE|IEMOBILE|NPAPI] [OS=wince]"
	@echo
	@echo "  If you omit MODE, the default is dbg."
	@echo "  If you omit BROWSER, all browsers available on the current OS are built."

.PHONY: prereqs genheaders modules clean help

$(COMMON_OUTDIR):
	"mkdir" -p $@
$(LIBGD_OUTDIR):
	"mkdir" -p $@
$(SQLITE_OUTDIR):
	"mkdir" -p $@
$(THIRD_PARTY_OUTDIR):
	"mkdir" -p $@
$(COMMON_OUTDIR)/genfiles:
	"mkdir" -p $@
$(COMMON_OUTDIRS_I18N):
	"mkdir" -p $@
$(FF_OUTDIR)/genfiles:
	"mkdir" -p $@
$(FF_OUTDIRS_I18N):
	"mkdir" -p $@
$(IE_OUTDIR)/genfiles:
	"mkdir" -p $@
$(VISTA_BROKER_OUTDIR):
	"mkdir" -p $@
$(IE_OUTDIRS_I18N):
	"mkdir" -p $@
$(NPAPI_OUTDIR)/genfiles:
	"mkdir" -p $@
$(NPAPI_OUTDIRS_I18N):
	"mkdir" -p $@
$(INSTALLERS_OUTDIR):
	"mkdir" -p $@

# M4 (GENERIC PREPROCESSOR) TARGETS

$(COMMON_OUTDIR)/genfiles/%: %.m4
	m4 $(M4FLAGS) $< > $@

$(FF_OUTDIR)/genfiles/%: %.m4
	m4 $(M4FLAGS) $< > $@

$(IE_OUTDIR)/genfiles/%: %.m4
	m4 $(M4FLAGS) $< > $@

$(NPAPI_OUTDIR)/genfiles/%: %.m4
	m4 $(M4FLAGS) $< > $@

# I18N M4 (GENERIC PREPROCESSOR) TARGETS

$(COMMON_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@
$(FF_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@
$(IE_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@
$(NPAPI_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@

# IDL TARGETS
# Need /base/common in the include path to derive from GearsBaseClassInterface
# (xpidl doesn't like slashes in #include "base_interface_ff.idl")

# todo(cprince): see whether we can remove the third_party/ part after
# the 1.9 inclusion is complete.
$(FF_OUTDIR)/genfiles/%.h: %.idl
	$(GECKO_SDK)/gecko_sdk/bin/xpidl -I base/common -I $(GECKO_SDK)/gecko_sdk/idl -I third_party/gecko_1.8 -m header -o $(FF_OUTDIR)/genfiles/$* $<

$(FF_OUTDIR)/genfiles/%.xpt: %.idl
	$(GECKO_SDK)/gecko_sdk/bin/xpidl -I base/common -I $(GECKO_SDK)/gecko_sdk/idl -I third_party/gecko_1.8 -m typelib -o $(FF_OUTDIR)/genfiles/$* $<

$(IE_OUTDIR)/genfiles/%.h: %.idl
	$(MIDL) $(MIDLFLAGS) $<

# Yacc UNTARGET, so we don't try to build sqlite's parse.c from parse.y.
%.c: %.y

# C/C++ TARGETS

$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CXXFLAGS) $<

$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CFLAGS) $<

$(FF_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(FF_CPPFLAGS) $(FF_CXXFLAGS) $<

$(FF_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FF_CPPFLAGS) $(FF_CFLAGS) $<

# These two targets handle both IE and IEMOBILE by using
# "$(BROWSER)_" to choose the correct lists.
$(IE_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CXXFLAGS) $<

$(IE_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CFLAGS) $<

$(VISTA_BROKER_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CXXFLAGS) $<

$(NPAPI_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(NPAPI_CPPFLAGS) $(NPAPI_CXXFLAGS) $<

$(NPAPI_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(NPAPI_CPPFLAGS) $(NPAPI_CFLAGS) $<

# Omit @$(MKDEP) for libgd and sqlite because they include files which aren't
# in the same directory, but don't use explicit paths.  All necessary -I
# flags are in LIBGD_CFLAGS and SQLITE_CFLAGS respectively.
$(LIBGD_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIBGD_CFLAGS) $<

$(SQLITE_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SQLITE_CPPFLAGS) $(SQLITE_CFLAGS) $<

$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CXXFLAGS) $<

$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CFLAGS) $<

# RESOURCE TARGETS

$(IE_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) $<

$(VISTA_BROKER_OUTDIR)/%.res: %.rc
	$(RC) $(RCFLAGS) $<

$(NPAPI_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_NPAPI=1 $<

# LINK TARGETS

# This target handles both IE and IEMOBILE by using
# "$(BROWSER)_" to choose the correct lists.
$(IE_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $($(BROWSER)_OBJS) $($(BROWSER)_LINK_EXTRAS)
	@echo $($(BROWSER)_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(MKSHLIB) $(SHLIBFLAGS) $($(BROWSER)_SHLIBFLAGS) $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $($(BROWSER)_LINK_EXTRAS) $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp 	

# Note the use of SHLIBFLAGS_NOPDB instead of SHLIBFLAGS here.
$(IEMOBILE_SETUP_DLL): $(IEMOBILE_SETUP_OBJS) $(IEMOBILE_SETUP_LINK_EXTRAS)
	$(MKSHLIB) $(SHLIBFLAGS_NOPDB) $(IEMOBILE_SETUP_LINK_EXTRAS) $($(BROWSER)_LIBS) $(IEMOBILE_SETUP_OBJS)	

$(VISTA_BROKER_EXE): $(VISTA_BROKER_OBJS) $(VISTA_BROKER_OUTDIR)/vista_broker.res
	@echo $(VISTA_BROKER_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(MKEXE) $(LINKFLAGS) /PDB:"$(@D)/vista_broker.pdb" $(VISTA_BROKER_OUTDIR)/vista_broker.res $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp

$(FF_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(FF_OBJS) $(FF_LINK_EXTRAS)
  ifeq ($(OS),linux)
        # TODO(playmobil): Find equivalent of "@args_file" for ld on Linux.
	$(MKSHLIB) $(SHLIBFLAGS) $(FF_SHLIBFLAGS) $(FF_OBJS) $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(FF_LINK_EXTRAS) $(FF_LIBS)
  else
	@echo $(FF_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(MKSHLIB) $(SHLIBFLAGS) $(FF_SHLIBFLAGS) $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(FF_LINK_EXTRAS) $(FF_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp
  endif

$(FF_MODULE_TYPELIB): $(FF_GEN_TYPELIBS)
	$(GECKO_SDK)/gecko_sdk/bin/xpt_link $@ $^

$(NPAPI_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(NPAPI_OBJS) $(NPAPI_LINK_EXTRAS)
	@echo $(NPAPI_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(MKSHLIB) $(SHLIBFLAGS) $(NPAPI_SHLIBFLAGS) $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(NPAPI_LINK_EXTRAS) $(NPAPI_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp

# INSTALLER TARGETS

$(FF_INSTALLER_XPI): $(FF_MODULE_DLL) $(FF_MODULE_TYPELIB) $(COMMON_RESOURCES) $(COMMON_M4FILES_I18N) $(FF_RESOURCES) $(FF_M4FILES_I18N) $(FF_OUTDIR)/genfiles/chrome.manifest	
	rm -rf $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/lib
	cp base/firefox/static_files/components/bootstrap.js $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	cp base/firefox/static_files/lib/updater.js $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/lib
	cp $(FF_OUTDIR)/genfiles/install.rdf $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/install.rdf
	cp $(FF_OUTDIR)/genfiles/chrome.manifest $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome.manifest
ifneq ($(OS),win32)
  # TODO(playmobil): Inspector should be located in extensions dir on win32.
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/inspector
	cp -R inspector/* $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/inspector
	cp sdk/gears_init.js $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/inspector/common
	cp sdk/samples/sample.js $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/inspector/common
endif
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/content
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/locale
	cp $(COMMON_RESOURCES) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/content
	cp $(FF_RESOURCES) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/content
	cp -R $(COMMON_OUTDIR)/genfiles/i18n/* $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/locale
	cp -R $(FF_OUTDIR)/genfiles/i18n/* $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/locale
ifneq ($(OS),osx)
	cp $(FF_MODULE_DLL) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	cp $(FF_MODULE_TYPELIB) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
ifeq ($(MODE),dbg)
ifdef IS_WIN32_OR_WINCE
	cp $(FF_OUTDIR)/$(MODULE).pdb $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
endif
endif
else
	cp $(FF_OUTDIR)/genfiles/$(LAUNCHURL) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources
    # For OSX, create a universal binary by combining the ppc and i386 versions
	/usr/bin/lipo -output $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(notdir $(FF_MODULE_DLL)) -create \
		$(OUTDIR)/$(OS)-i386/ff/$(notdir $(FF_MODULE_DLL)) \
		$(OUTDIR)/$(OS)-ppc/ff/$(notdir $(FF_MODULE_DLL))
    # And copy any xpt file to the output dir. (The i386 and ppc xpt files are identical.)
	cp $(OUTDIR)/$(OS)-i386/ff/$(notdir $(FF_MODULE_TYPELIB)) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
endif
    # Mark files writeable to allow .xpi rebuilds
	chmod -R 777 $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/*
	(cd $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME) && zip -r ../$(INSTALLER_BASE_NAME).xpi .)

$(WIN32_INSTALLER_MSI): $(FF_INSTALLER_XPI) $(IE_MODULE_DLL) $(WIXOBJ)
	light.exe -out $(WIN32_INSTALLER_MSI) $(WIXOBJ)

ifeq ($(OS),win32)
NAMESPACE_GUID = 36F65206-5D4E-4752-9D52-27708E10DA79
OUR_PRODUCT_ID = \
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
OUR_COMPONENT_GUID_NPAPI_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_NPAPI_FILES-$(VERSION))
OUR_COMPONENT_GUID_NPAPI_REGISTRY = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_NPAPI_REGISTRY-$(VERSION))

WIX_DEFINES_I18N = $(foreach lang,$(subst -,_,$(I18N_LANGS)),-dOurComponentGUID_FFLang$(lang)DirFiles=$(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_$(lang)_DIR_FILES-$(VERSION)))
endif

$(WIXOBJ): $(WIXSRC)
	candle.exe -out $(WIXOBJ) $(WIXSRC) \
	  -dOurIEPath=$(IE_OUTDIR) \
	  -dOurFFPath=$(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME) \
	  -dOurGSegmenterDict=third_party/google_segmenter/G_CJK.dic \
	  -dOurProductId=$(OUR_PRODUCT_ID) \
	  -dOurComponentGUID_FFComponentsDirFiles=$(OUR_COMPONENT_GUID_FF_COMPONENTS_DIR_FILES) \
	  -dOurComponentGUID_FFContentDirFiles=$(OUR_COMPONENT_GUID_FF_CONTENT_DIR_FILES) \
	  -dOurComponentGUID_FFDirFiles=$(OUR_COMPONENT_GUID_FF_DIR_FILES) \
	  -dOurComponentGUID_FFLibDirFiles=$(OUR_COMPONENT_GUID_FF_LIB_DIR_FILES) \
	  -dOurComponentGUID_FFRegistry=$(OUR_COMPONENT_GUID_FF_REGISTRY) \
	  -dOurComponentGUID_IEFiles=$(OUR_COMPONENT_GUID_IE_FILES) \
	  -dOurComponentGUID_IERegistry=$(OUR_COMPONENT_GUID_IE_REGISTRY) \
	  -dOurComponentGUID_NPAPIFiles=$(OUR_COMPONENT_GUID_NPAPI_FILES) \
	  -dOurComponentGUID_NPAPIRegistry=$(OUR_COMPONENT_GUID_NPAPI_REGISTRY) \
	  $(WIX_DEFINES_I18N)

# We generate dependency information for each source file as it is compiled.
# Here, we include the generated dependency information, which silently fails
# if the files do not exist.
-include $(DEPS)

ifeq ($(OS),wince)
$(IEMOBILE_INSTALLER_CAB): $(INFSRC) $(IE_MODULE_DLL) $(IEMOBILE_SETUP_DLL)
	cabwiz.exe $(INFSRC) /err cabwiz.log /compress
	mv -f $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME).cab $(IEMOBILE_INSTALLER_CAB)
endif
