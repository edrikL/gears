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

#include <gecko_sdk/include/nsIFile.h>
#include <gecko_sdk/include/nsIFileURL.h>
#include <gecko_sdk/include/nsIDOMHTMLElement.h>
#include <gecko_sdk/include/nsIDOMEvent.h>
#include <gecko_sdk/include/nsIDOMEventTarget.h>
#include <gecko_sdk/include/nsIIOService.h>
#include <gecko_sdk/include/nsILocalFile.h>
#include <gecko_sdk/include/nsISupportsPrimitives.h>
#include <gecko_sdk/include/nsIURI.h>
#include "gears/base/common/leak_counter.h"
#include "gears/desktop/file_dialog.h"


// Note that some Mozilla event names differ from the HTML5 standard event
// names - the latter being the one we expose in the Gears API. Specifically,
// Mozilla's "dragexit" is HTML5's "dragleave", and "dragdrop" is "drop".
const nsString DropTarget::kDragEnterAsString(STRING16(L"dragenter"));
const nsString DropTarget::kDragOverAsString(STRING16(L"dragover"));
const nsString DropTarget::kDragExitAsString(STRING16(L"dragexit"));
const nsString DropTarget::kDragDropAsString(STRING16(L"dragdrop"));


NS_IMPL_ISUPPORTS1(DropTarget, nsIDOMEventListener)


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

  if (drop_target->on_drag_enter_.get()) {
    event_target->AddEventListener(kDragEnterAsString,
                                   drop_target.get(), false);
  }
  if (drop_target->on_drag_over_.get()) {
    event_target->AddEventListener(kDragOverAsString,
                                   drop_target.get(), false);
  }
  if (drop_target->on_drag_leave_.get()) {
    event_target->AddEventListener(kDragExitAsString,
                                   drop_target.get(), false);
  }
  if (drop_target->on_drop_.get()) {
    event_target->AddEventListener(kDragDropAsString,
                                   drop_target.get(), false);
  }

  drop_target->ProvideDebugVisualFeedback(false);
  return drop_target.get();
}


