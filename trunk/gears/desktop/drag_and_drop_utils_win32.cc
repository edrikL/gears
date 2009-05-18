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

#include "gears/desktop/drag_and_drop_utils_common.h"
#if GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM

#include <mshtmdid.h>
#include <shlobj.h>
#include <windows.h>

#include "gears/desktop/drag_and_drop_utils_win32.h"

#include "gears/base/ie/activex_utils.h"
#include "gears/desktop/file_dialog.h"

#if BROWSER_FF
#include "gears/desktop/drag_and_drop_utils_ff.h"
#include "gears/base/firefox/dom_utils.h"
#elif BROWSER_IE
#include "gears/desktop/drag_and_drop_utils_ie.h"
#endif


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
  Ref();
  return 1;
}


STDMETHODIMP_(ULONG) DropTargetInterceptor::Release() {
  Unref();
  return 1;
}


STDMETHODIMP DropTargetInterceptor::DragEnter(
    IDataObject *object, DWORD state, POINTL point, DWORD *effect) {
  cached_meta_data_is_valid_ = false;
  cursor_type_ = DRAG_AND_DROP_CURSOR_INVALID;
  HRESULT hr = original_drop_target_->DragEnter(object, state, point, effect);
  if (SUCCEEDED(hr)) {
    if (cursor_type_ == DRAG_AND_DROP_CURSOR_COPY) {
      *effect = DROPEFFECT_COPY;
    } else if (cursor_type_ == DRAG_AND_DROP_CURSOR_NONE) {
      *effect = DROPEFFECT_NONE;
    }
  }
  return hr;
}


STDMETHODIMP DropTargetInterceptor::DragOver(
    DWORD state, POINTL point, DWORD *effect) {
  cursor_type_ = DRAG_AND_DROP_CURSOR_INVALID;
  HRESULT hr = original_drop_target_->DragOver(state, point, effect);
  if (SUCCEEDED(hr)) {
    if (cursor_type_ == DRAG_AND_DROP_CURSOR_COPY) {
      *effect = DROPEFFECT_COPY;
    } else if (cursor_type_ == DRAG_AND_DROP_CURSOR_NONE) {
      *effect = DROPEFFECT_NONE;
    }
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
#if BROWSER_IE
    // TODO(nigeltao): If AddFileDragAndDropData returns false (e.g. the user
    // is dragging Text or a URL), then we will be scanning the files on every
    // event. We should fix that.
    cached_meta_data_is_valid_ =
        AddFileDragAndDropData(module_environment_.get(), &cached_meta_data_);
#else
    // On Firefox, we let the AddFileDragAndDropData function in
    // drag_and_drop_utils_ff.cc do the work, (and manually get/set
    // cached_meta_data_is_valid_) since it needs the nsIDragSession pointer
    // to get the filenames. This isn't ideal -- it breaks encapsulation by
    // exposing the cached_meta_data_is_valid_ member variable.
    // TODO(nigeltao): Perhaps a better design is to add a void* parameter to
    // this function (and to AddFileDragAndDropData) that was NULL on IE and
    // the nsIDragSession on Firefox.
#endif
  }
  return cached_meta_data_;
}


void DropTargetInterceptor::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
  module_environment_->js_runner_->RemoveEventHandler(JSEVENT_UNLOAD, this);

  // Break the ModuleEnvironment <--> DropTargetInterceptor reference cycle.
  assert(module_environment_->drop_target_interceptor_ == this);
  module_environment_->drop_target_interceptor_.reset(NULL);

  is_revoked_ = true;
  // TODO(nigeltao): Should we check if we are still the hwnd's droptarget?
  RevokeDragDrop(hwnd_);
  if (original_drop_target_) {
    RegisterDragDrop(hwnd_, original_drop_target_);
  }
  // The following Unref balances the Ref() in the constructor, and may delete
  // this DropTargetInterceptor instance.
  Unref();
}


void DropTargetInterceptor::SetDragCursor(DragAndDropCursorType cursor_type) {
  cursor_type_ = cursor_type;
}


bool DropTargetInterceptor::GetCachedMetaDataIsValid() {
  return cached_meta_data_is_valid_;
}


