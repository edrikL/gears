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

include tools/version.mk
include tools/config.mk
include tools/sanity.mk

MODULE		= $(SHORT_NAME)

#-----------------------------------------------------------------------------
# third_party/sqlite_google

THIRD_PARTY_VPATH	+= \
		third_party/sqlite_google/src \
		third_party/sqlite_google/preprocessed \
		third_party/sqlite_google/ext/fts1 \
		third_party/sqlite_google/ext/fts2 \
		$(NULL)

SQLITE_CSRCS	+= \
		alter.c \
		analyze.c \
		attach.c \
		auth.c \
		btree.c \
		build.c \
		callback.c \
		complete.c \
		date.c \
		delete.c \
		expr.c \
		func.c \
		hash.c \
		insert.c \
		legacy.c \
		loadext.c \
		main.c \
		malloc.c \
		opcodes.c \
		os.c \
		os_os2.c \
		os_unix.c \
		os_win.c \
		pager.c \
		parse.c \
		pragma.c \
		prepare.c \
		printf.c \
		random.c \
		select.c \
		table.c \
		tokenize.c \
		trigger.c \
		update.c \
		utf.c \
		util.c \
		vacuum.c \
		vdbe.c \
		vdbeapi.c \
		vdbeaux.c \
		vdbeblob.c \
		vdbefifo.c \
		vdbemem.c \
		vtab.c \
		where.c \
		fts1.c \
		fts1_hash.c \
		fts1_porter.c \
		fts1_tokenizer1.c \
		fts2.c \
		fts2_hash.c \
		fts2_porter.c \
		fts2_tokenizer1.c \
		$(NULL)

ifeq ($(OS),wince)
# WinCE does not yet use icons, so does not need libpng or zlib
else

#-----------------------------------------------------------------------------
# third_party/libpng

THIRD_PARTY_VPATH	+= \
		third_party/libpng \
		$(NULL)

# Common Files
THIRD_PARTY_CSRCS	+= \
		png.c \
		pngerror.c \
		pngget.c \
		pngmem.c \
		pngset.c \
		pngtrans.c \
		$(NULL)
# Reading PNGs
THIRD_PARTY_CSRCS	+= \
		pngpread.c \
		pngread.c \
		pngrio.c \
		pngrtran.c \
		pngrutil.c \
		$(NULL)

ifeq ($(OFFICIAL_BUILD),1)
  # The following files are not yet needed; exclude them to save size.
else
# Writing PNGs
THIRD_PARTY_CSRCS	+= \
		pngwio.c \
		pngwrite.c \
		pngwtran.c \
		pngwutil.c \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# third_party/zlib

THIRD_PARTY_VPATH	+= \
		third_party/zlib \
		$(NULL)

# Common Files
THIRD_PARTY_CSRCS	+= \
		adler32.c \
		zutil.c \
		$(NULL)
# Inflate Algorithm (use inflate or infback, but not both)
THIRD_PARTY_CSRCS	+= \
		inflate.c \
		inffast.c \
		inftrees.c \
		$(NULL)

ifeq ($(OFFICIAL_BUILD),1)
  # The following files are not yet needed; exclude them to save size.
else
# Other Algorithms
THIRD_PARTY_CSRCS	+= \
		compress.c \
		deflate.c \
		uncompr.c \
		$(NULL)
# Other Common Files
THIRD_PARTY_CSRCS	+= \
		crc32.c \
		gzio.c \
		trees.c \
		$(NULL)
endif

endif  # wince

#-----------------------------------------------------------------------------
# third_party/libjpeg

ifneq ($(OFFICIAL_BUILD),1)
ifneq ($(OS),wince)
THIRD_PARTY_VPATH	+= \
		third_party/libjpeg \
		$(NULL)

