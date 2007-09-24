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

#include <nsIDOMHTMLInputElement.h>
#include <nsILocalFile.h>
#include <nsMemory.h>
#include <nsXPCOM.h>
#include "gears/third_party/gecko_internal/nsIDOMClassInfo.h"
#include "gears/third_party/gecko_internal/nsIFileProtocolHandler.h"
#include "gears/third_party/gecko_internal/nsIFileStreams.h"
#include "gears/third_party/gecko_internal/nsIMIMEService.h"
#include "gears/third_party/gecko_internal/nsIVariant.h"

#include "gears/localserver/firefox/resource_store_ff.h"

#include "gears/base/common/js_runner.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/url_utils.h"
#include "gears/base/firefox/dom_utils.h"
#include "gears/base/firefox/ns_file_utils.h"
#include "gears/localserver/firefox/file_submitter_ff.h"


// Boilerplate. == NS_IMPL_ISUPPORTS + ..._MAP_ENTRY_EXTERNAL_DOM_CLASSINFO
NS_IMPL_THREADSAFE_ADDREF(GearsResourceStore)
NS_IMPL_THREADSAFE_RELEASE(GearsResourceStore)
NS_INTERFACE_MAP_BEGIN(GearsResourceStore)
  NS_INTERFACE_MAP_ENTRY(GearsBaseClassInterface)
  NS_INTERFACE_MAP_ENTRY(GearsResourceStoreInterface)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, GearsResourceStoreInterface)
  NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GearsResourceStore)
NS_INTERFACE_MAP_END

// Object identifiers
const char *kGearsResourceStoreClassName = "GearsResourceStore";
const nsCID kGearsResourceStoreClassId = {0x600bf055, 0x4061, 0x4a77, {0x9e, 0xe2,
                                          0xb0, 0xde, 0x72, 0xc9, 0x46, 0x22}};
                                          // {600BF055-4061-4a77-9EE2-B0DE72C94622}


//-----------------------------------------------------------------------------
// StringBeginsWith - from nsReadableUtils
//-----------------------------------------------------------------------------
static PRBool StringBeginsWith(const nsAString &source,
                               const nsAString &substring) {
  nsAString::size_type src_len = source.Length();
  nsAString::size_type sub_len = substring.Length();
  if (sub_len > src_len)
    return PR_FALSE;
  return Substring(source, 0, sub_len).Equals(substring);
}

