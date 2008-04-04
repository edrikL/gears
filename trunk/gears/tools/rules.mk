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
FF_OUTDIR           = $(OUTDIR)/$(OS)-$(ARCH)/ff
IE_OUTDIR           = $(OUTDIR)/$(OS)-$(ARCH)/ie
NPAPI_OUTDIR        = $(OUTDIR)/$(OS)-$(ARCH)/npapi
LIBGD_OUTDIR        = $(COMMON_OUTDIR)/gd
SQLITE_OUTDIR       = $(COMMON_OUTDIR)/sqlite
THIRD_PARTY_OUTDIR  = $(COMMON_OUTDIR)/third_party
INSTALLERS_OUTDIR   = $(OUTDIR)/installers
VISTA_BROKER_OUTDIR = $(OUTDIR)/$(OS)-$(ARCH)/vista_broker
COMMON_OUTDIRS_I18N = $(foreach lang,$(I18N_LANGS),$(COMMON_OUTDIR)/genfiles/i18n/$(lang))
$(BROWSER)_OUTDIRS_I18N = $(foreach lang,$(I18N_LANGS),$($(BROWSER)_OUTDIR)/genfiles/i18n/$(lang))
# TODO(cprince): unify the Firefox directory name across the output dirs
# (where it is 'ff') and the source dirs (where it is 'firefox').  Changing
# the output dirs would require changing #includes that reference genfiles.

# This is the base directory used for I18N files.  Files used under it
# will keep their relative sub-directory.
I18N_INPUTS_BASEDIR = ui/generated

COMMON_OBJS = \
	$(patsubst %.cc,$(COMMON_OUTDIR)/%$(OBJ_SUFFIX),$(COMMON_CPPSRCS)) \
	$(patsubst %.c,$(COMMON_OUTDIR)/%$(OBJ_SUFFIX),$(COMMON_CSRCS))
$(BROWSER)_OBJS = \
	$(patsubst %.cc,$($(BROWSER)_OUTDIR)/%$(OBJ_SUFFIX),$($(BROWSER)_CPPSRCS)) \
	$(patsubst %.c,$($(BROWSER)_OUTDIR)/%$(OBJ_SUFFIX),$($(BROWSER)_CSRCS))
# TODO(cprince): Break all ties between OSX_LAUNCHURL and FF_OUTDIR when we
# support a non-Firefox browser on Mac.
OSX_LAUNCHURL_OBJS = \
	$(patsubst %.cc,$(FF_OUTDIR)/%$(OBJ_SUFFIX),$(OSX_LAUNCHURL_CPPSRCS))
VISTA_BROKER_OBJS = \
	$(patsubst %.cc,$(VISTA_BROKER_OUTDIR)/%$(OBJ_SUFFIX),$(VISTA_BROKER_CPPSRCS))
IE_WINCESETUP_OBJS = \
	$(patsubst %.cc,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(IE_WINCESETUP_CPPSRCS))
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
	$(NPAPI_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(LIBGD_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(SQLITE_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(THIRD_PARTY_OBJS:$(OBJ_SUFFIX)=.pp)

$(BROWSER)_GEN_HEADERS = \
	$(patsubst %.idl,$($(BROWSER)_OUTDIR)/genfiles/%.h,$($(BROWSER)_IDLSRCS))

FF_GEN_TYPELIBS = \
	$(patsubst %.idl,$(FF_OUTDIR)/genfiles/%.xpt,$(FF_IDLSRCS))

IE_OBJS += \
	$(patsubst %.idl,$(IE_OUTDIR)/%_i$(OBJ_SUFFIX),$(IE_IDLSRCS))

NPAPI_GEN_TYPELIBS = \
	$(patsubst %.idl,$(NPAPI_OUTDIR)/genfiles/%.xpt,$(NPAPI_IDLSRCS))

COMMON_M4FILES = \
	$(patsubst %.m4,$(COMMON_OUTDIR)/genfiles/%,$(COMMON_M4SRCS))
$(BROWSER)_M4FILES = \
	$(patsubst %.m4,$($(BROWSER)_OUTDIR)/genfiles/%,$($(BROWSER)_M4SRCS))

COMMON_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $(COMMON_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$(COMMON_M4SRCS_I18N))))
$(BROWSER)_M4FILES_I18N = \
	$(foreach lang,$(I18N_LANGS),$(addprefix $($(BROWSER)_OUTDIR)/genfiles/i18n/$(lang)/,$(patsubst %.m4,%,$($(BROWSER)_M4SRCS_I18N))))


$(BROWSER)_VPATH += $($(BROWSER)_OUTDIR)/genfiles
IE_VPATH += $(IE_OUTDIR)
IE_VPATH += $(VISTA_BROKER_OUTDIR)

# Make VPATH search our paths before third-party paths.
VPATH += $(COMMON_VPATH) $($(BROWSER)_VPATH) $(THIRD_PARTY_VPATH)

#-----------------------------------------------------------------------------
# OUTPUT FILENAMES

# no ARCH in INSTALLER_BASE_NAME because we created merged installers
INSTALLER_BASE_NAME = $(MODULE)-$(OS)-$(MODE)-$(VERSION)

$(BROWSER)_MODULE_DLL = $($(BROWSER)_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)

FF_MODULE_TYPELIB = $(FF_OUTDIR)/$(MODULE).xpt

FF_INSTALLER_XPI = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).xpi

