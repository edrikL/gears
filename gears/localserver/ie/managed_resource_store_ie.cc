// Copyright 2006, Google Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <assert.h>

#include "gears/localserver/ie/managed_resource_store_ie.h"

#include "gears/base/common/common.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"

//------------------------------------------------------------------------------
// FinalConstruct
//------------------------------------------------------------------------------
HRESULT GearsManagedResourceStore::FinalConstruct() {
  ATLTRACE(_T("GearsManagedResourceStore::FinalConstruct\n"));
  return S_OK;
}


//------------------------------------------------------------------------------
// FinalRelease
//------------------------------------------------------------------------------
void GearsManagedResourceStore::FinalRelease() {
  ATLTRACE(_T("GearsManagedResourceStore::FinalRelease\n"));

  if (update_task_.get()) {
    update_task_->SetListenerWindow(NULL, 0);
    update_task_->Abort();
    update_task_.release()->DeleteWhenDone();
  }

  MessageService::GetInstance()->RemoveObserver(this,
                                                observer_topic_.c_str());

  if (IsWindow()) {
    DestroyWindow();
  }
}

//------------------------------------------------------------------------------
// get_name
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::get_name(BSTR *name) {
  CComBSTR bstr(store_.GetName());
  *name = bstr.Detach();
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// get_requiredCookie
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::get_requiredCookie(BSTR *retval) {
  CComBSTR bstr(store_.GetRequiredCookie());
  *retval = bstr.Detach();
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// get_enabled
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::get_enabled(VARIANT_BOOL *enabled) {
  *enabled = store_.IsEnabled() ? VARIANT_TRUE : VARIANT_FALSE;
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// put_enabled
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::put_enabled(VARIANT_BOOL enabled) {
  if (!store_.SetEnabled(enabled ? true : false)) {
    RETURN_EXCEPTION(STRING16(L"Failed to set the enabled property."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// get_manifestUrl
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::get_manifestUrl(BSTR *retval) {
  CComBSTR bstr;
  std::string16 manifest_url;
  if (store_.GetManifestUrl(&manifest_url)) {
    bstr = manifest_url.c_str();
  } else {
    RETURN_EXCEPTION(STRING16(L"Failed to get manifest url."));
  }
  *retval = bstr.Detach();
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// put_manifestUrl
//------------------------------------------------------------------------------
STDMETHODIMP
GearsManagedResourceStore::put_manifestUrl(const BSTR manifest_url) {
  // Resetting the manifest url to an empty string is ok. A NULL BSTR
  // means empty string.
  if (!manifest_url || !manifest_url[0]) {
    if (!store_.SetManifestUrl(L"")) {
      RETURN_EXCEPTION(STRING16(L"Failed to reset manifest url."));
    }
    RETURN_NORMAL();
  }

  ATLTRACE(_T("ManagedResourceStore::put_manifestUrl( %s )\n"), manifest_url);

  std::string16 full_manifest_url;
  if (!ResolveAndNormalize(this->EnvPageLocationUrl().c_str(), manifest_url,
                           &full_manifest_url)) {
    RETURN_EXCEPTION(STRING16(L"Failed to resolve url."));
  }

  if (!EnvPageSecurityOrigin().IsSameOriginAsUrl(full_manifest_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Url is not from the same origin"));
  }

  if (!store_.SetManifestUrl(full_manifest_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Failed to set manifest url."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// get_lastUpdateCheckTime
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::get_lastUpdateCheckTime(long *time) {
  if (!time) {
    assert(time);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  *time = 0;
  int64 time64 = 0;
  WebCacheDB::UpdateStatus status;
  if (store_.GetUpdateInfo(&status, &time64, NULL, NULL)) {
    *time = static_cast<long>(time64/1000);  // convert to seconds
  } else {
    RETURN_EXCEPTION(STRING16(L"Failed to get update info."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// get_updateStatus
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::get_updateStatus(int *status) {
  if (!status) {
    assert(status);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  *status = WebCacheDB::UPDATE_OK;
  WebCacheDB::UpdateStatus update_status;
  int64 time64 = 0;
  if (store_.GetUpdateInfo(&update_status, &time64, NULL, NULL)) {
    *status = update_status;
  } else {
    RETURN_EXCEPTION(STRING16(L"Failed to get update info."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// get_lastErrorMessage
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::get_lastErrorMessage(
    BSTR *lastErrorMessage) {
  CComBSTR bstr;
  std::string16 last_error_message;
  WebCacheDB::UpdateStatus update_status;
  int64 time64 = 0;
  // TODO(aa): It's kinda lame that we have to get all these other
  // values to get the one we want. Also, updateStatus and
  // lastErrorMessage will typically be called one after another,
  // creating two sqlite calls. Can this be improved?
  if (store_.GetUpdateInfo(&update_status,
                                 &time64,
                                 NULL,
                                 &last_error_message)) {
    bstr = last_error_message.c_str();
    *lastErrorMessage = bstr.Detach();
  } else {
    RETURN_EXCEPTION(STRING16(L"Failed to get last error message."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// put_onerror
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::put_onerror(const VARIANT *in_value) {
  if (ActiveXUtils::VariantIsNullOrUndefined(in_value)) {
    onerror_handler_.reset(NULL);
  } else if (in_value->vt == VT_DISPATCH) {
    InitUnloadMonitor();
    observer_topic_ = UpdateTask::GetNotificationTopic(&store_);
    onerror_handler_.reset(new JsRootedCallback(NULL, *in_value));
    MessageService::GetInstance()->AddObserver(this,
                                               observer_topic_.c_str());
  } else {
    RETURN_EXCEPTION(STRING16(L"The onerror callback must be a function."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// put_onprogress
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::put_onprogress(
                                            const VARIANT *in_value) {
  if (ActiveXUtils::VariantIsNullOrUndefined(in_value)) {
    onprogress_handler_.reset(NULL);
  } else if (in_value->vt == VT_DISPATCH) {
    InitUnloadMonitor();
    observer_topic_ = UpdateTask::GetNotificationTopic(&store_);
    onprogress_handler_.reset(new JsRootedCallback(NULL, *in_value));
    MessageService::GetInstance()->AddObserver(this,
                                               observer_topic_.c_str());
  } else {
    RETURN_EXCEPTION(STRING16(L"The onprogress callback must be a function."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// put_oncomplete
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::put_oncomplete(
                                            const VARIANT *in_value) {
  if (ActiveXUtils::VariantIsNullOrUndefined(in_value)) {
    oncomplete_handler_.reset(NULL);
  } else if (in_value->vt == VT_DISPATCH) {
    InitUnloadMonitor();
    observer_topic_ = UpdateTask::GetNotificationTopic(&store_);
    oncomplete_handler_.reset(new JsRootedCallback(NULL, *in_value));
    MessageService::GetInstance()->AddObserver(this,
                                               observer_topic_.c_str());
  } else {
    RETURN_EXCEPTION(STRING16(L"The oncomplete callback must be a function."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// CreateWindowIfNeeded
//------------------------------------------------------------------------------
HRESULT GearsManagedResourceStore::CreateWindowIfNeeded() {
  if (!IsWindow()) {
    if (!Create(kMessageOnlyWindowParent,    // parent
                NULL,                        // position
                NULL,                        // name
                kMessageOnlyWindowStyle)) {  // style
      RETURN_EXCEPTION(STRING16(L"Failed to create message window."));
    }
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// InitUnloadMonitor
//------------------------------------------------------------------------------
void GearsManagedResourceStore::InitUnloadMonitor() {
  // Create an event monitor to alert us when the page unloads.
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD,
                                             this));
  }
}

//------------------------------------------------------------------------------
// HandleEvent
//------------------------------------------------------------------------------
void GearsManagedResourceStore::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);

  // Drop references, js context is going away.
  onerror_handler_.reset();
  onprogress_handler_.reset();
  oncomplete_handler_.reset();
  MessageService::GetInstance()->RemoveObserver(this,
                                                observer_topic_.c_str());
}

//------------------------------------------------------------------------------
// OnNotify
//------------------------------------------------------------------------------
void GearsManagedResourceStore::OnNotify(MessageService *service,
                                         const char16 *topic,
                                         const NotificationData *data) {
  scoped_ptr<JsRootedToken> param;
  JsRootedCallback *handler = 0;

  const UpdateTask::Event *event = static_cast<const UpdateTask::Event *>(data);
  switch(event->event_type()) {
    case UpdateTask::ERROR_EVENT: {
        if (!onerror_handler_.get()) return;
        handler = onerror_handler_.get();

        param.reset(GetJsRunner()->NewObject(NULL));
        if (!param.get()) return;

        const UpdateTask::ErrorEvent *error_event =
            static_cast<const UpdateTask::ErrorEvent *>(data);
        GetJsRunner()->SetPropertyString(param->token(),
                                         STRING16(L"message"),
                                         error_event->error_message().c_str());
      }
      break;

    case UpdateTask::PROGRESS_EVENT: {
        if (!onprogress_handler_.get()) return;
        handler = onprogress_handler_.get();

        param.reset(GetJsRunner()->NewObject(NULL));
        if (!param.get()) return;

        const UpdateTask::ProgressEvent *progress_event =
            static_cast<const UpdateTask::ProgressEvent *>(data);
        GetJsRunner()->SetPropertyInt(param->token(),
                                      STRING16(L"filesTotal"),
                                      progress_event->files_total());
        GetJsRunner()->SetPropertyInt(param->token(),
                                      STRING16(L"filesComplete"),
                                      progress_event->files_complete());
      }
      break;

    case UpdateTask::COMPLETION_EVENT: {
        if (!oncomplete_handler_.get()) return;
        handler = oncomplete_handler_.get();

        param.reset(GetJsRunner()->NewObject(NULL));
        if (!param.get()) return;

        const UpdateTask::CompletionEvent *completion_event =
            static_cast<const UpdateTask::CompletionEvent *>(data);
        GetJsRunner()->SetPropertyString(
                           param->token(),
                           STRING16(L"newVersion"),
                           completion_event->new_version_string().c_str());
      }
      break;

    default:
      return;
  }

  const int argc = 1;
  JsParamToSend argv[argc] = {
    { JSPARAM_OBJECT_TOKEN, param.get() }
  };
  GetJsRunner()->InvokeCallback(handler, argc, argv, NULL);
}

//------------------------------------------------------------------------------
// checkForUpdate
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::checkForUpdate(void) {
  if (update_task_.get()) {
    // We're already running an update task, just return
    RETURN_NORMAL();
  }

  update_task_.reset(new IEUpdateTask());
  if (!update_task_->Init(&store_)) {
    update_task_.reset(NULL);
    RETURN_EXCEPTION(STRING16(L"Failed to initialize update task."));
  }

  HRESULT hr = CreateWindowIfNeeded();
  if (FAILED(hr)) {
    update_task_.reset(NULL);
    return hr;
  }

  update_task_->SetListenerWindow(m_hWnd, kUpdateTaskMessageBase);
  if (!update_task_->Start()) {
    update_task_.reset(NULL);
    RETURN_EXCEPTION(STRING16(L"Failed to start update task."));
  }

  // We wait here so the updateStatus property reflects a running task
  // upon return
  update_task_->AwaitStartup();

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// OnUpdateTaskComplete
//------------------------------------------------------------------------------
LRESULT GearsManagedResourceStore::OnUpdateTaskComplete(UINT uMsg,
                                                          WPARAM wParam,
                                                          LPARAM lParam,
                                                          BOOL &bHandled) {
  IEUpdateTask *task = reinterpret_cast<IEUpdateTask*>(lParam);
  if (task && (task == update_task_.get())) {
    update_task_->SetListenerWindow(NULL, 0);
    update_task_.release()->DeleteWhenDone();
  }
  bHandled = TRUE;
  return 0;
}

//------------------------------------------------------------------------------
// get_currentVersion
//------------------------------------------------------------------------------
STDMETHODIMP GearsManagedResourceStore::get_currentVersion(BSTR *ver) {
  return GetVersionString(WebCacheDB::VERSION_CURRENT, ver);
}

//------------------------------------------------------------------------------
// GetVersionString
//------------------------------------------------------------------------------
HRESULT GearsManagedResourceStore::GetVersionString(
                    WebCacheDB::VersionReadyState state, BSTR *ver_out) {
  if (!ver_out) {
    assert(ver_out);
    RETURN_EXCEPTION(STRING16(L"Invalid parameters."));
  }
  CComBSTR ver_bstr;
  std::string16 ver;
  if (store_.GetVersionString(state, &ver)) {
    ver_bstr = ver.c_str();
  }
  *ver_out = ver_bstr.Detach();
  RETURN_NORMAL();
}
