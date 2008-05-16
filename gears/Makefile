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

ifeq ($(USING_SQLITE),1)
THIRD_PARTY_VPATH	+= \
		../third_party/sqlite_google/src \
		../third_party/sqlite_google/preprocessed \
		../third_party/sqlite_google/ext/fts1 \
		../third_party/sqlite_google/ext/fts2 \
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
endif # USING_SQLITE

#-----------------------------------------------------------------------------
# third_party/libpng

ifeq ($(USING_LIBPNG),1)
THIRD_PARTY_VPATH	+= \
		../third_party/libpng \
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
endif # USING_LIBPNG

#-----------------------------------------------------------------------------
# third_party/zlib

ifeq ($(USING_ZLIB),1)
THIRD_PARTY_VPATH	+= \
		../third_party/zlib \
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
endif # USING_ZLIB

#-----------------------------------------------------------------------------
# third_party/libjpeg

ifeq ($(USING_LIBJPEG),1)
ifneq ($(OFFICIAL_BUILD),1)
ifneq ($(OS),wince)
THIRD_PARTY_VPATH	+= \
		../third_party/libjpeg \
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
endif # USING_LIBJPEG

#-----------------------------------------------------------------------------
# third_party/libgd

ifeq ($(USING_LIBGD),1)
ifneq ($(OFFICIAL_BUILD),1)
ifneq ($(OS),wince)
THIRD_PARTY_VPATH	+= \
		../third_party/libgd \
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
endif # USING_LIBGD

#-----------------------------------------------------------------------------
# third_party/jsoncpp

THIRD_PARTY_VPATH	+= \
		../third_party/jsoncpp \
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
		../third_party/breakpad/src/client/windows/handler \
		../third_party/breakpad/src/common/windows \
		$(NULL)

THIRD_PARTY_CPPSRCS	+= \
		exception_handler.cc \
		guid_string.cc \
		$(NULL)


FF3_VPATH	+= \
		base/common \
		$(NULL)

FF3_CPPSRCS	+= \
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
# third_party/googleurl

# Only include gurl in NPAPI plugin builds.
ifeq ($(USING_NPAPI),1)
THIRD_PARTY_VPATH	+= \
		../third_party/googleurl/src \
		$(NULL)

THIRD_PARTY_CPPSRCS	+= \
		gurl.cc \
		url_canon_etc.cc \
		url_canon_fileurl.cc \
		url_canon_host.cc \
		url_canon_internal.cc \
		url_canon_ip.cc \
		url_canon_path.cc \
		url_canon_pathurl.cc \
		url_canon_query.cc \
		url_canon_relative.cc \
		url_canon_stdurl.cc \
		url_parse.cc \
		url_parse_file.cc \
		url_util.cc \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# third_party/icu38

ifeq ($(USING_ICU),1)
THIRD_PARTY_VPATH	+= \
		../third_party/icu38/source/common \
		$(NULL)

THIRD_PARTY_CPPSRCS	+= \
		utf_impl.cc \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# third_party/modp_b64

THIRD_PARTY_VPATH	+= \
		../third_party/modp_b64 \
		$(NULL)

THIRD_PARTY_CPPSRCS	+= \
		modp_b64.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# base/common

COMMON_VPATH	+= \
		base/common \
		installer \
		$(NULL)

COMMON_M4SRCS	+= \
		product_constants.h.m4 \
		win32_msi.wxs.m4 \
		wince_cab.inf.m4 \
		$(NULL)

COMMON_CPPSRCS	+= \
		$(NULL)
# WARNING: Think carefully before listing any non-third-party C/C++ files in
# COMMON_*SRCS. Such files only get built once per platform (not per browser),
# and BROWSER_* #defines are not obeyed.  Instead, use $(BROWSER)_CPPSRCS
# when possible.
# One important exception: unit-tested code is only built once per platform, so
# it should be listed in CPPSRCS. Thus it must not use any BROWSER_* #defines,
# directly or indirectly.
#
# TODO(cprince): investigate OS-specific sources (e.g. WIN32_CPPSRCS)


