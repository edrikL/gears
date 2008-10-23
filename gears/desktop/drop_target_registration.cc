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
#if defined(BROWSER_CHROME) || defined(BROWSER_WEBKIT) || \
    defined(OS_WINCE) || defined(OS_ANDROID)
  // The Drag-and-Drop API has not been implemented for Chrome, Safari,
  // PocketIE, and Android.
#else

#include "gears/desktop/drop_target_registration.h"


DECLARE_DISPATCHER(GearsDropTargetRegistration);

template<>
void Dispatcher<GearsDropTargetRegistration>::Init() {
  RegisterMethod("unregisterDropTarget",
                 &GearsDropTargetRegistration::UnregisterDropTarget);
}

const std::string GearsDropTargetRegistration::kModuleName(
    "GearsDropTargetRegistration");

GearsDropTargetRegistration::GearsDropTargetRegistration()
    : ModuleImplBaseClass(kModuleName) {}

void GearsDropTargetRegistration::SetDropTarget(DropTarget *drop_target) {
  assert(drop_target_.get() == NULL);
  drop_target_ = drop_target;
}

void GearsDropTargetRegistration::UnregisterDropTarget(JsCallContext *context) {
  if (drop_target_.get()) {
    drop_target_->UnregisterSelf();
    drop_target_ = NULL;
  } else {
    context->SetException(
        STRING16(L"unregisterDropTarget was called multiple times."));
  }
}


#endif  // BROWSER_CHROME, BROWSER_WEBKIT, OS_WINCE, OS_ANDROID
#endif  // OFFICIAL_BUILD
