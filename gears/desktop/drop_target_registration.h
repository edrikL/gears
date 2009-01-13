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

#ifndef GEARS_DESKTOP_DROP_TARGET_REGISTRATION_H__
#define GEARS_DESKTOP_DROP_TARGET_REGISTRATION_H__
#ifdef OFFICIAL_BUILD
// The Drag-and-Drop API has not been finalized for official builds.
#else
#include "gears/desktop/drop_target_base.h"
#ifdef GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM

#include "gears/base/common/base_class.h"
#include "gears/base/common/js_types.h"

#if BROWSER_FF
#include "gears/desktop/drop_target_ff.h"
#elif BROWSER_IE
#include "gears/desktop/drop_target_ie.h"
#elif BROWSER_SAFARI
#include "gears/desktop/drop_target_sf.h"
#endif

class GearsDropTargetRegistration : public ModuleImplBaseClass {
 public:
  static const std::string kModuleName;

  GearsDropTargetRegistration();

  // IN: void
  // OUT: void
  void UnregisterDropTarget(JsCallContext *context);

  void SetDropTarget(DropTarget *drop_target);

 private:
  scoped_refptr<DropTarget> drop_target_;

  DISALLOW_EVIL_CONSTRUCTORS(GearsDropTargetRegistration);
};

#endif  // GEARS_DRAG_AND_DROP_API_IS_SUPPORTED_FOR_THIS_PLATFORM
#endif  // OFFICIAL_BUILD
#endif  // GEARS_DESKTOP_DROP_TARGET_REGISTRATION_H__