void DropTargetInterceptor::SetCachedMetaDataIsValid(bool valid) {
  cached_meta_data_is_valid_= valid;
}


DropTargetInterceptor::DropTargetInterceptor(
    ModuleEnvironment *module_environment,
    HWND hwnd,
    IDropTarget *original_drop_target)
    : cached_meta_data_is_valid_(false),
      module_environment_(module_environment),
      hwnd_(hwnd),
      original_drop_target_(original_drop_target),
      cursor_type_(DRAG_AND_DROP_CURSOR_INVALID),
      is_revoked_(false) {
  LEAK_COUNTER_INCREMENT(DropTargetInterceptor);
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
  assert(is_revoked_);
  instances_.erase(hwnd_);
}


// The LPARAM argument to our custom EnumChildWindowsProc is a pointer to
// an instance of this struct.
struct EnumChildWindowsProcContext {
  HWND *result_ptr;
#if BROWSER_FF
  HWND tab_hwnd;
#endif
};


static BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM param) {
  EnumChildWindowsProcContext *context =
      reinterpret_cast<EnumChildWindowsProcContext*>(param);

#if BROWSER_FF
  HWND parent_hwnd = GetParent(hwnd);
  if (!parent_hwnd) {
    return TRUE;
  }
  HWND grandparent_hwnd = GetParent(parent_hwnd);
  if (grandparent_hwnd != context->tab_hwnd) {
    return TRUE;
  }

#elif BROWSER_IE
  WCHAR class_name[100];
  GetClassName(hwnd, static_cast<LPTSTR>(class_name), 100);
  if (wcscmp(class_name, L"Internet Explorer_Server")) {
    return TRUE;
  }
#endif

  DWORD process_id = 0;
  DWORD thread_id = GetWindowThreadProcessId(hwnd, &process_id);
  if (thread_id != GetCurrentThreadId() ||
      process_id != GetCurrentProcessId()) {
    return TRUE;
  }

  *(context->result_ptr) = hwnd;
#if BROWSER_FF
  // On Gecko, the content HWND is a grandchild of the tab HWND. There can be
  // more than one such grandchild, but last-one-wins seems to get the one
  // we want, even with multiple tabs in one browser window, and even with
  // <iframe>s present.
  return TRUE;
#elif BROWSER_IE
  return FALSE;
#endif
}


std::map<HWND, DropTargetInterceptor*> DropTargetInterceptor::instances_;


DropTargetInterceptor *DropTargetInterceptor::Intercept(
    ModuleEnvironment *module_environment) {
  HWND hwnd_to_intercept = 0;
  EnumChildWindowsProcContext context;
  context.result_ptr = &hwnd_to_intercept;
  HWND hwnd = 0;

#if BROWSER_FF
  if (NS_OK != DOMUtils::GetTabNativeWindow(
          module_environment->js_context_, &hwnd)) {
    return NULL;
  }
  context.tab_hwnd = hwnd;
#ifdef DEBUG
  WCHAR class_name[100];
  GetClassName(hwnd, static_cast<LPTSTR>(class_name), 100);
  assert(!wcscmp(class_name, L"MozillaContentWindowClass") ||
         !wcscmp(class_name, L"MozillaContentFrameWindowClass"));
#endif  // DEBUG

#elif BROWSER_IE
  if (!GetBrowserWindow(module_environment, &hwnd)) {
    return NULL;
  }
#endif  // BROWSER_XX

  EnumChildWindows(hwnd, EnumChildWindowsProc, (LPARAM)&context);
  if (!hwnd_to_intercept) {
    return NULL;
  }

  std::map<HWND, DropTargetInterceptor*>::iterator i =
      instances_.find(hwnd_to_intercept);
  DropTargetInterceptor *interceptor = NULL;
  if (i != instances_.end()) {
    interceptor = i->second;
  } else {
    IDropTarget *original_drop_target = static_cast<IDropTarget*>(
        GetProp(hwnd_to_intercept, L"OleDropTargetInterface"));
    if (original_drop_target) {
      interceptor = new DropTargetInterceptor(
          module_environment, hwnd_to_intercept, original_drop_target);
    }
  }
  return interceptor;
}


#endif  // GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM
