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


// Must include this before nsGUIEvent_edited_for_Gears.h.
#include <gecko_sdk/include/nsCOMPtr.h>

#include <gecko_internal/nsGUIEvent_edited_for_Gears.h>
#include <gecko_internal/nsIDragService.h>
#include <gecko_internal/nsIPrivateDOMEvent.h>
#include <gecko_internal/nsIXPConnect.h>
#include <gecko_sdk/include/nsIDOMEvent.h>
#include <gecko_sdk/include/nsILocalFile.h>
#include <gecko_sdk/include/nsISupportsPrimitives.h>
#include <gecko_sdk/include/nsXPCOM.h>

#include "gears/desktop/drag_and_drop_utils_ff.h"

#include "gears/base/common/file.h"
#include "gears/base/common/mime_detect.h"
#include "gears/base/firefox/ns_file_utils.h"
#include "gears/desktop/file_dialog.h"


// NOTE(nigeltao): XPConnect is not thread-safe, and this method will not
// work if called from worker threads. Hence, this function is defined here
// and not in the more generally applicable base/common/js_types.cc.
static bool JsvalToISupports(JSContext *context,
                             jsval object_as_jsval,
                             nsISupports **object_as_supports_out) {
  nsresult nr;
  nsCOMPtr<nsIXPConnect> xpc(
      do_GetService("@mozilla.org/js/xpc/XPConnect;1", &nr));
  if (NS_FAILED(nr)) return false;

  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  nr = xpc->GetWrappedNativeOfJSObject(context,
                                       JSVAL_TO_OBJECT(object_as_jsval),
                                       getter_AddRefs(wrapper));
  if (NS_FAILED(nr)) return false;

  nsCOMPtr<nsISupports> isupports = NULL;
  nr = wrapper->GetNative(getter_AddRefs(isupports));
  if (NS_FAILED(nr)) return false;

  *object_as_supports_out = isupports.get();
  (*object_as_supports_out)->AddRef();
  return true;
}


// Determines whether or not a given pointer is to something on the stack
// (as opposed to an object that has been malloc'ed or new'ed, for example).
// To use this function, pass NULL for the second and third arguments.
//
// This is implemented by recursively calling itself (to generate two new
// stack frames) and then checking that the candidate pointer is on the
// correct side of those stack frames.
static bool PointerIsOnTheStack(void *candidate,
                                void *pointer1,
                                void *pointer2) {
#if defined(LINUX) && !defined(OS_MACOSX)
  int something_on_the_stack;
  if (!pointer1) {
    return PointerIsOnTheStack(candidate, &something_on_the_stack, NULL);
  }
  if (!pointer2) {
    return PointerIsOnTheStack(candidate, pointer1, &something_on_the_stack);
  }
  size_t i0 = reinterpret_cast<size_t>(candidate);
  size_t i1 = reinterpret_cast<size_t>(pointer1);
  size_t i2 = reinterpret_cast<size_t>(pointer2);
  // Check that the sequence (i0, i1, i2) is strictly monotonic.
  return (i0 > i1 && i1 > i2) || (i0 < i1 && i1 < i2);
#else
  // TODO(nigeltao): the above works for GCC (i.e. Firefox/Linux and
  // Firefox/OSX), but does not for Visual C++ (i.e. Firefox/Windows).
  // The TODO is to implement this. It isn't a deal-breaker that this doesn't
  // work on Windows, since the if (!ns_mouse_event->widget) check during
  // GetDragAndDropEventType should be sufficient, but still, it would be
  // nice to have, as a second or third line of defence.
  return true;
#endif
}


enum DragAndDropEventType {
  DRAG_AND_DROP_EVENT_DRAGENTER,
  DRAG_AND_DROP_EVENT_DRAGOVER,
  DRAG_AND_DROP_EVENT_DRAGLEAVE,
  DRAG_AND_DROP_EVENT_DROP,
  DRAG_AND_DROP_EVENT_INVALID
};


// Note that some Mozilla event names differ from the HTML5 standard event
// names - the latter being the one we expose in the Gears API. Specifically,
// Mozilla's "dragexit" is HTML5's "dragleave", and "dragdrop" is "drop".
static const nsString kDragEnterAsString(STRING16(L"dragenter"));
static const nsString kDragOverAsString(STRING16(L"dragover"));
static const nsString kDragExitAsString(STRING16(L"dragexit"));
static const nsString kDragDropAsString(STRING16(L"dragdrop"));