THIRD_PARTY_CSRCS	+= \
		jcapimin.c \
		jcapistd.c \
		jccoefct.c \
		jccolor.c \
		jcdctmgr.c \
		jchuff.c \
		jcinit.c \
		jcmainct.c \
		jcmarker.c \
		jcmaster.c \
		jcomapi.c \
		jcparam.c \
		jcphuff.c \
		jcprepct.c \
		jcsample.c \
		jctrans.c \
		jdapimin.c \
		jdapistd.c \
		jdatadst.c \
		jdatasrc.c \
		jdcoefct.c \
		jdcolor.c \
		jddctmgr.c \
		jdhuff.c \
		jdinput.c \
		jdmainct.c \
		jdmarker.c \
		jdmaster.c \
		jdmerge.c \
		jdphuff.c \
		jdpostct.c \
		jdsample.c \
		jdtrans.c \
		jerror.c \
		jfdctflt.c \
		jfdctfst.c \
		jfdctint.c \
		jidctflt.c \
		jidctfst.c \
		jidctint.c \
		jidctred.c \
		jquant1.c \
		jquant2.c \
		jutils.c \
		jmemmgr.c \
		jmemnobs.c \
		$(NULL)
endif # not wince
endif # not OFFICIAL_BUILD

#-----------------------------------------------------------------------------
# third_party/libgd

ifneq ($(OFFICIAL_BUILD),1)
ifneq ($(OS),wince)
THIRD_PARTY_VPATH	+= \
		third_party/libgd \
		$(NULL)

LIBGD_CSRCS	+= \
		gd.c \
		gdfx.c \
		gd_security.c \
		gd_gd.c \
		gd_gd2.c \
		gd_io.c \
		gd_io_dp.c \
		gd_gif_in.c \
		gd_gif_out.c \
		gd_io_file.c \
		gd_io_ss.c \
		gd_jpeg.c \
		gd_png.c \
		gd_ss.c \
		gd_topal.c \
		gd_wbmp.c \
		gdcache.c \
		gdfontg.c \
		gdfontl.c \
		gdfontmb.c \
		gdfonts.c \
		gdfontt.c \
		gdft.c \
		gdhelpers.c \
		gdkanji.c \
		gdtables.c \
		gdxpm.c \
		wbmp.c \
		$(NULL)
endif # not wince
endif # not OFFICIAL_BUILD

#-----------------------------------------------------------------------------
# third_party/jsoncpp

THIRD_PARTY_VPATH	+= \
		third_party/jsoncpp \
		$(NULL)

THIRD_PARTY_CPPSRCS	+= \
		json_reader.cc \
		json_value.cc \
		json_writer.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# third_party/breakpad

ifeq ($(OS),win32)
THIRD_PARTY_VPATH	+= \
		third_party/breakpad/src/client/windows/handler \
		third_party/breakpad/src/client/windows/sender \
		third_party/breakpad/src/common/windows \
		$(NULL)

THIRD_PARTY_CPPSRCS	+= \
		exception_handler.cc \
		guid_string.cc \
		crash_report_sender.cc \
		http_upload.cc \
		$(NULL)


FF_VPATH	+= \
		base/common \
		$(NULL)

FF_CPPSRCS	+= \
		exception_handler_win32.cc \
		$(NULL)


IE_VPATH	+= \
		base/common \
		$(NULL)

IE_CPPSRCS	+= \
		exception_handler_win32.cc \
		$(NULL)


NPAPI_VPATH	+= \
		base/common \
		$(NULL)

NPAPI_CPPSRCS	+= \
		exception_handler_win32.cc \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# base/common

COMMON_VPATH	+= \
		base/common \
		installer \
		$(NULL)

COMMON_M4SRCS	+= \
		product_constants.h.m4 \
		product_version.rc.m4 \
		win32_msi.wxs.m4 \
		wince_cab.inf.m4 \
		$(NULL)

COMMON_CPPSRCS	+= \
		$(NULL)
# WARNING: Think carefully before listing any non-third-party C/C++ files in
# COMMON_*SRCS. Such files only get built once per platform (not per browser),
# and BROWSER_* #defines are not obeyed. Instead, use FF_CPPSRCS and IE_CPPSRCS
# when possible.
# One important exception: unit-tested code is only built once per platform, so
# it should be listed in CPPSRCS. Thus it must not use any BROWSER_* #defines,
# directly or indirectly.
#
# TODO(cprince): investigate OS-specific sources (e.g. WIN32_CPPSRCS)


#-----------------------------------------------------------------------------
# base/firefox

FF_VPATH	+= \
		base/common \
		base/firefox \
		$(NULL)