#-----------------------------------------------------------------------------
# base/common (built for all browsers)

$(BROWSER)_VPATH	+= \
		base/common \
		$(NULL)

$(BROWSER)_M4SRCS	+= \
		product_version.rc.m4 \
		$(NULL)

$(BROWSER)_CPPSRCS	+= \
		async_router.cc \
		base_class.cc \
		base64.cc \
		circular_buffer_test.cc \
		database_name_table.cc \
		event.cc \
		event_test.cc \
		file.cc \
		file_test.cc \
		html_event_monitor.cc \
		http_utils.cc \
		js_marshal.cc \
		js_runner_utils.cc \
		js_types.cc \
		message_queue.cc \
		message_service.cc \
		message_service_test.cc \
		mutex.cc \
		name_value_table.cc \
		name_value_table_test.cc \
		paths.cc \
		permissions_db.cc \
		permissions_db_test.cc \
		png_utils.cc \
		scoped_refptr_test.cc \
		security_model.cc \
		security_model_test.cc \
		serialization.cc \
		serialization_test.cc \
		shortcut_table.cc \
		sqlite_wrapper.cc \
		sqlite_wrapper_test.cc \
		stopwatch.cc \
		stopwatch_posix.cc \
		stopwatch_win32.cc \
		string16.cc \
		string_utils.cc \
		string_utils_test.cc \
		thread_locals.cc \
		url_utils.cc \
		url_utils_test.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# base/firefox

FF3_VPATH	+= \
		base/common \
		base/firefox \
		$(NULL)

FF3_M4SRCS	+= \
		install.rdf.m4 \
		$(NULL)

FF3_IDLSRCS	+= \
		base_interface_ff.idl \
		$(NULL)

FF3_CPPSRCS	+= \
		dom_utils.cc \
		file_posix.cc \
		file_win32.cc \
		html_event_monitor_ff.cc \
		ipc_message_queue_null.cc \
		js_runner_ff.cc \
		js_runner_ff_marshaling.cc \
		message_queue_ff.cc \
		module.cc \
		ns_file_utils.cc \
		paths_ff.cc \
		url_utils_ff.cc \
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
		bho.cc \
		detect_version_collision.cc \
		js_runner_ie.cc \
		message_queue_ie.cc \
		module.cc \
		module_wrapper.cc \
		paths_ie.cc \
		url_utils_ie.cc \
		vista_utils.cc \
		$(NULL)
ifeq ($(OS),win32)
IE_CPPSRCS	+= \
		file_win32.cc \
		html_event_monitor_ie.cc \
		ipc_message_queue_win32.cc \
		ipc_message_queue_win32_test.cc \
		ie_version.cc \
		$(NULL)
endif
ifeq ($(OS),wince)
IE_CPPSRCS	+= \
		common_ie.cc \
		file_wince.cc \
		ipc_message_queue_null.cc \
		wince_compatibility.cc \
		$(NULL)
endif

IE_LINK_EXTRAS	+= \
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
		browser_utils.cc \
		html_event_monitor_np.cc \
		ipc_message_queue_null.cc \
		js_runner_np.cc \
		module.cc \
		np_utils.cc \
		npn_bindings.cc \
		npp_bindings.cc \
		plugin.cc \
		url_utils_np.cc \
		vista_utils.cc \
		$(NULL)

ifeq ($(OS),win32)
NPAPI_CPPSRCS	+= \
		detect_version_collision.cc \
		file_win32.cc \
		ie_version.cc \
		message_queue_ie.cc \
		paths_np.cc \
		$(NULL)

# TODO(mpcomplete): use WIN32_LINK_EXTRAS here.
NPAPI_LINK_EXTRAS  += \
                $(NPAPI_OUTDIR)/module.res \
                $(NULL)
else
NPAPI_CPPSRCS	+= \
		file_posix.cc \
		$(NULL)
endif # WIN32

#-----------------------------------------------------------------------------
# base/safari

SF_VPATH	+= \
		base/common \
		base/npapi \
		base/safari \
		$(NULL)

