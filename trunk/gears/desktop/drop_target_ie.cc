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

#ifdef OFFICIAL_BUILD
// The Drag-and-Drop API has not been finalized for official builds.
// Nor is it implemented on Windows CE.
#else
#ifdef OS_WINCE
// The Drag-and-Drop API is not implemented on Windows CE.
#else
#include <shlobj.h>

#include "gears/desktop/drop_target_ie.h"

#include "gears/base/common/leak_counter.h"
#include "gears/base/common/string16.h"
#include "gears/base/ie/activex_utils.h"
#include "gears/desktop/drag_and_drop_utils_ie.h"
#include "gears/desktop/file_dialog.h"


_ATL_FUNC_INFO DropTarget::atl_func_info_ =
    { CC_STDCALL, VT_I4, 1, { VT_EMPTY } };


DropTarget::DropTarget(ModuleEnvironment *module_environment,
                       JsObject *options,
                       std::string16 *error_out)
    : DropTargetBase(module_environment, options, error_out),
      unregister_self_has_been_called_(false),
      will_accept_drop_(false) {
  LEAK_COUNTER_INCREMENT(DropTarget);
}


DropTarget::~DropTarget() {
  LEAK_COUNTER_DECREMENT(DropTarget);
}


void DropTarget::ProvideDebugVisualFeedback(bool is_drag_enter) {
#ifdef DEBUG
  if (!is_debugging_) {
    return;
  }
  html_style_->put_border(CComBSTR(
      is_drag_enter ? L"green 3px solid" : L"black 3px solid"));
#endif
}

  
DropTarget *DropTarget::CreateDropTarget(ModuleEnvironment *module_environment,
                                         JsDomElement &dom_element,
                                         JsObject *options,
                                         std::string16 *error_out) {
  HRESULT hr;
  IDispatch *dom_element_dispatch = dom_element.dispatch();

  scoped_refptr<DropTarget> drop_target(new DropTarget(
      module_environment, options, error_out));
  if (!error_out->empty()) {
    return NULL;
  }

  drop_target->interceptor_.reset(
      DropTargetInterceptor::Intercept(module_environment));
  if (!drop_target->interceptor_.get()) {
    return NULL;
  }

  hr = drop_target->DispEventAdvise(dom_element_dispatch,
                                    &DIID_HTMLElementEvents2);
  if (FAILED(hr)) { return NULL; }
  drop_target->event_source_ = dom_element_dispatch;
  CComQIPtr<IHTMLElement> html_element(dom_element_dispatch);
  if (!html_element) { return NULL; }
#ifdef DEBUG
  hr = html_element->get_style(&drop_target->html_style_);
  if (FAILED(hr)) { return NULL; }
#endif
  CComPtr<IDispatch> document_dispatch;
  hr = html_element->get_document(&document_dispatch);
  if (FAILED(hr)) { return NULL; }
  CComQIPtr<IHTMLDocument2> html_document_2(document_dispatch);
  if (!html_document_2) { return NULL; }
  hr = html_document_2->get_parentWindow(&drop_target->html_window_2_);
  if (FAILED(hr)) { return NULL; }
  drop_target->Ref();  // Balanced by a Unref() call during UnregisterSelf.
  drop_target->ProvideDebugVisualFeedback(false);
  return drop_target.get();
}


void DropTarget::AddEventToJsObject(JsObject *js_object) {
  CComPtr<IHTMLEventObj> html_event_obj;
  HRESULT hr = html_window_2_->get_event(&html_event_obj);
  if (FAILED(hr)) { return; }
  js_object->SetProperty(STRING16(L"event"),
                         CComVariant(static_cast<IDispatch*>(html_event_obj)));
}