FF_M4SRCS	+= \
		install.rdf.m4 \
		$(NULL)

FF_IDLSRCS	+= \
		base_interface_ff.idl \
		$(NULL)

FF_CPPSRCS	+= \
		async_router.cc \
		base_class.cc \
		base_class_test.cc \
		circular_buffer_test.cc \
		database_name_table.cc \
		dom_utils.cc \
		file.cc \
		file_posix.cc \
		file_test.cc \
		file_win32.cc \
		html_event_monitor.cc \
		html_event_monitor_ff.cc \
		ipc_message_queue_null.cc \
		http_utils.cc \
		js_runner_ff.cc \
		js_runner_ff_marshaling.cc \
		js_runner_utils.cc \
		js_types.cc \
		message_queue.cc \
		message_queue_ff.cc \
		message_service.cc \
		message_service_test.cc \
		module.cc \
		mutex.cc \
		name_value_table.cc \
		ns_file_utils.cc \
		paths.cc \
		paths_ff.cc \
		permissions_db.cc \
		png_utils.cc \
		scoped_refptr_test.cc \
		security_model.cc \
		security_model_test.cc \
		serialization.cc \
		serialization_test.cc \
		shortcut_table.cc \
		sqlite_wrapper.cc \
		stopwatch.cc \
		stopwatch_ff.cc \
		string16.cc \
		string_utils.cc \
		string_utils_test.cc \
		thread_locals.cc \
		url_utils.cc \
		url_utils_ff.cc \
		url_utils_test.cc \
		xpcom_dynamic_load.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# base/ie

IE_VPATH	+= \
		base/common \
		base/ie \
		$(NULL)

IE_M4SRCS	+= \
		bho.rgs.m4 \
		interfaces.idl.m4 \
		module.rgs.m4 \
		$(NULL)

IE_IDLSRCS	+= \
		interfaces.idl \
		$(NULL)

IE_CPPSRCS	+= \
		activex_utils.cc \
		async_router.cc \
		base_class.cc \
		base_class_test.cc \
		bho.cc \
		circular_buffer_test.cc \
		database_name_table.cc \
		detect_version_collision.cc \
		file.cc \
		file_test.cc \
		file_win32.cc \
		html_event_monitor.cc \
		html_event_monitor_ie.cc \
		ipc_message_queue_win32.cc \
		ipc_message_queue_win32_test.cc \
		http_utils.cc \
		ie_version.cc \
		js_runner_ie.cc \
		js_runner_utils.cc \
		js_types.cc \
		message_queue.cc \
		message_queue_ie.cc \
		message_service.cc \
		message_service_test.cc \
		module.cc \
		module_wrapper.cc \
		mutex.cc \
		name_value_table.cc \
		paths.cc \
		paths_ie.cc \
		permissions_db.cc \
		png_utils.cc \
		scoped_refptr_test.cc \
		security_model.cc \
		security_model_test.cc \
		serialization.cc \
		serialization_test.cc \
		shortcut_table.cc \
		sqlite_wrapper.cc \
		stopwatch.cc \
		stopwatch_ie.cc \
		string16.cc \
		string_utils.cc \
		string_utils_test.cc \
		thread_locals.cc \
		url_utils.cc \
		url_utils_ie.cc \
		url_utils_test.cc \
		vista_utils.cc \
		$(NULL)

IE_LINK_EXTRAS	+= \
		$(IE_OUTDIR)/module.res \
		$(NULL)

#-----------------------------------------------------------------------------
# base/iemobile

IEMOBILE_VPATH	+= \
		base/common \
		base/ie \
		$(NULL)

IEMOBILE_M4SRCS	+= \
		bho.rgs.m4 \
		interfaces.idl.m4 \
		module.rgs.m4 \
		$(NULL)

IEMOBILE_IDLSRCS	+= \
		interfaces.idl \
		$(NULL)

