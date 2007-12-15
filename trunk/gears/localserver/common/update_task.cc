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
#include <math.h>
#include <set>

#include "gears/localserver/common/update_task.h"

#include "gears/base/common/file.h"
#include "gears/base/common/stopwatch.h"
#include "gears/base/common/string_utils.h"
#include "gears/localserver/common/http_constants.h"
#include "gears/localserver/common/manifest.h"

const char16* kDefaultErrorMessage = STRING16(L"Internal error");
const char16 *kMissingManifestUrlErrorMessage =
                  STRING16(L"Manifest URL is not set");
const char16 *kManifestParseErrorMessagePrefix =
                  STRING16(L"Invalid manifest - ");
const char16 *kTooManyRedirectsErrorMessage =
                  STRING16(L"Redirect chain too long");
const char16 *kRedirectErrorMessage =
                  STRING16(L"Illegal redirect to a different origin");
const char16 *kEmptyManifestErrorMessage =
                  STRING16(L"No content returned");


//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
bool UpdateTask::Init(ManagedResourceStore *store) {
  if (!AsyncTask::Init()) {
    return false;
  }

  assert(store);

  if (!store->StillExistsInDB()) {
    is_initialized_ = false;
    return false;
  }

  is_aborted_ = false;
  is_initialized_ = store_.Clone(store);
  SetStartupSignal(false);
  return is_initialized_;
}

//------------------------------------------------------------------------------
// Run
//------------------------------------------------------------------------------
void UpdateTask::Run() {
  LOG(("UpdateTask::Run - starting\n"));

  bool success = false;

  if (is_initialized_ && !is_aborted_) {
    if (store_.SetUpdateInfo(WebCacheDB::UPDATE_CHECKING,
                             GetCurrentTimeMillis(),
                             NULL,
                             NULL)) {
      SetStartupSignal(true);

      // If the manifest url changes sometime after we start, re-run the task.
      // This can happen if a JavaScript client sets the ManifestUrl property
      // while an update task is running.
      std::string16 manifest_url_at_start;
      std::string16 manifest_url_at_end;
      do {
        success = false;

        if (!store_.GetManifestUrl(&manifest_url_at_start)) {
          success = false;
          break;
        }

        if (UpdateManifest()) {
          success = DownloadVersion();
        }

        if (!store_.GetManifestUrl(&manifest_url_at_end)) {
          success = false;
          break;
        }
      } while(manifest_url_at_start != manifest_url_at_end);

      if (success) {
        store_.SetUpdateInfo(WebCacheDB::UPDATE_OK,
                             GetCurrentTimeMillis(),
                             NULL,
                             NULL);
      } else {
        store_.SetUpdateInfo(WebCacheDB::UPDATE_FAILED,
                             GetCurrentTimeMillis(),
                             NULL,
                             !error_msg_.empty() ? error_msg_.c_str()
                                                 : kDefaultErrorMessage);
      }
    }
  }

  NotifyTaskComplete(success);

  LOG(("UpdateTask::Run - finished\n"));
}

//------------------------------------------------------------------------------
// NotifyTaskComplete
//------------------------------------------------------------------------------
void UpdateTask::NotifyTaskComplete(bool success) {
  // We set the startup signal here in case another task was running and this
  // task actually did not run and did not set the signal
  SetStartupSignal(true);
  NotifyListener(UPDATE_TASK_COMPLETE, success ? -1 : 0);
}

//------------------------------------------------------------------------------
// HttpGetUrl
//------------------------------------------------------------------------------
bool UpdateTask::HttpGetUrl(const char16 *full_url,
                            bool is_capturing,
                            const char16 *if_mod_since_date,
                            WebCacheDB::PayloadInfo *payload,
                            bool *was_redirected,
                            std::string16 *full_redirect_url) {
  // TODO(michaeln): I'm not sure what the right thing todo is here.
  // We have no way of knowing if server authentication is required to
  // capture a resource, and really no way of knowing if the user is
  // authenticated with the server.  A typical response when a user is
  // not signed-in is to reply with a redirect to a login page rather
  // than an HTTP error.
  //
  // Our plan of record to detect these kinds of errors is to modify the
  // server side of the webApp to NOT issue 302s to a login page when the
  // custom "X-Gears-Google" HTTP header is present.  That header indicates
  // a capture request. When that header is seen and the user is not
  // authenticated, the server will respond with an HTTP error instead. Can we
  // impose this requirement on developers?
  //
  // Our required cookie is a poor approximation of whether or not a user
  // authentication is actually required and whether or not the user is
  // authenticated with a server. Really, they are simply a way to serve
  // different content to different users, for a given URL -- regardless
  // of authentication.
  //
  // This also applies to CaptureTask.
  //
  // Perhaps the following test should not be here at all?
  //
  // Prior to fetching a url, ensure any required cookie is present.


  // Fetch the url from a server
  if (!AsyncTask::HttpGet(full_url,
                          is_capturing,
                          if_mod_since_date,
                          store_.GetRequiredCookie(),
                          payload,
                          was_redirected,
                          full_redirect_url,
                          &error_msg_)) {
    LOG(("UpdateTask::HttpGetUrl - failed to get url\n"));
    if (error_msg_.empty())
      SetHttpError(full_url, NULL);
    return false;  // TODO(michaeln): retry?
  }

  return true;
}