// Given a DOMEvent (as a JsObject), return what type of event we are in,
// also checking that we are in event dispatch (and not, for example, a
// situation where the JS keeps a reference to a previously issued event
// and passes that back to Gears outside of event dispatch).
DragAndDropEventType GetDragAndDropEventType(
    ModuleEnvironment *module_environment,
    JsObject *event_as_js_object,
    nsCOMPtr<nsIDOMEvent> *dom_event_out) {
  nsCOMPtr<nsISupports> event_as_supports;
  if (!JsvalToISupports(
          module_environment->js_runner_->GetContext(),
          event_as_js_object->token(),
          getter_AddRefs(event_as_supports))) {
    return DRAG_AND_DROP_EVENT_INVALID;
  }

  nsCOMPtr<nsIPrivateDOMEvent> private_dom_event(
      do_QueryInterface(event_as_supports));
  if (!private_dom_event) { return DRAG_AND_DROP_EVENT_INVALID; }

  nsEvent *ns_event;
  nsresult nr = private_dom_event->GetInternalNSEvent(&ns_event);
  if (NS_FAILED(nr)) { return DRAG_AND_DROP_EVENT_INVALID; }

  if (ns_event->eventStructType != NS_MOUSE_EVENT) {
    return DRAG_AND_DROP_EVENT_INVALID;
  }
  nsMouseEvent *ns_mouse_event = static_cast<nsMouseEvent*>(ns_event);
  if (!ns_mouse_event->widget) { return DRAG_AND_DROP_EVENT_INVALID; }

#if BROWSER_FF2
  // NOTE(nigeltao): For Gecko 1.8 (i.e. Firefox 2), a check for the
  // NS_EVENT_FLAG_DISPATCHING bit is ineffective in determining whether or
  // not a nsEvent instance is in fact in dispatch.
  //
  // This is because nsGlobalWindow::HandleDOMEvent (in
  // mozilla/dom/src/base/nsGlobalWindow.cpp) deals with both a nsEvent and
  // a nsIDOMEvent. At the start of the method, the nsIDOMEvent has a pointer
  // to that nsEvent, and NS_MARK_EVENT_DISPATCH_STARTED is called on the
  // nsEvent, which sets the bit on. At the end of the method,
  // NS_MARK_EVENT_DISPATCH_DONE is called, which is intended to clear that
  // bit. In the mean time, however, nsDOMEvent::DuplicatePrivateData is
  // called, which makes the nsIDOMEvent create a copy of the nsEvent (since
  // the nsEvent will live on the stack and will shortly be destroyed, but
  // the nsIDOMEvent is ref-counted and heap-allocated (it is created during
  // nsEventListenerManager::CreateEvent), and may live longer than the
  // current call chain), and this copy of the nsEvent also copies the
  // "is this nsEvent in dispatch" bit, *at the time of the copy*.
  // NS_MARK_EVENT_DISPATCH_DONE is applied to the original nsEvent, but not
  // to the newer copy of that nsEvent.
  //
  // Thus, after dispatch is complete, the nsIDOMEvent (which is what is
  // passed to this Gears function GetDragAndDropEventType) has a (internal)
  // nsEvent for which the NS_EVENT_FLAG_DISPATCHING can still be set even
  // if that nsEvent is not dispatching.
  //
  // On Firefox 2, then, we do not depend on the NS_EVENT_FLAG_DISPATCHING bit
  // and rely on checking ns_mouse_event->widget (done above), which will be
  // non-NULL during dispatch (as constructed during C++ files such as:
  //   mozilla/widget/src/gtk2/nsWindow.cpp,
  //   mozilla/widget/src/mac/nsMacEventHandler.cpp, and
  //   mozilla/widget/src/windows/nsNativeDragTarget.cpp), and NULL after
  // dispatch (since nsDOMEvent::DuplicatePrivateData copies an nsEvent's
  // fields, but is not sophisticated enough to downcast a nsEvent into a
  // nsGUIEvent and also copy the widget field, and the constructor for the
  // shiny new copy of the nsEvent sets the widget field to NULL).
#else
  // On Gecko 1.9 (i.e. Firefox 3), though, this oversight has been fixed,
  // since the NS_MARK_EVENT_DISPATCH_DONE is called *before* the copy is
  // made, during nsEventDispatcher::Dispatch in
  // mozilla/content/events/src/nsEventDispatcher.cpp, and so this bit-check is
  // a meaningful test. The nsEventDispatcher class is new in Gecko 1.9.
  if ((ns_event->flags & NS_EVENT_FLAG_DISPATCHING) == 0) {
    return DRAG_AND_DROP_EVENT_INVALID;
  }
#endif

  // Despite whether or not the NS_EVENT_FLAG_DISPATCHING bit is meaningful,
  // we can still require that the ns_event is a stack-allocated (and not a
  // heap-allocated) structure, which is true during event dispatch, and
  // false otherwise.
  if (!PointerIsOnTheStack(ns_event, NULL, NULL)) {
    return DRAG_AND_DROP_EVENT_INVALID;
  }

  *dom_event_out = do_QueryInterface(event_as_supports);
  nsString event_type;
  nr = (*dom_event_out)->GetType(event_type);
  if (NS_FAILED(nr)) { return DRAG_AND_DROP_EVENT_INVALID; }

  if (event_type.Equals(kDragOverAsString)) {
    return DRAG_AND_DROP_EVENT_DRAGOVER;
  } else if (event_type.Equals(kDragEnterAsString)) {
#if BROWSER_FF2 && defined(LINUX) && !defined(OS_MACOSX)
    // The intended Gecko drag and drop model is that (1) the DragService
    // starts a DragSession (via StartDragSession()), and then (2) a
    // NS_DRAGDROP_ENTER event is dispatched.
    // The relevant source files (under mozilla/widget/src) where both
    // StartDragSession is called and the NS_DRAGDROP_ENTER event is
    // dispatched are
    //   Linux: gtk2/nsWindow.cpp
    //   Mac: cocoa/nsCocoaWindow.mm (Gecko 1.8), and
    //        cocoa/nsChildView.mm (Gecko 1.9)
    //   Windows: windows/nsNativeDragTarget.cpp
    // Windows and Mac follow this model, on Firefox 2 and Firefox 3, and
    // Linux does this for Firefox 3, but does not for Firefox 2. Instead,
    // Firefox2/Linux will do these two things in reverse order, i.e. first
    // dispatch the NS_DRAGDROP_ENTER event, and then StartDragSession. This
    // is a Gecko bug in nsWindow::OnDragEnter that was fixed in CVS revision
    // 1.149 of mozilla/widget/src/gtk2/nsWindow.cpp, according to
    // http://bonsai.mozilla.org/cvslog.cgi?
    //     file=mozilla/widget/src/gtk2/nsWindow.cpp&rev=1.149
    //
    // Gears, however, needs to access the DragSession when handling
    // drag-enter, to discover file metadata like file count and total size,
    // and hence we workaround this Firefox2/Linux bug by explicitly calling
    // StartDragSession here.
    //
    // Note that nsBaseDragService::StartDragSession is idempotent, in that
    // all it does is to set a private member variable (mDoingDrag) to true.
    // This XPCOM method does return NS_ERROR_FAILURE rather than NS_OK on
    // the second and subsequent invocations, but that has no practical
    // significance because StartDragSession is called in only one place
    // (apart from here in Gears code), namely during nsWindow::OnDragEnter,
    // and in this case, the nsresult return value is ignored.
    nsCOMPtr<nsIDragService> drag_service =
        do_GetService("@mozilla.org/widget/dragservice;1");
    if (drag_service) {
      drag_service->StartDragSession();
    }
#endif
    return DRAG_AND_DROP_EVENT_DRAGENTER;
  } else if (event_type.Equals(kDragExitAsString)) {
    return DRAG_AND_DROP_EVENT_DRAGLEAVE;
  } else if (event_type.Equals(kDragDropAsString)) {
    return DRAG_AND_DROP_EVENT_DROP;
  }
  return DRAG_AND_DROP_EVENT_INVALID;
}


