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
#include "gears/desktop/drop_target_ff.h"

#include <gecko_sdk/include/nsIDOMEvent.h>
#include <gecko_sdk/include/nsIDOMEventTarget.h>
#include <gecko_sdk/include/nsIDOMHTMLElement.h>
#include <gecko_sdk/include/nsIFile.h>
#include <gecko_sdk/include/nsIFileURL.h>
#include <gecko_sdk/include/nsILocalFile.h>
#include <gecko_sdk/include/nsISupportsPrimitives.h>
#include <gecko_sdk/include/nsIURI.h>
#include "gears/base/common/leak_counter.h"
#include "gears/base/firefox/ns_file_utils.h"
#include "gears/desktop/drag_and_drop_utils_ff.h"
#include "gears/desktop/file_dialog.h"


// Note that some Mozilla event names differ from the HTML5 standard event
// names - the latter being the one we expose in the Gears API. Specifically,
// Mozilla's "dragexit" is HTML5's "dragleave", and "dragdrop" is "drop".
static const nsString kDragEnterAsString(STRING16(L"dragenter"));
static const nsString kDragOverAsString(STRING16(L"dragover"));
static const nsString kDragExitAsString(STRING16(L"dragexit"));
static const nsString kDragDropAsString(STRING16(L"dragdrop"));


NS_IMPL_ISUPPORTS1(DropTarget, nsIDOMEventListener)


DropTarget::DropTarget(ModuleEnvironment *module_environment,
                       JsObject *options,
                       std::string16 *error_out)
    : DropTargetBase(module_environment, options, error_out),
      unregister_self_has_been_called_(false),
      will_accept_drop_(false) {
  LEAK_COUNTER_INCREMENT(DropTarget);
}


DropTarget::~DropTarget() {
  LEAK_COUNTER_DECREMENT(DropTarget);
}


void DropTarget::ProvideDebugVisualFeedback(bool is_drag_enter) {
#ifdef DEBUG
  if (!is_debugging_) {
    return;
  }

  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(html_element_));
  if (element) {
    static const nsString kEnterStyle(STRING16(L"border:green 3px solid;"));
    static const nsString kOtherStyle(STRING16(L"border:black 3px solid;"));
    static const nsString kStyle(STRING16(L"style"));
    nsString current_style;
    element->GetAttribute(kStyle, current_style);
    current_style.Append(is_drag_enter ? kEnterStyle : kOtherStyle);
    element->SetAttribute(kStyle, current_style);
  }
#endif
}


DropTarget *DropTarget::CreateDropTarget(ModuleEnvironment *module_environment,
                                         JsDomElement &dom_element,
                                         JsObject *options,
                                         std::string16 *error_out) {
  nsCOMPtr<nsIDOMEventTarget> event_target =
      do_QueryInterface(dom_element.dom_html_element());
  if (!event_target) {
    return NULL;
  }

  scoped_refptr<DropTarget> drop_target(new DropTarget(
      module_environment, options, error_out));
  if (!error_out->empty()) {
    return NULL;
  }
  drop_target->event_target_ = event_target;
  drop_target->html_element_ = dom_element.dom_html_element();
  drop_target->AddRef();  // Balanced by a Release() call during UnregisterSelf.

  // TODO(nigeltao): we should check for errors during AddEventListener (and
  // similary for IE and Safari).
  event_target->AddEventListener(kDragEnterAsString,
                                 drop_target.get(), false);
  event_target->AddEventListener(kDragOverAsString,
                                 drop_target.get(), false);
  event_target->AddEventListener(kDragExitAsString,
                                 drop_target.get(), false);
  event_target->AddEventListener(kDragDropAsString,
                                 drop_target.get(), false);

  drop_target->ProvideDebugVisualFeedback(false);
  return drop_target.get();
}


void DropTarget::UnregisterSelf() {
  if (unregister_self_has_been_called_) {
    return;
  }
  unregister_self_has_been_called_ = true;

  if (event_target_) {
    event_target_->RemoveEventListener(kDragEnterAsString, this, false);
    event_target_->RemoveEventListener(kDragOverAsString, this, false);
    event_target_->RemoveEventListener(kDragExitAsString, this, false);
    event_target_->RemoveEventListener(kDragDropAsString, this, false);
  }

  Release();  // Balanced by an AddRef() call during CreateDropTarget.
}


