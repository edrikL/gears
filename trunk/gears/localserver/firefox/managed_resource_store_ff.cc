// Copyright 2005, Google Inc.
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

#include "gecko_internal/nsIDOMClassInfo.h"
#include "gecko_internal/nsIVariant.h"

#include "gears/localserver/firefox/managed_resource_store_ff.h"

#include "gears/base/common/url_utils.h"
#include "gears/base/firefox/dom_utils.h"


// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_THREADSAFE_ADDREF(GearsManagedResourceStore)
NS_IMPL_THREADSAFE_RELEASE(GearsManagedResourceStore)
NS_INTERFACE_MAP_BEGIN(GearsManagedResourceStore)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsManagedResourceStoreInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsManagedResourceStoreInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsManagedResourceStore)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsManagedResourceStoreClassName = "GearsManagedResourceStore";
const nsCID kGearsManagedResourceStoreClassId = {0x924e3de8, 0xa842, 0x4982, {0xb7, 0x5b,
                                                 0x96, 0xfa, 0x77, 0x9f, 0xa9, 0x37}};
                                                 // {924E3DE8-A842-4982-B75B-96FA779FA937}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
GearsManagedResourceStore::~GearsManagedResourceStore() {
  if (update_task_.get()) {
    update_task_->SetListener(NULL);
    update_task_->Abort();
    update_task_.release()->DeleteWhenDone();
  }
}

//------------------------------------------------------------------------------
// GetName
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsManagedResourceStore::GetName(nsAString &name) {
  name.Assign(store_.GetName());
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetRequiredCookie
//------------------------------------------------------------------------------
NS_IMETHODIMP
GearsManagedResourceStore::GetRequiredCookie(nsAString &cookie) {
  cookie.Assign(store_.GetRequiredCookie());
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetEnabled
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsManagedResourceStore::GetEnabled(PRBool *enabled) {
  *enabled = store_.IsEnabled() ? PR_TRUE : PR_FALSE;
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// SetEnabled
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsManagedResourceStore::SetEnabled(PRBool enabled) {
  if (!store_.SetEnabled(enabled ? true : false)) {
    RETURN_EXCEPTION(STRING16(L"Failed to set the enabled property."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetManifestUrl
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsManagedResourceStore::GetManifestUrl(nsAString &url_out) {
  std::string16 manifest_url;
  if (store_.GetManifestUrl(&manifest_url)) {
    url_out.Assign(manifest_url.c_str());
  } else {
    RETURN_EXCEPTION(STRING16(L"Failed to get manifest url."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// SetManifestUrl
//------------------------------------------------------------------------------
NS_IMETHODIMP 
GearsManagedResourceStore::SetManifestUrl(const nsAString &url_abstract) {
  // Resetting the manifest url to an empty string is ok.
  if (url_abstract.Length() == 0) {
    if (!store_.SetManifestUrl(STRING16(L""))) {
      RETURN_EXCEPTION(STRING16(L"Failed to reset manifest url."));
    }
    RETURN_NORMAL();
  }

  nsString url(url_abstract); // nsAString doesn't have get()
  std::string16 full_url;    
  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url.get(),
                           &full_url)) {
    RETURN_EXCEPTION(STRING16(L"Failed to resolve url."));
  }
  if (!EnvPageSecurityOrigin().IsSameOriginAsUrl(full_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Url is not from the same origin"));
  }

  if (!store_.SetManifestUrl(full_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Failed to set manifest url."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetLastUpdateCheckTime
//------------------------------------------------------------------------------
NS_IMETHODIMP 
GearsManagedResourceStore::GetLastUpdateCheckTime(PRInt32 *time) {
  if (!time) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  *time = 0;
  int64 time64 = 0;
  WebCacheDB::UpdateStatus status;
  if (store_.GetUpdateInfo(&status, &time64, NULL, NULL)) {
    *time = static_cast<long>(time64/1000); // convert to seconds
  } else {
    RETURN_EXCEPTION(STRING16(L"Failed to get update info."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetUpdateStatus
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsManagedResourceStore::GetUpdateStatus(PRInt32 *status) {
  if (!status) {
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
// GetLastErrorMessage
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsManagedResourceStore::GetLastErrorMessage(
    nsAString &error_message_out) {
  std::string16 error_message;
  WebCacheDB::UpdateStatus update_status;
  int64 time64 = 0;
  if (store_.GetUpdateInfo(&update_status,
                           &time64,
                           NULL,
                           &error_message)) {
    error_message_out.Assign(error_message.c_str());
  } else {
    RETURN_EXCEPTION(STRING16(L"Failed to get last error message."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// CheckForUpdate
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsManagedResourceStore::CheckForUpdate() {
  if (update_task_.get()) {
    // We're already running an update task, just return
    RETURN_NORMAL();
  }

  update_task_.reset(new FFUpdateTask());
  if (!update_task_->Init(&store_)) {
    update_task_.reset(NULL);
    RETURN_EXCEPTION(STRING16(L"Failed to initialize update task."));
  }

  update_task_->SetListener(this);
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
// GetCurrentVersion
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsManagedResourceStore::GetCurrentVersion(nsAString &ver) {
  GetAppVersionString(WebCacheDB::VERSION_CURRENT, ver);
  return NS_OK;
}

//------------------------------------------------------------------------------
// GetAppVersionString
//------------------------------------------------------------------------------
void GearsManagedResourceStore::GetAppVersionString(
                                      WebCacheDB::VersionReadyState state,
                                      nsAString &ver_out) {
  std::string16 ver;
  if (store_.GetVersionString(state, &ver)) {
    ver_out.Assign(ver.c_str());
  } else {
    ver_out.SetLength(0);
  }
}

//------------------------------------------------------------------------------
// HandleEvent
//------------------------------------------------------------------------------
void GearsManagedResourceStore::HandleEvent(int code, int param,
                                            AsyncTask *source) {
  FFUpdateTask* task = reinterpret_cast<FFUpdateTask*>(source);
  if (task && (task == update_task_.get())) {
    if (code == FFUpdateTask::UPDATE_TASK_COMPLETE) {
      update_task_->SetListener(NULL);
      update_task_.release()->DeleteWhenDone();
    }
  }
}
