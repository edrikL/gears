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

#ifndef GEARS_DESKTOP_DROP_TARGET_IE_H__
#define GEARS_DESKTOP_DROP_TARGET_IE_H__
#ifdef OFFICIAL_BUILD
// The Drag-and-Drop API has not been finalized for official builds.
#else
#ifdef OS_WINCE
// The Drag-and-Drop API is not implemented on Windows CE.
#else

#include <mshtmdid.h>
#include <windows.h>

#include "gears/base/common/base_class.h"
#include "gears/base/common/js_dom_element.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/scoped_refptr.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

// The 1 in the IDispEventSimpleImpl line is just an arbitrary positive
// integer, as says the IDispEventSimpleImpl documentation at
// http://msdn.microsoft.com/en-us/library/fwy24613(VS.80).aspx
// This 1 is the same nID as used in the SINK_ENTRY_INFO calls below.
//
// We subclass IDispEventSimpleImpl in order to intercept drag enter, drag
// over, drag leave, and drop events. A previous approach implemented
// IDispatchImpl instead of IDispEventSimpleImpl, and attached to the DOM
// element via an IElementBehavior, but it turned out that this approach
// interfered with the IDispatch of the DOM element, and the DOM element lost
// its scripted properties (and therefore things like document.getElementById
// failed).
//
// Note that we subclass RefCounted even though IDispEventSimpleImpl
// nominally implements AddRef and Release, because IDispEventSimpleImpl's
// actual implementation is merely { return 1; } without actually doing
// real ref-counting. See Visual C++ 2008's atlcom.h near line 4123.
// Thus, for DropTarget's ref-counting, we use RefCounted from
// base/common/scoped_refptr.h.
class DropTarget
    : public IDispEventSimpleImpl<1, DropTarget, &DIID_HTMLElementEvents2>,
      public JsEventHandlerInterface,
      public RefCounted {
 public:
  BEGIN_SINK_MAP(DropTarget)
    SINK_ENTRY_INFO(1, DIID_HTMLElementEvents2,
                    DISPID_HTMLELEMENTEVENTS_ONDRAGENTER,
                    &HandleOnDragEnter,
                    &atl_func_info_)
    SINK_ENTRY_INFO(1, DIID_HTMLElementEvents2,
                    DISPID_HTMLELEMENTEVENTS_ONDRAGOVER,
                    &HandleOnDragOver,
                    &atl_func_info_)
    SINK_ENTRY_INFO(1, DIID_HTMLElementEvents2,
                    DISPID_HTMLELEMENTEVENTS_ONDRAGLEAVE,
                    &HandleOnDragLeave,
                    &atl_func_info_)
    SINK_ENTRY_INFO(1, DIID_HTMLElementEvents2,
                    DISPID_HTMLELEMENTEVENTS_ONDROP,
                    &HandleOnDragDrop,
                    &atl_func_info_)
  END_SINK_MAP()

  CComPtr<IHTMLWindow2> html_window_2_;

  scoped_refptr<ModuleEnvironment> module_environment_;
  scoped_ptr<JsEventMonitor> unload_monitor_;
  scoped_ptr<JsRootedCallback> on_drag_enter_;
  scoped_ptr<JsRootedCallback> on_drag_over_;
  scoped_ptr<JsRootedCallback> on_drag_leave_;
  scoped_ptr<JsRootedCallback> on_drop_;

  virtual ~DropTarget();
  static DropTarget *CreateDropTarget(JsDomElement &dom_element);

  HRESULT GetHtmlDataTransfer(CComPtr<IHTMLEventObj> &html_event_obj,
                              CComPtr<IHTMLDataTransfer> &html_data_transfer);
  HRESULT CancelEventBubble(CComPtr<IHTMLEventObj> &html_event_obj,
                            CComPtr<IHTMLDataTransfer> &html_data_transfer);

  STDMETHOD(HandleOnDragEnter)();
  STDMETHOD(HandleOnDragOver)();
  STDMETHOD(HandleOnDragLeave)();
  STDMETHOD(HandleOnDragDrop)();

  void UnregisterSelf();

  virtual void HandleEvent(JsEventType event_type);

 private:
  bool unregister_self_has_been_called_;
  CComPtr<IDispatch> event_source_;

  DropTarget();

  void AddEventToJsObject(JsObject *js_object);

  static _ATL_FUNC_INFO atl_func_info_;
  DISALLOW_EVIL_CONSTRUCTORS(DropTarget);
};

#endif  // OS_WINCE
#endif  // OFFICIAL_BUILD
#endif  // GEARS_DESKTOP_DROP_TARGET_IE_H__