IEMOBILE_CPPSRCS	+= \
		activex_utils.cc \
		async_router.cc \
		base_class.cc \
		bho.cc \
		circular_buffer_test.cc \
		common_ie.cc \
		database_name_table.cc \
		detect_version_collision.cc \
		file.cc \
		file_test.cc \
		file_wince.cc \
		ipc_message_queue_null.cc \
		http_utils.cc \
		js_runner_ie.cc \
		js_runner_utils.cc \
		js_types.cc \
		message_queue.cc \
		message_queue_ie.cc \
		message_service.cc \
		message_service_test.cc \
		module.cc \
		module_wrapper.cc \
		mutex.cc \
		name_value_table.cc \
		paths.cc \
		paths_ie.cc \
		permissions_db.cc \
		scoped_refptr_test.cc \
		security_model.cc \
		security_model_test.cc \
		serialization.cc \
		serialization_test.cc \
		shortcut_table.cc \
		sqlite_wrapper.cc \
		stopwatch.cc \
		stopwatch_ie.cc \
		string16.cc \
		string_utils.cc \
		string_utils_test.cc \
		thread_locals.cc \
		url_utils.cc \
		url_utils_ie.cc \
		url_utils_test.cc \
		vista_utils.cc \
		wince_compatibility.cc \
		$(NULL)

IEMOBILE_LINK_EXTRAS	+= \
		$(IE_OUTDIR)/module.res \
		$(NULL)

#-----------------------------------------------------------------------------
# base/npapi

NPAPI_VPATH	+= \
		base/common \
		base/npapi \
		base/ie \
		$(NULL)

NPAPI_M4SRCS	+= \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS	+= \
		activex_utils.cc \
		async_router.cc \
		base_class.cc \
		base_class_test.cc \
		browser_utils.cc \
		circular_buffer_test.cc \
		database_name_table.cc \
		file.cc \
		file_win32.cc \
		file_test.cc \
		html_event_monitor.cc \
		html_event_monitor_np.cc \
		ipc_message_queue_null.cc \
		http_utils.cc \
		ie_version.cc \
		js_runner_np.cc \
		js_runner_utils.cc \
		js_types.cc \
		message_queue.cc \
		message_queue_ie.cc \
		message_service.cc \
		message_service_test.cc \
		module.cc \
		mutex.cc \
		name_value_table.cc \
		np_utils.cc \
		npn_bindings.cc \
		npp_bindings.cc \
		paths.cc \
		paths_np.cc \
		permissions_db.cc \
		plugin.cc \
		png_utils.cc \
		scoped_refptr_test.cc \
		security_model.cc \
		security_model_test.cc \
		serialization.cc \
		serialization_test.cc \
		shortcut_table.cc \
		sqlite_wrapper.cc \
		stopwatch.cc \
		stopwatch_ie.cc \
		string16.cc \
		string_utils.cc \
		string_utils_test.cc \
		thread_locals.cc \
		url_utils.cc \
		url_utils_ie.cc \
		url_utils_test.cc \
		vista_utils.cc \
		$(NULL)

# TODO(mpcomplete): use WIN32_LINK_EXTRAS here.
ifeq ($(OS),win32)
NPAPI_LINK_EXTRAS  += \
                $(NPAPI_OUTDIR)/module.res \
                $(NULL)
endif

#-----------------------------------------------------------------------------
# vista_broker

IE_VPATH += \
		vista_broker \
		$(NULL)

VISTA_BROKER_CPPSRCS += \
		shortcut_utils_win32.cc \
		string16.cc \
		string_utils.cc \
		vista_broker.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# console/firefox

FF_VPATH	+= \
		console/common \
		console/firefox \
		$(NULL)

FF_M4SRCS	+= \
		console.idl.m4 \
		$(NULL)

FF_IDLSRCS	+= \
		console.idl \
		$(NULL)

# console/common
FF_CPPSRCS	+= \
		console.cc \
		js_callback_logging_backend.cc \
		$(NULL)

# console/firefox
FF_CPPSRCS	+= \
		console_ff.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# console/ie

IE_VPATH	+= \
		console/common \
		console/ie \
		$(NULL)

IE_IDLSRCS	+= \
		console.idl \
		$(NULL)

# console/common
IE_CPPSRCS	+= \
		console.cc \
		js_callback_logging_backend.cc \
		$(NULL)

# console/ie
IE_CPPSRCS	+= \
		console_ie.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# console/iemobile

