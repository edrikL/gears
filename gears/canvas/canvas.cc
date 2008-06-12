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

#ifndef OFFICIAL_BUILD
// The Image API has not been finalized for official builds.

#include "gears/base/common/dispatcher.h"
#include "gears/base/common/module_wrapper.h"
#include "gears/canvas/canvas.h"
#include "gears/canvas/canvas_rendering_context_2d.h"

DECLARE_GEARS_WRAPPER(GearsCanvas);
const std::string GearsCanvas::kModuleName("GearsCanvas");

GearsCanvas::GearsCanvas()
    : ModuleImplBaseClassVirtual(kModuleName),
      width_(300),
      height_(150),
      rendering_context_(NULL) {
}

template<>
void Dispatcher<GearsCanvas>::Init() {
  RegisterMethod("load", &GearsCanvas::Load);
  RegisterMethod("toBlob", &GearsCanvas::ToBlob);
  RegisterMethod("clone", &GearsCanvas::Clone);
  RegisterProperty("originalPalette", &GearsCanvas::GetOriginalPalette, NULL);
  RegisterMethod("crop", &GearsCanvas::Crop);
  RegisterMethod("resize", &GearsCanvas::Resize);
  RegisterProperty("width", &GearsCanvas::GetWidth, &GearsCanvas::SetWidth);
  RegisterProperty("height", &GearsCanvas::GetHeight, &GearsCanvas::SetHeight);
  RegisterProperty("colorMode", &GearsCanvas::GetColorMode,
      &GearsCanvas::SetColorMode);
  RegisterMethod("getContext", &GearsCanvas::GetContext);
}

const std::string16 GearsCanvas::kColorModeRgba32 = STRING16(L"rgba_32");
const std::string16 GearsCanvas::kColorModeRgb24 = STRING16(L"rgb_24");

void GearsCanvas::Load(JsCallContext *context) {
  JsObject blob;
  JsArgument args[] = {
    { JSPARAM_REQUIRED, JSPARAM_OBJECT, &blob },
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvas::ToBlob(JsCallContext *context) {
  std::string16 mime_type;
  JsObject attributes;
  JsArgument args[] = {
    {JSPARAM_OPTIONAL, JSPARAM_STRING16, &mime_type},
    {JSPARAM_OPTIONAL, JSPARAM_OBJECT, &attributes}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvas::Clone(JsCallContext *context) {
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvas::GetOriginalPalette(JsCallContext *context) {
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvas::Crop(JsCallContext *context) {
  int x, y, width, height;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &x},
    {JSPARAM_REQUIRED, JSPARAM_INT, &y},
    {JSPARAM_REQUIRED, JSPARAM_INT, &width},
    {JSPARAM_REQUIRED, JSPARAM_INT, &height}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvas::Resize(JsCallContext *context) {
  int width, height;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &width},
    {JSPARAM_REQUIRED, JSPARAM_INT, &height}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
}

void GearsCanvas::GetWidth(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_INT, &width_);
}

void GearsCanvas::GetHeight(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_INT, &height_);
}

void GearsCanvas::SetWidth(JsCallContext *context) {
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &width_}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
  ResetToBlankPixels();
}

void GearsCanvas::SetHeight(JsCallContext *context) {
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_INT, &height_}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  context->SetException(STRING16(L"Unimplemented"));
  ResetToBlankPixels();
}

void GearsCanvas::GetColorMode(JsCallContext *context) {
  context->SetReturnValue(JSPARAM_STRING16, &color_mode_);
}

void GearsCanvas::SetColorMode(JsCallContext *context) {
  std::string16 new_mode;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_STRING16, &new_mode}
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  if (new_mode != kColorModeRgba32 && new_mode != kColorModeRgb24) {
    context->SetException(STRING16(L"Unrecognized color mode"));
    return;
  }
  color_mode_ = new_mode;
  context->SetException(STRING16(L"Unimplemented"));
  ResetToBlankPixels();
}

void GearsCanvas::ResetToBlankPixels() {
  // TODO(kart): Implement this.
}

void GearsCanvas::GetContext(JsCallContext *context) {
  std::string16 contextId;
  JsArgument args[] = {
    {JSPARAM_REQUIRED, JSPARAM_STRING16, &contextId},
  };
  context->GetArguments(ARRAYSIZE(args), args);
  if (context->is_exception_set())
    return;
  if (contextId != STRING16(L"gears-2d")) {
    context->SetReturnValue(JSPARAM_NULL, 0);
    return;  // As per the canvas spec.
  }
  if (rendering_context_.get() == NULL) {
    CreateModule<GearsCanvasRenderingContext2D>(GetJsRunner(),
        &rendering_context_);
    if (!rendering_context_->InitBaseFromSibling(this)) {
      context->SetException(STRING16(L"Initializing base class failed."));
      // Don't return a broken object the next time this function is called:
      rendering_context_.reset(NULL);
      return;
    }
    rendering_context_->InitCanvasField(this);
  }
  context->SetReturnValue(JSPARAM_DISPATCHER_MODULE, rendering_context_.get());
}

#endif  // OFFICIAL_BUILD
