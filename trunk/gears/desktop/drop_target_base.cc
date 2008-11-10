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

#if BROWSER_FF
#include "gears/desktop/drop_target_ff.h"
#elif BROWSER_IE && !defined(OS_WINCE)
#include "gears/desktop/drop_target_ie.h"
#endif


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


DropTargetBase::DropTargetBase(ModuleEnvironment *module_environment,
                               JsObject *options,
                               std::string16 *error_out)
    : module_environment_(module_environment)
{
#ifdef DEBUG
  if (!options->GetPropertyAsBool(STRING16(L"debug"), &is_debugging_)) {
    is_debugging_ = false;
  }
#endif
  unload_monitor_.reset(new JsEventMonitor(
      module_environment->js_runner_, JSEVENT_UNLOAD, this));
  InitializeCallback(STRING16(L"ondragenter"), options,
                     &on_drag_enter_, error_out);
  InitializeCallback(STRING16(L"ondragover"), options,
                     &on_drag_over_, error_out);
  InitializeCallback(STRING16(L"ondragleave"), options,
                     &on_drag_leave_, error_out);
  InitializeCallback(STRING16(L"ondrop"), options,
                     &on_drop_, error_out);
}


#endif  // GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM
#endif  // OFFICIAL_BUILD
