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

#include "gears/desktop/drag_and_drop_utils_common.h"
#if GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM

#include <mshtmdid.h>
#include <shlobj.h>
#include <windows.h>

#include "gears/desktop/drag_and_drop_utils_ie.h"

#include "gears/base/ie/activex_utils.h"
#include "gears/desktop/drag_and_drop_utils_win32.h"


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
      (window == NULL) ||
      FAILED(window->get_event(&window_event)) ||
      (window_event == NULL) ||
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
  DragAndDropCursorType cursor_type = acceptance
      ? DRAG_AND_DROP_CURSOR_COPY
      : DRAG_AND_DROP_CURSOR_NONE;
  SetDragCursor(module_environment, event, cursor_type, error_out);
  if (!error_out->empty()) {
    return;
  }
  CComPtr<IHTMLEventObj> window_event;
  DragAndDropEventType type = GetWindowEvent(module_environment, window_event);
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

void SetDragCursor(ModuleEnvironment *module_environment,
                   JsObject *event,
                   DragAndDropCursorType cursor_type,
                   std::string16 *error_out) {
  CComPtr<IHTMLEventObj> window_event;
  DragAndDropEventType type = GetWindowEvent(module_environment, window_event);
  if (type == DRAG_AND_DROP_EVENT_INVALID) {
    *error_out = STRING16(L"The drag-and-drop event is invalid.");
    return;
  }
  if (module_environment->drop_target_interceptor_) {
    module_environment->drop_target_interceptor_->SetDragCursor(cursor_type);
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


#endif  // GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM
#endif  // OFFICIAL_BUILD
