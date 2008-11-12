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
#include "gears/desktop/drop_target_sf.h"

#include "gears/base/common/leak_counter.h"
#include "gears/base/npapi/scoped_npapi_handles.h"
#include "gears/desktop/drag_and_drop_utils_osx.h"
#include "gears/desktop/file_dialog.h"


class GearsHtmlEventListener : public ModuleImplBaseClass {
 public:
  static const std::string kModuleName;

  GearsHtmlEventListener();

  void HandleEvent(JsCallContext *context);

  void SetCallback(bool (*callback)(JsObject *, void *),
                   void *callback_data);

 private:
  bool (*callback_)(JsObject *event, void *callback_data);
  void *callback_data_;
  ScopedNPVariant compiled_prevent_default_script_;

  bool PreventDefaultEventBehavior(JsObject *event);

  DISALLOW_EVIL_CONSTRUCTORS(GearsHtmlEventListener);
};


DECLARE_DISPATCHER(GearsHtmlEventListener);

template<>
void Dispatcher<GearsHtmlEventListener>::Init() {
  RegisterMethod("handleEvent", &GearsHtmlEventListener::HandleEvent);
}


const std::string GearsHtmlEventListener::kModuleName("GearsHtmlEventListener");


GearsHtmlEventListener::GearsHtmlEventListener()
    : ModuleImplBaseClass(kModuleName),
      callback_(NULL),
      callback_data_(NULL) {
}


void GearsHtmlEventListener::HandleEvent(JsCallContext *context) {
  if (!callback_) {
    return;
  }

  scoped_ptr<JsObject> event;
  JsArgument argv[] = {
    { JSPARAM_REQUIRED, JSPARAM_OBJECT, as_out_parameter(event) }
  };
  context->GetArguments(ARRAYSIZE(argv), argv);
  if (context->is_exception_set()) {
    return;
  }

  // TODO(nigeltao): Does this break Text and URL drag and drop? Perhaps we
  // should push this down into the DropTarget class where we can check if
  // we are in a file drop.
  PreventDefaultEventBehavior(event.get());

  bool result = callback_(event.get(), callback_data_);
  context->SetReturnValue(JSPARAM_BOOL, &result);
}


void GearsHtmlEventListener::SetCallback(
      bool (*callback)(JsObject *, void *),
      void *callback_data) {
  callback_ = callback;
  callback_data_ = callback_data;
}


// TODO(nigeltao): It seems weird to compile a hard-coded script just to
// execute a known method on a known object, but trying to invoke the
// method directly via event.GetPropertyAsFunction followed by js_runner->
// InvokeCallback leads to a mysterious TypeError, on Safari / Mac OS X.
// Ideally we would find the cause of the error (which is possibly in Gears'
// NPAPI binding code) and do it directly, but in the meantime, here is an
// indirect workaround.
bool GearsHtmlEventListener::PreventDefaultEventBehavior(JsObject *event) {
  NPP npp = module_environment_->js_runner_->GetContext();
  if (NPVARIANT_IS_VOID(compiled_prevent_default_script_)) {
    static const char script[] = "(function(e) { e.preventDefault(); })";
    NPString np_script = { script, ARRAYSIZE(script) - 1 };
    if (!NPN_Evaluate(npp, module_environment_->js_runner_->GetGlobalObject(),
                      &np_script, &compiled_prevent_default_script_) ||
        !NPVARIANT_IS_OBJECT(compiled_prevent_default_script_)) {
      return false;
    }
  }

  ScopedNPVariant ignored_result;
  const int argc = 1;
  scoped_array<ScopedNPVariant> argv(new ScopedNPVariant[argc]);
  argv[0].Reset(event->token());
  NPN_InvokeDefault(npp, NPVARIANT_TO_OBJECT(compiled_prevent_default_script_),
                    argv.get(), argc, &ignored_result);
  return true;
}


// Similarly to GearsHtmlEventListener::PreventDefaultEventBehavior, we use
// an indirect script to invoke our known callback, in order to workaround
// a mysterious TypeError when trying to invoke it directly.
bool DropTarget::InvokeCallback(JsRootedCallback *callback, JsObject *arg) {
  NPP npp = module_environment_->js_runner_->GetContext();
  if (NPVARIANT_IS_VOID(compiled_invoke_callback_script_)) {
    static const char script[] = "(function(f, arg) { f(arg); })";
    NPString np_script = { script, ARRAYSIZE(script) - 1 };
    if (!NPN_Evaluate(npp, module_environment_->js_runner_->GetGlobalObject(),
                      &np_script, &compiled_invoke_callback_script_) ||
        !NPVARIANT_IS_OBJECT(compiled_invoke_callback_script_)) {
      return false;
    }
  }

  ScopedNPVariant ignored_result;
  const int argc = 2;
  scoped_array<ScopedNPVariant> argv(new ScopedNPVariant[argc]);
  argv[0].Reset(callback->token());
  argv[1].Reset(arg->token());
  NPN_InvokeDefault(npp, NPVARIANT_TO_OBJECT(compiled_invoke_callback_script_),
                    argv.get(), argc, &ignored_result);
  return true;
}


DropTarget::DropTarget(ModuleEnvironment *module_environment,
                       JsObject *options,
                       std::string16 *error_out)
    : DropTargetBase(module_environment, options, error_out),
      unregister_self_has_been_called_(false) {
  LEAK_COUNTER_INCREMENT(DropTarget);
}


DropTarget::~DropTarget() {
  LEAK_COUNTER_DECREMENT(DropTarget);
}


bool DropTarget::OnDragEnter(JsObject *event, void *callback_data) {
  DropTarget *drop_target = reinterpret_cast<DropTarget *>(callback_data);
  return drop_target->HandleDragAndDropEvent(event,
                                             drop_target->on_drag_enter_.get(),
                                             HTML_EVENT_TYPE_DRAG_ENTER);
}