void DropTarget::UnregisterSelf() {
  if (unregister_self_has_been_called_) {
    return;
  }
  unregister_self_has_been_called_ = true;

  if (event_target_) {
    if (on_drag_enter_.get()) {
      event_target_->RemoveEventListener(kDragEnterAsString, this, false);
    }
    if (on_drag_over_.get()) {
      event_target_->RemoveEventListener(kDragOverAsString, this, false);
    }
    if (on_drag_leave_.get()) {
      event_target_->RemoveEventListener(kDragExitAsString, this, false);
    }
    if (on_drop_.get()) {
      event_target_->RemoveEventListener(kDragDropAsString, this, false);
    }
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


NS_IMETHODIMP DropTarget::HandleEvent(nsIDOMEvent *event) {
  nsCOMPtr<nsIDragService> drag_service =
      do_GetService("@mozilla.org/widget/dragservice;1");
  if (!drag_service) { return false; }
  nsCOMPtr<nsIDragSession> drag_session;
  nsresult nr = drag_service->GetCurrentSession(getter_AddRefs(drag_session));
  if (NS_FAILED(nr) || !drag_session.get()) { return false; }
  nr = drag_session->SetDragAction(nsIDragService::DRAGDROP_ACTION_COPY);
  if (NS_FAILED(nr)) { return false; }

  nsString event_type;
  event->GetType(event_type);

  if (on_drop_.get() && event_type.Equals(kDragDropAsString)) {
    ProvideDebugVisualFeedback(false);
    std::string16 error;
    scoped_ptr<JsArray> file_array(
        module_environment_->js_runner_->NewArray());
    if (!GetDroppedFiles(drag_session.get(), file_array.get(), &error)) {
      return NS_ERROR_FAILURE;
    }

    // If we've got this far, then the drag-and-dropped data was indeed one or
    // more files, so we will notify our callback.  We also stop the event
    // propagation, to avoid the browser doing the default action, which is to
    // load that file (and navigate away from the current page). I (nigeltao)
    // would have thought that event->PreventDefault() would be the way to do
    // that, but that doesn't seem to work, whilst StopPropagation() does.
    event->StopPropagation();

    scoped_ptr<JsObject> context_object(
        module_environment_->js_runner_->NewObject());
    context_object->SetPropertyArray(STRING16(L"files"), file_array.get());
    AddEventToJsObject(context_object.get(), event);

    const int argc = 1;
    JsParamToSend argv[argc] = {
      { JSPARAM_OBJECT, context_object.get() }
    };
    module_environment_->js_runner_->InvokeCallback(
        on_drop_.get(), argc, argv, NULL);

  } else {
    JsRootedCallback *callback = NULL;
    if (on_drag_enter_.get() && event_type.Equals(kDragEnterAsString)) {
      ProvideDebugVisualFeedback(true);
      callback = on_drag_enter_.get();
    } else if (on_drag_over_.get() && event_type.Equals(kDragOverAsString)) {
      callback = on_drag_over_.get();
    } else if (on_drag_leave_.get() && event_type.Equals(kDragExitAsString)) {
      ProvideDebugVisualFeedback(false);
      callback = on_drag_leave_.get();
    }
    if (callback) {
      scoped_ptr<JsObject> context_object(
          module_environment_->js_runner_->NewObject());
      AddEventToJsObject(context_object.get(), event);
      const int argc = 1;
      JsParamToSend argv[argc] = {
        { JSPARAM_OBJECT, context_object.get() }
      };
      module_environment_->js_runner_->InvokeCallback(
          callback, argc, argv, NULL);
    }
  }
  return NS_OK;
}


bool DropTarget::GetDroppedFiles(
    nsIDragSession *drag_session,
    JsArray *files_out,
    std::string16 *error_out) {
#if defined(LINUX) && !defined(OS_MACOSX)
  // Note to future maintainers: the nsIIOService docs say that it may only be
  // used from the main thread. On the other hand, all we're using it for is
  // converting a string like "file:///blah" into a nsIFileURL, rather than
  // doing any actual I/O, so it's probably safe, regardless.
  nsCOMPtr<nsIIOService> io_service =
      do_GetService("@mozilla.org/network/io-service;1");
  if (!io_service) { return false; }

  // Although Firefox's underlying XPCOM widget library aims to present a
  // consistent cross-platform interface, there are still significant
  // differences in drag-and-drop. In particular, different OSes support
  // different "data flavors". On Windows and on Mac, we can specify the
  // kFileMime flavor to get a nsIFile object off the clipboard, but on
  // Linux, that doesn't work, and we have to specify kURLMime instead to
  // get the "file:///home/user/foo.txt" string to convert to a filename.
  // For implementation details, look at Firefox's source code, particularly
  // nsClipboard.cpp and nsDragService.cpp in widget/src/{gtk2,mac,windows}.
  // On platforms, such as Windows, where both kURLMime and kFileMime are
  // supported, then the latter is preferred because it allows us to follow
  // links (e.g. .lnk files on Windows, and alias files on Mac).
  const char *flavor = kURLMime;
#else
  const char *flavor = kFileMime;
#endif

  PRUint32 num_drop_items;
  nsresult nr = drag_session->GetNumDropItems(&num_drop_items);
  if (NS_FAILED(nr) || num_drop_items <= 0) { return false; }

  std::vector<std::string16> filenames;
  for (int i = 0; i < static_cast<int>(num_drop_items); i++) {
    nsCOMPtr<nsITransferable> transferable =
      do_CreateInstance("@mozilla.org/widget/transferable;1", &nr);
    if (NS_FAILED(nr)) { return false; }
    transferable->AddDataFlavor(flavor);
    drag_session->GetData(transferable, i);

    nsCOMPtr<nsISupports> data;
    PRUint32 data_len;
    nr = transferable->GetTransferData(flavor, getter_AddRefs(data), &data_len);
    if (NS_FAILED(nr)) { return false; }

    // Here the code diverges depending on the platform. On Linux, we
    // crack a nsIURI out of the transferable and convert the string
    // to a file path. On Windows and Mac, we get a nsIFile out of the
    // transferable and query it directly for its file path.
#if defined(LINUX) && !defined(OS_MACOSX)
    nsCOMPtr<nsISupportsString> data_as_xpcom_string(do_QueryInterface(data));
    nsString data_as_string;
    data_as_xpcom_string->GetData(data_as_string);

    nsCString data_as_cstring;
    nr = NS_UTF16ToCString(
        data_as_string, NS_CSTRING_ENCODING_UTF8, data_as_cstring);
    nsCOMPtr<nsIURI> uri;
    nr = io_service->NewURI(data_as_cstring, NULL, NULL, getter_AddRefs(uri));
    if (NS_FAILED(nr)) { return false; }

    nsCOMPtr<nsIFileURL> file_url(do_QueryInterface(uri));
    if (!file_url) { return false; }
    nsCOMPtr<nsIFile> file;
    nr = file_url->GetFile(getter_AddRefs(file));
    if (NS_FAILED(nr)) { return false; }
    nsString filename;
    nr = file->GetPath(filename);
    if (NS_FAILED(nr)) { return false; }
#else
    nsCOMPtr<nsIFile> file(do_QueryInterface(data));
    if (!file) { return false; }
    nsString path;
    nr = file->GetPath(path);
    if (NS_FAILED(nr)) { return false; }

    nsCOMPtr<nsILocalFile> local_file =
        do_CreateInstance("@mozilla.org/file/local;1", &nr);
    if (NS_FAILED(nr)) { return false; }
    nr = local_file->SetFollowLinks(PR_TRUE);
    if (NS_FAILED(nr)) { return false; }
    nr = local_file->InitWithPath(path);
    if (NS_FAILED(nr)) { return false; }

    nsString filename;
    PRBool bool_result;
    if (NS_SUCCEEDED(file->IsSymlink(&bool_result)) && bool_result) {
      nr = local_file->GetTarget(filename);
    } else if (NS_SUCCEEDED(file->IsFile(&bool_result)) && bool_result) {
      nr = local_file->GetPath(filename);
    } else {
      nr = NS_ERROR_FAILURE;
    }
    if (NS_FAILED(nr)) { return false; }
#endif

    filenames.push_back(std::string16(filename.get()));
  }

  return FileDialog::FilesToJsObjectArray(
      filenames, module_environment_.get(), files_out, error_out);
}


// A DropTarget instance automatically de-registers itself, on page unload.
void DropTarget::HandleEvent(JsEventType event_type) {
  assert(event_type == JSEVENT_UNLOAD);
  UnregisterSelf();
}


void DropTarget::Ref() {
  AddRef();
}


void DropTarget::Unref() {
  Release();
}


#endif  // OFFICIAL_BUILD
