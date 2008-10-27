// Copyright 2008, Google Inc.
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

#include "gears/installer/iemobile/periodic_checker.h"

#include "gears/base/ie/activex_utils.h"
#include "gears/blob/blob_interface.h"
#include "gears/blob/blob_utils.h"
#include "gears/installer/iemobile/resource.h"
#include "gears/localserver/common/async_task.h"
#include "gears/localserver/common/http_constants.h"
#include "genfiles/product_constants.h"

const char16* kUpgradeUrl = L"http://tools.google.com/service/update2/ff?"
                            L"guid=%7Bc3fc95dBb-cd75-4f3d-a586-bcb7D004784c%7D"
                            L"&version=" PRODUCT_VERSION_STRING
                            L"&appversion=1.0"
                            L"&os=wince"
                            L"&dist=google"
#ifdef OFFICIAL_BUILD
                            // Only pass 'dev=1' for non-official builds.
#else
                            L"&dev=1"
#endif
                            L"&application=";

// String constants for XML parsing
// Query language
const char16* kQueryLanguage = STRING16(L"XPath");
const char16* kQueryLanguageProperty = STRING16(L"SelectionLanguage");
// The document namespaces (space delimited)
const char16* kNamespace =
    STRING16(L"xmlns:RDF='http://www.w3.org/1999/02/22-rdf-syntax-ns#' "
             L"xmlns:em='http://www.mozilla.org/2004/em-rdf#'");
const char16* kNamespaceProperty = STRING16(L"SelectionNamespaces");
// The query for version elements: select the first em:version descendant
// of the root element.
const char16* kVersionQuery = STRING16(L"//em:version[1]");
// The query for update link elements: select the first em:updateLink descendant
// of the root element.
const char16* kUpdateLinkQuery = STRING16(L"//em:updateLink[1]");


// An implementation of AsyncTaks that makes a request to the upgrade URL and
// parses the response to extract the latest version number and download URL.
// Note that redirects are not followed.
class VersionFetchTask : public AsyncTask {
 public:
  // Factory method
  static VersionFetchTask *Create(const std::string16 &application_id,
                                  HWND listener_window,
                                  int listener_message_base);

  // Signals the worker thread to stop and asynchronously deletes the object.
  // No further messages will be sent to the listener once a call to this method
  // completes.
  void StopThreadAndDelete();

  const char16* Url() const;
  const char16* LatestVersion() const;

 private:
  // Use Create to create a new instance and StopThreadAndDelete to destroy.
  VersionFetchTask(const std::string16 &application_id);
  virtual ~VersionFetchTask() {}

  // AsyncTask implementation
  virtual void Run();

  // Parses XML and extracts the Gears version number and download URL.
  bool ExtractVersionAndDownloadUrl(const std::string16& xml);

  std::string16 url_;
  std::string16 latest_version_;

  std::string16 application_id_;

  DISALLOW_EVIL_CONSTRUCTORS(VersionFetchTask);
};


// static
PeriodicChecker* PeriodicChecker::CreateChecker() {
  return new PeriodicChecker();
}

bool PeriodicChecker::Init(int32 first_period,
                           int32 normal_period,
                           int32 grace_period,
                           const std::string16 &application_id,
                           ListenerInterface *listener) {
  first_period_ = first_period;
  normal_period_ = normal_period;
  grace_period_ = grace_period;
  application_id_ = application_id;
  listener_ = listener;
  return (TRUE == stop_event_.Create(NULL, FALSE, FALSE, NULL)) &&
      (TRUE == thread_complete_event_.Create(NULL, FALSE, FALSE, NULL));
}

PeriodicChecker::PeriodicChecker()
    : first_period_(0),
      normal_period_(0),
      grace_period_(0),
#ifdef DEBUG
      wait_for_cleanup_(true),
#endif
      thread_(NULL),
      timer_(1),
      task_(NULL),
      window_(NULL),
      normal_timer_event_(false) {
}

PeriodicChecker::~PeriodicChecker() {
#ifdef DEBUG
  if (wait_for_cleanup_) {
    ASSERT(!thread_);
  }
#endif
}

void PeriodicChecker::Start() {
  ASSERT(!thread_);
  // Start the thread.
  thread_ = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &ThreadMain,
                                                    this, 0, NULL));
  ASSERT(thread_);
}

void PeriodicChecker::Stop(bool wait_for_cleanup) {
  stop_event_.Set();
  if (wait_for_cleanup) {
    WaitForSingleObject(thread_complete_event_, INFINITE);
  } else {
#ifdef DEBUG
    wait_for_cleanup_ = false;
#endif
  }
}