IE_WINCESETUP_DLL = $(IE_OUTDIR)/setup$(DLL_SUFFIX)

OSX_LAUNCHURL_EXE = $(FF_OUTDIR)/launch_url_with_browser

# Note: We use IE_OUTDIR so that relative path from gears.dll is same in
# development environment as deployment environment.
VISTA_BROKER_EXE = $(IE_OUTDIR)/vista_broker.exe

WIN32_INSTALLER_MSI = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).msi
WIXOBJ = $(COMMON_OUTDIR)/win32_msi.wxiobj
WIXSRC = $(COMMON_OUTDIR)/genfiles/win32_msi.wxs

WINCE_INSTALLER_CAB = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).cab
INFSRC_BASE_NAME = wince_cab
INFSRC = $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME).inf


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

        # For win32, also build a cross-browser MSI.
	$(MAKE) win32_installer

  else
  ifeq ($(OS),wince)
	$(MAKE) prereqs    BROWSER=IE
	$(MAKE) genheaders BROWSER=IE
	$(MAKE) modules    BROWSER=IE
	$(MAKE) installer  BROWSER=IE

	$(MAKE) wince_installer
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


# Cross-browser targets.
prereqs::     $(COMMON_OUTDIR)     $(COMMON_OUTDIR)/genfiles     $(COMMON_OUTDIRS_I18N)     $(COMMON_M4FILES)     $(COMMON_M4FILES_I18N)
prereqs:: $($(BROWSER)_OUTDIR) $($(BROWSER)_OUTDIR)/genfiles $($(BROWSER)_OUTDIRS_I18N) $($(BROWSER)_M4FILES) $($(BROWSER)_M4FILES_I18N)
prereqs:: $(LIBGD_OUTDIR) $(SQLITE_OUTDIR) $(THIRD_PARTY_OUTDIR) $(INSTALLERS_OUTDIR)
modules::
genheaders:: $($(BROWSER)_GEN_HEADERS)
installer::

# Browser-specific targets.
ifeq ($(BROWSER),FF)
modules:: $(FF_MODULE_DLL) $(FF_MODULE_TYPELIB)
ifeq ($(OS),osx)
modules:: $(OSX_LAUNCHURL_EXE)
endif
installer:: $(FF_INSTALLER_XPI)
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

# Other targets.
win32_installer:: $(WIN32_INSTALLER_MSI)
wince_installer:: $(WINCE_INSTALLER_CAB)


clean::
ifdef CMD_LINE_MODE  # If MODE is specified on command line.
	rm -rf $(OUTDIR)
else
	rm -rf bin-dbg
	rm -rf bin-opt
endif

help::
	@echo "Usage: make [MODE=dbg|opt] [BROWSER=FF|IE|NPAPI] [OS=wince]"
	@echo
	@echo "  If you omit MODE, the default is dbg."
	@echo "  If you omit BROWSER, all browsers available on the current OS are built."

.PHONY: prereqs genheaders modules clean help

$(COMMON_OUTDIR):
	"mkdir" -p $@
$(COMMON_OUTDIR)/genfiles:
	"mkdir" -p $@
$(COMMON_OUTDIRS_I18N):
	"mkdir" -p $@
$($(BROWSER)_OUTDIR):
	"mkdir" -p $@
$($(BROWSER)_OUTDIR)/genfiles:
	"mkdir" -p $@