SF_CPPSRCS	+= \
		browser_utils.cc \
		browser_utils_sf.cc \
		common_sf.mm \
		cf_string_utils.cc \
		file_posix.cc \
		html_event_monitor_np.cc \
		ipc_message_queue_null.cc \
		js_runner_np.cc \
		message_queue_sf.cc \
		module.cc \
		np_utils.cc \
		npn_bindings.cc \
		npp_bindings.cc \
		nsstring_utils.mm \
		paths_sf.mm \
		paths_sf_more.mm \
		plugin.cc \
		safari_workaround.m \
		url_utils_np.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# vista_broker

IE_VPATH += \
		base/common \
		desktop \
		vista_broker \
		$(NULL)

VISTA_BROKER_CPPSRCS += \
		shortcut_utils_win32.cc \
		string16.cc \
		string_utils.cc \
		vista_broker.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# launch_url_with_browser

ifeq ($(OS),osx)
COMMON_VPATH += \
		base/safari \
		$(NULL)  

OSX_LAUNCHURL_CPPSRCS += \
		launch_url_in_browser.cc \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# Safari InputManager

SF_VPATH += \
		base/safari \
		$(NULL)

SF_INPUTMANAGER_CPPSRCS += \
		enabler.m \
		http_handler.mm \
		loader.mm \
		$(NULL)


#-----------------------------------------------------------------------------
# tools/perf

COMMON_VPATH += \
		tools/perf \
		$(NULL)

PERF_TOOL_CPPSRCS += \
		measure_startup.cc \
		$(NULL)


#-----------------------------------------------------------------------------
# console/common (built for all browsers)

$(BROWSER)_VPATH	+= \
		console/common \
		$(NULL)

# console/common
$(BROWSER)_CPPSRCS	+= \
		console.cc \
		js_callback_logging_backend.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# console/firefox

FF3_VPATH	+= \
		console/firefox \
		$(NULL)

FF3_M4SRCS	+= \
		console.idl.m4 \
		$(NULL)

FF3_IDLSRCS	+= \
		console.idl \
		$(NULL)

FF3_CPPSRCS	+= \
		console_ff.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# console/ie

IE_VPATH	+= \
		console/ie \
		$(NULL)

IE_IDLSRCS	+= \
		console.idl \
		$(NULL)

IE_CPPSRCS	+= \
		console_ie.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# console/npapi

ifeq ($(USING_NPAPI),1)
$(BROWSER)_VPATH	+= \
		console/npapi \
		$(NULL)

$(BROWSER)_IDLSRCS	+= \
		$(NULL)

$(BROWSER)_CPPSRCS	+= \
		console_np.cc \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# crash_sender

COMMON_VPATH += \
		crash_sender \
		../third_party/breakpad/src/client/windows/sender \
		../third_party/breakpad/src/common/windows \
		$(NULL)

CRASH_SENDER_CPPSRCS  += \
		crash_report_sender.cc \
		crash_sender_win32.cc \
		http_upload.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# canvas

$(BROWSER)_VPATH += \
		canvas \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		canvas.cc \
		canvas_rendering_context_2d.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# database/common (built for all browsers)

$(BROWSER)_VPATH	+= \
		database/common \
		$(NULL)

$(BROWSER)_CPPSRCS	+= \
		database.cc \
		database_utils.cc \
		database_utils_test.cc \
		result_set.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# database/firefox

FF3_VPATH	+= \
		database/firefox \
		$(NULL)

FF3_M4SRCS	+= \
		database.idl.m4 \
		$(NULL)

FF3_IDLSRCS	+= \
		database.idl \
		$(NULL)

FF3_CPPSRCS	+= \
		$(NULL)

#-----------------------------------------------------------------------------
# database/ie

IE_VPATH	+= \
		database/ie \
		$(NULL)

IE_IDLSRCS	+= \
		database.idl \
		$(NULL)

IE_CPPSRCS	+= \
		$(NULL)

#-----------------------------------------------------------------------------
# database/npapi
ifeq ($(USING_NPAPI),1)
$(BROWSER)_VPATH	+= \
		database/npapi \
		$(NULL)