unsigned int __stdcall PeriodicChecker::ThreadMain(void* checker) {
  PeriodicChecker* self = reinterpret_cast<PeriodicChecker*>(checker);
  self->RunMessageLoop();
  return 0;
}

void PeriodicChecker::RunMessageLoop() {
  // Create the thread window.
  window_ = Create(kMessageOnlyWindowParent,    // parent
                   NULL,                        // position
                   NULL,                        // name
                   kMessageOnlyWindowStyle);    // style
  // Set the timer for the first wait.
  SetTimer(static_cast<unsigned>(timer_),
           static_cast<unsigned>(first_period_),
           NULL);
  con_mgr_msg_ = RegisterWindowMessage(CONNMGR_STATUS_CHANGE_NOTIFICATION_MSG);
  // Run the loop.
  bool done = false;
  HANDLE handles[] = { stop_event_ };
  // Run the message loop.
  while (!done) {
    DWORD rv = MsgWaitForMultipleObjectsEx(ARRAYSIZE(handles),
                                           handles,
                                           INFINITE,
                                           QS_POSTMESSAGE | WM_TIMER,
                                           0);
    if (rv == WAIT_OBJECT_0) {
      // stop_event was signalled.
      done = true;
    } else if (rv == WAIT_OBJECT_0 + ARRAYSIZE(handles)) {
      // We have a posted message or a WM_TIMER in the queue. Dispatch them
      // and go back to sleep.
      MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        // We handle this here since, at compile time, we don't know
        // the value of con_mgr_msg_.
        if (msg.message == con_mgr_msg_) OnConnectionStatusChanged();
        DispatchMessage(&msg);
      }
    } else {
      // Something else happened.
      done = true;
    }
  }
  // Kill the timer.
  KillTimer(timer_);
  // Stop the task, if it's running.
  if (task_) {
    task_->StopThreadAndDelete();
    task_ = NULL;
  }
  // Destroy the thread window.
  DestroyWindow();
  // Close and null the thread handle.
  CloseHandle(thread_);
  thread_ = NULL;

  thread_complete_event_.Set();
}

LRESULT PeriodicChecker::OnTimer(UINT message,
                                 WPARAM w,
                                 LPARAM l,
                                 BOOL& handled) {
  if (!normal_timer_event_) {
    normal_timer_event_ = true;
    ResetTimer(normal_period_);
  }
  // Depending on how the periodic checker is configured, this can fire
  // while a task is running. We ignore such events.
  if (!task_) {
    task_ = VersionFetchTask::Create(application_id_,
                                     window_,
                                     kUpdateTaskMessageBase);
  }
  handled = TRUE;
  return TRUE;
}

LRESULT PeriodicChecker::OnComplete(UINT message,
                                    WPARAM success,
                                    LPARAM task,
                                    BOOL& handled) {
  assert(task_);
  if (success) {
    // We check for differences between the version strings. If there is a
    // difference, the server version wins.
    if (wcslen(task_->LatestVersion()) &&
        wcslen(task_->Url()) &&
        wcscmp(task_->LatestVersion(), PRODUCT_VERSION_STRING) != 0) {
      // Call listener
      listener_->UpdateUrlAvailable(task_->Url());
    }
  } else {
    // Failed attempt.
    // If it appears that we don't have a connection, wait for one to appear.
    // Otherwise, the error must have been on the network, so we'll try again
    // when the timer next expires.
    if (!ActiveXUtils::IsOnline()) {
      ConnMgrRegisterForStatusChangeNotification(TRUE, window_);
    }
  }

  // Now that we have handled the task message, we can delete it.
  task_->StopThreadAndDelete();
  task_ = NULL;

  handled = TRUE;
  return TRUE;
}

void PeriodicChecker::OnConnectionStatusChanged() {
  // Unregister from connection events. We'll register again
  // if the update check fails.
  if (ActiveXUtils::IsOnline()) {
    ConnMgrRegisterForStatusChangeNotification(FALSE, window_);
    // Schedule a check after the grace period.
    normal_timer_event_ = false;
    ResetTimer(grace_period_);
  }
}

void PeriodicChecker::ResetTimer(int32 period) {
  KillTimer(timer_);
  SetTimer(static_cast<unsigned>(timer_),
           static_cast<unsigned>(period),
           NULL);
}

// -----------------------------------------------------------------------------
// VersionFetchTask

// static
VersionFetchTask *VersionFetchTask::Create(
    const std::string16 &application_id,
    HWND listener_window,
    int listener_message_base) {
  VersionFetchTask *task = new VersionFetchTask(application_id);
  task->SetListenerWindow(listener_window, listener_message_base);
  if (!task) {
    assert(false);
    return NULL;
  }
  if (!task->Init() || !task->Start()) {
    delete task;
    return NULL;
  }
  return task;
}

