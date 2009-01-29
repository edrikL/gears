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
#else

#include "gears/desktop/drop_target_base.h"
#if GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM

#include <mshtmdid.h>
#include <shlobj.h>
#include <windows.h>

#include "gears/desktop/drag_and_drop_utils_ie.h"

#include "gears/base/ie/activex_utils.h"
#include "gears/desktop/file_dialog.h"


// This function overwrites buffer in-place.
static HRESULT ResolveShortcut(TCHAR *buffer) {
  static CComPtr<IShellLink> link;
  if (!link && FAILED(link.CoCreateInstance(CLSID_ShellLink))) {
    return E_FAIL;
  }

  CComQIPtr<IPersistFile> file(link);
  if (!file) return E_FAIL;

  // Open the shortcut file, load its content.
  HRESULT hr = file->Load(buffer, STGM_READ);
  if (FAILED(hr)) return hr;

  // Resolve the target of the shortcut.
  hr = link->Resolve(NULL, SLR_NO_UI);
  if (FAILED(hr)) return hr;

  // Read the target path.
  hr = link->GetPath(buffer, MAX_PATH, 0, SLGP_UNCPRIORITY);
  if (FAILED(hr)) return hr;

  return S_OK;
}


HRESULT GetHtmlDataTransfer(
    IHTMLWindow2 *html_window_2,
    CComPtr<IHTMLEventObj> &html_event_obj,
    CComPtr<IHTMLDataTransfer> &html_data_transfer) {
  HRESULT hr;
  hr = html_window_2->get_event(&html_event_obj);
  if (FAILED(hr)) return hr;
  CComQIPtr<IHTMLEventObj2> html_event_obj_2(html_event_obj);
  if (!html_event_obj_2) return E_FAIL;
  hr = html_event_obj_2->get_dataTransfer(&html_data_transfer);
  return hr;
}