IEMOBILE_VPATH  += \
		console\common \
		console\ie \
		$(NULL)

IEMOBILE_IDLSRCS += \
	 	console.idl \
		$(NULL)

# console/common
IEMOBILE_CPPSRCS += \
		console.cc \
		js_callback_logging_backend.cc \
		$(NULL)

# console/ie
IEMOBILE_CPPSRCS += \
		console_ie.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# database/firefox

FF_VPATH	+= \
		database/common \
		database/firefox \
		$(NULL)

FF_M4SRCS	+= \
		database.idl.m4 \
		$(NULL)

FF_IDLSRCS	+= \
		database.idl \
		$(NULL)

FF_CPPSRCS	+= \
		database.cc \
		database_utils.cc \
		database_utils_test.cc \
		result_set.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# database/ie

IE_VPATH	+= \
		database/common \
		database/ie \
		$(NULL)

IE_IDLSRCS	+= \
		database.idl \
		$(NULL)

IE_CPPSRCS	+= \
		database.cc \
		database_utils.cc \
		database_utils_test.cc \
		result_set.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# database/iemobile

IEMOBILE_VPATH	+= \
		database/common \
		database/ie \
		$(NULL)

IEMOBILE_IDLSRCS += \
		database.idl \
		$(NULL)

IEMOBILE_CPPSRCS += \
		database.cc \
		database_utils.cc \
		database_utils_test.cc \
		result_set.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# database/npapi

NPAPI_VPATH	+= \
		database/common \
		database/npapi \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS	+= \
		database.cc \
		database_utils.cc \
		database_utils_test.cc \
		result_set.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# desktop

# TODO: Implement desktop for NPAPI
ifneq ($(BROWSER),NPAPI)

SHARED_VPATH	  += \
		desktop \
		$(NULL)

SHARED_CPPSRCS	+= \
		desktop.cc \
		desktop_linux.cc \
		desktop_osx.cc \
		desktop_win32.cc \
		file_dialog.cc \
		file_dialog_gtk.cc \
		file_dialog_osx.cc \
		file_dialog_utils.cc \
		file_dialog_win32.cc \
		shortcut_utils_win32.cc \
		$(NULL)

endif

#-----------------------------------------------------------------------------
# test

ifeq ($(MODE),dbg)

SHARED_VPATH += \
		cctests \
		$(NULL)

SHARED_CPPSRCS += \
		test.cc \
		$(NULL)

endif

#-----------------------------------------------------------------------------
# ui/generated
#
# Anything with the _I18N suffix will be expanded for each language in
# I18N_LANGS

I18N_LANGS	+= \
		en-US \
		$(NULL)

# ui/generated is not specified here, because it's included in the generated
# name for _I18N source files.
COMMON_VPATH	+= \
		$(NULL)

COMMON_M4SRCS_I18N += \
		permissions_dialog.html.m4 \
		settings_dialog.html.m4 \
		shortcuts_dialog.html.m4 \
		$(NULL)

FF_VPATH	+= \
		$(NULL)

FF_M4SRCS_I18N	+= \
		i18n.dtd.m4 \
		$(NULL)

IE_VPATH	+= \
		ui/generated \
		$(NULL)

IE_M4SRCS	+= \
		ui_html.rc.m4 \
		$(NULL)

IEMOBILE_VPATH	+= \
		ui/generated \
		$(NULL)

IEMOBILE_M4SRCS	+= \
		ui_html.rc.m4 \
		$(NULL)

#-----------------------------------------------------------------------------
# ui/firefox

FF_VPATH	+= \
		ui/common \
		ui/firefox \
		$(NULL)

FF_M4SRCS	+= \
		browser-overlay.js.m4 \
		browser-overlay.xul.m4 \
		chrome.manifest.m4 \
		$(NULL)

FF_IDLSRCS	+= \
		ui_utils.idl \
		$(NULL)

FF_CPPSRCS	+= \
		html_dialog.cc \
		permissions_dialog.cc \
		settings_dialog.cc \
		ui_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# ui/ie

IE_VPATH	+= \
		ui/common \
		ui/ie \
		$(NULL)

IE_M4SRCS	+= \
		ui_resources.rc.m4 \
		tools_menu_item.rgs.m4 \
		$(NULL)