bool DropTarget::OnDragOver(JsObject *event, void *callback_data) {
  DropTarget *drop_target = reinterpret_cast<DropTarget *>(callback_data);
  return drop_target->HandleDragAndDropEvent(event,
                                             drop_target->on_drag_over_.get(),
                                             HTML_EVENT_TYPE_DRAG_OVER);
}


bool DropTarget::OnDragLeave(JsObject *event, void *callback_data) {
  DropTarget *drop_target = reinterpret_cast<DropTarget *>(callback_data);
  return drop_target->HandleDragAndDropEvent(event,
                                             drop_target->on_drag_leave_.get(),
                                             HTML_EVENT_TYPE_DRAG_LEAVE);
}


bool DropTarget::OnDrop(JsObject *event, void *callback_data) {
  DropTarget *drop_target = reinterpret_cast<DropTarget *>(callback_data);
  return drop_target->HandleDragAndDropEvent(event,
                                             drop_target->on_drop_.get(),
                                             HTML_EVENT_TYPE_DROP);
}


bool DropTarget::HandleDragAndDropEvent(JsObject *event,
                                        JsRootedCallback *callback,
                                        HtmlEventType type) {
  if ((type == HTML_EVENT_TYPE_DROP && !IsInADropOperation()) ||
      (type != HTML_EVENT_TYPE_DROP && !IsInADragOperation())) {
    // We're in a synthetic drag and drop callback invocation (i.e. one
    // spoofed from JavaScript, possibly via dispatchEvent()) instead of
    // a genuine one.
    return false;
  }
  scoped_ptr<JsObject> context_object(
      module_environment_->js_runner_->NewObject());
  context_object->SetPropertyObject(STRING16(L"event"), event);
  if (type == HTML_EVENT_TYPE_DROP) {
    scoped_ptr<JsArray> file_array(
        module_environment_->js_runner_->NewArray());
    if (GetDroppedFiles(module_environment_.get(), file_array.get(), true)) {
      context_object->SetPropertyArray(STRING16(L"files"), file_array.get());
    }
  }
  InvokeCallback(callback, context_object.get());
  return true;
}


void DropTarget::SetEventListener(
    const char *event_name,
    scoped_refptr<GearsHtmlEventListener> *listener,
    bool (*callback)(JsObject *, void *)) {
  if (callback) {
    if (!CreateModule<GearsHtmlEventListener>(
            module_environment_.get(), NULL, listener)) {
      return;
    }
    (*listener)->SetCallback(callback, this);
  } else {
    (*listener)->SetCallback(NULL, NULL);
  }

  NPVariant args[3];
  STRINGZ_TO_NPVARIANT(event_name, args[0]);
  args[1] = (*listener)->GetWrapperToken();
  BOOLEAN_TO_NPVARIANT(false, args[2]);

  // TODO(nigeltao): For some reason, the removeEventListener call isn't
  // working, and our GearsHtmlEventListeners handleEvent is still called.
  NPIdentifier method_id = NPN_GetStringIdentifier(
      callback ? "addEventListener" : "removeEventListener");
  NPObject *element = NPVARIANT_TO_OBJECT(element_);
  ScopedNPVariant ignored_result;
  NPN_Invoke(module_environment_->js_runner_->GetContext(), element,
             method_id, args, 3, &ignored_result);

  if (!callback) {
    listener->reset(NULL);
  }
}


DropTarget *DropTarget::CreateDropTarget(ModuleEnvironment *module_environment,
                                         JsDomElement &dom_element,
                                         JsObject *options,
                                         std::string16 *error_out) {
  scoped_refptr<DropTarget> drop_target(new DropTarget(
      module_environment, options, error_out));
  if (!error_out->empty()) {
    return NULL;
  }

  drop_target->Ref();  // Balanced by an Unref() call during UnregisterSelf.

  JsObject *js_object = dom_element.js_object();
  drop_target->element_.Reset(js_object->token());
  if (drop_target->on_drag_enter_.get()) {
    drop_target->SetEventListener("dragenter",
                                  &drop_target->drag_enter_listener_,
                                  &OnDragEnter);
  }
  if (drop_target->on_drag_over_.get()) {
    drop_target->SetEventListener("dragover",
                                  &drop_target->drag_over_listener_,
                                  &OnDragOver);
  }
  if (drop_target->on_drag_leave_.get()) {
    drop_target->SetEventListener("dragleave",
                                  &drop_target->drag_leave_listener_,
                                  &OnDragLeave);
  }
  if (drop_target->on_drop_.get()) {
    drop_target->SetEventListener("drop",
                                  &drop_target->drop_listener_,
                                  &OnDrop);
  }
  return drop_target.get();
}


void DropTarget::UnregisterSelf() {
  if (unregister_self_has_been_called_) {
    return;
  }
  unregister_self_has_been_called_ = true;

  if (drag_enter_listener_.get()) {
    SetEventListener("dragenter", &drag_enter_listener_, NULL);
  }
  if (drag_over_listener_.get()) {
    SetEventListener("dragover", &drag_over_listener_, NULL);
  }
  if (drag_leave_listener_.get()) {
    SetEventListener("dragleave", &drag_leave_listener_, NULL);
  }
  if (drop_listener_.get()) {
    SetEventListener("drop", &drop_listener_, NULL);
  }

  Unref();  // Balanced by a Ref() call during CreateDropTarget.
}


// A DropTarget instance automatically de-registers itself, on page unload.
void DropTarget::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
  UnregisterSelf();
}


#endif  // OFFICIAL_BUILD