void DropTarget::AddEventToJsObject(JsObject *js_object, nsIDOMEvent *event) {
  if (!xp_connect_) {
    nsresult nr = NS_OK;
    xp_connect_ = do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr);
    if (NS_FAILED(nr) || !xp_connect_) {
      return;
    }
  }

  JSContext *js_context = module_environment_->js_runner_->GetContext();
#if BROWSER_FF2
  // All we want to do, inside this #if BROWSER_FF2 block, is to call
  //   JSObject *scope = JS_GetScopeChain(js_context);
  // However, Mozilla bug 390160 says that JS_GetScopeChain sometimes returns
  // NULL, resulting in a "Nothing active on context" JavaScript error.
  // This bug was fixed for Gecko 1.9 (i.e. Firefox 3), but we have to
  // backport the JS_GetScopeChain patch for Firefox2. The patch is at
  // https://bugzilla.mozilla.org/attachment.cgi?id=274863
  // and the original bug report is at
  // https://bugzilla.mozilla.org/show_bug.cgi?id=390160
  JSObject *global_object = JS_GetGlobalObject(js_context);
  JSObject *scope = global_object;
  if (scope) {
    // The remainder of the backported patch is to call
    //   OBJ_TO_INNER_OBJECT(js_context, scope);
    // but we don't have access to the OBJ_TO_INNER_OBJECT macro (it is
    // defined inside mozilla/js/src/jsobj.h), so we just do it manually.
    JSClass *global_object_class = JS_GetClass(js_context, global_object);
    if (global_object_class->flags & JSCLASS_IS_EXTENDED) {
      JSExtendedClass *extended_class = (JSExtendedClass*) global_object_class;
      if (extended_class->innerObject) {
        scope = extended_class->innerObject(js_context, scope);
      }
    }
  }
#else
  JSObject *scope = JS_GetScopeChain(js_context);
#endif

  // For more on XPConnect and its various wrapper objects, see
  // http://wiki.mozilla.org/XPConnect_object_wrapping
  static const nsIID isupports_iid = NS_GET_IID(nsISupports);
  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  if (NS_FAILED(xp_connect_->WrapNative(js_context, scope, event,
                                        isupports_iid,
                                        getter_AddRefs(holder)))) {
    return;
  }

  JSObject *object = NULL;
  if (NS_FAILED(holder->GetJSObject(&object))) {
    return;
  }
  js_object->SetProperty(STRING16(L"event"), OBJECT_TO_JSVAL(object));
}


bool DropTarget::ExecJsCallback(DragAndDropEventType type,
                                nsIDragSession *drag_session,
                                nsIDOMEvent *event) {
  JsRootedCallback *callback = NULL;
  switch (type) {
    case DRAG_AND_DROP_EVENT_DRAGENTER:
      callback = on_drag_enter_.get();
      break;
    case DRAG_AND_DROP_EVENT_DRAGOVER:
      callback = on_drag_over_.get();
      break;
    case DRAG_AND_DROP_EVENT_DRAGLEAVE:
      callback = on_drag_leave_.get();
      break;
    case DRAG_AND_DROP_EVENT_DROP:
      callback = on_drop_.get();
      break;
    default:
      assert(false);
      break;
  }
  if (!callback) {
    return false;
  }

  std::string16 ignored;
  scoped_ptr<JsObject> context_object(
      module_environment_->js_runner_->NewObject());
  // TODO(nigeltao): Should we be checking the will_accept_drop_ state? What
  // should we do when Firefox sends us a DROP event even if the script has
  // been rejecting the drop (via returning true) - should we fire a DROP or
  // a DRAGLEAVE from Gears up to script?
  if (will_accept_drop_ &&
      type != DRAG_AND_DROP_EVENT_DRAGLEAVE &&
      !AddFileDragAndDropData(module_environment_.get(),
                              drag_session,
                              type,
                              context_object.get(),
                              &ignored)) {
    context_object.reset(module_environment_->js_runner_->NewObject());
  }
  AddEventToJsObject(context_object.get(), event);

  scoped_ptr<JsRootedToken> return_value;
  const int argc = 1;
  JsParamToSend argv[argc] = {
    { JSPARAM_OBJECT, context_object.get() }
  };
  module_environment_->js_runner_->InvokeCallback(
      callback, NULL, argc, argv, as_out_parameter(return_value));

  // The HTML5 specification (section 5.4.5) says that an event handler
  // returning *false* means that we should not perform the default
  // action (i.e. the web-app wants Gears' file drop behavior, and not
  // the default browser behavior of navigating away from the current
  // page to the file being dropped).
  return return_value.get() &&
      JSVAL_IS_BOOLEAN(return_value->token()) &&
      JSVAL_TO_BOOLEAN(return_value->token()) == false;
}


