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

#if defined(LINUX) && !defined(OS_MACOSX)
#include <gtk/gtk.h>
#endif

#include "gears/desktop/drag_and_drop_utils_ff.h"

#include "gears/base/common/file.h"
#include "gears/base/common/mime_detect.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/firefox/ns_file_utils.h"
#include "gears/desktop/file_dialog.h"

#ifdef WIN32
#include "gears/desktop/drag_and_drop_utils_win32.h"
#endif

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


// Note that some Mozilla event names differ from the HTML5 standard event
// names - the latter being the one we expose in the Gears API. Specifically,
// Mozilla's "dragexit" is HTML5's "dragleave", and "dragdrop" is "drop".
static const nsString kDragEnterAsString(STRING16(L"dragenter"));
static const nsString kDragOverAsString(STRING16(L"dragover"));
static const nsString kDragExitAsString(STRING16(L"dragexit"));
static const nsString kDragDropAsString(STRING16(L"dragdrop"));

#if BROWSER_FF31
// Firefox 3.5 (and above) use the HTML5 event names: "dragleave", "drop".
static const nsString kDragLeaveAsString(STRING16(L"dragleave"));
static const nsString kDropAsString(STRING16(L"drop"));
#endif

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
  nsresult nr;
#if BROWSER_FF31
  ns_event = private_dom_event->GetInternalNSEvent();
  if (!ns_event) { return DRAG_AND_DROP_EVENT_INVALID; }
#else
  nr = private_dom_event->GetInternalNSEvent(&ns_event);
  if (NS_FAILED(nr)) { return DRAG_AND_DROP_EVENT_INVALID; }
#endif

#if BROWSER_FF31
  if (ns_event->eventStructType != NS_DRAG_EVENT) {
    return DRAG_AND_DROP_EVENT_INVALID;
  }
#else
  if (ns_event->eventStructType != NS_MOUSE_EVENT) {
    return DRAG_AND_DROP_EVENT_INVALID;
  }
#endif

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
#if BROWSER_FF31
  } else if (event_type.Equals(kDragLeaveAsString)) {
    return DRAG_AND_DROP_EVENT_DRAGLEAVE;
  } else if (event_type.Equals(kDropAsString)) {
    return DRAG_AND_DROP_EVENT_DROP;
  }
#else
  } else if (event_type.Equals(kDragExitAsString)) {
    return DRAG_AND_DROP_EVENT_DRAGLEAVE;
  } else if (event_type.Equals(kDragDropAsString)) {
    return DRAG_AND_DROP_EVENT_DROP;
  }
#endif

  return DRAG_AND_DROP_EVENT_INVALID;
}


void SetDragCursor(ModuleEnvironment *module_environment,
                   JsObject *event_as_js_object,
                   DragAndDropCursorType cursor_type,
                   std::string16 *error_out) {
  nsCOMPtr<nsIDOMEvent> dom_event;
  DragAndDropEventType type = GetDragAndDropEventType(
      module_environment, event_as_js_object, &dom_event);
  if (type == DRAG_AND_DROP_EVENT_INVALID) {
    *error_out = STRING16(L"The drag-and-drop event is invalid.");
    return;
  }
  if (type == DRAG_AND_DROP_EVENT_DROP ||
      cursor_type == DRAG_AND_DROP_CURSOR_INVALID) {
    return;
  }

#ifdef WIN32
  if (module_environment->drop_target_interceptor_) {
    module_environment->drop_target_interceptor_->SetDragCursor(cursor_type);
  }

#else
  // TODO(nigeltao): Check that this works on Mac, not just Linux.
  if (type == DRAG_AND_DROP_EVENT_DRAGENTER ||
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

    if (cursor_type == DRAG_AND_DROP_CURSOR_COPY) {
      nr = drag_session->SetDragAction(nsIDragService::DRAGDROP_ACTION_COPY);
    } else if (cursor_type == DRAG_AND_DROP_CURSOR_NONE) {
      nr = drag_session->SetDragAction(nsIDragService::DRAGDROP_ACTION_NONE);
    } else {
      assert(false);
    }
    if (NS_FAILED(nr)) {
      *error_out = GET_INTERNAL_ERROR_MESSAGE();
      return;
    }
  }
#endif
}