void AcceptDrag(ModuleEnvironment *module_environment,
                JsObject *event_as_js_object,
                bool acceptance,
                std::string16 *error_out) {
  nsCOMPtr<nsIDOMEvent> dom_event;
  DragAndDropEventType type = GetDragAndDropEventType(
      module_environment, event_as_js_object, &dom_event);
  if (type == DRAG_AND_DROP_EVENT_INVALID) {
    *error_out = STRING16(L"The drag-and-drop event is invalid.");
    return;
  }

  if (type == DRAG_AND_DROP_EVENT_DROP) {
    dom_event->StopPropagation();

  } else if (type == DRAG_AND_DROP_EVENT_DRAGENTER ||
             type == DRAG_AND_DROP_EVENT_DRAGOVER) {
    nsCOMPtr<nsIDragService> drag_service =
        do_GetService("@mozilla.org/widget/dragservice;1");
    if (!drag_service) {
      *error_out = GET_INTERNAL_ERROR_MESSAGE();
      return;
    }

    nsCOMPtr<nsIDragSession> drag_session;
    nsresult nr = drag_service->GetCurrentSession(getter_AddRefs(drag_session));
    if (NS_FAILED(nr) || !drag_session.get()) {
      *error_out = GET_INTERNAL_ERROR_MESSAGE();
      return;
    }

    nr = drag_session->SetDragAction(acceptance
        ? static_cast<int>(nsIDragService::DRAGDROP_ACTION_COPY)
        : static_cast<int>(nsIDragService::DRAGDROP_ACTION_NONE));
    if (NS_FAILED(nr)) {
      *error_out = GET_INTERNAL_ERROR_MESSAGE();
      return;
    }
  }
}