NS_IMETHODIMP DropTarget::HandleEvent(nsIDOMEvent *event) {
  nsString event_type;
  event->GetType(event_type);
  DragAndDropEventType type = DRAG_AND_DROP_EVENT_INVALID;
  if (event_type.Equals(kDragOverAsString)) {
    type = DRAG_AND_DROP_EVENT_DRAGOVER;
  } else if (event_type.Equals(kDragEnterAsString)) {
    type = DRAG_AND_DROP_EVENT_DRAGENTER;
    ProvideDebugVisualFeedback(true);
  } else if (event_type.Equals(kDragExitAsString)) {
    type = DRAG_AND_DROP_EVENT_DRAGLEAVE;
    ProvideDebugVisualFeedback(false);
  } else if (event_type.Equals(kDragDropAsString)) {
    type = DRAG_AND_DROP_EVENT_DROP;
    ProvideDebugVisualFeedback(false);
  } else {
    assert(false);
    return NS_ERROR_FAILURE;
  }
  
  nsCOMPtr<nsIDragService> drag_service =
      do_GetService("@mozilla.org/widget/dragservice;1");
  if (!drag_service) { return NS_ERROR_FAILURE; }
#if BROWSER_FF2 && defined(LINUX) && !defined(OS_MACOSX)
  if (type == DRAG_AND_DROP_EVENT_DRAGENTER) {
    // On Firefox2/Linux, the DragSession is incorrectly started *after* the
    // first dragenter event. We workaround this by explicitly starting the
    // drag session. For more information, see the similar workaround in the
    // drag_and_drop_utils_ff code.
    drag_service->StartDragSession();
  }
#endif
  nsCOMPtr<nsIDragSession> drag_session;
  nsresult nr = drag_service->GetCurrentSession(getter_AddRefs(drag_session));
  if (NS_FAILED(nr) || !drag_session.get()) { return NS_ERROR_FAILURE; }

  // TODO(nigeltao): Fix up the case where the script rejects the files, but
  // the user still drops (and Firefox / Linux still sends us a DROP event).
  // Perhaps we need to do something like this:
  // if (type == DROP && !will_accept_drop_) {
  //   type = LEAVE;
  // }
  // although we should check that dropping files that the script rejects
  // does not cause navigation away from the page.
  bool callback_returned_false =
      ExecJsCallback(type, drag_session.get(), event);

  if (type == DRAG_AND_DROP_EVENT_DRAGENTER ||
      type == DRAG_AND_DROP_EVENT_DRAGOVER) {
    nr = drag_session->SetDragAction(will_accept_drop_
        ? static_cast<int>(nsIDragService::DRAGDROP_ACTION_COPY)
        : static_cast<int>(nsIDragService::DRAGDROP_ACTION_NONE));
    if (NS_FAILED(nr)) { return NS_ERROR_FAILURE; }
    will_accept_drop_ = callback_returned_false;

  } else if (type == DRAG_AND_DROP_EVENT_DRAGLEAVE) {
    will_accept_drop_ = false;

  } else if (type == DRAG_AND_DROP_EVENT_DROP) {
    // Prevent the default browser behavior of navigating away from the
    // current page to the file being dropped.
    nr = event->StopPropagation();
    if (NS_FAILED(nr)) { return NS_ERROR_FAILURE; }
    if (will_accept_drop_) {
      nr = drag_session->SetDragAction(nsIDragService::DRAGDROP_ACTION_COPY);
      if (NS_FAILED(nr)) { return NS_ERROR_FAILURE; }
    }
    will_accept_drop_ = false;
  }

  return NS_OK;
}


void DropTarget::Ref() {
  AddRef();
}


void DropTarget::Unref() {
  Release();
}


#endif  // OFFICIAL_BUILD