IE_IDLSRCS	+= \
		html_dialog_host.idl \
		$(NULL)

IE_CPPSRCS	+= \
		html_dialog.cc \
		html_dialog_host.cc \
		permissions_dialog.cc \
		settings_dialog.cc \
		tools_menu_item.cc \
		$(NULL)

IE_LINK_EXTRAS	+= \
		$(IE_OUTDIR)/ui_resources.res \
		$(NULL)

#-----------------------------------------------------------------------------
# ui/iemobile

IEMOBILE_VPATH	+= \
		ui/common \
		ui/ie \
		$(NULL)

IEMOBILE_M4SRCS	+= \
		ui_resources.rc.m4 \
		html_dialog_bridge_iemobile.rgs.m4 \
		tools_menu_item.rgs.m4 \
		$(NULL)

IEMOBILE_IDLSRCS	+= \
		html_dialog_host.idl \
		html_dialog_host_iemobile.idl \
		html_dialog_bridge_iemobile.idl \
		$(NULL)

IEMOBILE_CPPSRCS	+= \
		html_dialog.cc \
		html_dialog_host_iemobile.cc \
		permissions_dialog.cc \
		html_dialog_bridge_iemobile.cc \
		settings_dialog.cc \
		tools_menu_item.cc \
		$(NULL)

IEMOBILE_LINK_EXTRAS	+= \
		$(IE_OUTDIR)/ui_resources.res \
		$(NULL)

#-----------------------------------------------------------------------------
# ui/npapi

NPAPI_VPATH	+= \
		ui/common \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS	+= \
		html_dialog.cc \
		permissions_dialog.cc \
		settings_dialog.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# workerpool/firefox

FF_VPATH	+= \
		workerpool/common \
		workerpool/firefox \
		$(NULL)

FF_M4SRCS	+= \
		workerpool.idl.m4 \
		$(NULL)

FF_IDLSRCS	+= \
		workerpool.idl \
		$(NULL)

FF_CPPSRCS	+= \
		workerpool.cc \
		workerpool_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# workerpool/ie

IE_VPATH	+= \
		workerpool/common \
		workerpool/ie \
		$(NULL)

IE_IDLSRCS	+= \
		workerpool.idl \
		$(NULL)

IE_CPPSRCS	+= \
		workerpool.cc \
		workerpool_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# workerpool/iemobile

IEMOBILE_VPATH	+= \
		workerpool/common \
		workerpool/ie \
		$(NULL)

IEMOBILE_IDLSRCS	+= \
		workerpool.idl \
		$(NULL)

IEMOBILE_CPPSRCS	+= \
		workerpool.cc \
		workerpool_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# workerpool/npapi

NPAPI_VPATH	+= \
		workerpool/common \
		workerpool/npapi \
		$(NULL)

NPAPI_CPPSRCS	+= \
		workerpool.cc \
		workerpool_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# timer

SHARED_VPATH += \
		timer \
		$(NULL)

SHARED_CPPSRCS += \
		timer.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# localserver/ff

FF_VPATH	+= \
		base/common \
		localserver/common \
		localserver/firefox \
		$(NULL)

FF_M4SRCS	+= \
		localserver.idl.m4 \
		$(NULL)

FF_IDLSRCS	+= \
		localserver.idl \
		$(NULL)

# localserver/common
FF_CPPSRCS	+= \
		blob_store.cc \
		capture_task.cc \
		file_store.cc \
		http_constants.cc \
		http_cookies.cc \
		localserver.cc \
		localserver_db.cc \
		managed_resource_store.cc \
		manifest.cc \
		name_value_table_test.cc \
		permissions_db_test.cc \
		resource_store.cc \
		sqlite_wrapper_test.cc \
		update_task.cc \
		$(NULL)

# localserver/firefox
FF_CPPSRCS	+= \
		async_task_ff.cc \
		cache_intercept.cc \
		file_submitter_ff.cc \
		http_request_ff.cc \
		localserver_ff.cc \
		managed_resource_store_ff.cc \
		resource_store_ff.cc \
		update_task_ff.cc \
		$(NULL)		

