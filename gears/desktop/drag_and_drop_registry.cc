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
#include "gears/desktop/drag_and_drop_registry.h"


// TODO(nigeltao): Delete this file, since it's just a thin middleman, and move
// any useful code to either drop_target_registration, or drop_target_{ff,ie}.


#if BROWSER_FF
#include <gecko_sdk/include/nsIDOMHTMLElement.h>
#include <gecko_sdk/include/nsIDOMEvent.h>
#include <gecko_sdk/include/nsIDOMEventListener.h>
#include <gecko_sdk/include/nsIDOMEventTarget.h>
#include "gears/desktop/drop_target_ff.h"

#elif BROWSER_IE && !defined(OS_WINCE)
#include <windows.h>
#include "gears/desktop/drop_target_ie.h"

#endif

#include "gears/base/common/js_dom_element.h"
#include "third_party/scoped_ptr/scoped_ptr.h"


#if BROWSER_FF || (BROWSER_IE && !defined(OS_WINCE))
static bool InitializeCallback(const std::string16 name,
                               JsObject *options,
                               scoped_ptr<JsRootedCallback> *scoped_callback,
                               std::string16 *error_out) {
  JsParamType property_type = options->GetPropertyType(name);
  if (property_type != JSPARAM_UNDEFINED &&
      property_type != JSPARAM_FUNCTION) {
    *error_out = STRING16(L"options.");
    *error_out += name;
    *error_out += STRING16(L" should be a function.");
    return false;
  }
  JsRootedCallback *callback;
  if (options->GetPropertyAsFunction(name, &callback)) {
    scoped_callback->reset(callback);
  }
  return true;
}


static bool InitializeDropTarget(ModuleImplBaseClass *sibling_module,
                                 JsObject *options,
                                 DropTarget *drop_target,
                                 std::string16 *error_out) {
  sibling_module->GetModuleEnvironment(&drop_target->module_environment_);
#ifdef DEBUG
  if (!options->GetPropertyAsBool(STRING16(L"debug"),
                                  &drop_target->is_debugging_)) {
    drop_target->is_debugging_ = false;
  }
#endif
  drop_target->unload_monitor_.reset(
      new JsEventMonitor(sibling_module->GetJsRunner(),
                         JSEVENT_UNLOAD,
                         drop_target));
  return InitializeCallback(STRING16(L"ondragenter"), options,
                            &drop_target->on_drag_enter_, error_out) &&
         InitializeCallback(STRING16(L"ondragover"), options,
                            &drop_target->on_drag_over_, error_out) &&
         InitializeCallback(STRING16(L"ondragleave"), options,
                            &drop_target->on_drag_leave_, error_out) &&
         InitializeCallback(STRING16(L"ondrop"), options,
                            &drop_target->on_drop_, error_out);
}
#endif


DropTarget *DragAndDropRegistry::RegisterDropTarget(
    ModuleImplBaseClass *sibling_module,
    JsDomElement &dom_element,
    JsObject *options,
    std::string16 *error_out) {
#if BROWSER_FF
  DropTarget *drop_target = new DropTarget;
  if (!InitializeDropTarget(sibling_module, options, drop_target, error_out)) {
    delete drop_target;
    *error_out = GET_INTERNAL_ERROR_MESSAGE();
    return NULL;
  }

  drop_target->SetDomElement(dom_element);
  return drop_target;

#elif BROWSER_IE && !defined(OS_WINCE)
  scoped_ptr<DropTarget> drop_target(DropTarget::CreateDropTarget(dom_element));
  if (!drop_target.get()) {
    *error_out = STRING16(L"Could not create a DropTarget.");
    *error_out = GET_INTERNAL_ERROR_MESSAGE();
    return NULL;
  }
  if (!InitializeDropTarget(
          sibling_module, options, drop_target.get(), error_out)) {
    *error_out = GET_INTERNAL_ERROR_MESSAGE();
    return NULL;
  }
  return drop_target.release();

#else
  *error_out = STRING16(L"Desktop.registerDropTarget is not implemented.");
  return NULL;
#endif
}


#endif  // OFFICIAL_BUILD
