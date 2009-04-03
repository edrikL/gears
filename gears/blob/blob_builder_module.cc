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


bool GearsBlobBuilder::Append(
    BlobBuilder *builder,
    const JsToken &token,
    JsContextPtr js_context,
    AbstractJsTokenVector *array_stack) {
  int type = JsTokenGetType(token, js_context);
  if (type == JSPARAM_INT) {
    int token_as_int;
    if (!JsTokenToInt_NoCoerce(token, js_context, &token_as_int)) {
      return false;
    }
    uint8 byte = static_cast<uint8>(token_as_int);
    builder->AddData(&byte, 1);
    return true;

  } else if (type == JSPARAM_STRING16) {
    std::string16 token_as_string;
    if (!JsTokenToString_NoCoerce(token, js_context, &token_as_string)) {
      return false;
    }
    builder->AddString(token_as_string);
    return true;

  } else if (type == JSPARAM_OBJECT || type == JSPARAM_MODULE) {
    ModuleImplBaseClass *token_as_module = NULL;
    if (!JsTokenToModule(module_environment_->js_runner_,
            js_context, token, &token_as_module) ||
        GearsBlob::kModuleName != token_as_module->get_module_name()) {
      return false;
    }
    scoped_refptr<BlobInterface> blob_interface;
    static_cast<GearsBlob*>(token_as_module)->GetContents(&blob_interface);
    builder->AddBlob(blob_interface.get());
    return true;

  } else if (type == JSPARAM_ARRAY) {
    scoped_ptr<JsArray> token_as_array;
    int array_length;
    if (!JsTokenToArray_NoCoerce(
            token, js_context, as_out_parameter(token_as_array)) ||
        !token_as_array.get() ||
        !token_as_array->GetLength(&array_length)) {
      return false;
    }
    scoped_ptr<AbstractJsTokenVector> scoped_array_stack;
    if (array_stack == NULL) {
      scoped_array_stack.reset(new AbstractJsTokenVector);
      array_stack = scoped_array_stack.get();
    } else {
      AbstractJsToken abstract_js_token = JsTokenPtrToAbstractJsToken(
          const_cast<JsToken*>(&token));
      for (AbstractJsTokenVector::iterator iter = array_stack->begin();
           iter != array_stack->end();
           ++iter) {
        if (module_environment_->js_runner_->
                AbstractJsTokensAreEqual(abstract_js_token, *iter)) {
          return false;
        }
      }
    }
    array_stack->push_back(JsTokenPtrToAbstractJsToken(
        const_cast<JsToken*>(&token)));
    for (int i = 0; i < array_length; i++) {
      JsScopedToken array_element;
      if (!token_as_array->GetElement(i, &array_element) ||
          !Append(builder, array_element, js_context, array_stack)) {
        return false;
      }
    }
    array_stack->pop_back();
    return true;

  }
  return false;
}


void GearsBlobBuilder::Append(JsCallContext *context) {
  int argc = context->GetArgumentCount();
  if (argc != 1) {
    context->SetException(STRING16(argc > 1
        ? L"Too many parameters."
        : L"Required argument 1 is missing."));
    return;
  }
  const JsToken &token = context->GetArgument(0);
  JsContextPtr js_context = context->js_context();
  bool token_is_array = JsTokenGetType(token, js_context) == JSPARAM_ARRAY;
  scoped_ptr<BlobBuilder> intermediate_builder;
  BlobBuilder *builder = builder_.get();
  if (token_is_array) {
    // We create an intermediate_builder because we want Append to be atomic --
    // when appending an array of things, either all or none will be appended.
    intermediate_builder.reset(new BlobBuilder());
    builder = intermediate_builder.get();
  }
  if (!Append(builder, token, js_context, NULL)) {
    context->SetException(
        STRING16(L"Parameter must be an int, string, Blob or array of such."));
    return;
  }
  if (token_is_array) {
    scoped_refptr<BlobInterface> intermediate_blob;
    intermediate_builder->CreateBlob(&intermediate_blob);
    builder_->AddBlob(intermediate_blob.get());
  }
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