$(BROWSER)_IDLSRCS	+= \
		$(NULL)

$(BROWSER)_CPPSRCS	+= \
		$(NULL)
endif		
#-----------------------------------------------------------------------------
# desktop

$(BROWSER)_VPATH += \
		desktop \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		desktop.cc \
		desktop_linux.cc \
		desktop_osx.cc \
		desktop_win32.cc \
		dll_data_wince.cc \
		shortcut_utils_win32.cc \
		$(NULL)

# TODO: Implement file dialogs for NPAPI
ifneq ($(BROWSER),NPAPI)
$(BROWSER)_CPPSRCS += \
		file_dialog.cc \
		file_dialog_gtk.cc \
		file_dialog_osx.cc \
		file_dialog_utils.cc \
		file_dialog_win32.cc \
		$(NULL)
endif

# The browser module also needs these files, to communicate with the notifier.
$(BROWSER)_VPATH += \
		notifier \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		notifier_process_linux.cc \
		notifier_process_osx.cc \
		notifier_process_win32.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# desktop_notifier

COMMON_VPATH	+= \
		notifier \
		$(NULL)

NOTIFIER_CPPSRCS += \
		ipc_message_queue_win32.cc \
		message_queue.cc \
		message_queue_ie.cc \
		mutex.cc \
		notification_manager.cc \
		notification_manager_test.cc \
		notifier.cc \
		notifier_process_linux.cc \
		notifier_process_osx.cc \
		notifier_process_win32.cc \
		serialization.cc \
		stopwatch.cc \
		stopwatch_posix.cc \
		stopwatch_win32.cc \
		string16.cc \
		string_utils.cc \
		unit_test.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# test

$(BROWSER)_VPATH += \
		cctests \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		test.cc \
		$(NULL)

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
$(BROWSER)_VPATH += \
		$(NULL)

$(BROWSER)_M4SRCS_I18N += \
		permissions_dialog.html.m4 \
		settings_dialog.html.m4 \
		shortcuts_dialog.html.m4 \
		$(NULL)

FF3_VPATH	+= \
		$(NULL)

FF3_M4SRCS_I18N	+= \
		i18n.dtd.m4 \
		$(NULL)

IE_M4SRCS	+= \
		ui_html.rc.m4 \
		$(NULL)

NPAPI_VPATH	+= \
		ui/generated \
		$(NULL)

NPAPI_M4SRCS	+= \
		ui_html.rc.m4 \
		$(NULL)

# Some files should be omitted on Mac and Linux...
ifeq ($(OS),osx)
else
ifeq ($(OS),linux)
else
# ...but included on Win32 and WinCE, for all targets.
$(BROWSER)_VPATH += \
		ui/generated \
		$(NULL)

$(BROWSER)_LINK_EXTRAS += \
		$($(BROWSER)_OUTDIR)/string_table.res \
		$(NULL)

VISTA_BROKER_LINK_EXTRAS += \
		$(VISTA_BROKER_OUTDIR)/string_table.res \
		$(NULL)
endif
endif

#-----------------------------------------------------------------------------
# ui/common (built for all browsers)

$(BROWSER)_VPATH += \
		ui/common \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		html_dialog.cc \
		permissions_dialog.cc \
		settings_dialog.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# ui/firefox

FF3_VPATH	+= \
		ui/firefox \
		$(NULL)

FF3_M4SRCS	+= \
		browser-overlay.js.m4 \
		browser-overlay.xul.m4 \
		chrome.manifest.m4 \
		$(NULL)

FF3_IDLSRCS	+= \
		ui_utils.idl \
		$(NULL)

FF3_CPPSRCS	+= \
		ui_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# ui/ie

IE_VPATH	+= \
		ui/ie \
		$(NULL)

IE_M4SRCS	+= \
		registry_strings.h.m4 \
		tools_menu_item.rgs.m4 \
		ui_resources.rc.m4 \
		$(NULL)

IE_IDLSRCS	+= \
		html_dialog_host.idl \
		$(NULL)

IE_CPPSRCS	+= \
		tools_menu_item.cc \
		$(NULL)

