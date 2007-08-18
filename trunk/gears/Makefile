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
endif

#-----------------------------------------------------------------------------
# base/common

COMMON_VPATH	+= \
		base/common \
		installer \
		$(NULL)

COMMON_M4SRCS	+= \
		product_name_constants.h.m4 \
		product_version_constants.h.m4 \
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
		factory.idl \
		base_interface_ff.idl \
		$(NULL)

FF_CPPSRCS	+= \
		base_class.cc \
		base_class_test.cc \
		dom_utils.cc \
		factory.cc \
		factory_utils.cc \
		file.cc \
		file_ff.cc \
		file_test.cc \
		html_event_monitor.cc \
		html_event_monitor_ff.cc \
		http_utils.cc \
		js_runner_ff.cc \
		js_runner_ff_marshaling.cc \
		js_runner_utils.cc \
		module.cc \
		mutex.cc \
		name_value_table.cc \
		ns_file_utils.cc \
		paths.cc \
		paths_ff.cc \
		permissions_db.cc \
		product_version.cc \
		security_model.cc \
		security_model_test.cc \
		sqlite_wrapper.cc \
		stopwatch.cc \
		stopwatch_ff.cc \
		string16.cc \
		string_utils.cc \
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
		factory.rgs.m4 \
		interfaces.idl.m4 \
		module.rgs.m4 \
		$(NULL)

IE_IDLSRCS	+= \
		factory.idl \
		interfaces.idl \
		$(NULL)

IE_CPPSRCS	+= \
		activex_utils.cc \
		base_class.cc \
		base_class_test.cc \
		bho.cc \
		detect_version_collision.cc \
		factory.cc \
		factory_utils.cc \
		file.cc \
		file_ie.cc \
		file_test.cc \
		html_event_monitor.cc \
		html_event_monitor_ie.cc \
		http_utils.cc \
		js_runner_ie.cc \
		js_runner_utils.cc \
		module.cc \
		mutex.cc \
		name_value_table.cc \
		paths.cc \
		paths_ie.cc \
		permissions_db.cc \
		product_version.cc \
		security_model.cc \
		security_model_test.cc \
		sqlite_wrapper.cc \
		stopwatch.cc \
		stopwatch_ie.cc \
		string16.cc \
		string_utils.cc \
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
# ui/common

COMMON_VPATH	+= \
		ui/common \
		$(NULL)

COMMON_M4SRCS	+= \
		permissions_dialog.html.m4 \
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
		i18n-en-US.dtd.m4 \
		$(NULL)

FF_IDLSRCS	+= \
		ui_utils.idl \
		$(NULL)

FF_CPPSRCS	+= \
		html_dialog.cc \
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
		settings_dialog.cc \
		tools_menu_item.cc \
		$(NULL)

IE_LINK_EXTRAS	+= \
		$(IE_OUTDIR)/ui_resources.res \
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
# timer/firefox

FF_VPATH	+= \
		timer/firefox \
		$(NULL)

FF_IDLSRCS	+= \
		timer.idl \
		$(NULL)

FF_CPPSRCS	+= \
		timer.cc \
		$(NULL)

#-----------------------------------------------------------------------------
# timer/ie

IE_VPATH	+= \
		timer/ie \
		$(NULL)

IE_IDLSRCS	+= \
		timer.idl \
		$(NULL)

IE_CPPSRCS	+= \
		timer.cc \
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
		localserver_tests.cc \
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
		localserver_tests.cc \
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

include tools/rules.mk
