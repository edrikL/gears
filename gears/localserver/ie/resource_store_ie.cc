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
#include <dispex.h>
#ifdef WINCE
#include <webvw.h>  // For IPIEHTMLInputTextElement
#endif

#include <deque>
#include <vector>

#include "gears/localserver/ie/resource_store_ie.h"

#include "gears/base/common/common.h"
#include "gears/base/common/file.h"
#include "gears/base/common/scoped_win32_handles.h"
#include "gears/base/common/security_model.h"
#include "gears/base/common/string16.h"
#include "gears/base/common/url_utils.h"
#ifdef WINCE
#include "gears/base/common/wince_compatibility.h"
#endif
#include "gears/base/ie/activex_utils.h"
#include "gears/base/ie/atl_headers.h"
#ifdef WINCE
// FileSubmitter is not implemented for WinCE.
#else
#include "gears/localserver/ie/file_submitter_ie.h"
#endif

//------------------------------------------------------------------------------
// FinalConstruct
//------------------------------------------------------------------------------
HRESULT GearsResourceStore::FinalConstruct() {
  LOG16((L"ResourceStore::FinalConstruct\n"));
  next_capture_id_ = 0;
  return S_OK;
}


//------------------------------------------------------------------------------
// FinalRelease
//------------------------------------------------------------------------------
void GearsResourceStore::FinalRelease() {
  LOG16((L"ResourceStore::FinalRelease\n"));

  if (capture_task_.get()) {
    capture_task_->SetListenerWindow(NULL, 0);
    capture_task_->Abort();
    capture_task_.release()->DeleteWhenDone();
  }

  std::deque<IECaptureRequest*>::iterator iter;
  for (iter = pending_requests_.begin();
       iter < pending_requests_.end();
       iter++) {
    delete (*iter);
  }
  pending_requests_.clear();

  if (IsWindow()) {
    DestroyWindow();
  }
}

//------------------------------------------------------------------------------
// get_name
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::get_name(BSTR *name) {
  CComBSTR bstr(store_.GetName());
  *name = bstr.Detach();
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// get_requiredCookie
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::get_requiredCookie(BSTR *cookie) {
  CComBSTR bstr(store_.GetRequiredCookie());
  *cookie = bstr.Detach();
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// get_enabled
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::get_enabled(VARIANT_BOOL *enabled) {
  *enabled = store_.IsEnabled() ? VARIANT_TRUE : VARIANT_FALSE;
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// put_enabled
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::put_enabled(VARIANT_BOOL enabled) {
  if (!store_.SetEnabled(enabled ? true : false)) {
    RETURN_EXCEPTION(STRING16(L"Failed to set the enabled property."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// capture
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::capture(
      /* [in] */ const VARIANT *urls,
      /* [in] */ VARIANT *completion_callback,
      /* [retval][out] */ long *capture_id) {
  if (!urls || !capture_id) {
    assert(urls);
    assert(capture_id);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  IDispatch *callback_dispatch = NULL;
  if (!ActiveXUtils::VariantIsNullOrUndefined(completion_callback)) {
    if (completion_callback->vt == VT_DISPATCH) {
      callback_dispatch = completion_callback->pdispVal;
    } else {
      RETURN_EXCEPTION(STRING16(L"Invalid callback parameter."));
    }
  }

  *capture_id = next_capture_id_++;

  LOG16((L"ResourceStore::capture - id = %d\n", *capture_id));

  scoped_ptr<IECaptureRequest> request(new IECaptureRequest);
  request->id = *capture_id;
  request->completion_callback = callback_dispatch;

  HRESULT hr;

  if (urls->vt == VT_BSTR) {
    hr = ResolveAndAppendUrl(urls->bstrVal, request.get());
    if (FAILED(hr)) {
      return hr;
    }
  } else {
    JsArray url_array;
    int count;

    if (!url_array.SetArray(*urls, NULL) ||
        !url_array.GetLength(&count)) {
      RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
    }

    // Resolve each url and add it to the request
    for (int i = 0; i < count; ++i) {
      std::string16 url;
      if (!url_array.GetElementAsString(i, &url)) {
        RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
      }
      hr = ResolveAndAppendUrl(url.c_str(), request.get());
      if (FAILED(hr)) {
        return hr;
      }
    }
  }

  pending_requests_.push_back(request.release());

  hr = StartCaptureTaskIfNeeded(false);
  if (FAILED(hr)) {
    return hr;
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// ResolveAndAppendUrl
//------------------------------------------------------------------------------
HRESULT GearsResourceStore::ResolveAndAppendUrl(const char16 *url,
                                                IECaptureRequest *request) {
    if (!url || !url[0]) {
      RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
    }

    LOG16((L"  %s\n", url));

    std::string16 full_url;
    HRESULT hr = ResolveUrl(url, &full_url);
    if (FAILED(hr)) {
      return hr;
    }
    request->urls.push_back(std::string16());
    request->urls.back() = url;
    request->full_urls.push_back(full_url);
    RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// This helper does several things:
// - resolve relative urls based on the page location, the 'url' may also
//   be an absolute url to start with, if so this step does not modify it
// - normalizes the resulting absolute url, ie. removes path navigation
// - removes the fragment part of the url, ie. truncates at the '#' character
// - ensures the the resulting url is from the same-origin
//------------------------------------------------------------------------------
HRESULT GearsResourceStore::ResolveUrl(const char16 *url,
                                       std::string16 *resolved_url) {
  if (!ResolveAndNormalize(EnvPageLocationUrl().c_str(), url, resolved_url)) {
    RETURN_EXCEPTION(STRING16(L"Failed to resolve url."));
  }
  if (!EnvPageSecurityOrigin().IsSameOriginAsUrl(resolved_url->c_str())) {
    RETURN_EXCEPTION(STRING16(L"Url is not from the same origin"));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// StartCaptureTaskIfNeeded
//------------------------------------------------------------------------------
HRESULT
GearsResourceStore::StartCaptureTaskIfNeeded(bool fire_events_on_failure) {
  if (capture_task_.get()) {
    assert(current_request_.get());
    RETURN_NORMAL();
  }

  if (pending_requests_.empty()) {
    RETURN_NORMAL();
  }

  assert(!current_request_.get());
  current_request_.reset(pending_requests_.front());
  pending_requests_.pop_front();

  capture_task_.reset(new CaptureTask());
  if (!capture_task_->Init(&store_, current_request_.get())) {
    scoped_ptr<IECaptureRequest> failed_request(current_request_.release());
    capture_task_.reset(NULL);
    if (fire_events_on_failure) {
      FireFailedEvents(failed_request.get());
    }
    RETURN_EXCEPTION(STRING16(L"Failed to initialize the capture task."));
  }

  HRESULT hr = CreateWindowIfNeeded();
  if (FAILED(hr)) {
    scoped_ptr<IECaptureRequest> failed_request(current_request_.release());
    capture_task_.reset(NULL);
    if (fire_events_on_failure) {
      FireFailedEvents(failed_request.get());
    }
    RETURN_EXCEPTION(STRING16(L"Failed to create message window."));
  }

  capture_task_->SetListenerWindow(m_hWnd, kCaptureTaskMessageBase);
  if (!capture_task_->Start()) {
    scoped_ptr<IECaptureRequest> failed_request(current_request_.release());
    capture_task_.reset(NULL);
    if (fire_events_on_failure) {
      FireFailedEvents(failed_request.get());
    }
    RETURN_EXCEPTION(STRING16(L"Failed to start the capture task."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// OnCaptureUrlComplete
//------------------------------------------------------------------------------
LRESULT GearsResourceStore::OnCaptureUrlComplete(UINT uMsg,
                                                 WPARAM wParam,
                                                 LPARAM lParam,
                                                 BOOL& bHandled) {
  CaptureTask* task = reinterpret_cast<CaptureTask*>(lParam);
  if (task && (task == capture_task_.get())) {
    if (current_request_.get()) {
      int index = wParam;
      bool success = (uMsg == WM_CAPTURE_URL_SUCCEEDED);
      LOG16((L"ResourceStore::OnCaptureUrlComplete( %s, %s %d)\n",
             current_request_->urls[index].c_str(),
             success ? L"SUCCESS" : L"FAILED",
             current_request_->id));
      FireEvent(current_request_->completion_callback,
                current_request_->urls[index].c_str(),
                success, current_request_->id);
    }
  }
  bHandled = TRUE;
  return 0;
}

//------------------------------------------------------------------------------
// OnCaptureTaskComplete
//------------------------------------------------------------------------------
LRESULT GearsResourceStore::OnCaptureTaskComplete(UINT uMsg,
                                                  WPARAM wParam,
                                                  LPARAM lParam,
                                                  BOOL& bHandled) {
  CaptureTask* task = reinterpret_cast<CaptureTask*>(lParam);
  if (task && (task == capture_task_.get())) {
    capture_task_->SetListenerWindow(NULL, 0);
    capture_task_.release()->DeleteWhenDone();
    current_request_.reset(NULL);
  }
  StartCaptureTaskIfNeeded(true);
  bHandled = TRUE;
  return 0;
}

//------------------------------------------------------------------------------
// abortCapture
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::abortCapture(
      /* [in] */ long capture_id)  {
  LOG16((L"ResourceStore::abortCapture( %d )\n", capture_id));

  if (current_request_.get() && (current_request_->id == capture_id)) {
    // The caller is aborting the task that we're running
    assert(capture_task_.get());
    if (capture_task_.get()) {
      capture_task_->Abort();
    }
    RETURN_NORMAL();
  }

  // Search for capture_id in our pending queue
  std::deque<IECaptureRequest*>::iterator iter;
  for (iter = pending_requests_.begin();
       iter < pending_requests_.end();
       iter++) {
    if ((*iter)->id == capture_id) {
      // Remove it from the queue and fire completion events
      IECaptureRequest* request = (*iter);
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
// isCaptured
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::isCaptured(
      /* [in] */ const BSTR url,
      /* [retval][out] */ VARIANT_BOOL *retval) {
  if (!url || !url[0] || !retval) {
    assert(url && url[0]);
    assert(retval);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  std::string16 full_url;
  HRESULT hr = ResolveUrl(url, &full_url);
  if (FAILED(hr)) {
    return hr;
  }
  *retval = store_.IsCaptured(full_url.c_str()) ? VARIANT_TRUE : VARIANT_FALSE;
  LOG16((L"ResourceStore::isCaptured( %s ) = %s\n",
         url, *retval ? L"TRUE" : L"FALSE"));
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// remove
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::remove(
      /* [in] */ const BSTR url) {
  if (!url || !url[0]) {
    assert(url && url[0]);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }
  std::string16 full_url;
  HRESULT hr = ResolveUrl(url, &full_url);
  if (FAILED(hr)) {
    return hr;
  }
  if (!store_.Delete(full_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Failure removing url."));
  }

  LOG16((L"ResourceStore::remove( %s )\n", url));

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// rename
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::rename(
      /* [in] */ const BSTR src_url,
      /* [in] */ const BSTR dst_url) {
  if (!src_url || !src_url[0] || !dst_url || !dst_url[0]) {
    assert(src_url && src_url[0]);
    assert(dst_url && dst_url[0]);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  LOG16((L"ResourceStore::rename( %s, %s )\n", src_url, dst_url));

  std::string16 full_src_url;
  HRESULT hr = ResolveUrl(src_url, &full_src_url);
  if (FAILED(hr)) {
    return hr;
  }
  std::string16 full_dest_url;
  hr = ResolveUrl(dst_url, &full_dest_url);
  if (FAILED(hr)) {
    return hr;
  }

  if (!store_.Rename(full_src_url.c_str(), full_dest_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Failure renaming url."));
  }

  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// copy
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::copy(
      /* [in] */ const BSTR src_url,
      /* [in] */ const BSTR dst_url) {
  if (!src_url || !src_url[0] || !dst_url || !dst_url[0]) {
    assert(src_url && src_url[0]);
    assert(dst_url && dst_url[0]);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  LOG16((L"ResourceStore::copy( %s, %s )\n", src_url, dst_url));

  std::string16 full_src_url;
  HRESULT hr = ResolveUrl(src_url, &full_src_url);
  if (FAILED(hr)) {
    return hr;
  }
  std::string16 full_dest_url;
  hr = ResolveUrl(dst_url, &full_dest_url);
  if (FAILED(hr)) {
    return hr;
  }

  if (!store_.Copy(full_src_url.c_str(), full_dest_url.c_str())) {
    RETURN_EXCEPTION(STRING16(L"Failure copying url."));
  }
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// captureFile
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::captureFile(
      /* [in] */ IDispatch *file_input_element,
      /* [in] */ const BSTR url) {
  if (EnvIsWorker()) {
    RETURN_EXCEPTION(STRING16(L"captureFile cannot be called in a worker."));
  }

  if (!url || !url[0] || !file_input_element) {
    assert(url && url[0]);
    assert(file_input_element);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  // Verify that file_input_element is actually a form of type
  // <input type=file>
#ifdef WINCE
  // If it implements the IPIEHTMLInputTextElement interface, and has type
  // 'file', then accept it.
  CComQIPtr<IPIEHTMLInputTextElement> input(file_input_element);
  CComBSTR type;
  if (FAILED(input->get_type(&type)) || type != L"file") {
    input.Release();
  }
#else
  // If it implements the IHTMLInputFileElemement interface, then accept it.
  CComQIPtr<IHTMLInputFileElement> input(file_input_element);
#endif
  if (!input) {
    RETURN_EXCEPTION(STRING16(L"Invalid file input parameter."));
  }

  // Get the filepath out of the <input type=file> element.
  CComBSTR filepath;
  HRESULT hr = input->get_value(&filepath);
  if (FAILED(hr) || filepath.Length() <= 0) {
    RETURN_EXCEPTION(STRING16(L"File path is empty."));
  }

  LOG16((L"ResourceStore::captureFile( %s, %s )\n", filepath.m_str, url));

  // Resolve the url this file is to be registered under.
  std::string16 full_url;
  hr = ResolveUrl(url, &full_url);
  if (FAILED(hr)) {
    return hr;
  }
  ResourceStore::Item item;
  item.entry.url = full_url;
  item.payload.status_code = HttpConstants::HTTP_OK;
  item.payload.status_line = HttpConstants::kOKStatusLine;

  // Read the contents of the file into memory.
  item.payload.data.reset(new std::vector<uint8>);
  if (!File::ReadFileToVector(filepath.m_str, item.payload.data.get())) {
    RETURN_EXCEPTION(STRING16(L"Failed to read the file."));
  }

  // Get the mimetype for the file.
  const WCHAR *kOctetStreamType = L"application/octet-stream";
  WCHAR *mimetype = NULL;
  DWORD length = static_cast< DWORD >(item.payload.data->size());
  void* data = length ? &item.payload.data->at(0) : NULL;
  hr = FindMimeFromData(NULL, PathFindExtensionW(filepath), data, length,
                        kOctetStreamType, 0, &mimetype, 0);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"FindMimeFromData failed."));
  }

  // Synthesize the headers to save with the item
  CStringW headers;
  const WCHAR *filename = PathFindFileNameW(filepath.m_str);
  headers.AppendFormat(L"%s:%s\r\n",
                       HttpConstants::kContentTypeHeader,
                       mimetype ? mimetype : kOctetStreamType);
  headers.AppendFormat(L"%s:%d\r\n",
                       HttpConstants::kContentLengthHeader,
                       item.payload.data->size());
  headers.AppendFormat(L"%s:%s\r\n",
                       HttpConstants::kXCapturedFilenameHeader,
                       filename);
  // TODO(michaeln): provide a flag on scriptable captureFile() method
  // that controls the addition of this attribute
  //headers.AppendFormat(L"%s:attachment; filename=\"%s\"\r\n",
  //                     HttpConstants::kContentDispositionHeader,
  //                     filename);
  headers.Append(HttpConstants::kCrLf);
  item.payload.headers = headers;

  CoTaskMemFree(mimetype);

  // Persist the file contents and meta info into the store
  if (!store_.PutItem(&item)) {
    RETURN_EXCEPTION(STRING16(L"PutItem failed."));
  }

  RETURN_NORMAL();
}


//------------------------------------------------------------------------------
// getCapturedFileName
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::getCapturedFileName(
      /* [in] */ const BSTR url,
      /* [retval][out] */ BSTR *file_name) {
  if (!url || !url[0] || !file_name) {
    assert(url && url[0]);
    assert(file_name);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  std::string16 full_url;
  HRESULT hr = ResolveUrl(url, &full_url);
  if (FAILED(hr)) {
    return hr;
  }

  std::string16 file_name_str;
  if (!store_.GetCapturedFileName(full_url.c_str(), &file_name_str)) {
    RETURN_EXCEPTION(STRING16(L"GetCapturedFileName failed."));
  }

  CComBSTR file_name_bstr(file_name_str.c_str());
  *file_name = file_name_bstr.Detach();
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// getHeader
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::getHeader(
      /* [in] */ const BSTR url,
      /* [in] */ const BSTR header,
      /* [retval][out] */ BSTR *value) {
  if (!url || !url[0] || !header || !header[0] || !value) {
    assert(url && url[0]);
    assert(header && header[0]);
    assert(value);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  std::string16 full_url;
  HRESULT hr = ResolveUrl(url, &full_url);
  if (FAILED(hr)) {
    return hr;
  }

  CComBSTR value_bstr;
  std::string16 value_str;
  if (store_.GetHeader(full_url.c_str(), header, &value_str)) {
    value_bstr = value_str.c_str();
  }

  *value = value_bstr.Detach();
  RETURN_NORMAL();
}

//------------------------------------------------------------------------------
// getAllHeaders
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::getAllHeaders(
      /* [in] */ const BSTR url,
      /* [retval][out] */ BSTR *all_headers) {
  if (!url || !url[0] || !all_headers) {
    assert(url && url[0]);
    assert(all_headers);
    RETURN_EXCEPTION(STRING16(L"Invalid parameter."));
  }

  std::string16 full_url;
  HRESULT hr = ResolveUrl(url, &full_url);
  if (FAILED(hr)) {
    return hr;
  }

  std::string16 all_headers_str;
  if (!store_.GetAllHeaders(full_url.c_str(), &all_headers_str)) {
    RETURN_EXCEPTION(STRING16(L"GetAllHeaders failed."));
  }

  CComBSTR all_headers_bstr(all_headers_str.c_str());
  *all_headers = all_headers_bstr.Detach();
  RETURN_NORMAL();
}

#ifdef WINCE
// FileSubmitter is not implemented for WinCE.
#else
//------------------------------------------------------------------------------
// createFileSubmitter
//------------------------------------------------------------------------------
STDMETHODIMP GearsResourceStore::createFileSubmitter(
      /* [retval][out] */ GearsFileSubmitterInterface **file_submitter) {
  if (EnvIsWorker()) {
    RETURN_EXCEPTION(
        STRING16(L"createFileSubmitter cannot be called in a worker."));
  }
  CComObject<GearsFileSubmitter> *submitter = NULL;
  HRESULT hr = CComObject<GearsFileSubmitter>::CreateInstance(&submitter);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to CreateInstance."));
  }

  CComPtr< CComObject<GearsFileSubmitter> > reference_adder(submitter);

  if (!submitter->InitBaseFromSibling(this)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize base class."));
  }

  if (!submitter->store_.Clone(&store_)) {
    RETURN_EXCEPTION(STRING16(L"Failed to initialize GearsFileSubmitter."));
  }

  hr = submitter->QueryInterface(file_submitter);
  if (FAILED(hr)) {
    RETURN_EXCEPTION(STRING16(L"Failed to QueryInterface for"
                              L" GearsFileSubmitterInterface."));
  }

  RETURN_NORMAL();
}
#endif

//------------------------------------------------------------------------------
// CreateWindowIfNeeded
//------------------------------------------------------------------------------
HRESULT GearsResourceStore::CreateWindowIfNeeded() {
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
// FireFailedEvents
//------------------------------------------------------------------------------
void GearsResourceStore::FireFailedEvents(IECaptureRequest* request) {
  if (request && request->completion_callback) {
    for (size_t i = 0; i < request->urls.size(); ++i) {
      FireEvent(request->completion_callback,
                request->urls[i].c_str(),
                false,
                request->id);
    }
  }
}

//------------------------------------------------------------------------------
// FireEvent
//------------------------------------------------------------------------------
void GearsResourceStore::FireEvent(IDispatch *handler,
                                   const char16 *url,
                                   bool success,
                                   int id) {
  // TODO(michaeln): Convert this function to use JsRunner::InvokeCallback(),
  // just like the Firefox version (and most Gears code) does.  See if the
  // problems described below go away when using InvokeCallback().

  if (!handler) {
    return;
  }

  // IE gives a dispinterface JScriptTypeInfo as our
  // onxxxxx property values.
  //  classid: C59C6B12-F6C1-11CF-8835-00A0C911E8B2
  // see: http://groups.google.com/group/
  //  microsoft.public.vc.activex.templatelib/browse_thread/9a98afa0c0db7579/
  //  3d90ad264227e793?lnk=st&q=C59C6B12-F6C1-11CF-8835-00A0C911E8B2
  //  &rnum=1#3d90ad264227e793
  // see: http://www.microsoft.com/mind/1099/dynamicobject/dynamicobject.asp
  //
  // We invoke "dispatch id 0" to call the associated function

  const DISPID kDispId0 = 0;

  HRESULT hr = 0;
  DISPPARAMS dispparams = {0};
  CComBSTR url_bstr(url);
  CComQIPtr<IDispatchEx> dispatchex = handler;

  if (dispatchex) {
    // We prefer to call things thru IDispatchEx in order to use DISPID_THIS
    // Note: strangely, the parameters passed thru IDispatch are in reverse
    // order as compared to the method signature being invoked
    DISPID disp_this = DISPID_THIS;
    VARIANT var[4];
    var[0].vt = VT_DISPATCH;
    var[0].pdispVal = dispatchex;
    var[1].vt = VT_I4;
    var[1].intVal = id;
    var[2].vt = VT_BOOL;
    var[2].boolVal = success ? VARIANT_TRUE : VARIANT_FALSE;
    var[3].vt = VT_BSTR;
    var[3].bstrVal = url_bstr.m_str;

    dispparams.rgvarg = var;
    dispparams.rgdispidNamedArgs = &disp_this;
    dispparams.cNamedArgs = 1;
    dispparams.cArgs = 4;

    // TODO (michaeln): Something is not right here, when executing JavaScript
    // wired to these callbacks, this == wcs evaulates to false.  Local
    // variables are properly in scope for "inline" handlers, but "this" is off?
    hr = dispatchex->InvokeEx(
        kDispId0, LOCALE_USER_DEFAULT,
        DISPATCH_METHOD, &dispparams,
        NULL, NULL, NULL);

  } else if (handler) {
    // Fallback on IDispatch if needed.
    // Note: strangely, the parameters passed thru IDispatch are in reverse
    // order as compared to the method signature being invoked
    UINT arg_err = 0;
    VARIANT var[3];
    var[0].vt = VT_I4;
    var[0].intVal = id;
    var[1].vt = VT_BOOL;
    var[1].boolVal = success ? VARIANT_TRUE : VARIANT_FALSE;
    var[2].vt = VT_BSTR;
    var[2].bstrVal = url_bstr.m_str;

    dispparams.rgvarg = var;
    dispparams.cArgs = 3;

    hr = handler->Invoke(
        kDispId0, IID_NULL, LOCALE_SYSTEM_DEFAULT,
        DISPATCH_METHOD, &dispparams,
        NULL, NULL, &arg_err);

  }
}