IE_LINK_EXTRAS	+= \
		$(IE_OUTDIR)/ui_resources.res \
		$(NULL)

# Additional files specific to Win32 or WinCE.
ifeq ($(OS),win32)
IE_CPPSRCS	+= \
		html_dialog_host.cc \
		$(NULL)
endif

ifeq ($(OS),wince)
IE_M4SRCS	+= \
		html_dialog_bridge_iemobile.rgs.m4 \
		$(NULL)

IE_IDLSRCS	+= \
		html_dialog_host_iemobile.idl \
		html_dialog_bridge_iemobile.idl \
		$(NULL)

IE_CPPSRCS	+= \
		html_dialog_host_iemobile.cc \
		html_dialog_bridge_iemobile.cc \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# ui/npapi

NPAPI_VPATH	+= \
		ui/ie \
		$(NULL)

NPAPI_M4SRCS	+= \
		ui_resources.rc.m4 \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS	+= \
		$(NULL)

NPAPI_LINK_EXTRAS += \
		$(NPAPI_OUTDIR)/ui_resources.res \
		$(NULL)

#-----------------------------------------------------------------------------
# ui/safari

SF_VPATH	+= \
		ui/safari \
		$(NULL)

SF_CPPSRCS	+= \
		html_dialog_sf.mm \
		$(NULL)

#-----------------------------------------------------------------------------
# workerpool/firefox

FF3_VPATH	+= \
		workerpool/common \
		workerpool/firefox \
		$(NULL)

FF3_M4SRCS	+= \
		workerpool.idl.m4 \
		$(NULL)

FF3_IDLSRCS	+= \
		workerpool.idl \
		$(NULL)

FF3_CPPSRCS	+= \
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

$(BROWSER)_VPATH += \
		timer \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		timer.cc \
		$(NULL)


#-----------------------------------------------------------------------------
# localserver/common (built for all browsers)

$(BROWSER)_VPATH += \
		localserver/common \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		blob_store.cc \
		capture_task.cc \
		file_store.cc \
		http_constants.cc \
		http_cookies.cc \
		localserver.cc \
		localserver_db.cc \
		managed_resource_store.cc \
		manifest.cc \
		resource_store.cc \
		update_task.cc \
		update_task_single_process.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# localserver/firefox

FF3_VPATH	+= \
		localserver/firefox \
		$(NULL)

FF3_M4SRCS	+= \
		localserver.idl.m4 \
		$(NULL)

FF3_IDLSRCS	+= \
		localserver.idl \
		$(NULL)

FF3_CPPSRCS	+= \
		async_task_ff.cc \
		cache_intercept.cc \
		file_submitter_ff.cc \
		http_request_ff.cc \
		localserver_ff.cc \
		managed_resource_store_ff.cc \
		resource_store_ff.cc \
		safe_http_request.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# localserver/ie

IE_VPATH	+= \
		localserver/ie \
		$(NULL)

IE_IDLSRCS	+= \
		localserver.idl \
		$(NULL)

IE_CPPSRCS	+= \
		async_task_ie.cc \
		http_handler_ie.cc \
		http_request_ie.cc \
		localserver_ie.cc \
		managed_resource_store_ie.cc \
		resource_store_ie.cc \
		update_task_ie.cc \
		urlmon_utils.cc \
		$(NULL)

# Additional files specific to Win32 or WinCE.
ifeq ($(OS),win32)
IE_CPPSRCS	+= \
		file_submit_behavior.cc \
		file_submitter_ie.cc \
		$(NULL)
endif

ifeq ($(OS),wince)
endif

#-----------------------------------------------------------------------------
# localserver/npapi

# TODO(mpcomplete): Remove NPAPI reliance on localserver/ie, someday.
NPAPI_VPATH	+= \
		localserver/npapi \
		localserver/ie \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

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
# localserver/safari

SF_VPATH	+= \
		localserver/npapi \
		localserver/safari \
		$(NULL)

SF_CPPSRCS	+= \
		async_task_sf.mm \
		file_submitter_np.cc \
		http_cookies_sf.mm \
		http_request_delegate.mm \
		http_request_sf.mm \
		localserver_db_proxy.mm \
		localserver_np.cc \
		managed_resource_store_np.cc \
		resource_store_np.cc \
		$(NULL)		

#-----------------------------------------------------------------------------
# httprequest

$(BROWSER)_VPATH += \
		httprequest \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		httprequest.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# blob

ifneq ($(OFFICIAL_BUILD),1)
# (built for all browsers)
$(BROWSER)_VPATH += \
		blob \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		blob.cc \
		blob_test.cc \
		buffer_blob.cc \
		file_blob.cc \
		slice_blob.cc \
		$(NULL)

# TODO(bpm): Make this cross-browser, not Firefox- or Safari-specific.
FF3_CPPSRCS	+= \
		blob_input_stream_ff.cc \
		blob_input_stream_ff_test.cc \
		$(NULL)

SF_CPPSRCS += \
		blob_input_stream_sf.mm \
		$(NULL)
endif # not OFFICIAL_BUILD


#-----------------------------------------------------------------------------
# image

ifneq ($(OFFICIAL_BUILD),1)
ifneq ($(OS),wince)
$(BROWSER)_VPATH += \
		image \
		$(NULL)

$(BROWSER)_CPPSRCS += \
		backing_image.cc \
		image.cc \
		image_loader.cc \
		$(NULL)
endif # not wince
endif # not OFFICIAL_BUILD

#-----------------------------------------------------------------------------
# factory/firefox

FF3_VPATH	+= \
		factory/common \
		factory/firefox \
		$(NULL)

FF3_IDLSRCS	+= \
		factory.idl \
		$(NULL)

FF3_CPPSRCS	+= \
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
# factory/npapi

ifeq ($(USING_NPAPI),1)
$(BROWSER)_VPATH	+= \
		factory/common \
		factory/npapi \
		$(NULL)

$(BROWSER)_IDLSRCS	+= \
		$(NULL)

$(BROWSER)_CPPSRCS	+= \
		factory.cc \
		factory_wrapper.cc \
		factory_utils.cc \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# installer/iemobile

ifeq ($(OS),wince)
IE_VPATH	+= \
		installer/iemobile \
		$(NULL)

IE_WINCESETUP_CPPSRCS += \
		process_restarter.cc \
		setup.cc \
		$(NULL)

IE_WINCESETUP_LINK_EXTRAS += \
		$(IE_OUTDIR)/setup.res \
		$(NULL)

# List these setup-related items in IE_FOO just so they get built before used.
IE_M4SRCS	+= \
		setup.rc.m4 \
		$(NULL)

IE_LINK_EXTRAS += \
		$(IE_OUTDIR)/setup.res \
		$(NULL)

# TODO(cprince): move this to another source path; IE_MODULE_DLL links it.
IE_CPPSRCS += \
		cab_updater.cc \
		$(NULL)

endif # wince

#-----------------------------------------------------------------------------
# geolocation

ifeq ($(OFFICIAL_BUILD),1)
# The Geolocation API has not been finalized for official builds.
else
$(BROWSER)_VPATH	+= \
		geolocation \
		$(NULL)

$(BROWSER)_IDLSRCS	+= \
		$(NULL)

$(BROWSER)_CPPSRCS	+= \
		device_data_provider.cc \
		empty_device_data_provider.cc \
		geolocation.cc \
		geolocation_test.cc \
		location_provider.cc \
		network_location_provider.cc \
		network_location_request.cc \
		radio_data_provider_wince.cc \
		$(NULL)
endif


#-----------------------------------------------------------------------------
# Add any hard-coded "FF3_" sources to the FF2_ sources.

FF2_VPATH += $(FF3_VPATH)
FF2_M4SRCS += $(FF3_M4SRCS)
FF2_M4SRCS_I18N += $(FF3_M4SRCS_I18N)
FF2_IDLSRCS += $(FF3_IDLSRCS)
FF2_CPPSRCS += $(FF3_CPPSRCS)


#-----------------------------------------------------------------------------
include tools/rules.mk
