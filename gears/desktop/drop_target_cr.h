// Copyright 2009, Google Inc.
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

#ifndef GEARS_DESKTOP_DROP_TARGET_CR_H_
#define GEARS_DESKTOP_DROP_TARGET_CR_H_

#ifdef OFFICIAL_BUILD
  // The Drag-and-Drop API has not been finalized for official builds.
#else

#include "gears/base/common/common.h"
#include "gears/base/npapi/scoped_npapi_handles.h"
#include "gears/desktop/drop_target_base.h"

class DragSession;
class EventListener;

class DropTarget : public DropTargetBase, public RefCounted {
 public:
  static DropTarget *CreateDropTarget(ModuleEnvironment *,
      JsDomElement &element, JsObject *options, std::string16 *error);

  DropTarget(ModuleEnvironment *,
      JsObject *options, std::string16 *error);
  virtual ~DropTarget();

  bool AddDropTarget(JsDomElement &element, std::string16 *error);
  virtual void UnregisterSelf();

 private:
  bool ElementAttach(JsObject *element);
  bool AddEventListener(const char *event, int event_id);
  void ElementDetach();
  bool RemoveEventListener(const char *event);
  void SetElementStyle(const char *style);

  static bool Event(int event_id, JsObject *event, void *data);
  bool HandleOnDragEnter(JsObject *event);
  bool HandleOnDragOver(JsObject *event);
  bool HandleOnDragLeave(JsObject *event);
  bool HandleOnDragDrop(JsObject *event);

  bool OnDragActive(const JsRootedCallback *callback, JsObject *event);
  void OnDragLeave(const JsRootedCallback *callback, JsObject *event);
  void OnDragDrop(const JsRootedCallback *callback, JsObject *event);

 private:
  bool unregister_self_has_been_called_;
  JsRunnerInterface *runner_;
  ScopedNPObject window_;
  NPP npp_;

  ScopedNPVariant element_;
  EventListener *listener_;

  bool accept_;
  int dragging_;
  bool debug_;

  DISALLOW_EVIL_CONSTRUCTORS(DropTarget);
};

#endif  // OFFICIAL_BUILD
#endif  // GEARS_DESKTOP_DROP_TARGET_CR_H_
