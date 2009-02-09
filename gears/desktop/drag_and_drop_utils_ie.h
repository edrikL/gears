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

#ifndef GEARS_DESKTOP_DRAG_AND_DROP_UTILS_IE_H__
#define GEARS_DESKTOP_DRAG_AND_DROP_UTILS_IE_H__

#include <map>

#include "gears/base/common/base_class.h"
#include "gears/base/common/js_runner.h"
#include "gears/desktop/drag_and_drop_utils_common.h"


// A DropTargetInterceptor is a IDropTarget implementation that sits in front
// of (and mostly delegates to) Internet Explorer's default IDropTarget. Its
// purpose is to allow Gears to respond last to drag and drop events, which
// allows us to set an effect (i.e. cursor) of DROPEFFECT_NONE, when rejecting
// a file drop. In comparison, trying to set event.dataTransfer.dropEffect to
// 'none', via IE's JavaScript is overridden by IE's default behavior for file
// drops, which is to show DROPEFFECT_COPY or DROPEFFECT_LINK, but not
// DROPEFFECT_NONE.
//
// This is also where we cache file drag-and-drop metadata so that, for
// example, during the numerous DRAGOVER events, we only query the clipboard
// (and the filesystem) once.
class DropTargetInterceptor
    : public IDropTarget,
      public RefCounted,
      public JsEventHandlerInterface {
 public:
  static DropTargetInterceptor *Intercept(
      ModuleEnvironment *module_environment);

  STDMETHODIMP QueryInterface(REFIID riid, void **object);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();
  STDMETHODIMP DragEnter(
      IDataObject *object, DWORD state, POINTL point, DWORD *effect);
  STDMETHODIMP DragOver(DWORD state, POINTL point, DWORD *effect);
  STDMETHODIMP DragLeave();
  STDMETHODIMP Drop(
      IDataObject *object, DWORD state, POINTL point, DWORD *effect);

  FileDragAndDropMetaData &GetFileDragAndDropMetaData();

  virtual void HandleEvent(JsEventType event_type);
  void SetDragCursor(DragAndDropCursorType cursor_type);

 private:
  static std::map<HWND, DropTargetInterceptor*> instances_;

  FileDragAndDropMetaData cached_meta_data_;
  bool cached_meta_data_is_valid_;

  scoped_refptr<ModuleEnvironment> module_environment_;
  HWND hwnd_;
  CComPtr<IDropTarget> original_drop_target_;
  DragAndDropCursorType cursor_type_;
  bool is_revoked_;

  DropTargetInterceptor(
      ModuleEnvironment *module_environment,
      HWND hwnd,
      IDropTarget *original_drop_target);
  ~DropTargetInterceptor();

  DISALLOW_EVIL_CONSTRUCTORS(DropTargetInterceptor);
};


HRESULT GetHtmlDataTransfer(
    IHTMLWindow2 *html_window_2,
    CComPtr<IHTMLEventObj> &html_event_obj,
    CComPtr<IHTMLDataTransfer> &html_data_transfer);

void AcceptDrag(ModuleEnvironment *module_environment,
                JsObject *event,
                bool acceptance,
                std::string16 *error_out);

bool GetDragData(ModuleEnvironment *module_environment,
                 JsObject *event,
                 JsObject *data_out,
                 std::string16 *error_out);

void SetDragCursor(ModuleEnvironment *module_environment,
                   JsObject *event,
                   DragAndDropCursorType cursor_type,
                   std::string16 *error_out);

#endif  // GEARS_DESKTOP_DRAG_AND_DROP_UTILS_IE_H__
