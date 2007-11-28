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

include tools/version.mk
include tools/config.mk

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

#-----------------------------------------------------------------------------
# third_party/libpng

# TODO(andreip): fix errors in zlib before building this
ifneq ($(BROWSER),IEMOBILE)
THIRD_PARTY_VPATH	+= \
		third_party/libpng \
		$(NULL)

THIRD_PARTY_CSRCS	+= \
		png.c \
		pngerror.c \
		pnggccrd.c \
		pngget.c \
		pngmem.c \
		pngpread.c \
		pngread.c \
		pngrio.c \
		pngrtran.c \
		pngrutil.c \
		pngset.c \
		pngtrans.c \
		pngvcrd.c \
		pngwio.c \
		pngwrite.c \
		pngwtran.c \
		pngwutil.c \
		$(NULL)
endif

#-----------------------------------------------------------------------------
# third_party/zlib

# TODO(andreip): fix errors in gzio.c
ifneq ($(BROWSER),IEMOBILE)
THIRD_PARTY_VPATH	+= \
		third_party/zlib \
		$(NULL)

THIRD_PARTY_CSRCS	+= \
		adler32.c \
		compress.c \
		crc32.c \
		deflate.c \
		gzio.c \
		infback.c \
		inffast.c \
		inflate.c \
		inftrees.c \
		trees.c \
		uncompr.c \
		zutil.c \
		$(NULL)
endif

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
ifneq ($(BROWSER),IEMOBILE)
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
endif
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
		base_class.cc \
		base_class_test.cc \
		dom_utils.cc \
		file.cc \
		file_ff.cc \
		file_linux.cc \
		file_osx.cc \
		file_test.cc \
		file_win32.cc \
		html_event_monitor.cc \
		html_event_monitor_ff.cc \
		http_utils.cc \
		js_runner_ff.cc \
		js_runner_ff_marshaling.cc \
		js_runner_utils.cc \
		js_types.cc \
		module.cc \
		mutex.cc \
		name_value_table.cc \
		ns_file_utils.cc \
		paths.cc \
		paths_ff.cc \
		permissions_db.cc \
		png_utils.cc \
		security_model.cc \
		security_model_test.cc \
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
		base_class.cc \
		base_class_test.cc \
		bho.cc \
		detect_version_collision.cc \
		file.cc \
		file_ie.cc \
		file_test.cc \
		file_win32.cc \
		html_event_monitor.cc \
		html_event_monitor_ie.cc \
		http_utils.cc \
		ie_version.cc \
		js_runner_ie.cc \
		js_runner_utils.cc \
		js_types.cc \
		module.cc \
		mutex.cc \
		name_value_table.cc \
		paths.cc \
		paths_ie.cc \
		permissions_db.cc \
		png_utils.cc \
		security_model.cc \
		security_model_test.cc \
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
		base_class.cc \
		bho.cc \
		detect_version_collision.cc \
		file.cc \
		file_ie.cc \
		file_test.cc \
		html_event_monitor.cc \
		html_event_monitor_ie.cc \
		http_utils.cc \
		js_runner_ie.cc \
		js_runner_utils.cc \
		js_types.cc \
		module.cc \
		mutex.cc \
		name_value_table.cc \
		paths.cc \
		paths_ie.cc \
		permissions_db.cc \
		security_model.cc \
		shortcut_table.cc \
		sqlite_wrapper.cc \
		stopwatch.cc \
		stopwatch_ie.cc \
		string16.cc \
		string_utils.cc \
		thread_locals.cc \
		url_utils.cc \
		url_utils_ie.cc \
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
		$(NULL)

NPAPI_M4SRCS	+= \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS	+= \
		base_class.cc \
		base_class_test.cc \
		browser_utils.cc \
		file.cc \
		file_ie.cc \
		file_test.cc \
		html_event_monitor.cc \
		html_event_monitor_np.cc \
		http_utils.cc \
		js_runner_np.cc \
		js_runner_utils.cc \
		js_types.cc \
		module.cc \
		mutex.cc \
		name_value_table.cc \
		np_utils.cc \
		npn_bindings.cc \
		npp_bindings.cc \
		paths.cc \
		paths_ie.cc \
		permissions_db.cc \
		png_utils.cc \
		security_model.cc \
		security_model_test.cc \
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
		database_wrapper.cc \
		database_utils.cc \
		result_set.cc \
		result_set_wrapper.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# desktop

FF_VPATH	+= \
		desktop \
		$(NULL)

FF_IDLSRCS	+= \
		desktop_ff.idl \
		$(NULL)

FF_CPPSRCS	+= \
		desktop.cc \
		$(NULL)

IE_VPATH	+= \
		desktop \
		$(NULL)
		
IEMOBILE_VPATH	+= \
		desktop \
		$(NULL)

IE_IDLSRCS	+= \
		desktop_ie.idl \
		$(NULL)

IEMOBILE_IDLSRCS += \
		desktop_ie.idl \
		$(NULL)

IE_CPPSRCS	+= \
		desktop.cc \
		$(NULL)

NPAPI_VPATH     += \
                desktop \
                $(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS   += \
                $(NULL)
                
#-----------------------------------------------------------------------------
# test

ifeq ($(MODE),dbg)

FF_VPATH	+= \
		cctests \
		$(NULL)

FF_M4SRCS	+= \
		test_ff.idl.m4 \
		$(NULL)
		
FF_IDLSRCS	+= \
		test_ff.idl \
		$(NULL)

FF_CPPSRCS	+= \
		test.cc \
		$(NULL)

IE_VPATH	+= \
		cctests \
		$(NULL)
		
IE_IDLSRCS	+= \
		test_ie.idl \
		$(NULL)

IE_CPPSRCS	+= \
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
		tools_menu_item.rgs.m4 \
		$(NULL)

IEMOBILE_IDLSRCS	+= \
		html_dialog_host.idl \
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
		workerpool_wrapper.cc \
		workerpool_utils.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# timer

FF_VPATH	+= \
		timer\
		$(NULL)

FF_IDLSRCS	+= \
		timer_ff.idl \
		$(NULL)

FF_CPPSRCS	+= \
		timer.cc \
		$(NULL)

IE_VPATH	+= \
		timer \
		$(NULL)

IE_IDLSRCS	+= \
		timer_ie.idl \
		$(NULL)

IE_CPPSRCS	+= \
		timer.cc \
		$(NULL)

IEMOBILE_VPATH	+= \
		timer \
		$(NULL)

IEMOBILE_IDLSRCS	+= \
		timer_ie.idl \
		$(NULL)

IEMOBILE_CPPSRCS	+= \
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
		http_cookies.cc \
		localserver.cc \
		localserver_db.cc \
		managed_resource_store.cc \
		manifest.cc \
		resource_store.cc \
		http_constants.cc \
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
# httprequest/ff

FF_VPATH	+= \
		httprequest/firefox \
		$(NULL)

FF_IDLSRCS	+= \
		httprequest.idl \
		$(NULL)

FF_CPPSRCS	+= \
		httprequest_ff.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# localserver/npapi

NPAPI_VPATH	+= \
		base/common \
		localserver/common \
		localserver/npapi \
		$(NULL)

NPAPI_M4SRCS	+= \
		$(NULL)

NPAPI_IDLSRCS	+= \
		$(NULL)

NPAPI_CPPSRCS	+= \
		$(NULL)

# localserver/common
NPAPI_CPPSRCS	+= \
		http_constants.cc \
		localserver_db_stub.cc \
		$(NULL)

# localserver/npapi
NPAPI_CPPSRCS	+= \
		file_submitter_np.cc \
		file_submitter_wrapper.cc \
		localserver_np.cc \
		localserver_wrapper.cc \
		managed_resource_store_np.cc \
		managed_resource_store_wrapper.cc \
		resource_store_np.cc \
		resource_store_wrapper.cc \
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
# channel/ff

FF_VPATH	+= \
		channel/firefox \
		$(NULL)

FF_IDLSRCS	+= \
		channel.idl \
		$(NULL)

FF_CPPSRCS	+= \
		channel.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# channel/ie

IE_VPATH	+= \
		channel/ie \
		$(NULL)

IE_IDLSRCS	+= \
		channel.idl \
		$(NULL)

IE_CPPSRCS	+= \
		channel.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# channel/iemobile

IEMOBILE_VPATH	+= \
		channel/ie \
		$(NULL)

IEMOBILE_IDLSRCS	+= \
		channel.idl \
		$(NULL)

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

include tools/rules.mk
