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

#include "gears/blob/blob_builder_module.h"

#include "gears/blob/blob.h"
#include "gears/blob/blob_builder.h"

DECLARE_DISPATCHER(GearsBlobBuilder);

template<>
void Dispatcher<GearsBlobBuilder>::Init() {
  RegisterMethod("append", &GearsBlobBuilder::Append);
  RegisterMethod("getAsBlob", &GearsBlobBuilder::GetAsBlob);
}


const std::string GearsBlobBuilder::kModuleName("GearsBlobBuilder");


GearsBlobBuilder::GearsBlobBuilder()
    : ModuleImplBaseClass(kModuleName),
      builder_(new BlobBuilder()) {
}


GearsBlobBuilder::~GearsBlobBuilder() {
}


void GearsBlobBuilder::Append(JsCallContext *context) {
  int appendee_type = context->GetArgumentType(0);

  std::string16 appendee_as_string;
  ModuleImplBaseClass *appendee_as_module = NULL;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_UNKNOWN, NULL },
  };
  if (appendee_type == JSPARAM_STRING16) {
    argv[0].type = JSPARAM_STRING16;
    argv[0].value_ptr = &appendee_as_string;
  } else {
    // We set argv[0].type to ask for a JSPARAM_MODULE. If the first argument
    // was neither a string nor a Gears module, then the context->GetArguments
    // call a few lines down will fail.
    argv[0].type = JSPARAM_MODULE;
    argv[0].value_ptr = &appendee_as_module;
  }

  if (!context->GetArguments(ARRAYSIZE(argv), argv)) {
    assert(context->is_exception_set());
    return;
  }

  if (appendee_type == JSPARAM_STRING16) {
    builder_->AddString(appendee_as_string);
    return;
  }

  if (appendee_as_module &&
      GearsBlob::kModuleName != appendee_as_module->get_module_name()) {
    context->SetException(
        STRING16(L"First parameter must be a Blob or a string."));
    return;
  }

  scoped_refptr<BlobInterface> blob_interface;
  static_cast<GearsBlob*>(appendee_as_module)->GetContents(&blob_interface);
  builder_->AddBlob(blob_interface.get());
}


void GearsBlobBuilder::GetAsBlob(JsCallContext *context) {
  scoped_refptr<BlobInterface> blob_interface;
  builder_->CreateBlob(&blob_interface);
  assert(blob_interface.get());
  scoped_refptr<GearsBlob> gears_blob;
  if (!CreateModule<GearsBlob>(module_environment_.get(),
                               context, &gears_blob)) {
    return;
  }
  gears_blob->Reset(blob_interface.get());
  context->SetReturnValue(JSPARAM_MODULE, gears_blob.get());
}