bool AddFileDragAndDropData(ModuleEnvironment *module_environment,
                            FileDragAndDropMetaData *meta_data_out) {
  CComPtr<IHTMLWindow2> html_window_2;
  if (FAILED(ActiveXUtils::GetHtmlWindow2(
          module_environment->iunknown_site_, &html_window_2))) {
    return false;
  }

  CComPtr<IHTMLEventObj> html_event_obj;
  CComPtr<IHTMLDataTransfer> html_data_transfer;
  HRESULT hr = GetHtmlDataTransfer(
      html_window_2, html_event_obj, html_data_transfer);
  if (FAILED(hr)) return false;

  CComPtr<IServiceProvider> service_provider;
  hr = html_data_transfer->QueryInterface(&service_provider);
  if (FAILED(hr)) return false;
  CComPtr<IDataObject> data_object;
  hr = service_provider->QueryService<IDataObject>(
      IID_IDataObject, &data_object);
  if (FAILED(hr)) return false;

  std::vector<std::string16> filenames;
  FORMATETC desired_format_etc =
    { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
  STGMEDIUM stg_medium;
  hr = data_object->GetData(&desired_format_etc, &stg_medium);
  if (SUCCEEDED(hr)) {
    HDROP hdrop = static_cast<HDROP>(GlobalLock(stg_medium.hGlobal));
    if (hdrop == NULL) {
      ReleaseStgMedium(&stg_medium);
      return false;
    }

    UINT num_files = DragQueryFile(hdrop, -1, 0, 0);
    WCHAR buffer[MAX_PATH];
    for (UINT i = 0; i < num_files; ++i) {
      DragQueryFile(hdrop, i, buffer, MAX_PATH);
      SHFILEINFO sh_file_info;
      if (!SHGetFileInfo(buffer, 0, &sh_file_info, sizeof(sh_file_info),
                         SHGFI_ATTRIBUTES) ||
          !(sh_file_info.dwAttributes & SFGAO_FILESYSTEM) ||
          (sh_file_info.dwAttributes & SFGAO_FOLDER)) {
        continue;
      }
      if ((sh_file_info.dwAttributes & SFGAO_LINK) &&
          FAILED(ResolveShortcut(buffer))) {
        continue;
      }
      filenames.push_back(std::string16(buffer));
    }

    GlobalUnlock(stg_medium.hGlobal);
    ReleaseStgMedium(&stg_medium);
  }
  meta_data_out->SetFilenames(filenames);
  return true;
}


DragAndDropEventType GetWindowEvent(ModuleEnvironment *module_environment,
                                    CComPtr<IHTMLEventObj> &window_event) {
  // We ignore event_as_js_object, since Gears can access the IHTMLWindow2
  // and its IHTMLEventObj directly, via COM, and that seems more trustworthy
  // than event_as_js_object, which is supplied by (potentially malicious)
  // JavaScript. Note that, even if the script passes the genuine window.event
  // object, it appears to be different than the one we get from
  // IHTMLWindow2::get_event (different in that, querying both for their
  // IUnknown's gives different pointers).
  CComPtr<IHTMLWindow2> window;
  CComBSTR type;
  if (FAILED(ActiveXUtils::GetHtmlWindow2(
          module_environment->iunknown_site_, &window)) ||
      FAILED(window->get_event(&window_event)) ||
      FAILED(window_event->get_type(&type))) {
    // If we get here, then there is no window.event, so we are not in
    // the browser's event dispatch.
    return DRAG_AND_DROP_EVENT_INVALID;
  }

  if (type == CComBSTR(L"dragover")) {
    return DRAG_AND_DROP_EVENT_DRAGOVER;
  } else if (type == CComBSTR(L"dragenter")) {
    return DRAG_AND_DROP_EVENT_DRAGENTER;
  } else if (type == CComBSTR(L"dragleave")) {
    return DRAG_AND_DROP_EVENT_DRAGLEAVE;
  } else if (type == CComBSTR(L"drop")) {
    return DRAG_AND_DROP_EVENT_DROP;
  }
  // If we get here, then we are in a non drag-and-drop event, such as a
  // keypress event.
  return DRAG_AND_DROP_EVENT_INVALID;
}


void AcceptDrag(ModuleEnvironment *module_environment,
                JsObject *event,
                bool acceptance,
                std::string16 *error_out) {
  CComPtr<IHTMLEventObj> window_event;
  DragAndDropEventType type = GetWindowEvent(module_environment, window_event);
  if (type == DRAG_AND_DROP_EVENT_INVALID) {
    *error_out = STRING16(L"The drag-and-drop event is invalid.");
    return;
  }

  if (type != DRAG_AND_DROP_EVENT_DROP) {
    CComQIPtr<IHTMLEventObj2> window_event2(window_event);
    CComPtr<IHTMLDataTransfer> data_transfer;
    if (!window_event2 ||
        FAILED(window_event->put_returnValue(CComVariant(false))) ||
        FAILED(window_event->put_cancelBubble(VARIANT_TRUE)) ||
        FAILED(window_event2->get_dataTransfer(&data_transfer)) ||
        FAILED(data_transfer->put_dropEffect(acceptance ? L"copy" : L"none"))) {
      *error_out = GET_INTERNAL_ERROR_MESSAGE();
      return;
    }
  }
}


bool GetDragData(ModuleEnvironment *module_environment,
                 JsObject *event_as_js_object,
                 JsObject *data_out,
                 std::string16 *error_out) {
  CComPtr<IHTMLEventObj> window_event;
  DragAndDropEventType type = GetWindowEvent(module_environment, window_event);
  if (type == DRAG_AND_DROP_EVENT_INVALID) {
    *error_out = STRING16(L"The drag-and-drop event is invalid.");
    return false;
  }

  FileDragAndDropMetaData meta_data;
  return AddFileDragAndDropData(module_environment, &meta_data) &&
      meta_data.ToJsObject(
          module_environment,
          type == DRAG_AND_DROP_EVENT_DROP,
          data_out,
          error_out);
}


STDMETHODIMP DropTargetInterceptor::QueryInterface(REFIID riid, void **object) {
  if ((riid == IID_IUnknown) || (riid == IID_IDropTarget)) {
    *object = this;
    AddRef();
    return S_OK;
  } else {
    return E_NOINTERFACE;
  }
}


STDMETHODIMP_(ULONG) DropTargetInterceptor::AddRef() {
  // TODO(nigeltao): Remove this debugging statement (and the corresponding
  // ones in Release, constructor and destructor).
  LOG16((L"DropTargetInterceptor::AddRef   %p %ld (+1)\n", this, GetRef()));
  Ref();
  return 1;
}


STDMETHODIMP_(ULONG) DropTargetInterceptor::Release() {
  LOG16((L"DropTargetInterceptor::Release  %p %ld (-1)\n", this, GetRef()));
  Unref();
  return 1;
}


STDMETHODIMP DropTargetInterceptor::DragEnter(
    IDataObject *object, DWORD state, POINTL point, DWORD *effect) {
  cached_meta_data_is_valid_ = false;
  will_accept_drop_ = true;
  HRESULT hr = original_drop_target_->DragEnter(object, state, point, effect);
  if (SUCCEEDED(hr) && !will_accept_drop_) {
    *effect = DROPEFFECT_NONE;
  }
  return hr;
}


STDMETHODIMP DropTargetInterceptor::DragOver(
    DWORD state, POINTL point, DWORD *effect) {
  will_accept_drop_ = true;
  HRESULT hr = original_drop_target_->DragOver(state, point, effect);
  if (SUCCEEDED(hr) && !will_accept_drop_) {
    *effect = DROPEFFECT_NONE;
  }
  return hr;
}


STDMETHODIMP DropTargetInterceptor::DragLeave() {
  HRESULT hr = original_drop_target_->DragLeave();
  cached_meta_data_is_valid_ = false;
  return hr;
}


STDMETHODIMP DropTargetInterceptor::Drop(
    IDataObject *object, DWORD state, POINTL point, DWORD *effect) {
  HRESULT hr = original_drop_target_->Drop(object, state, point, effect);
  cached_meta_data_is_valid_ = false;
  return hr;
}


FileDragAndDropMetaData &DropTargetInterceptor::GetFileDragAndDropMetaData() {
  if (!cached_meta_data_is_valid_) {
    cached_meta_data_.Reset();
    cached_meta_data_is_valid_ =
        AddFileDragAndDropData(module_environment_.get(), &cached_meta_data_);
  }
  return cached_meta_data_;
}


void DropTargetInterceptor::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
  module_environment_->js_runner_->RemoveEventHandler(JSEVENT_UNLOAD, this);
  is_revoked_ = true;
  // TODO(nigeltao): Should we check if we are still the hwnd's droptarget?
  RevokeDragDrop(hwnd_);
  RegisterDragDrop(hwnd_, original_drop_target_);
  // The following Unref balances the Ref() in the constructor, and may delete
  // this DropTargetInterceptor instance.
  Unref();
}