static void JsObjectSetPropertyStringArray(
    ModuleEnvironment *module_environment,
    JsObject *data_out,
    const char16 *property_key,
    std::set<std::string16> &strings) {
  scoped_ptr<JsArray> array(module_environment->js_runner_->NewArray());
  int i = 0;
  for (std::set<std::string16>::iterator iter = strings.begin();
       iter != strings.end();
       ++iter) {
    // TODO(nigeltao): Should we skip over empty string values?
    array->SetElementString(i++, *iter);
  }
  data_out->SetPropertyArray(property_key, array.get());
}


void GetDragData(ModuleEnvironment *module_environment,
                 JsObject *event_as_js_object,
                 JsObject *data_out,
                 std::string16 *error_out) {
  nsCOMPtr<nsIDOMEvent> dom_event;
  DragAndDropEventType type = GetDragAndDropEventType(
      module_environment, event_as_js_object, &dom_event);
  if (type == DRAG_AND_DROP_EVENT_INVALID) {
    *error_out = STRING16(L"The drag-and-drop event is invalid.");
    return;
  }

  // TODO(nigeltao): Should we early exit for DRAG_AND_DROP_EVENT_DRAGLEAVE
  // (and not provide e.g. fileCount, fileTotalBytes, etcetera)? The answer
  // probably depends on what is feasible (e.g. can we distinguish a dragover
  // and a dragleave on Safari) on other browser/OS combinations.

  nsCOMPtr<nsIDragService> drag_service =
      do_GetService("@mozilla.org/widget/dragservice;1");
  if (!drag_service) {
    *error_out = GET_INTERNAL_ERROR_MESSAGE();
    return;
  }

  nsCOMPtr<nsIDragSession> drag_session;
  nsresult nr = drag_service->GetCurrentSession(getter_AddRefs(drag_session));
  if (NS_FAILED(nr) || !drag_session.get()) {
    *error_out = GET_INTERNAL_ERROR_MESSAGE();
    return;
  }

  // TODO(nigeltao): cache the filenames (and their MIME types, extensions,
  // etcetera), to avoid hitting the file system on every DnD event.
  // This probably requires being able to reliably distinguish when a
  // drag session ends and a new one begins, which might not be trivial if
  // you get the same nsIDragSession pointer (since it's just the singleton
  // DragService that's been QueryInterface'd) for two separate sessions.
  std::vector<std::string16> filenames;
  std::set<std::string16> file_extensions;
  std::set<std::string16> file_mime_types;
  int64 fileTotalBytes = 0;
  if (!GetDroppedFiles(module_environment,
                       drag_session.get(),
                       &filenames,
                       &file_extensions,
                       &file_mime_types,
                       &fileTotalBytes)) {
    // If GetDroppedFiles fails, then GetDroppedFiles may have added partial
    // results to the filenames vector, and other accumulators, which we
    // should clear out.
    // TODO(nigeltao): Should we throw an error here, or just happily return
    // empty? How should we behave if the user drags and drops Text or a URL?
    filenames.clear();
    file_extensions.clear();
    file_mime_types.clear();
    fileTotalBytes = 0;
  }

  data_out->SetPropertyInt(STRING16(L"fileCount"), filenames.size());
  data_out->SetPropertyDouble(STRING16(L"fileTotalBytes"), fileTotalBytes);
  JsObjectSetPropertyStringArray(module_environment, data_out,
                                 STRING16(L"fileExtensions"), file_extensions);
  JsObjectSetPropertyStringArray(module_environment, data_out,
                                 STRING16(L"fileMimeTypes"), file_mime_types);

  if (type == DRAG_AND_DROP_EVENT_DROP) {
    scoped_ptr<JsArray> file_array(
        module_environment->js_runner_->NewArray());
    if (!FileDialog::FilesToJsObjectArray(filenames, module_environment,
                                          file_array.get(), error_out)) {
      // FilesToJsObjectArray will set error_out.
      return;
    }
    data_out->SetPropertyArray(STRING16(L"files"), file_array.get());
  }
}


