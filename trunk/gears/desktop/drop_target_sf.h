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

#ifndef GEARS_DESKTOP_DROP_TARGET_SF_H__
#define GEARS_DESKTOP_DROP_TARGET_SF_H__
#ifdef OFFICIAL_BUILD
// The Drag-and-Drop API has not been finalized for official builds.
#else

#include "gears/base/common/base_class.h"
#include "gears/base/common/js_dom_element.h"
#include "gears/base/common/js_runner.h"
#include "gears/base/common/js_types.h"
#include "gears/base/common/scoped_refptr.h"
#include "gears/desktop/drop_target_base.h"
#include "third_party/scoped_ptr/scoped_ptr.h"

class GearsHtmlEventListener;

class DropTarget
    : public DropTargetBase,
      public RefCounted {
 public:
  virtual ~DropTarget();

  // The result should be held within a scoped_refptr.
  static DropTarget *CreateDropTarget(ModuleEnvironment *module_environment,
                                      JsDomElement &dom_element,
                                      JsObject *options,
                                      std::string16 *error_out);

  void UnregisterSelf();

  virtual void HandleEvent(JsEventType event_type);

  enum HtmlEventType {
    HTML_EVENT_TYPE_DRAG_ENTER,
    HTML_EVENT_TYPE_DRAG_OVER,
    HTML_EVENT_TYPE_DRAG_LEAVE,
    HTML_EVENT_TYPE_DROP
  };
  bool HandleDragAndDropEvent(JsObject *event,
                              JsRootedCallback *callback,
                              HtmlEventType type);

 private:
  ScopedNPVariant element_;
  scoped_refptr<GearsHtmlEventListener> drag_enter_listener_;
  scoped_refptr<GearsHtmlEventListener> drag_over_listener_;
  scoped_refptr<GearsHtmlEventListener> drag_leave_listener_;
  scoped_refptr<GearsHtmlEventListener> drop_listener_;
  ScopedNPVariant compiled_invoke_callback_script_;
  bool unregister_self_has_been_called_;

  static bool OnDragEnter(JsObject *event, void *drop_target);
  static bool OnDragOver(JsObject *event, void *drop_target);
  static bool OnDragLeave(JsObject *event, void *drop_target);
  static bool OnDrop(JsObject *event, void *drop_target);

  DropTarget(ModuleEnvironment *module_environment,
             JsObject *options,
             std::string16 *error_out);

  bool InvokeCallback(JsRootedCallback *callback, JsObject *arg);

  // Pass a NULL callback to remove a previously added listener.
  void SetEventListener(const char *event_name,
                        scoped_refptr<GearsHtmlEventListener> *listener,
                        bool (*callback)(JsObject *, void *));

  DISALLOW_EVIL_CONSTRUCTORS(DropTarget);
};

#endif  // OFFICIAL_BUILD
#endif  // GEARS_DESKTOP_DROP_TARGET_SF_H__