void DropTargetInterceptor::SetWillAcceptDrop(bool will_accept_drop) {
  will_accept_drop_ = will_accept_drop;
}


DropTargetInterceptor::DropTargetInterceptor(
    ModuleEnvironment *module_environment,
    HWND hwnd,
    IDropTarget *original_drop_target)
    : cached_meta_data_is_valid_(false),
      module_environment_(module_environment),
      hwnd_(hwnd),
      original_drop_target_(original_drop_target),
      will_accept_drop_(true),
      is_revoked_(false) {
  LEAK_COUNTER_INCREMENT(DropTargetInterceptor);
  LOG16((L"DropTargetInterceptor::ctor     %p\n", this));
  assert(hwnd_);
  assert(original_drop_target_);
  module_environment_->js_runner_->AddEventHandler(JSEVENT_UNLOAD, this);
  // We maintain a reference to ourselves, for the lifetime of the page (i.e.
  // until page unload), since the alternative is to be tied to the lifetime
  // of some script-level object, which may be more fragile (e.g. if that
  // object is garbage collected during the execution of a DnD event handler).
  // The Ref() is balanced by an Unref() during HandleEvent(JSEVENT_UNLOAD).
  Ref();
  // TODO(nigeltao): Check if the following two calls return something other
  // than S_OK, and react appropriately.
  RevokeDragDrop(hwnd_);
  RegisterDragDrop(hwnd_, this);
  instances_[hwnd_] = this;
}


DropTargetInterceptor::~DropTargetInterceptor() {
  LEAK_COUNTER_DECREMENT(DropTargetInterceptor);
  LOG16((L"DropTargetInterceptor::dtor     %p\n", this));
  assert(is_revoked_);
  instances_.erase(hwnd_);
}


static BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM param) {
  WCHAR class_name[100];
  GetClassName(hwnd, static_cast<LPTSTR>(class_name), 100);
  if (wcscmp(class_name, L"Internet Explorer_Server")) {
    return TRUE;
  }

  DWORD process_id = 0;
  DWORD thread_id = GetWindowThreadProcessId(hwnd, &process_id);

  if (thread_id == GetCurrentThreadId() &&
      process_id == GetCurrentProcessId()) {
    *(reinterpret_cast<HWND*>(param)) = hwnd;
    return FALSE;
  } else {
    return TRUE;
  }
}


std::map<HWND, DropTargetInterceptor*> DropTargetInterceptor::instances_;


DropTargetInterceptor *DropTargetInterceptor::Intercept(
    ModuleEnvironment *module_environment) {
  HWND browser_window = 0;
  HWND child = 0;
  if (GetBrowserWindow(module_environment, &browser_window)) {
    EnumChildWindows(browser_window, EnumChildWindowsProc, (LPARAM)&child);
  }
  if (!child) {
    return NULL;
  }

  std::map<HWND, DropTargetInterceptor*>::iterator i =
      instances_.find(child);
  DropTargetInterceptor *interceptor = NULL;
  if (i != instances_.end()) {
    interceptor = i->second;
  } else {
    IDropTarget *original_drop_target =
        static_cast<IDropTarget*>(GetProp(child, L"OleDropTargetInterface"));
    if (original_drop_target) {
      interceptor = new DropTargetInterceptor(
          module_environment, child, original_drop_target);
    }
  }
  return interceptor;
}


#endif  // GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM
#endif  // OFFICIAL_BUILD
