# Copyright 2008, Google Inc.
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

# This is a temporary file used to filter out the files that have
# not been ported to Symbian yet. It will be eventually removed
# when the port is complete.

CPPSRCS_TO_REMOVE	+= \
		async_router.cc \
		byte_store.cc \
		byte_store_test.cc \
		event.cc \
		event_test.cc \
		message_queue.cc \
		message_service.cc \
		message_service_test.cc \
		process_utils_win32.cc \
		png_utils.cc \
		$(NULL)

# console
CPPSRCS_TO_REMOVE	+= \
		console.cc \
		js_callback_logging_backend.cc \
		$(NULL)

# canvas
CPPSRCS_TO_REMOVE += \
		canvas.cc \
		canvas_rendering_context_2d.cc \
		$(NULL)

# database2
CPPSRCS_TO_REMOVE += \
		connection.cc \
		commands.cc \
		database2.cc \
		database2_common.cc \
		interpreter.cc \
		manager.cc \
		result_set2.cc \
		statement.cc \
		transaction.cc \
		$(NULL)

# desktop
CPPSRCS_TO_REMOVE += \
		desktop.cc \
		desktop_linux.cc \
		desktop_osx.cc \
		desktop_win32.cc \
		dll_data_wince.cc \
		shortcut_utils_win32.cc \
		$(NULL)
    
CPPSRCS_TO_REMOVE += \
		file_dialog.cc \
		file_dialog_gtk.cc \
		file_dialog_osx.cc \
		file_dialog_utils.cc \
		file_dialog_win32.cc \
		$(NULL)

CPPSRCS_TO_REMOVE += \
		notifier_process_linux.cc \
		notifier_process_osx.cc \
		notifier_process_win32.cc \
		notification.cc \
		$(NULL)
    
# test
CPPSRCS_TO_REMOVE += \
		test.cc \
		$(NULL)

# permission dialog
CPPSRCS_TO_REMOVE += \
		settings_dialog.cc \
		$(NULL)

# workerpool/npapi
CPPSRCS_TO_REMOVE += \
		pool_threads_manager.cc \
		workerpool.cc \
		workerpool_utils.cc \
		$(NULL)

# timer
CPPSRCS_TO_REMOVE += \
		timer.cc \
		$(NULL)

CPPSRCS_TO_REMOVE += \
		blob_store.cc \
		capture_task.cc \
		file_store.cc \
		http_cookies.cc \
		localserver.cc \
		localserver_db.cc \
		localserver_perf_test.cc \
		managed_resource_store.cc \
		managed_resource_store_module.cc \
		manifest.cc \
		resource_store.cc \
		update_task.cc \
		update_task_single_process.cc \
		async_task_np.cc \
		file_submitter_np.cc \
		http_handler_ie.cc \
		http_request_ie.cc \
		localserver_np.cc \
		update_task_np.cc \
		urlmon_utils.cc \
		resource_store_np.cc \
		$(NULL)

# httprequest
CPPSRCS_TO_REMOVE += \
		httprequest.cc \
		httprequest_upload.cc \
		$(NULL)

# blob
CPPSRCS_TO_REMOVE += \
		blob.cc \
		blob_builder.cc \
		blob_test.cc \
		buffer_blob.cc \
		file_blob.cc \
		join_blob.cc \
		slice_blob.cc \
		$(NULL)

# image
CPPSRCS_TO_REMOVE += \
		backing_image.cc \
		image.cc \
		image_loader.cc \
		$(NULL)

# geolocation
CPPSRCS_TO_REMOVE += \
		device_data_provider.cc \
		empty_device_data_provider.cc \
		geolocation.cc \
		geolocation_test.cc \
		location_provider.cc \
		location_provider_pool.cc \
		network_location_provider.cc \
		network_location_request.cc \
		radio_data_provider_wince.cc \
		thread.cc \
		wifi_data_provider_linux.cc \
		wifi_data_provider_win32.cc \
		wifi_data_provider_wince.cc \
		wifi_data_provider_windows_common.cc \
		$(NULL)

NPAPI_CPPSRCS := $(filter-out $(CPPSRCS_TO_REMOVE), $(NPAPI_CPPSRCS))

# The unit tests drag in symbols we haven't sorted out yet for Symbian builds.
# NPAPI_CPPSRCS := $(filter-out %_test.cc,$(NPAPI_CPPSRCS))

NPAPI_LINK_EXTRAS :=