//-----------------------------------------------------------------------------
// AppendHeader
//-----------------------------------------------------------------------------
static void AppendHeader(nsCString &headers,
                         const char *name,
                         const char *value) {
  const char *kDelimiter = ": ";
  headers.Append(name);
  headers.Append(kDelimiter);
  headers.Append(value);
  headers.Append(HttpConstants::kCrLfAscii);
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
GearsResourceStore::~GearsResourceStore() {
  AbortAllRequests();
}

//------------------------------------------------------------------------------
// GetName
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::GetName(nsAString &name) {
  name.Assign(store_.GetName());
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetRequiredCookie
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::GetRequiredCookie(nsAString &cookie) {
  cookie.Assign(store_.GetRequiredCookie());
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetEnabled
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::GetEnabled(PRBool *enabled) {
  *enabled = store_.IsEnabled() ? PR_TRUE : PR_FALSE;
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// SetEnabled
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::SetEnabled(PRBool enabled) {
  if (!store_.SetEnabled(enabled ? true : false)) {
    RETURN_EXCEPTION(STRING16(L"Failed to set the enabled property."));
  }
  RETURN_NORMAL();
}

// TODO(cprince): maybe someday change this C++ prototype, commenting out the
// params so JsWrapper layer doesn't even _try_ to convert the array and
// callback params, which we fetch using JsParamFetcher anyway.
//------------------------------------------------------------------------------
// Capture
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::Capture(
    nsIVariant *urls,
    ResourceCaptureCompletionHandler *completion_callback,
    PRInt32 *capture_id_retval) {
  if (!urls) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  *capture_id_retval = next_capture_id_++;

  LOG(("ResourceStore::capture - id = %d\n", *capture_id_retval));

  scoped_ptr<FFCaptureRequest> request(new FFCaptureRequest);
  request->id = *capture_id_retval;

  // store the callback
  // don't use 'completion_callback' directly because this code needs to work
  // in worker threads too
  JsParamFetcher js_params(this);

  const int kIndexCallbackFunction = 1;
  JsRootedCallback *callback;
  if (!js_params.GetAsNewRootedCallback(kIndexCallbackFunction, &callback)) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  request->callback.reset(callback);

  // get the 'urls' param as a raw JS token, so we can tell whether it's
  // a string or array-of-strings.
  std::string16 url;
  JsToken array;
  int array_length;

  if (js_params.GetAsString(0, &url)) {
    // 'urls' was a string
    if (!ResolveAndAppendUrl(url.c_str(), request.get())) {
      RETURN_EXCEPTION(exception_message_.c_str());
    }

  } else if (js_params.GetAsArray(0, &array, &array_length)) {
    // 'urls' was an array of strings

    for (int i = 0; i < array_length; ++i) {
      if (!js_params.GetFromArrayAsString(array, i, &url)) {
        RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
      }

      if (!ResolveAndAppendUrl(url.c_str(), request.get())) {
        RETURN_EXCEPTION(exception_message_.c_str());
      }
    }

  } else {
    // 'urls' was an unsupported type
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  pending_requests_.push_back(request.release());

  if (!StartCaptureTaskIfNeeded(false)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// AbortCapture
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::AbortCapture(PRInt32 capture_id) {
  if (current_request_.get() && (current_request_->id == capture_id)) {
    // The caller is aborting the task that we're running
    assert(capture_task_.get());
    if (capture_task_.get()) {
      capture_task_->Abort();
    }
    RETURN_NORMAL();
  }

  // Search for capture_id in our pending queue
  std::deque<FFCaptureRequest*>::iterator iter;
  for (iter = pending_requests_.begin();
       iter < pending_requests_.end();
       iter++) {
    if ((*iter)->id == capture_id) {
      // Remove it from the queue and fire completion events
      FFCaptureRequest *request = (*iter);
      pending_requests_.erase(iter);
      FireFailedEvents(request);
      delete request;
      RETURN_NORMAL();
      // Note: the deque.erase() call is safe here since we return and
      // do not continue the iteration
    }
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// IsCaptured
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::IsCaptured(const nsAString &url,
                                             PRBool *is_captured_retval) {
  if (url.IsEmpty()) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  nsString url_concrete(url);
  std::string16 full_url;
  if (!ResolveUrl(url_concrete.get(), &full_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }
  *is_captured_retval = store_.IsCaptured(full_url.c_str()) ? PR_TRUE
                                                            : PR_FALSE;
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// Remove
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::Remove(const nsAString &url) {
  if (url.IsEmpty()) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  nsString url_concrete(url);
  std::string16 full_url;
  if (!ResolveUrl(url_concrete.get(), &full_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }

  if (!store_.Delete(full_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Failure removing url."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// Rename
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::Rename(const nsAString &src_url,
                                         const nsAString &dst_url) {
  if (src_url.IsEmpty() || dst_url.IsEmpty()) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  nsString src_url_concrete(src_url);
  std::string16 full_src_url;
  if (!ResolveUrl(src_url_concrete.get(), &full_src_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }
  nsString dst_url_concrete(dst_url);
  std::string16 full_dest_url;
  if (!ResolveUrl(dst_url_concrete.get(), &full_dest_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }

  if (!store_.Rename(full_src_url.c_str(), full_dest_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Failure renaming url."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// Copy
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::Copy(const nsAString &src_url,
                                       const nsAString &dst_url) {
  if (src_url.IsEmpty() || dst_url.IsEmpty()) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  nsString src_url_concrete(src_url);
  std::string16 full_src_url;
  if (!ResolveUrl(src_url_concrete.get(), &full_src_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }
  nsString dst_url_concrete(dst_url);
  std::string16 full_dest_url;
  if (!ResolveUrl(dst_url_concrete.get(), &full_dest_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }

  if (!store_.Copy(full_src_url.c_str(), full_dest_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Failure copying url."));
  }
  RETURN_NORMAL();
}


//------------------------------------------------------------------------------
// CaptureFile
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::CaptureFile(nsISupports *file_input_element,
                                              const nsAString &url_astring) {
  if (EnvIsWorker()) {
    RETURN_EXCEPTION(STRING16(L"captureFile cannot be called in a worker."));
  }

  // Get the url param as a std::string16 instead
  JsParamFetcher js_params(this);
  std::string16 url;
  if (!js_params.GetAsString(1, &url)) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  // Verify the input element is actually is what we're expecting.
  // <input type=file>
  nsCOMPtr<nsIDOMHTMLInputElement> input;
  nsresult nr = DOMUtils::VerifyAndGetFileInputElement(file_input_element,
                                                       getter_AddRefs(input));
  if (NS_FAILED(nr) || !input) {
    RETURN_EXCEPTION(STRING16(L"Invalid file input parameter."));
  }

  // Get the filepath out of the input element
  nsString filepath;
  nr = input->GetValue(filepath);
  if (NS_FAILED(nr) || filepath.IsEmpty()) {
    RETURN_EXCEPTION(STRING16(L"File path is empty."));
  }

  // Get the full normalized url
  std::string16 full_url;
  if (!ResolveUrl(url.c_str(), &full_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }

  // Capture that filepath
  nr = CaptureFile(filepath, full_url.c_str());
  if (NS_FAILED(nr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to capture the file."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// CaptureFile
//------------------------------------------------------------------------------
nsresult GearsResourceStore::CaptureFile(const nsAString &filepath,
                                         const char16 *full_url) {
  nsresult nr = NS_OK;

  // Create an nsIFile given the file path
  nsCOMPtr<nsIFile> file;

  // If its really a file url, handle it differently. Gecko handles file
  // input elements in this way when submitting forms.
  if (StringBeginsWith(filepath, NS_LITERAL_STRING("file:"))) {
    // Converts the URL string into the corresponding nsIFile if possible.
    NSFileUtils::GetFileFromURLSpec(filepath, getter_AddRefs(file));
  }

  if (!file) {
    // This is no file url, try as a local file path
    nsCOMPtr<nsILocalFile> localFile;
    nr = NS_NewLocalFile(filepath, PR_FALSE, getter_AddRefs(localFile));
    NS_ENSURE_SUCCESS(nr, nr);
    file = localFile;
  }

  // Capture that nsIFile
  return CaptureFile(file, full_url);
}

//------------------------------------------------------------------------------
// CaptureFile
//------------------------------------------------------------------------------
nsresult GearsResourceStore::CaptureFile(nsIFile *file,
                                         const char16* full_url) {
  // Make sure the file exists and is readable.
  PRBool readable;
  nsresult nr = file->IsReadable(&readable);
  NS_ENSURE_SUCCESS(nr, nr);
  NS_ENSURE_TRUE(readable, NS_ERROR_FILE_ACCESS_DENIED);

  // Get the name and size of the file
  nsString filename;
  nr = file->GetLeafName(filename);
  NS_ENSURE_SUCCESS(nr, nr);
  PRInt64 file_size = 0;
  nr = file->GetFileSize(&file_size);
  NS_ENSURE_SUCCESS(nr, nr);
  NS_ENSURE_TRUE(file_size >= 0, NS_ERROR_FAILURE);

  // Prepare an item for insertion into the store
  ResourceStore::Item item;
  item.entry.url = full_url;
  item.payload.status_code = HttpConstants::HTTP_OK;
  item.payload.status_line = HttpConstants::kOKStatusLine;

  // We don't support very large files yet
  // TODO(michaeln): support large files
  NS_ENSURE_TRUE(file_size <= PR_INT32_MAX, NS_ERROR_OUT_OF_MEMORY);
  PRInt32 data_len = static_cast<PRInt32>(file_size);

  // Get the file input stream
  nsCOMPtr<nsIInputStream> stream;
  nr = NSFileUtils::NewLocalFileInputStream(getter_AddRefs(stream),
      file, -1, -1, nsIFileInputStream::CLOSE_ON_EOF);
  NS_ENSURE_SUCCESS(nr, nr);
  NS_ENSURE_TRUE(stream, NS_ERROR_FAILURE);

  // Read the file data into memory
  item.payload.data.reset(new std::vector<uint8>);
  if (data_len > 0) {
    item.payload.data->resize(data_len);
    NS_ENSURE_TRUE(
        item.payload.data->size() == static_cast<PRUint32>(data_len),
        NS_ERROR_OUT_OF_MEMORY);
    PRUint32 data_actually_read = 0;
    nr = stream->Read(reinterpret_cast<char*>(&(item.payload.data->at(0))),
                      data_len, &data_actually_read);
    NS_ENSURE_SUCCESS(nr, nr);
    NS_ENSURE_TRUE(static_cast<PRUint32>(data_len) == data_actually_read,
                   NS_ERROR_FAILURE);
  }

  // Get the mime type
  nsCOMPtr<nsIMIMEService> mime_service = do_GetService("@mozilla.org/mime;1");
  NS_ENSURE_STATE(mime_service);
  nsCString mime_type;
  nr = mime_service->GetTypeFromFile(file, mime_type);
  if (NS_FAILED(nr)) {
    mime_type = "application/octet-stream";
  }

  nsCString native_filename;
  nr = file->GetNativeLeafName(native_filename);
  NS_ENSURE_SUCCESS(nr, nr);

  // Synthesize the http headers we'll store with this item
  const char *kContentLengthHeader = "Content-Length";
  const char *kContentTypeHeader = "Content-Type";
  const char *kXCapturedFilenameHeader = "X-Captured-Filename";
  nsCString headers;
  std::string data_len_str;
  IntegerToString(data_len, &data_len_str);
  AppendHeader(headers, kContentLengthHeader, data_len_str.c_str());
  AppendHeader(headers, kContentTypeHeader, mime_type.get());
  AppendHeader(headers, kXCapturedFilenameHeader, native_filename.get());
  // TODO(michaeln): provide a flag on scriptable captureFile() method
  // that controls the addition of this attribute
  //const char *kContentDispositionHeader = "Content-Disposition";
  //const char *kContentDispositionValuePrefix = "attachment" "; filename=\"";
  //const char *kContentDispositionValueSuffix = "\"";
  //AppendHeader(headers,
  //             kContentDispositionHeader,
  //             kContentDispositionValuePrefix);
  //headers.SetLength(headers.Length() - 2);  // remove trailing CRLF
  //headers.Append(native_filename);
  //headers.Append(kContentDispositionValueSuffix);
  //headers.Append(HttpConstants::kCrLfAscii);
  headers.Append(HttpConstants::kCrLfAscii);  // Terminiate with a blank line
  if (!UTF8ToString16(headers.get(), headers.Length(), &item.payload.headers)) {
    return NS_ERROR_FAILURE;
  }

  // Put data into our store
  if (!store_.PutItem(&item)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// GetCapturedFileName
//------------------------------------------------------------------------------
NS_IMETHODIMP
GearsResourceStore::GetCapturedFileName(const nsAString &url,
                                        nsAString &file_name_retval) {
  if (url.IsEmpty()) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  nsString url_concrete(url);
  std::string16 full_url;
  if (!ResolveUrl(url_concrete.get(), &full_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }

  std::string16 file_name;
  if (!store_.GetCapturedFileName(full_url.c_str(), &file_name)) {
    RETURN_EXCEPTION(STRING16(L"GetCapturedFileName failed."));
  }

  file_name_retval.Assign(file_name.c_str());
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetHeader
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::GetHeader(const nsAString &url,
                                            const nsAString &header_in,
                                            nsAString &retval) {
  if (url.IsEmpty() || header_in.IsEmpty()) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  nsString url_concrete(url);
  std::string16 full_url;
  if (!ResolveUrl(url_concrete.get(), &full_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }

  nsString header(header_in);
  std::string16 value;
  if (store_.GetHeader(full_url.c_str(), header.get(), &value)) {
    retval.Assign(value.c_str());
  } else {
    retval.SetLength(0);
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// GetAllHeaders
//------------------------------------------------------------------------------
NS_IMETHODIMP GearsResourceStore::GetAllHeaders(const nsAString &url,
                                                nsAString &retval) {
  if (url.IsEmpty()) {
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  nsString url_concrete(url);
  std::string16 full_url;
  if (!ResolveUrl(url_concrete.get(), &full_url)) {
    RETURN_EXCEPTION(exception_message_.c_str());
  }

  std::string16 all_headers;
  if (!store_.GetAllHeaders(full_url.c_str(), &all_headers)) {
    RETURN_EXCEPTION(STRING16(L"GetAllHeaders failed."));
  }

  retval.Assign(all_headers.c_str());
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// CreateFileSubmitter
//------------------------------------------------------------------------------
NS_IMETHODIMP
GearsResourceStore::CreateFileSubmitter(GearsFileSubmitterInterface **retval) {
  if (EnvIsWorker()) {
    RETURN_EXCEPTION(
        STRING16(L"createFileSubmitter cannot be called in a worker."));
  }
  nsCOMPtr<GearsFileSubmitter> submitter(new GearsFileSubmitter());
  if (!submitter->InitBaseFromSibling(this) ||
      !submitter->store_.Clone(&store_)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize FileSubmitter."));
  }

  NS_ADDREF(*retval = submitter);
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// StartCaptureTaskIfNeeded
//------------------------------------------------------------------------------
bool
GearsResourceStore::StartCaptureTaskIfNeeded(bool fire_events_on_failure) {
  if (page_is_unloaded_) {
    // We silently fail for this particular error condition to prevent callers
    // from detecting errors and making noises after the page has been unloaded
    return true;
  }

  // Create an event monitor to alert us when the page unloads.
  if (unload_monitor_ == NULL) {
    unload_monitor_.reset(new JsEventMonitor(GetJsRunner(), JSEVENT_UNLOAD,
                                             this));
  }

  if (capture_task_.get()) {
    assert(current_request_.get());
    return true;
  }

  if (pending_requests_.empty()) {
    return true;
  }

  assert(!current_request_.get());
  current_request_.reset(pending_requests_.front());
  pending_requests_.pop_front();

  capture_task_.reset(new CaptureTask());
  if (!capture_task_->Init(&store_, current_request_.get())) {
    scoped_ptr<FFCaptureRequest> failed_request(current_request_.release());
    capture_task_.reset(NULL);
    if (fire_events_on_failure) {
      FireFailedEvents(failed_request.get());
    }
    exception_message_ = STRING16(L"Failed to initialize capture task.");
    return false;
  }

  capture_task_->SetListener(this);
  if (!capture_task_->Start()) {
    scoped_ptr<FFCaptureRequest> failed_request(current_request_.release());
    capture_task_.reset(NULL);
    if (fire_events_on_failure) {
      FireFailedEvents(failed_request.get());
    }
    exception_message_ = STRING16(L"Failed to start capture task.");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
// HandleEvent
//------------------------------------------------------------------------------
void GearsResourceStore::HandleEvent(int code, int param,
                                     AsyncTask *source) {
  CaptureTask *task = reinterpret_cast<CaptureTask*>(source);
  if (task && (task == capture_task_.get())) {
    if (code == CaptureTask::CAPTURE_TASK_COMPLETE) {
      OnCaptureTaskComplete();
    } else {
      // param = the index of the url that has been processed
      bool success = (code == CaptureTask::CAPTURE_URL_SUCCEEDED);
      OnCaptureUrlComplete(param, success);
    }
  }
}

//------------------------------------------------------------------------------
// OnCaptureUrlComplete
//------------------------------------------------------------------------------
void GearsResourceStore::OnCaptureUrlComplete(int index, bool success) {
  if (current_request_.get()) {
    InvokeCompletionCallback(current_request_.get(),
                             current_request_->urls[index],
                             current_request_->id,
                             success);
  }
}

//------------------------------------------------------------------------------
// OnCaptureTaskComplete
//------------------------------------------------------------------------------
void GearsResourceStore::OnCaptureTaskComplete() {
  capture_task_->SetListener(NULL);
  capture_task_.release()->DeleteWhenDone();
  current_request_.reset(NULL);
  StartCaptureTaskIfNeeded(true);
}

//------------------------------------------------------------------------------
// HandleEvent
//------------------------------------------------------------------------------
void GearsResourceStore::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);

  page_is_unloaded_ = true;
  AbortAllRequests();
}

//------------------------------------------------------------------------------
// AbortAllRequests
//------------------------------------------------------------------------------
void GearsResourceStore::AbortAllRequests() {
  if (capture_task_.get()) {
    capture_task_->SetListener(NULL);
    capture_task_->Abort();
    capture_task_.release()->DeleteWhenDone();
  }

  std::deque<FFCaptureRequest*>::iterator iter;
  for (iter = pending_requests_.begin();
       iter < pending_requests_.end();
       ++iter) {
    delete (*iter);
  }
  pending_requests_.clear();
}

//------------------------------------------------------------------------------
// FireFailedEvents
//------------------------------------------------------------------------------
void GearsResourceStore::FireFailedEvents(FFCaptureRequest *request) {
  assert(request);
  for (size_t i = 0; i < request->urls.size(); ++i) {
    InvokeCompletionCallback(request,
                             request->urls[i],
                             request->id,
                             false);
  }
}

//------------------------------------------------------------------------------
// InvokeCompletionCallback
//------------------------------------------------------------------------------
void GearsResourceStore::InvokeCompletionCallback(
                             FFCaptureRequest *request,
                             const std::string16 &capture_url,
                             int capture_id,
                             bool succeeded) {
  // If completion callback was not set, return immediately
  if (!request->callback.get()) { return; }

  const int argc = 3;
  JsParamToSend argv[argc] = {
    { JSPARAM_STRING16, &capture_url },
    { JSPARAM_BOOL, &succeeded },
    { JSPARAM_INT, &capture_id }
  };
  GetJsRunner()->InvokeCallback(request->callback.get(), argc, argv, NULL);
}

//------------------------------------------------------------------------------
// ResolveAndAppendUrl
//------------------------------------------------------------------------------
bool GearsResourceStore::ResolveAndAppendUrl(const std::string16 &url,
                                             FFCaptureRequest *request) {
  std::string16 full_url;
  if (!ResolveUrl(url.c_str(), &full_url)) {
    return false;
  }
  request->urls.push_back(url);
  request->full_urls.push_back(full_url);
  return true;
}

//------------------------------------------------------------------------------
// This helper does several things:
// - resolve relative urls based on the page location, the 'url' may also
//   be an absolute url to start with, if so this step does not modify it
// - normalizes the resulting absolute url, ie. removes path navigation
// - removes the fragment part of the url, ie. truncates at the '#' character
// - ensures the the resulting url is from the same-origin
// - rejects the URL to our canned scour.js file as input
//------------------------------------------------------------------------------
bool GearsResourceStore::ResolveUrl(const char16 *url,
                                    std::string16 *resolved_url) {
  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url, resolved_url)) {
    exception_message_ = STRING16(L"Failed to resolve url.");
    return false;
  }
  if (!EnvPageSecurityOrigin().IsSameOriginAsUrl(resolved_url->c_str())) {
    exception_message_ = STRING16(L"Url is not from the same origin");
    return false;
  }
  return true;
}