bool GetDragData(ModuleEnvironment *module_environment,
                 JsObject *event_as_js_object,
                 JsObject *data_out,
                 std::string16 *error_out) {
  nsCOMPtr<nsIDOMEvent> dom_event;
  DragAndDropEventType type = GetDragAndDropEventType(
      module_environment, event_as_js_object, &dom_event);
  if (type == DRAG_AND_DROP_EVENT_INVALID) {
    *error_out = STRING16(L"The drag-and-drop event is invalid.");
    return false;
  }

  // TODO(nigeltao): Should we early exit for DRAG_AND_DROP_EVENT_DRAGLEAVE
  // (and not provide e.g. fileCount, fileTotalBytes, etcetera)? The answer
  // probably depends on what is feasible (e.g. can we distinguish a dragover
  // and a dragleave on Safari) on other browser/OS combinations.

  nsCOMPtr<nsIDragService> drag_service =
      do_GetService("@mozilla.org/widget/dragservice;1");
  if (!drag_service) {
    *error_out = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  nsCOMPtr<nsIDragSession> drag_session;
  nsresult nr = drag_service->GetCurrentSession(getter_AddRefs(drag_session));
  if (NS_FAILED(nr) || !drag_session.get()) {
    *error_out = GET_INTERNAL_ERROR_MESSAGE();
    return false;
  }

  return AddFileDragAndDropData(module_environment,
                                drag_session.get(),
                                type,
                                data_out,
                                error_out);
}


#if defined(LINUX) && !defined(OS_MACOSX)
// On Firefox2 / GTK / Linux, nsIDragSession->GetData is only valid during
// dragover and drop events (and in particular not during dragenter events),
// since it is only during the first two events that Gecko calls
// dragSessionGTK->TargetSetLastContext with a valid GTK widget, in
// widget/src/gtk2/nsWindow.cpp.
//
// Nonetheless, we still want to provide (aggregate) file metadata via Gears'
// JavaScript API during dragenter and dragover. To workaround the lack of a
// nsIDragSession, we interrogate the X server directly about its
// XdndSelection, via GTK / GDK.
//
// GTK emits "signals" for UI events such as drag_motion and drag_leave.
// Gecko adds itself as the first handler of such events on the main GTK
// window (in mozilla/widget/src/gtk2/nsWindow.cpp), and signal handlers
// can return FALSE to stop the signal propagating to later handlers that
// have been connected via g_signal_connect.
//
// In order to be notified of this signal *before* the nsWindow is, we install
// an "emission hook", exactly once per application, and never remove it.
// The "emission hook" is not tied to any particular GtkWidget, and always
// returns TRUE (because returning FALSE will uninstall the hook). The
// emission hook runs before conventional signal handlers, and in it we query
// the X server for what's being dragged over the application.
static enum {
  XDND_SELECTION_STATE_NONE,
  XDND_SELECTION_STATE_REQUESTING_CLIPBOARD,
  XDND_SELECTION_STATE_AWAITING_CLIPBOARD,
  XDND_SELECTION_STATE_TIMED_OUT,
  XDND_SELECTION_STATE_REPLY_RECEIVED
} g_xdnd_selection_state = XDND_SELECTION_STATE_NONE;


static scoped_ptr<FileDragAndDropMetaData> g_file_drag_and_drop_meta_data;


// When we request clipboard information on the first drag_motion event (i.e.
// during drag enter), GTK will call back here.
static gboolean on_selection(GtkWidget *widget,
                             GtkSelectionData *selection_data,
                             guint time,
                             gpointer user_data) {
  if (g_xdnd_selection_state != XDND_SELECTION_STATE_AWAITING_CLIPBOARD) {
    // If we are not AWAITING_CLIPBOARD, then we have either TIMED_OUT, or
    // the drag is from the same process and we ignore it.
    if (g_xdnd_selection_state == XDND_SELECTION_STATE_REQUESTING_CLIPBOARD) {
      g_xdnd_selection_state = XDND_SELECTION_STATE_REPLY_RECEIVED;
    }
    return TRUE;
  }
  g_xdnd_selection_state = XDND_SELECTION_STATE_REPLY_RECEIVED;
  if (selection_data->data == NULL) {
    // The selection does not contain text/uri-list data (i.e. files).
    return TRUE;
  }

  std::vector<std::string> filenames_as_ascii_urls;
  std::string uri_list(reinterpret_cast<const char*>(selection_data->data));
  // Even though Unix line endings are traditionally \n and not \r\n, the
  // text-uri format specifies \r\n as delimeters.
  Tokenize<std::string>(uri_list, "\r\n", &filenames_as_ascii_urls);

  // Convert from file:// URLs to an actual filename (which also involves
  // decoding %20s into spaces, for example).
  std::vector<std::string16> filenames;
  for (std::vector<std::string>::iterator i = filenames_as_ascii_urls.begin();
       i != filenames_as_ascii_urls.end(); ++i) {
    std::string16 filename16;
    UTF8ToString16((*i).c_str(), &filename16);

    nsString filename_nsstring(filename16.c_str(), filename16.length());
    nsCOMPtr<nsIFile> file;
    nsresult nr = NSFileUtils::GetFileFromURLSpec(
        filename_nsstring, getter_AddRefs(file));
    if (NS_FAILED(nr)) { filenames.clear(); break; }
    nsString filename;
    nr = file->GetPath(filename);
    if (NS_FAILED(nr)) { filenames.clear(); break; }

    PRBool bool_result;
    nr = file->IsDirectory(&bool_result);
    if (NS_FAILED(nr) || bool_result) { filenames.clear(); break; }
    nr = file->IsSpecial(&bool_result);
    if (NS_FAILED(nr) || bool_result) { filenames.clear(); break; }
    filenames.push_back(std::string16(filename.get()));
  }

  assert(g_file_drag_and_drop_meta_data->IsEmpty());
  g_file_drag_and_drop_meta_data->SetFilenames(filenames);
  return TRUE;
}


static gboolean on_drag_motion_signal_hook(GSignalInvocationHint *ihint,
                                           guint n_param_values,
                                           const GValue *param_values,
                                           gpointer data) {
  if (g_xdnd_selection_state != XDND_SELECTION_STATE_NONE) {
    return TRUE;
  }

  // Create an off-screen window in order to talk to the X server.
  static GtkWidget *selection_widget = NULL;
  static GdkAtom atom1 = GDK_NONE;
  static GdkAtom atom2 = GDK_NONE;
  if (!selection_widget) {
    selection_widget = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_screen(GTK_WINDOW(selection_widget),
                          gtk_widget_get_screen(selection_widget));
    gtk_window_resize(GTK_WINDOW(selection_widget), 1, 1);
    gtk_window_move(GTK_WINDOW(selection_widget), -100, -100);
    gtk_widget_show(selection_widget);
    g_signal_connect(
        selection_widget, "selection_received", G_CALLBACK(on_selection), NULL);
    atom1 = gdk_atom_intern("XdndSelection", FALSE);
    atom2 = gdk_atom_intern("text/uri-list", FALSE);
  }

  g_file_drag_and_drop_meta_data->Reset();
  g_xdnd_selection_state = XDND_SELECTION_STATE_REQUESTING_CLIPBOARD;
  gtk_selection_convert(selection_widget, atom1, atom2, GDK_CURRENT_TIME);

  // If the gtk_selection_convert call above immediately triggers the
  // selection_received signal (which changes g_xdnd_selection_state), then
  // the drag source is in the same process (i.e. it is from the Firefox
  // chrome, or it is from a web page), and we don't trust the source (in case
  // an arbitrary web page added a "file:///etc/passwd" URL as a drag source
  // to try to read the user's file system). Instead, we only accept a drag
  // source if it is from a separate process, such as a file manager.
  if (g_xdnd_selection_state != XDND_SELECTION_STATE_REQUESTING_CLIPBOARD) {
    return TRUE;
  }

  g_xdnd_selection_state = XDND_SELECTION_STATE_AWAITING_CLIPBOARD;
  PRTime entryTime = PR_Now();
  while (g_xdnd_selection_state == XDND_SELECTION_STATE_AWAITING_CLIPBOARD) {
    // We're copying Gecko, which sleeps for 20ms between iterations (as seen
    // in mozilla/widget/src/gtk2/nsDragService.cpp).
    usleep(20);
    // Gecko's NS_DND_TIMEOUT is 500ms, or equivalently, 500000 microseconds.
    if (PR_Now() - entryTime > 500000) {
      g_xdnd_selection_state = XDND_SELECTION_STATE_TIMED_OUT;
      break;
    }
    gtk_main_iteration();
  }

  return TRUE;
}

static gboolean on_drag_leave_signal_hook(GSignalInvocationHint *ihint,
                                          guint n_param_values,
                                          const GValue *param_values,
                                          gpointer data) {
  g_xdnd_selection_state = XDND_SELECTION_STATE_NONE;
  return TRUE;
}


void InitializeGtkSignalEmissionHooks() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;
  g_signal_add_emission_hook(
      g_signal_lookup("drag_motion", GTK_TYPE_WIDGET), 0,
      on_drag_motion_signal_hook, NULL, NULL);
  g_signal_add_emission_hook(
      g_signal_lookup("drag_leave", GTK_TYPE_WIDGET), 0,
      on_drag_leave_signal_hook, NULL, NULL);
  g_file_drag_and_drop_meta_data.reset(new FileDragAndDropMetaData);
}
#endif  // defined(LINUX) && !defined(OS_MACOSX)


