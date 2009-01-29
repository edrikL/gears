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

#include "gears/desktop/drop_target_base.h"

class DropTargetInterceptor;

// The 1 in the IDispEventSimpleImpl line is just an arbitrary positive
// integer, as says the IDispEventSimpleImpl documentation at
// http://msdn.microsoft.com/en-us/library/fwy24613(VS.80).aspx
// This 1 is the same nID as used in the SINK_ENTRY_INFO calls below.
//
// Note that we subclass RefCounted even though IDispEventSimpleImpl
// nominally implements AddRef and Release, because IDispEventSimpleImpl's
// actual implementation is merely { return 1; } without actually doing
// real ref-counting. See Visual C++ 2008's atlcom.h near line 4123.
// Thus, for DropTarget's ref-counting, we use RefCounted from
// base/common/scoped_refptr.h.
class DropTarget
    : public DropTargetBase,
      public IDispEventSimpleImpl<1, DropTarget, &DIID_HTMLElementEvents2>,
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

  virtual ~DropTarget();

  // The result should be held within a scoped_refptr.
  static DropTarget *CreateDropTarget(ModuleEnvironment *module_environment,
                                      JsDomElement &dom_element,
                                      JsObject *options,
                                      std::string16 *error_out);

  HRESULT CancelEventBubble(CComPtr<IHTMLEventObj> &html_event_obj,
                            CComPtr<IHTMLDataTransfer> &html_data_transfer);

  STDMETHOD(HandleOnDragEnter)();
  STDMETHOD(HandleOnDragOver)();
  STDMETHOD(HandleOnDragLeave)();
  STDMETHOD(HandleOnDragDrop)();

  virtual void UnregisterSelf();

 private:
  CComPtr<IDispatch> event_source_;
  CComPtr<IHTMLWindow2> html_window_2_;
  bool unregister_self_has_been_called_;
  bool will_accept_drop_;

  // TODO(nigeltao): This is an interceptor for the "gears-calls-the-webapp"
  // or "version 1" DnD API, but we will also need an interceptor for the
  // "the-webapp-calls-gears" or "version 2" DnD API, which will probably be
  // a member variable of the ModuleEnvironment. If that happens, then we
  // will not need a per-DropTarget interceptor, and can therefore eliminate
  // this member variable.
  scoped_refptr<DropTargetInterceptor> interceptor_;

#ifdef DEBUG
  CComPtr<IHTMLStyle> html_style_;
#endif

  DropTarget(ModuleEnvironment *module_environment,
             JsObject *options,
             std::string16 *error_out);

  void AddEventToJsObject(JsObject *js_object);
  void ProvideDebugVisualFeedback(bool is_drag_enter);

  static _ATL_FUNC_INFO atl_func_info_;
  DISALLOW_EVIL_CONSTRUCTORS(DropTarget);
};

#endif  // OS_WINCE
#endif  // OFFICIAL_BUILD
#endif  // GEARS_DESKTOP_DROP_TARGET_IE_H__