bool GetDroppedFiles(
    ModuleEnvironment *module_environment,
    nsIDragSession *drag_session,
    std::vector<std::string16> *filenames_out,
    std::set<std::string16> *file_extensions_out,
    std::set<std::string16> *file_mime_types_out,
    int64 *file_total_bytes_out) {
  filenames->clear();
  file_extensions->clear();
  file_mime_types->clear();
  *fileTotalBytes = 0;
#if defined(LINUX) && !defined(OS_MACOSX)
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
    nr = data_as_xpcom_string->GetData(data_as_string);
    if (NS_FAILED(nr)) { return false; }
    nsCOMPtr<nsIFile> file;
    nr = NSFileUtils::GetFileFromURLSpec(data_as_string, getter_AddRefs(file));
    if (NS_FAILED(nr)) { return false; }
    nsString filename;
    nr = file->GetPath(filename);
    if (NS_FAILED(nr)) { return false; }
    PRInt64 file_size = 0;
    nr = file->GetFileSize(&file_size);
    if (NS_FAILED(nr)) { return false; }
#else
    nsCOMPtr<nsIFile> file(do_QueryInterface(data));
    if (!file) { return false; }
    nsString path;
    nr = file->GetPath(path);
    if (NS_FAILED(nr)) { return false; }

    PRBool bool_result = false;
    if (NS_FAILED(file->IsDirectory(&bool_result)) || bool_result) {
      continue;
    }

    nsCOMPtr<nsILocalFile> local_file =
        do_CreateInstance("@mozilla.org/file/local;1", &nr);
    if (NS_FAILED(nr)) { return false; }
    nr = local_file->SetFollowLinks(PR_TRUE);
    if (NS_FAILED(nr)) { return false; }
    nr = local_file->InitWithPath(path);
    if (NS_FAILED(nr)) { return false; }

    nr = NS_ERROR_FAILURE;
    nsString filename;
    if (NS_SUCCEEDED(file->IsSymlink(&bool_result)) && bool_result) {
      nr = local_file->GetTarget(filename);
    } else if (NS_SUCCEEDED(file->IsFile(&bool_result)) && bool_result) {
      nr = local_file->GetPath(filename);
    }
    if (NS_FAILED(nr)) { return false; }
    // TODO(nigeltao): Check that this gets the size of the link target,
    // in the case of .lnk (Windows) or alias (OSX) files.
    PRInt64 file_size = 0;
    nr = local_file->GetFileSize(&file_size);
    if (NS_FAILED(nr)) { return false; }
#endif

    filenames_out->push_back(std::string16(filename.get()));
    // TODO(nigeltao): Decide whether we should insert ".txt" or "txt" -
    // that is, does the file extension include the dot at the start.
    file_extensions_out->insert(File::GetFileExtension(filename.get()));
    // TODO(nigeltao): Should we also keep an array of per-file MIME types,
    // not just the overall set of MIME types. If so, the mimeType should
    // probably be a property of the file JavaScript object (i.e. the thing
    // with a name and blob property), not a separate array to the files array.
    file_mime_types_out->insert(DetectMimeTypeOfFile(filename.get()));
    *file_total_bytes_out += file_size;
  }
  return true;
}