//------------------------------------------------------------------------------
// UpdateManifest
//------------------------------------------------------------------------------
bool UpdateTask::UpdateManifest() {
  WebCacheDB::ServerInfo server;
  if (!store_.GetServer(&server)) {
    return false;
  }

  if (server.manifest_url.empty()) {
    LOG(("UpdateTask::UpdateManifest - no manifest url\n"));
    error_msg_ = kMissingManifestUrlErrorMessage;
    return false;
  }

  assert(store_.GetSecurityOrigin().IsSameOriginAsUrl(
                                        server.manifest_url.c_str()));

  // Fetch a current manifest file from the server
  WebCacheDB::PayloadInfo manifest_payload;
  bool was_redirected = false;
  std::string16 manifest_redirect_url;
  if (!HttpGetUrl(server.manifest_url.c_str(),
                  false,  // not for capture into cache
                  server.manifest_date_header.c_str(),
                  &manifest_payload,
                  &was_redirected,
                  &manifest_redirect_url)) {
    return false;
  }

  const char16 *actual_manifest_url = was_redirected
                                        ? manifest_redirect_url.c_str()
                                        : server.manifest_url.c_str();
  if (was_redirected) {
    if (!store_.GetSecurityOrigin().IsSameOriginAsUrl(actual_manifest_url)) {
      LOG(("UpdateTask::UpdateManifest - illegal manifest url redirect\n"));
      error_msg_ = kRedirectErrorMessage;
      return false;
    }
  }

  if (manifest_payload.status_code == HttpConstants::HTTP_NOT_MODIFIED) {
    // We already have the most recent manifest
    // TODO(michaeln): what if the manifest is different, older?
    LOG(("UpdateTask::UpdateManifest - received HTTP_NOT_MODIFIED\n"));
    store_.SetUpdateInfo(WebCacheDB::UPDATE_CHECKING,
                         GetCurrentTimeMillis(),  NULL, NULL);
  } else if (manifest_payload.status_code == HttpConstants::HTTP_OK) {
    // Parse the manifest json data
    Manifest manifest;
    if (manifest_payload.data->size() <= 0) {
      LOG(("UpdateTask::UpdateManifest - manifest.Parse failed\n"));
      error_msg_ = kManifestParseErrorMessagePrefix;
      error_msg_ += kEmptyManifestErrorMessage;
      return false;
    }
    if (!manifest.Parse(actual_manifest_url,
                        reinterpret_cast<const char*>
                            (&(*manifest_payload.data)[0]),
                        manifest_payload.data->size())) {
      LOG(("UpdateTask::UpdateManifest - manifest.Parse failed\n"));
      error_msg_ = kManifestParseErrorMessagePrefix;
      error_msg_ += manifest.GetErrorMessage();
      return false;
    }

    WebCacheDB *db = WebCacheDB::GetDB();
    if (!db) {
      LOG(("UpdateTask::UpdateManifest - GetDB failed\n"));
      return false;
    }

    // Get info about what versions we currently have in the DB
    // There are at most two versions per application, one in each
    // possible ready state.
    std::vector<WebCacheDB::VersionInfo> versions;
    if (!db->FindVersions(store_.GetServerID(), &versions)) {
      LOG(("UpdateTask::UpdateManifest - FindVersions failed\n"));
      return false;
    }
    std::string16 *current_version_str = NULL;
    std::string16 *downloading_version_str = NULL;
    int64 current_version_id = WebCacheDB::kInvalidID;
    int64 downloading_version_id = WebCacheDB::kInvalidID;
    for (int i = 0; i < static_cast<int>(versions.size()); i++) {
      switch(versions[i].ready_state) {
        case WebCacheDB::VERSION_CURRENT:
          assert(!current_version_str);
          current_version_str = &(versions[i].version_string);
          current_version_id = versions[i].id;
          break;
        case WebCacheDB::VERSION_DOWNLOADING:
          assert(!downloading_version_str);
          downloading_version_str = &(versions[i].version_string);
          downloading_version_id = versions[i].id;
          break;
        default:
          assert(false);
          break;
      }
    }

    // Determine what action to take with the version represented in
    // the manifest file we've fetched
    const char16* manifest_version = manifest.GetVersion();

    if (current_version_str && ((*current_version_str) == manifest_version)) {
      // We already have this version as current, so we can delete any others
      LOG(("UpdateTask::UpdateManifest - already current manifest file\n"));
      if (downloading_version_str &&
          !db->DeleteVersion(downloading_version_id)) {
        LOG(("UpdateTask::UpdateManifest - DeleteVersion failed\n"));
        return false;
      }
    } else if (downloading_version_str &&
               ((*downloading_version_str) == manifest_version)) {
      // We're already downloading this version, no action required
      LOG(("UpdateTask::UpdateManifest - already downloading manifest file\n"));
    } else {
      // We don't know about this version, we need to add it to the DB
      // as the version we're downloading. If we already have a downloading
      // version, that version is deleted.
      LOG(("UpdateTask::UpdateManifest - received new manifest file\n"));
      if (!store_.AddManifestAsDownloadingVersion(&manifest, NULL)) {
        LOG(("UpdateTask::UpdateManifest - "
             "AddManifestAsDownloadingVersion failed\n"));
        return false;
      }
      std::string16 manifest_date;
      manifest_payload.GetHeader(HttpConstants::kLastModifiedHeader,
                                 &manifest_date);
      store_.SetUpdateInfo(WebCacheDB::UPDATE_CHECKING,
                           GetCurrentTimeMillis(),
                           manifest_date.c_str(),
                           NULL);
    }

  } else {
    // We received a bad response
    LOG(("UpdateTask::UpdateManifest - received bad response %d\n",
         manifest_payload.status_code));

    SetHttpError(server.manifest_url.c_str(), &manifest_payload.status_code);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
// DownloadVersion
//------------------------------------------------------------------------------
bool UpdateTask::DownloadVersion() {
  typedef std::set<std::string16> String16Set;
  typedef std::vector<WebCacheDB::EntryInfo> EntryInfoVector;

  WebCacheDB *db = WebCacheDB::GetDB();
  if (!db) {
    return false;
  }

  // Consult the DB for which version we should be downloading if any
  WebCacheDB::VersionInfo version;
  if (!store_.GetVersion(WebCacheDB::VERSION_DOWNLOADING, &version)) {
    LOG(("UpdateTask::DownloadVersion - nothing to download\n"));
    return true;
  }

  // Find entries that have not yet been filled (downloaded)
  EntryInfoVector entries;
  if (!db->FindEntriesHavingNoResponse(version.id, &entries)) {
    LOG(("UpdateTask::DownloadVersion - FindEntriesHavingNoResponse failed\n"));
    return false;
  }

  if (entries.size() != 0) {
    // Build a set of unique urls that need to be processed
    String16Set urls;
    for (EntryInfoVector::iterator entry = entries.begin();
        entry != entries.end();
        ++entry) {
      urls.insert(!entry->src.empty() ? entry->src : entry->url);
    }
    entries.clear();

    store_.SetUpdateInfo(WebCacheDB::UPDATE_DOWNLOADING,
                        GetCurrentTimeMillis(), NULL, NULL);

    LOG(("UpdateTask::DownloadVersion - %d urls to process\n", urls.size()));

    // Process each unique url, downloading only if needed, and update
    // all relevent entries to refer to the same payload
    for (String16Set::iterator url = urls.begin(); url != urls.end(); ++url) {
      if (!store_.StillExistsInDB()) {
        LOG(("UpdateTask exitting, store no longer exists\n"));
        return false;
      }

      bool success = ProcessUrl(*url, &version, NULL);
      if (!success) {
        LOG(("UpdateTask::DownloadVersion - ProcessUrl failed\n"));
        return false;
      }
    }
  }

  // Transition this version from downloading to current
  if (!store_.SetDownloadingVersionAsCurrent()) {
    LOG(("UpdateTask::DownloadVersion - "
         "SetDownloadingVersionAsCurrent failed\n"));
    return false;
  }

  return true;
}


//------------------------------------------------------------------------------
// ProcessUrl
//------------------------------------------------------------------------------
bool UpdateTask::ProcessUrl(const std::string16 &url,
                            WebCacheDB::VersionInfo *version,
                            int64 *payload_id_out) {
  WebCacheDB *db = WebCacheDB::GetDB();
  if (!db) {
    return false;
  }

  // Should already have been checked when parsing the manifest file
  assert(store_.GetSecurityOrigin().IsSameOriginAsUrl(url.c_str()));

  int64 payload_id = WebCacheDB::kInvalidID;
  std::string16 redirect_url;

  // Retrieve info about our most recent entry for this url
  std::string16 previous_version_mod_date;
  std::string16 previous_version_redirect_url;
  int64 previous_version_payload_id = WebCacheDB::kInvalidID;
  FindPreviousVersionPayload(version->server_id,
                             url.c_str(),
                             &previous_version_payload_id,
                             &previous_version_redirect_url,
                             &previous_version_mod_date);

  // Make an HTTP GET IF_MODIFIED_SINCE request
  WebCacheDB::PayloadInfo new_payload;
  if (!HttpGetUrl(url.c_str(),
                  true,  // for capture into cache
                  previous_version_mod_date.c_str(),
                  &new_payload,
                  NULL, NULL)) {
    return false;
  }

  SQLTransaction transaction(db->GetSQLDatabase(), "UpdateTask::ProcessUrl");
  if (new_payload.status_code == HttpConstants::HTTP_NOT_MODIFIED) {
    // TODO(michaeln): what if mod-date is older than what we have?
    LOG(("UpdateTask::ProcessUrl - received HTTP_NOT_MODIFIED\n"));
    payload_id = previous_version_payload_id;
    redirect_url = previous_version_redirect_url;
    if (!transaction.Begin()) {
      return false;
    }
  } else if (new_payload.status_code ==  HttpConstants::HTTP_OK) {
    LOG(("UpdateTask::ProcessUrl - received new payload\n"));
    if (!transaction.Begin()) {
      return false;
    }
    File::ClearLastFileError();
    if (!db->InsertPayload(version->server_id, url.c_str(), &new_payload)) {
      LOG(("UpdateTask::ProcessUrl - InsertPayload failed\n"));
      File::GetLastFileError(&error_msg_);
      return false;
    }
    payload_id = new_payload.id;
  } else {
    LOG(("UpdateTask::ProcessUrl - received bad response %d\n",
          new_payload.status_code));
    SetHttpError(url.c_str(), &new_payload.status_code);
    return false;
  }

  assert(payload_id != WebCacheDB::kInvalidID);

  // Update applicable entries to refer to this payload
  if (is_aborted_ ||
      !db->UpdateEntriesWithNewPayload(version->id,
                                       url.c_str(),
                                       payload_id,
                                       redirect_url.c_str())) {
    LOG(("UpdateTask::ProcessUrl - UpdateEntriesWithNewPayload failed\n"));
    return false;
  }

  return transaction.Commit();
}


//------------------------------------------------------------------------------
// FindPreviousVersionPayload
//------------------------------------------------------------------------------
bool UpdateTask::FindPreviousVersionPayload(int64 server_id,
                                            const char16* url,
                                            int64 *payload_id,
                                            std::string16 *redirect_url,
                                            std::string16 *mod_date) {
  assert(url);
  assert(payload_id);
  assert(redirect_url);
  assert(mod_date);

  WebCacheDB *db = WebCacheDB::GetDB();
  if (!db) {
    return false;
  }

  WebCacheDB::PayloadInfo payload;
  if (!db->FindMostRecentPayload(server_id, url, &payload)) {
    return false;
  }

  if (!payload.GetHeader(HttpConstants::kLastModifiedHeader, mod_date)) {
    return false;
  }

  *payload_id = payload.id;
  return true;
}

//------------------------------------------------------------------------------
// SetHttpError
// http_status is optional. If NULL, error message will be generic error.
//
// TODO(aa): It would nice to be able to also take the first 50 or so chars of
// the response text as well.
//------------------------------------------------------------------------------
bool UpdateTask::SetHttpError(const char16 *url, const int *http_status) {
  assert(url);

  error_msg_ = STRING16(L"Download of '");
  error_msg_ += url;

  if (!http_status) {
    error_msg_ += STRING16(L"' failed");
  } else {
    // http status codes are always three digits.
    if (*http_status >= pow(10.0, HttpConstants::kHttpStatusCodeMaxDigits)) {
      LOG(("Unexpected status code '%d'", *http_status));
      error_msg_ += STRING16(L"' failed");
      return false;
    }

    error_msg_ += STRING16(L"' returned response code ");
    error_msg_ += IntegerToString16(*http_status);
  }

  return true;
}