bool AddFileDragAndDropData(ModuleEnvironment *module_environment,
                            nsIDragSession *drag_session,
                            DragAndDropEventType type,
                            JsObject *data_out,
                            std::string16 *error_out) {
  assert(type != DRAG_AND_DROP_EVENT_INVALID);
  if (type == DRAG_AND_DROP_EVENT_DRAGLEAVE) {
    return false;
  }

#if defined(LINUX) && !defined(OS_MACOSX)
  // On Linux, we ignore the nsIDragSession, and instead query the
  // g_file_drag_and_drop_meta_data object that was set up earlier, during
  // our signal emission hooks for GTK's "drag-motion" signal.
  return g_file_drag_and_drop_meta_data->ToJsObject(
      module_environment,
      type == DRAG_AND_DROP_EVENT_DROP,
      data_out,
      error_out);
#else

#if defined(WIN32)
  if (!module_environment->drop_target_interceptor_) {
    return false;
  }

  // On Windows, we do a similar thing -- the DropTargetInterceptor has the
  // FileDragAndDropMetaData object.
  if (module_environment->drop_target_interceptor_->
          GetCachedMetaDataIsValid()) {
    return module_environment->drop_target_interceptor_->
        GetFileDragAndDropMetaData().ToJsObject(
            module_environment,
            type == DRAG_AND_DROP_EVENT_DROP,
            data_out,
            error_out);
  }
#endif

  PRUint32 num_drop_items;
  nsresult nr = drag_session->GetNumDropItems(&num_drop_items);
  if (NS_FAILED(nr) || num_drop_items <= 0) { return false; }

  bool has_files = false;
  std::vector<std::string16> filenames;
  for (int i = 0; i < static_cast<int>(num_drop_items); i++) {
    nsCOMPtr<nsITransferable> transferable =
      do_CreateInstance("@mozilla.org/widget/transferable;1", &nr);
    if (NS_FAILED(nr)) { filenames.clear(); break; }
    transferable->AddDataFlavor(kFileMime);
    drag_session->GetData(transferable, i);

    nsCOMPtr<nsISupports> data;
    PRUint32 data_len;
    nr = transferable->GetTransferData(
        kFileMime, getter_AddRefs(data), &data_len);
    if (NS_FAILED(nr)) { filenames.clear(); break; }

    nsCOMPtr<nsIFile> file(do_QueryInterface(data));
    if (!file) { filenames.clear(); break;; }
    nsString path;
    nr = file->GetPath(path);
    if (NS_FAILED(nr)) { filenames.clear(); break; }

    has_files = true;

    nsCOMPtr<nsILocalFile> local_file =
        do_CreateInstance("@mozilla.org/file/local;1", &nr);
    if (NS_FAILED(nr)) { filenames.clear(); break; }
    nr = local_file->SetFollowLinks(PR_TRUE);
    if (NS_FAILED(nr)) { filenames.clear(); break; }
    nr = local_file->InitWithPath(path);
    if (NS_FAILED(nr)) { filenames.clear(); break; }

    nr = NS_ERROR_FAILURE;
    nsString filename;
    PRBool bool_result = false;
    if (NS_SUCCEEDED(file->IsSymlink(&bool_result)) && bool_result) {
      nr = local_file->GetTarget(filename);
    } else if (NS_SUCCEEDED(file->IsFile(&bool_result)) && bool_result) {
      nr = local_file->GetPath(filename);
    }
    if (NS_FAILED(nr)) { filenames.clear(); break; }

    bool_result = false;
    if (NS_FAILED(local_file->IsDirectory(&bool_result)) || bool_result) {
      filenames.clear();
      break;
    }
#if defined(OS_MACOSX)
    // Mozilla's nsIFile::IsSpecial is not implemented for Mac OS X, and
    // always fails (returning NS_ERROR_NOT_IMPLEMENTED).
#else
    bool_result = false;
    if (NS_FAILED(local_file->IsSpecial(&bool_result)) || bool_result) {
      filenames.clear();
      break;
    }
#endif

    filenames.push_back(std::string16(filename.get()));
  }

#if defined(WIN32)
  FileDragAndDropMetaData &file_drag_and_drop_meta_data =
      module_environment->drop_target_interceptor_->
          GetFileDragAndDropMetaData();
  module_environment->drop_target_interceptor_->SetCachedMetaDataIsValid(true);
#else
  FileDragAndDropMetaData file_drag_and_drop_meta_data;
#endif
  if (has_files) {
    file_drag_and_drop_meta_data.SetFilenames(filenames);
  }
  return file_drag_and_drop_meta_data.ToJsObject(
          module_environment,
          type == DRAG_AND_DROP_EVENT_DROP,
          data_out,
          error_out);
#endif
}