$($(BROWSER)_OUTDIRS_I18N):
	"mkdir" -p $@
$(LIBGD_OUTDIR):
	"mkdir" -p $@
$(SQLITE_OUTDIR):
	"mkdir" -p $@
$(THIRD_PARTY_OUTDIR):
	"mkdir" -p $@
$(VISTA_BROKER_OUTDIR):
	"mkdir" -p $@
$(INSTALLERS_OUTDIR):
	"mkdir" -p $@

# M4 (GENERIC PREPROCESSOR) TARGETS

$(COMMON_OUTDIR)/genfiles/%: %.m4
	m4 $(M4FLAGS) $< > $@

$($(BROWSER)_OUTDIR)/genfiles/%: %.m4
	m4 $(M4FLAGS) $< > $@

# I18N M4 (GENERIC PREPROCESSOR) TARGETS

$(COMMON_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@
$($(BROWSER)_OUTDIR)/genfiles/i18n/%: $(I18N_INPUTS_BASEDIR)/%.m4
	m4 $(M4FLAGS) $< > $@

# IDL TARGETS

# Need /base/common in the include path to derive from GearsBaseClassInterface
# (xpidl doesn't like slashes in #include "base_interface_ff.idl")
#
# TODO(cprince): see whether we can remove the extra include paths after
# the 1.9 inclusion is complete.
$(FF_OUTDIR)/genfiles/%.h: %.idl
	$(GECKO_BIN)/xpidl -I base/common -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m header -o $(FF_OUTDIR)/genfiles/$* $<
$(FF_OUTDIR)/genfiles/%.xpt: %.idl
	$(GECKO_BIN)/xpidl -I base/common -I $(GECKO_SDK)/gecko_sdk/idl -I $(GECKO_BASE) -m typelib -o $(FF_OUTDIR)/genfiles/$* $<

$(IE_OUTDIR)/genfiles/%.h: %.idl
	midl $(CPPFLAGS) -env win32 -Oicf -tlb "$(@D)/$*.tlb" -h "$(@D)/$*.h" -iid "$(IE_OUTDIR)/$*_i.c" -proxy "$(IE_OUTDIR)/$*_p.c" -dlldata "$(IE_OUTDIR)/$*_d.c" $<


# Yacc UNTARGET, so we don't try to build sqlite's parse.c from parse.y.
%.c: %.y

# C/C++ TARGETS

$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CFLAGS) $<
$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CXXFLAGS) $<

$($(BROWSER)_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CFLAGS) $<
$($(BROWSER)_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CXXFLAGS) $<

$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CFLAGS) $<
$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CXXFLAGS) $<