HRESULT DropTarget::CancelEventBubble(
    CComPtr<IHTMLEventObj> &html_event_obj,
    CComPtr<IHTMLDataTransfer> &html_data_transfer)
{
  if (!will_accept_drop_) return S_OK;
  HRESULT hr;
  hr = html_data_transfer->put_dropEffect(L"copy");
  if (FAILED(hr)) return hr;
  hr = html_event_obj->put_returnValue(CComVariant(false));
  if (FAILED(hr)) return hr;
  hr = html_event_obj->put_cancelBubble(VARIANT_TRUE);
  return hr;
}


HRESULT DropTarget::HandleOnDragEnter()
{
  // TODO(nigeltao): Does will_accept_drop_ need to be a member variable, or
  // can it be just a local variable for HandleOnDragEnter and HandleOnDragOver
  // (and hence we can eliminate DropTarget::CancelEventBubble), since fiddling
  // around with things like event.dataTransfer.dropEffect is probably
  // overridden by whatever we do in the DropTargetInterceptor.
  will_accept_drop_ = false;
  ProvideDebugVisualFeedback(true);
  CComPtr<IHTMLEventObj> html_event_obj;
  CComPtr<IHTMLDataTransfer> html_data_transfer;
  HRESULT hr = GetHtmlDataTransfer(
      html_window_2_, html_event_obj, html_data_transfer);
  if (FAILED(hr)) return hr;

  CComPtr<IServiceProvider> service_provider;
  hr = html_data_transfer->QueryInterface(&service_provider);
  if (FAILED(hr)) return hr;
  CComPtr<IDataObject> data_object;
  hr = service_provider->QueryService<IDataObject>(IID_IDataObject,
                                                   &data_object);
  if (FAILED(hr)) return hr;

  CComPtr<IEnumFORMATETC> enum_format_etc;
  hr = data_object->EnumFormatEtc(DATADIR_GET, &enum_format_etc);
  if (FAILED(hr)) return hr;
  FORMATETC format_etc;
  bool in_file_drop = false;
  while (enum_format_etc->Next(1, &format_etc, NULL) == S_OK) {
    if (format_etc.cfFormat == CF_HDROP) {
      in_file_drop = true;
      break;
    }
  }
  if (!in_file_drop) return E_FAIL;

  if (on_drag_enter_.get()) {
    scoped_ptr<JsObject> context_object(
        module_environment_->js_runner_->NewObject());
    AddEventToJsObject(context_object.get());
    std::string16 error;
    interceptor_->GetFileDragAndDropMetaData().ToJsObject(
        module_environment_.get(), false, context_object.get(), &error);
    if (!error.empty()) {
      return E_FAIL;
    }
    const int argc = 1;
    JsParamToSend argv[argc] = {
      { JSPARAM_OBJECT, context_object.get() }
    };
    scoped_ptr<JsRootedToken> return_value;
    module_environment_->js_runner_->InvokeCallback(
        on_drag_enter_.get(), NULL, argc, argv, as_out_parameter(return_value));
    // The HTML5 specification (section 5.4.5) says that an event handler
    // returning *false* means that we should not perform the default action
    // (i.e. the web-app wants Gears' file drop behavior, and not the default
    // browser behavior of navigating away from the current page to the file
    // being dropped).
    will_accept_drop_ = return_value.get() &&
        V_VT(&return_value->token()) == VT_BOOL &&
        V_BOOL(&return_value->token()) == false;
    interceptor_->SetWillAcceptDrop(will_accept_drop_);
  }

  hr = CancelEventBubble(html_event_obj, html_data_transfer);
  if (FAILED(hr)) return hr;
  return S_OK;
}


