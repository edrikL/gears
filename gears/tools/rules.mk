# vim:set ts=8 sw=8 sts=8 noet:

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
# - Quotes around "mkdir" are required so Win32 cmd.exe uses mkdir.exe
#     instead of built-in mkdir command.  (Running mkdir.exe without
#     quotes creates a directory named '.exe'!!)

OUTDIR = bin-$(MODE)/$(OS)

# SQLITE_OUTDIR and THIRD_PARTY_OUTDIR are separate from COMMON_OUTDIR
# because we want different build flags for them, and flags are set per
# output directory.
#
# INSTALLERS_OUTDIR doesn't include $(ARCH) because OSes that support
# multiple CPU architectures (namely, OSX) have merged install packages.
COMMON_OUTDIR	   = $(OUTDIR)/$(ARCH)/common
SQLITE_OUTDIR	   = $(COMMON_OUTDIR)/sqlite
THIRD_PARTY_OUTDIR = $(COMMON_OUTDIR)/third_party
IE_OUTDIR	   = $(OUTDIR)/$(ARCH)/ie
FF_OUTDIR	   = $(OUTDIR)/$(ARCH)/ff
INSTALLERS_OUTDIR  = $(OUTDIR)/installers
# TODO(cprince): unify the Firefox directory name across the output dirs
# (where it is 'ff') and the source dirs (where it is 'firefox').  Changing
# the output dirs would require changing #includes that reference genfiles.

FF_OBJS = \
	$(patsubst %.cc,$(FF_OUTDIR)/%$(OBJ_SUFFIX),$(FF_CPPSRCS)) \
	$(patsubst %.c,$(FF_OUTDIR)/%$(OBJ_SUFFIX),$(FF_CSRCS))
IE_OBJS = \
	$(patsubst %.cc,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(IE_CPPSRCS)) \
	$(patsubst %.c,$(IE_OUTDIR)/%$(OBJ_SUFFIX),$(IE_CSRCS))
COMMON_OBJS = \
	$(patsubst %.cc,$(COMMON_OUTDIR)/%$(OBJ_SUFFIX),$(COMMON_CPPSRCS)) \
	$(patsubst %.c,$(COMMON_OUTDIR)/%$(OBJ_SUFFIX),$(COMMON_CSRCS))
SQLITE_OBJS = \
	$(patsubst %.c,$(SQLITE_OUTDIR)/%$(OBJ_SUFFIX),$(SQLITE_CSRCS))
THIRD_PARTY_OBJS = \
	$(patsubst %.cc,$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX),$(THIRD_PARTY_CPPSRCS)) \
	$(patsubst %.c,$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX),$(THIRD_PARTY_CSRCS))
TEST_OBJS = \
	$(patsubst %.cc,$(COMMON_OUTDIR)/%$(OBJ_SUFFIX),$(TEST_CPPSRCS))

# IMPORTANT: If you change these files, you need to change the corresponding
# files in win32_msi.wxs.m4 as well.
# TODO(aa): We should somehow generate win32_msi.wxs because it is lame to
# repeat the file list.
COMMON_RESOURCES = \
	ui/common/button_row_background.gif \
	ui/common/html_dialog.css \
	ui/common/html_dialog.js \
	third_party/jsonjs/json_noeval.js \
	$(COMMON_OUTDIR)/genfiles/permissions_dialog.html \
	$(COMMON_OUTDIR)/genfiles/settings_dialog.html

FF_RESOURCES = \
	ui/common/icon_32x32.png \
	$(FF_OUTDIR)/genfiles/browser-overlay.*

FF_LOCALE = \
	$(FF_OUTDIR)/genfiles/i18n-en-US.dtd

IE_RESOURCES = \
	ui/common/icon_merged.ico