#-----------------------------------------------------------------------------
# localserver/ie

IE_VPATH	+= \
		base/common \
		localserver/common \
		localserver/ie \
		$(NULL)

IE_IDLSRCS	+= \
		localserver.idl \
		$(NULL)

# localserver/common		
IE_CPPSRCS	+= \
		blob_store.cc \
		capture_task.cc \
		file_store.cc \
		http_constants.cc \
		http_cookies.cc \
		localserver.cc \
		localserver_db.cc \
		managed_resource_store.cc \
		manifest.cc \
		name_value_table_test.cc \
		permissions_db_test.cc \
		resource_store.cc \
		sqlite_wrapper_test.cc \
		update_task.cc \
		$(NULL)

# localserver/ie
IE_CPPSRCS	+= \
		async_task_ie.cc \
		file_submit_behavior.cc \
		file_submitter_ie.cc \
		http_handler_ie.cc \
		http_request_ie.cc \
		localserver_ie.cc \
		managed_resource_store_ie.cc \
		resource_store_ie.cc \
		update_task_ie.cc \
		urlmon_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# localserver/iemobile

IEMOBILE_VPATH	+= \
		base/common \
		localserver/common \
		localserver/ie \
		$(NULL)

IEMOBILE_IDLSRCS	+= \
		localserver.idl \
		$(NULL)

# localserver/common		
IEMOBILE_CPPSRCS	+= \
		blob_store.cc \
		capture_task.cc \
		file_store.cc \
		http_constants.cc \
		http_cookies.cc \
		localserver.cc \
		localserver_db.cc \
		managed_resource_store.cc \
		manifest.cc \
		name_value_table_test.cc \
		permissions_db_test.cc \
		resource_store.cc \
		sqlite_wrapper_test.cc \
		update_task.cc \
		$(NULL)

# localserver/iemobile
IEMOBILE_CPPSRCS	+= \
		async_task_ie.cc \
		http_handler_ie.cc \
		http_request_ie.cc \
		localserver_ie.cc \
		managed_resource_store_ie.cc \
		resource_store_ie.cc \
		update_task_ie.cc \
		urlmon_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# localserver/npapi

NPAPI_VPATH	+= \
		base/common \
		localserver/common \
		localserver/npapi \
		localserver/ie \
		$(NULL)

NPAPI_M4SRCS	+= \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS	+= \
		$(NULL)

# localserver/common
NPAPI_CPPSRCS	+= \
		blob_store.cc \
		capture_task.cc \
		file_store.cc \
		http_constants.cc \
		http_cookies.cc \
		localserver.cc \
		localserver_db.cc \
		managed_resource_store.cc \
		manifest.cc \
		name_value_table_test.cc \
		permissions_db_test.cc \
		resource_store.cc \
		sqlite_wrapper_test.cc \
		update_task.cc \
		$(NULL)

# localserver/npapi
NPAPI_CPPSRCS	+= \
		async_task_np.cc \
		file_submitter_np.cc \
		http_handler_ie.cc \
		http_request_ie.cc \
		localserver_np.cc \
		managed_resource_store_np.cc \
		update_task_np.cc \
		urlmon_utils.cc \
		resource_store_np.cc \
		$(NULL)		

#-----------------------------------------------------------------------------
# httprequest/ff

FF_VPATH	+= \
		httprequest/firefox \
		$(NULL)

FF_M4SRCS	+= \
		httprequest.idl.m4 \
		$(NULL)

FF_IDLSRCS	+= \
		httprequest.idl \
		$(NULL)

FF_CPPSRCS	+= \
		httprequest_ff.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# httprequest/ie

IE_VPATH	+= \
		httprequest/ie \
		$(NULL)

IE_IDLSRCS	+= \
		httprequest.idl \
		$(NULL)

IE_CPPSRCS	+= \
		httprequest_ie.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# httprequest/iemobile

IEMOBILE_VPATH	+= \
		httprequest/ie \
		$(NULL)

IEMOBILE_IDLSRCS	+= \
		httprequest.idl \
		$(NULL)

IEMOBILE_CPPSRCS	+= \
		httprequest_ie.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# httprequest/npapi

NPAPI_VPATH += \
		httprequest/npapi \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS	+= \
		httprequest_np.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# blob