HRESULT DropTarget::HandleOnDragOver()
{
  will_accept_drop_ = false;
  CComPtr<IHTMLEventObj> html_event_obj;
  CComPtr<IHTMLDataTransfer> html_data_transfer;
  HRESULT hr = GetHtmlDataTransfer(
      html_window_2_, html_event_obj, html_data_transfer);
  if (FAILED(hr)) return hr;

  if (on_drag_over_.get()) {
    scoped_ptr<JsObject> context_object(
        module_environment_->js_runner_->NewObject());
    AddEventToJsObject(context_object.get());
    std::string16 error;
    interceptor_->GetFileDragAndDropMetaData().ToJsObject(
        module_environment_.get(), false, context_object.get(), &error);
    if (!error.empty()) {
      return E_FAIL;
    }
    const int argc = 1;
    JsParamToSend argv[argc] = {
      { JSPARAM_OBJECT, context_object.get() }
    };
    scoped_ptr<JsRootedToken> return_value;
    module_environment_->js_runner_->InvokeCallback(
        on_drag_over_.get(), NULL, argc, argv, as_out_parameter(return_value));
    will_accept_drop_ = return_value.get() &&
        V_VT(&return_value->token()) == VT_BOOL &&
        V_BOOL(&return_value->token()) == false;
    interceptor_->SetWillAcceptDrop(will_accept_drop_);
  }

  hr = CancelEventBubble(html_event_obj, html_data_transfer);
  if (FAILED(hr)) return hr;
  return S_OK;
}


HRESULT DropTarget::HandleOnDragLeave()
{
  ProvideDebugVisualFeedback(false);
  will_accept_drop_ = false;
  CComPtr<IHTMLEventObj> html_event_obj;
  CComPtr<IHTMLDataTransfer> html_data_transfer;
  HRESULT hr = GetHtmlDataTransfer(
      html_window_2_, html_event_obj, html_data_transfer);
  if (FAILED(hr)) return hr;

  if (on_drag_leave_.get()) {
    scoped_ptr<JsObject> context_object(
        module_environment_->js_runner_->NewObject());
    AddEventToJsObject(context_object.get());
    std::string16 error;
    // TODO(nigeltao): To match the behavior on other browsers, we should not
    // pass up file drag-and-drop metadata on DRAGLEAVE, but only on DRAGENTER,
    // DRAGOVER and DROP.
    interceptor_->GetFileDragAndDropMetaData().ToJsObject(
        module_environment_.get(), false, context_object.get(), &error);
    if (!error.empty()) {
      return E_FAIL;
    }
    const int argc = 1;
    JsParamToSend argv[argc] = {
      { JSPARAM_OBJECT, context_object.get() }
    };
    module_environment_->js_runner_->InvokeCallback(
        on_drag_leave_.get(), NULL, argc, argv, NULL);
  }

  hr = CancelEventBubble(html_event_obj, html_data_transfer);
  if (FAILED(hr)) return hr;
  return S_OK;
}


HRESULT DropTarget::HandleOnDragDrop()
{
  ProvideDebugVisualFeedback(false);
  if (!will_accept_drop_) return S_OK;
  will_accept_drop_ = false;

  if (on_drop_.get()) {
    scoped_ptr<JsObject> context_object(
        module_environment_->js_runner_->NewObject());
    AddEventToJsObject(context_object.get());
    std::string16 error;
    interceptor_->GetFileDragAndDropMetaData().ToJsObject(
        module_environment_.get(), true, context_object.get(), &error);
    if (!error.empty()) {
      return E_FAIL;
    }

    const int argc = 1;
    JsParamToSend argv[argc] = {
      { JSPARAM_OBJECT, context_object.get() }
    };
    module_environment_->js_runner_->InvokeCallback(
        on_drop_.get(), NULL, argc, argv, NULL);
  }

  return S_OK;
}


void DropTarget::UnregisterSelf() {
  if (unregister_self_has_been_called_) {
    return;
  }
  unregister_self_has_been_called_ = true;

  if (event_source_) {
    DispEventUnadvise(event_source_, &DIID_HTMLElementEvents2);
  }
  interceptor_.reset(NULL);
  Unref();  // Balanced by an Ref() call during CreateDropTarget.
}


#endif  // OS_WINCE
#endif  // OFFICIAL_BUILD