$(VISTA_BROKER_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $($(BROWSER)_CPPFLAGS) $($(BROWSER)_CXXFLAGS) $<

# Omit @$(MKDEP) for libgd and sqlite because they include files which aren't
# in the same directory, but don't use explicit paths.  All necessary -I
# flags are in LIBGD_CFLAGS and SQLITE_CFLAGS respectively.
$(LIBGD_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIBGD_CFLAGS) $<

$(SQLITE_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SQLITE_CPPFLAGS) $(SQLITE_CFLAGS) $<


# RESOURCE TARGETS

$(IE_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_IE=1 $<

$(NPAPI_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES)
	$(RC) $(RCFLAGS) /DBROWSER_NPAPI=1 $<

$(VISTA_BROKER_OUTDIR)/%.res: %.rc
	$(RC) $(RCFLAGS) $<

# LINK TARGETS

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
	$(GECKO_BIN)/xpt_link $@ $^

$(IE_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(IE_OBJS) $(IE_LINK_EXTRAS)
	@echo $(IE_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(MKSHLIB) $(SHLIBFLAGS) $(IE_SHLIBFLAGS) $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(IE_LINK_EXTRAS) $(IE_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp 	

# Note the use of SHLIBFLAGS_NOPDB instead of SHLIBFLAGS here.
$(IE_WINCESETUP_DLL): $(IE_WINCESETUP_OBJS) $(IE_WINCESETUP_LINK_EXTRAS)
	$(MKSHLIB) $(SHLIBFLAGS_NOPDB) $(IE_WINCESETUP_LINK_EXTRAS) $(IE_LIBS) $(IE_WINCESETUP_OBJS)

$(NPAPI_MODULE_DLL): $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(NPAPI_OBJS) $(NPAPI_LINK_EXTRAS)
	@echo $(NPAPI_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(MKSHLIB) $(SHLIBFLAGS) $(NPAPI_SHLIBFLAGS) $(COMMON_OBJS) $(LIBGD_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(NPAPI_LINK_EXTRAS) $(NPAPI_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp

$(VISTA_BROKER_EXE): $(VISTA_BROKER_OBJS) $(VISTA_BROKER_OUTDIR)/vista_broker.res
	@echo $(VISTA_BROKER_OBJS) | $(TRANSLATE_LINKER_FILE_LIST) > $(OUTDIR)/obj_list.temp
	$(MKEXE) $(LINKFLAGS) /PDB:"$(@D)/vista_broker.pdb" $(VISTA_BROKER_OUTDIR)/vista_broker.res $($(BROWSER)_LIBS) $(EXT_LINKER_CMD_FLAG)$(OUTDIR)/obj_list.temp
	rm $(OUTDIR)/obj_list.temp

# TODO(cprince): Remove hard-coded build flags here.  Switch to using
# MKEXE + EXEFLAGS when those are added for all platforms.
$(OSX_LAUNCHURL_EXE): $(OSX_LAUNCHURL_OBJS)
	 $(CXX) -o $@ -mmacosx-version-min=10.2 -framework CoreFoundation -framework ApplicationServices -lstdc++ $(OSX_LAUNCHURL_OBJS)

# INSTALLER TARGETS

ifeq ($(OS),osx)
$(FF_INSTALLER_XPI): $(FF_MODULE_DLL) $(FF_MODULE_TYPELIB) $(COMMON_RESOURCES) $(COMMON_M4FILES_I18N) $(FF_RESOURCES) $(FF_M4FILES_I18N) $(FF_OUTDIR)/genfiles/chrome.manifest $(OSX_LAUNCHURL_EXE)
else
$(FF_INSTALLER_XPI): $(FF_MODULE_DLL) $(FF_MODULE_TYPELIB) $(COMMON_RESOURCES) $(COMMON_M4FILES_I18N) $(FF_RESOURCES) $(FF_M4FILES_I18N) $(FF_OUTDIR)/genfiles/chrome.manifest
endif
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
    # For OSX, create universal binaries by combining the ppc and i386 versions.
	/usr/bin/lipo -output $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(notdir $(FF_MODULE_DLL)) -create \
		$(OUTDIR)/$(OS)-i386/ff/$(notdir $(FF_MODULE_DLL)) \
		$(OUTDIR)/$(OS)-ppc/ff/$(notdir $(FF_MODULE_DLL))
    # And copy any xpt file to the output dir. (The i386 and ppc xpt files are identical.)
	cp $(OUTDIR)/$(OS)-i386/ff/$(notdir $(FF_MODULE_TYPELIB)) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
    # Also create a universal binary for OSX_LAUNCHURL_EXE.
	/usr/bin/lipo -output $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/resources/$(notdir $(OSX_LAUNCHURL_EXE)) -create \
		$(OUTDIR)/$(OS)-i386/ff/$(notdir $(OSX_LAUNCHURL_EXE)) \
		$(OUTDIR)/$(OS)-ppc/ff/$(notdir $(OSX_LAUNCHURL_EXE))
endif
    # Mark files writeable to allow .xpi rebuilds
	chmod -R 777 $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/*
	(cd $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME) && zip -r ../$(INSTALLER_BASE_NAME).xpi .)

# We can't list the following as dependencies, because we don't define BROWSER
# for this target, therefore our $(BROWSER)_FOO variables and rules don't exist.
#   $(IE_MODULE_DLL) $(FF_INSTALLER_XPI)
$(WIN32_INSTALLER_MSI): $(WIXOBJ)
	light.exe -out $(WIN32_INSTALLER_MSI) $(WIXOBJ)

# We can't list the following as dependencies, because we don't define BROWSER
# for this target, therefore our $(BROWSER)_FOO variables and rules don't exist.
#   $(IE_MODULE_DLL) $(IE_WINCESETUP_DLL)
$(WINCE_INSTALLER_CAB): $(INFSRC) 
	cabwiz.exe $(INFSRC) /err cabwiz.log /compress
	mv -f $(COMMON_OUTDIR)/genfiles/$(INFSRC_BASE_NAME).cab $(WINCE_INSTALLER_CAB)

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
	  -dOurIEPath=$(OUTDIR)/$(OS)-$(ARCH)/ie \
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