ifneq ($(OFFICIAL_BUILD),1)
FF_VPATH	+= \
		blob \
		$(NULL)

FF_IDLSRCS	+= \
		blob_ff.idl \
		$(NULL)

FF_CPPSRCS	+= \
		blob_ff.cc \
		blob_test.cc \
		buffer_blob.cc \
		file_blob.cc \
		slice_blob.cc \
		$(NULL)

IE_VPATH	+= \
		blob \
		$(NULL)

IE_IDLSRCS	+= \
		blob_ie.idl \
		$(NULL)

IE_CPPSRCS	+= \
		blob_ie.cc \
		blob_test.cc \
		buffer_blob.cc \
		file_blob.cc \
		slice_blob.cc \
		$(NULL)

NPAPI_VPATH	+= \
		blob \
		$(NULL)

NPAPI_CPPSRCS	+= \
		blob_test.cc \
		buffer_blob.cc \
		file_blob.cc \
		slice_blob.cc \
		$(NULL)

IEMOBILE_VPATH	+= \
		blob \
		$(NULL)

IEMOBILE_IDLSRCS	+= \
		blob_ie.idl \
		$(NULL)

IEMOBILE_CPPSRCS	+= \
		blob_ie.cc \
		blob_test.cc \
		buffer_blob.cc \
		file_blob.cc \
		slice_blob.cc \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# image/ff

ifneq ($(OFFICIAL_BUILD),1)
FF_VPATH	+= \
		image/common \
		image/firefox \
		$(NULL)

FF_M4SRCS	+= \
		image.idl.m4 \
		$(NULL)

FF_IDLSRCS	+= \
		image.idl \
		$(NULL)

FF_CPPSRCS	+= \
		image.cc \
		image_ff.cc \
		image_loader_ff.cc \
		$(NULL)

IE_VPATH	+= \
		image/common \
		image/ie \
		$(NULL)

IE_IDLSRCS	+= \
		image.idl \
		$(NULL)

IE_CPPSRCS	+= \
		image.cc \
		image_ie.cc \
		image_loader_ie.cc \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# factory/ff

FF_VPATH	+= \
		factory/common \
		factory/firefox \
		$(NULL)

FF_IDLSRCS	+= \
		factory.idl \
		$(NULL)

FF_CPPSRCS	+= \
		factory.cc \
		factory_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# factory/ie

IE_VPATH	+= \
		factory/common \
		factory/ie \
		$(NULL)

IE_M4SRCS	+= \
		factory.rgs.m4 \
		$(NULL)

IE_IDLSRCS	+= \
		factory.idl \
		$(NULL)

IE_CPPSRCS	+= \
		factory.cc \
		factory_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# factory/iemobile

IEMOBILE_VPATH	+= \
		factory/common \
		factory/ie \
		$(NULL)

IEMOBILE_M4SRCS	+= \
		factory.rgs.m4 \
		$(NULL)

IEMOBILE_IDLSRCS += \
		factory.idl \
		$(NULL)

IEMOBILE_CPPSRCS += \
		factory.cc \
		factory_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# factory/npapi

NPAPI_VPATH	+= \
		factory/common \
		factory/npapi \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS	+= \
		factory.cc \
		factory_wrapper.cc \
		factory_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# launchurl/os x
LAUNCHURLHELPER_CPPSRC = base/safari/launch_url_in_browser.cc

#-----------------------------------------------------------------------------
# installer/iemobile

IEMOBILE_VPATH	+= \
		installer/iemobile \
		base/common \
		$(NULL)

IEMOBILE_M4SRCS	+= \
		setup.rc.m4 \
		$(NULL)

IEMOBILE_CPPSRCS += \
		cab_updater.cc \
		$(NULL)

IEMOBILE_LINK_EXTRAS += \
		$(IE_OUTDIR)/setup.res \
		$(NULL)

IEMOBILE_SETUP_CPPSRCS += \
		process_restarter.cc \
		setup.cc \
		$(NULL)

IEMOBILE_SETUP_LINK_EXTRAS += \
		$(IE_OUTDIR)/setup.res \
		$(NULL)

include tools/rules.mk