DEPS = \
	$(FF_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(IE_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(SQLITE_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(THIRD_PARTY_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(COMMON_OBJS:$(OBJ_SUFFIX)=.pp) \
	$(TEST_OBJS:$(OBJ_SUFFIX)=.pp)

FF_GEN_HEADERS = \
	$(patsubst %.idl,$(FF_OUTDIR)/genfiles/%.h,$(FF_IDLSRCS))
FF_GEN_TYPELIBS = \
	$(patsubst %.idl,$(FF_OUTDIR)/genfiles/%.xpt,$(FF_IDLSRCS))

IE_GEN_HEADERS = \
	$(patsubst %.idl,$(IE_OUTDIR)/genfiles/%.h,$(IE_IDLSRCS))
IE_OBJS += \
	$(patsubst %.idl,$(IE_OUTDIR)/%_i$(OBJ_SUFFIX),$(IE_IDLSRCS))

COMMON_M4FILES = \
	$(patsubst %.m4,$(COMMON_OUTDIR)/genfiles/%,$(COMMON_M4SRCS))
FF_M4FILES = \
	$(patsubst %.m4,$(FF_OUTDIR)/genfiles/%,$(FF_M4SRCS))
IE_M4FILES = \
	$(patsubst %.m4,$(IE_OUTDIR)/genfiles/%,$(IE_M4SRCS))

FF_VPATH += $(FF_OUTDIR)/genfiles

IE_VPATH += $(IE_OUTDIR)/genfiles
IE_VPATH += $(IE_OUTDIR)

# Make VPATH search our paths before third-party paths.
VPATH += $(COMMON_VPATH) $($(BROWSER)_VPATH) $(THIRD_PARTY_VPATH)

#-----------------------------------------------------------------------------
# OUTPUT FILENAMES

# no ARCH in TARGET_BASE_NAME because we created merged installers
INSTALLER_BASE_NAME = $(MODULE)-$(OS)-$(MODE)-$(VERSION)

FF_MODULE_DLL     = $(FF_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
FF_MODULE_TYPELIB = $(FF_OUTDIR)/$(MODULE).xpt
FF_INSTALLER_XPI = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).xpi

IE_MODULE_DLL     = $(IE_OUTDIR)/$(DLL_PREFIX)$(MODULE)$(DLL_SUFFIX)
WIN32_INSTALLER_MSI = $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME).msi
WIXOBJ = $(COMMON_OUTDIR)/win32_msi.wxiobj
WIXSRC = $(COMMON_OUTDIR)/genfiles/win32_msi.wxs

# BUILD TARGETS

default::
ifneq "$(BROWSER)" ""
# build for just the selected browser
	make prereqs BROWSER=$(BROWSER)
	make genheaders BROWSER=$(BROWSER)
	make modules BROWSER=$(BROWSER)
	make installer BROWSER=$(BROWSER)
else
    # build for all browsers valid on this OS
ifneq ($(OS),osx)
	make prereqs BROWSER=FF
	make genheaders BROWSER=FF
	make modules BROWSER=FF
else
        # OSX needs to build for each supported architecture
	make prereqs BROWSER=FF ARCH=i386
	make prereqs BROWSER=FF ARCH=ppc
	make genheaders BROWSER=FF ARCH=i386
	make genheaders BROWSER=FF ARCH=ppc
	make modules BROWSER=FF ARCH=i386
	make modules BROWSER=FF ARCH=ppc
endif
	make installer BROWSER=FF
ifeq ($(OS),win32)
	make prereqs BROWSER=IE
	make genheaders BROWSER=IE
	make modules BROWSER=IE
	make installer BROWSER=IE
	make windowsinstaller
endif
endif

windowsinstaller:: $(WIN32_INSTALLER_MSI)

prereqs:: $(COMMON_OUTDIR) $(SQLITE_OUTDIR) $(THIRD_PARTY_OUTDIR) $(COMMON_OUTDIR)/genfiles $(INSTALLERS_OUTDIR)

genheaders::

ifeq ($(BROWSER),FF)
prereqs:: $(FF_OUTDIR)/genfiles $(COMMON_M4FILES) $(FF_M4FILES)
genheaders:: $(FF_GEN_HEADERS)
modules:: $(FF_MODULE_DLL) $(FF_MODULE_TYPELIB)
installer:: $(FF_INSTALLER_XPI)
endif

ifeq ($(BROWSER),IE)
prereqs:: $(IE_OUTDIR)/genfiles $(COMMON_M4FILES) $(IE_M4FILES)
genheaders:: $(IE_GEN_HEADERS)
modules:: $(IE_MODULE_DLL)
endif

clean::
	rm -rf $(OUTDIR)

.PHONY: prereqs genheaders modules clean

$(COMMON_OUTDIR):
	"mkdir" -p $@
$(SQLITE_OUTDIR):
	"mkdir" -p $@
$(THIRD_PARTY_OUTDIR):
	"mkdir" -p $@
$(COMMON_OUTDIR)/genfiles:
	"mkdir" -p $@
$(FF_OUTDIR)/genfiles:
	"mkdir" -p $@
$(IE_OUTDIR)/genfiles:
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

# IDL TARGETS
# Need /base/common in the include path to derive from GearsBaseClassInterface
# (xpidl doesn't like slashes in #include "base_interface_ff.idl")

$(FF_OUTDIR)/genfiles/%.h: %.idl
	$(GECKO_SDK)/bin/xpidl -I base/common -I $(GECKO_SDK)/idl -m header -o $(FF_OUTDIR)/genfiles/$* $<

$(FF_OUTDIR)/genfiles/%.xpt: %.idl
	$(GECKO_SDK)/bin/xpidl -I base/common -I $(GECKO_SDK)/idl -m typelib -o $(FF_OUTDIR)/genfiles/$* $<

$(IE_OUTDIR)/genfiles/%.h: %.idl
	$(MIDL) $(MIDLFLAGS) $<

# Yacc UNTARGET, so we don't try to build sqlite's parse.c from parse.y.
%.c: %.y

# C/C++ TARGETS

$(FF_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(FF_CPPFLAGS) $(FF_CXXFLAGS) $<

$(FF_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FF_CPPFLAGS) $(FF_CFLAGS) $<

$(IE_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(IE_CPPFLAGS) $(IE_CXXFLAGS) $<

$(IE_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(IE_CPPFLAGS) $(IE_CFLAGS) $<

$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CXXFLAGS) $<

$(COMMON_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_CPPFLAGS) $(COMMON_CFLAGS) $<

# Omit @$(MKDEP) in this case because sqlite files include files which
# aren't in the same directory, but doesn't use explicit paths.  All
# necessary -I flags are in SQLITE_CFLAGS.
$(SQLITE_OUTDIR)/%$(OBJ_SUFFIX): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SQLITE_CPPFLAGS) $(SQLITE_CFLAGS) $<

$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.cc
	@$(MKDEP)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CXXFLAGS) $<

$(THIRD_PARTY_OUTDIR)/%$(OBJ_SUFFIX): %.c
	@$(MKDEP)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THIRD_PARTY_CPPFLAGS) $(THIRD_PARTY_CFLAGS) $<

# RESOURCE TARGETS

$(IE_OUTDIR)/%.res: %.rc $(COMMON_RESOURCES) $(IE_RESOURCES)
	$(RC) $(RCFLAGS) $<

# LINK TARGETS

$(IE_MODULE_DLL): $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(IE_OBJS) $(IE_LINK_EXTRAS)
	$(MKSHLIB) $(SHLIBFLAGS) $(IE_SHLIBFLAGS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(IE_OBJS) $(IE_LINK_EXTRAS) $(IE_LIBS)

$(FF_MODULE_DLL): $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(FF_OBJS) $(FF_LINK_EXTRAS)
	$(MKSHLIB) $(SHLIBFLAGS) $(FF_SHLIBFLAGS) $(COMMON_OBJS) $(SQLITE_OBJS) $(THIRD_PARTY_OBJS) $(FF_OBJS) $(FF_LINK_EXTRAS) $(FF_LIBS)

$(FF_MODULE_TYPELIB): $(FF_GEN_TYPELIBS)
	$(GECKO_SDK)/bin/xpt_link $@ $^

# INSTALLER TARGETS

$(FF_INSTALLER_XPI): $(FF_MODULE_DLL) $(FF_MODULE_TYPELIB) $(COMMON_RESOURCES) $(FF_RESOURCES) $(FF_LOCALE) $(FF_OUTDIR)/genfiles/chrome.manifest
	rm -rf $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)
	cp -R base/firefox/static_files/components $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	cp -R base/firefox/static_files/lib        $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/lib
	cp $(FF_OUTDIR)/genfiles/install.rdf $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/install.rdf
	cp $(FF_OUTDIR)/genfiles/chrome.manifest $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome.manifest
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/content
	"mkdir" -p $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/locale/en-US
	cp $(COMMON_RESOURCES) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/content
	cp $(FF_RESOURCES) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/content
	cp $(FF_LOCALE) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/chrome/chromeFiles/locale/en-US/i18n.dtd
ifneq ($(OS),osx)
	cp $(FF_MODULE_DLL) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	cp $(FF_MODULE_TYPELIB) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
ifeq ($(OS),win32)
        # TODO(cprince): remove the PDB before launch, or make it debug-only
	cp $(FF_OUTDIR)/$(MODULE).pdb $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
	chmod -R 777 $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/*
endif
else
    # For OSX, create a universal binary by combining the ppc and i386 versions
	/usr/bin/lipo -output $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components/$(notdir $(FF_MODULE_DLL)) -create \
		$(OUTDIR)/i386/ff/$(notdir $(FF_MODULE_DLL)) \
		$(OUTDIR)/ppc/ff/$(notdir $(FF_MODULE_DLL))
    # And copy any xpt file to the output dir. (The i386 and ppc xpt files are identical.)
	cp $(OUTDIR)/i386/ff/$(notdir $(FF_MODULE_TYPELIB)) $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME)/components
endif
	(cd $(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME) && zip -r ../$(INSTALLER_BASE_NAME).xpi .)

$(WIN32_INSTALLER_MSI): $(FF_INSTALLER_XPI) $(IE_MODULE_DLL) $(WIXOBJ)
	light.exe -out $(WIN32_INSTALLER_MSI) $(WIXOBJ)

ifeq ($(OS),win32)
NAMESPACE_GUID = 36F65206-5D4E-4752-9D52-27708E10DA79
OUR_PRODUCT_ID = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_PRODUCT_ID-$(VERSION))
OUR_PACKAGE_ID = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_PACKAGE_ID-$(VERSION))
OUR_COMPONENT_GUID_FF_COMPONENTS_DIR_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_COMPONENTS_DIR_FILES-$(VERSION))
OUR_COMPONENT_GUID_FF_CONTENT_DIR_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_CONTENT_DIR_FILES-$(VERSION))
OUR_COMPONENT_GUID_FF_DIR_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_DIR_FILES-$(VERSION))
OUR_COMPONENT_GUID_FF_ENUS_DIR_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_ENUS_DIR_FILES-$(VERSION))
OUR_COMPONENT_GUID_FF_LIB_DIR_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_LIB_DIR_FILES-$(VERSION))
OUR_COMPONENT_GUID_FF_REGISTRY = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_FF_REGISTRY-$(VERSION))
OUR_COMPONENT_GUID_IE_FILES = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_IE_FILES-$(VERSION))
OUR_COMPONENT_GUID_IE_REGISTRY = \
  $(shell $(GGUIDGEN) $(NAMESPACE_GUID) OUR_COMPONENT_GUID_IE_REGISTRY-$(VERSION))
endif

$(WIXOBJ): $(WIXSRC)
	candle.exe -out $(WIXOBJ) $(WIXSRC) \
	  -dOurIEPath=$(IE_OUTDIR) \
	  -dOurFFPath=$(INSTALLERS_OUTDIR)/$(INSTALLER_BASE_NAME) \
	  -dOurGSegmenterDict=third_party/google_segmenter/G_CJK.dic \
	  -dOurProductId=$(OUR_PRODUCT_ID) \
	  -dOurPackageId=$(OUR_PACKAGE_ID) \
	  -dOurComponentGUID_FFComponentsDirFiles=$(OUR_COMPONENT_GUID_FF_COMPONENTS_DIR_FILES) \
	  -dOurComponentGUID_FFContentDirFiles=$(OUR_COMPONENT_GUID_FF_CONTENT_DIR_FILES) \
	  -dOurComponentGUID_FFDirFiles=$(OUR_COMPONENT_GUID_FF_DIR_FILES) \
	  -dOurComponentGUID_FFEnUsDirFiles=$(OUR_COMPONENT_GUID_FF_ENUS_DIR_FILES) \
	  -dOurComponentGUID_FFLibDirFiles=$(OUR_COMPONENT_GUID_FF_LIB_DIR_FILES) \
	  -dOurComponentGUID_FFRegistry=$(OUR_COMPONENT_GUID_FF_REGISTRY) \
	  -dOurComponentGUID_IEFiles=$(OUR_COMPONENT_GUID_IE_FILES) \
	  -dOurComponentGUID_IERegistry=$(OUR_COMPONENT_GUID_IE_REGISTRY)

# We generate dependency information for each source file as it is compiled.
# Here, we include the generated dependency information, which silently fails
# if the files do not exist.
-include $(DEPS)