VersionFetchTask::VersionFetchTask(const std::string16 &application_id)
    : AsyncTask(NULL),
      application_id_(application_id) {
}

void VersionFetchTask::StopThreadAndDelete() {
  SetListenerWindow(NULL, 0);
  Abort();
  DeleteWhenDone();
}

const char16* VersionFetchTask::LatestVersion() const {
  return latest_version_.c_str();
}

const char16* VersionFetchTask::Url() const {
  return url_.c_str();
}

// AsyncTask
void VersionFetchTask::Run() {
  WebCacheDB::PayloadInfo payload;
  scoped_refptr<BlobInterface> payload_data;
  bool success = false;
  std::string16 url = kUpgradeUrl + application_id_;
  if (ActiveXUtils::IsOnline() &&
      AsyncTask::HttpGet(url.c_str(),
                         true,
                         NULL,
                         NULL,
                         NULL,
                         &payload,
                         &payload_data,
                         NULL,  // was_redirected
                         NULL,  // full_redirect_url
                         NULL)) {  // error_msg
    std::string16 xml;
    const std::string16 charset;
    // payload.data can be empty in case of a 30x response.
    // The update server does not redirect, so we treat this as an error.
    if (payload_data->Length() &&
        BlobToString16(payload_data.get(), charset, &xml) &&
        ExtractVersionAndDownloadUrl(xml)) {
      // If the URL uses HTTPS, switch the scheme to HTTP, because some WinCE
      // devices complain about certificate permissions when using HTTPS.
      if (url_.find(HttpConstants::kHttpsScheme) == 0) {
        url_.replace(0, 5, HttpConstants::kHttpScheme);
      }
      success = true;
    }
  }
  NotifyListener(PeriodicChecker::kVersionFetchTaskMessageId, success);
}

// Internal
bool VersionFetchTask::ExtractVersionAndDownloadUrl(const std::string16& xml) {
  CComPtr<IXMLDOMDocument2> document;
  // Note that CoInitializeEx is called by the parent class.
  if (FAILED(document.CoCreateInstance(
      CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER))) {
    return false;
  }
  ASSERT(document);
  document->put_async(VARIANT_FALSE);
  VARIANT_BOOL success;
  // Load the document
  if (FAILED(document->loadXML(const_cast<BSTR>(xml.c_str()), &success)) ||
      success == VARIANT_FALSE) {
    return false;
  }
  // Set the query language and namespace
  VARIANT  var = {0};
  CComBSTR bstr(kNamespace);
  bstr.CopyTo(&var);
  HRESULT hr = document->setProperty(const_cast<BSTR>(kNamespaceProperty), var);
  VariantClear(&var);
  if (FAILED(hr)) return false;

  bstr = kQueryLanguage;  // this frees the old string stored by bstr.
  bstr.CopyTo(&var);
  hr = document->setProperty(const_cast<BSTR>(kQueryLanguageProperty), var);
  VariantClear(&var);
  if (FAILED(hr)) return false;

  // Run the version query
  IXMLDOMNodeList* node_list = NULL;
  long number_of_nodes = 0;  // get_length() needs a ptr to long
  if (FAILED(document->selectNodes(const_cast<BSTR>(kVersionQuery),
                                   &node_list)) || 
      FAILED(node_list->get_length(&number_of_nodes))) {
    return false;
  }
  // We expect 0 or 1 element, depending on whether an update is available.
  // We return early if we get 0 elements (no updates available).
  if (number_of_nodes == 0) return false;
  ASSERT(number_of_nodes == 1);
  IXMLDOMNode* node;
  if (FAILED(node_list->get_item(0, &node))) {
    return false;
  }
  BSTR version_text;
  if (FAILED(node->get_text(&version_text))) {
    return false;
  }
  latest_version_ = version_text;
  // Hurray, done with the version. Now the update link.
  number_of_nodes = 0;
  if (FAILED(document->selectNodes(const_cast<BSTR>(kUpdateLinkQuery),
                                   &node_list)) ||
      FAILED(node_list->get_length(&number_of_nodes))) {
    return false;
  }
  // We expect 0 or 1 element, depending on whether an update is available.
  // We return early if we get 0 elements (no updates available).
  if (number_of_nodes == 0) return false;
  ASSERT(number_of_nodes == 1);
  if (FAILED(node_list->get_item(0, &node))) {
    return false;
  }
  BSTR update_link;
  if (FAILED(node->get_text(&update_link))) {
    return false;
  }
  url_ = update_link;
  return true;
}
