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

#import <Cocoa/Cocoa.h>
#import <WebKit/DOM.h>
#import <WebKit/WebView.h>
#import <objc/objc-class.h>

#import "gears/desktop/drag_and_drop_utils_sf.h"

#import "gears/base/common/common.h"
#import "gears/base/safari/nsstring_utils.h"
#import "gears/desktop/file_dialog.h"

static FileDragAndDropMetaData *g_file_drag_and_drop_meta_data = NULL;
static NSInteger g_last_seen_dragging_sequence_number = 0;
static bool g_is_in_a_drag_operation = false;
static bool g_is_in_a_drop_operation = false;
// The next two variables are for when the page's JavaScript explicitly
// accepts or rejects the drag, via desktop.acceptDrag(..., bool).
// In either case, g_drag_operation_has_been_set becomes true, and
// g_drag_operation takes NSDragOperationCopy or NSDragOperationNone.
static bool g_drag_operation_has_been_set = false;
static NSDragOperation g_drag_operation = NSDragOperationNone;

bool IsInADragOperation() {
  return g_is_in_a_drag_operation;
}

bool IsInADropOperation() {
  return g_is_in_a_drop_operation;
}

// This function is based on http://developer.apple.com/documentation/Cocoa/
//   Conceptual/LowLevelFileMgmt/Tasks/ResolvingAliases.html
void ResolveAlias(NSString *path, std::string16 *resolved_path_out) {
  NSString *resolved_path = path;
  CFURLRef url = CFURLCreateWithFileSystemPath(
      kCFAllocatorDefault, (CFStringRef)path, kCFURLPOSIXPathStyle, NO);
  if (url != NULL){
    FSRef fs_ref;
    if (CFURLGetFSRef(url, &fs_ref)){
      Boolean ignored, was_aliased;
      OSErr err = FSResolveAliasFile(&fs_ref, true, &ignored, &was_aliased);
      if ((err == noErr) && was_aliased) {
        CFURLRef resolved_url =
            CFURLCreateFromFSRef(kCFAllocatorDefault, &fs_ref);
        if (resolved_url != NULL) {
          resolved_path = (NSString*)
              CFURLCopyFileSystemPath(resolved_url, kCFURLPOSIXPathStyle);
          CFRelease(resolved_url);
        }
      }
    }
    CFRelease(url);
  }
  [resolved_path string16:resolved_path_out];
  if (resolved_path != path) {
    // If resolved_path is different, then it must have been set to the result
    // of CFURLCopyFileSystemPath, and since it is a copy, we should release
    // it when we're done with it.
    CFRelease(resolved_path);
  }
}

// We "swizzle" some Cocoa method implementations to insert a little code
// before and after certain events get processed. For more, see
// http://www.cocoadev.com/index.pl?MethodSwizzling
bool MethodSwizzle(Class klass, SEL old_selector, SEL new_selector) {
  Method old_method = class_getInstanceMethod(klass, old_selector);
  Method new_method = class_getInstanceMethod(klass, new_selector);
  bool result = false;
  if (old_method && new_method) {
    char *temp1 = old_method->method_types;
    old_method->method_types = new_method->method_types;
    new_method->method_types = temp1;

    IMP temp2 = old_method->method_imp;
    old_method->method_imp = new_method->method_imp;
    new_method->method_imp = temp2;
    result = true;
  }
#ifdef DEBUG
  NSLog(@"Gears: Swizzling \"%@\" for class %@ %@",
        NSStringFromSelector(old_selector),
        NSStringFromClass(klass),
        result ? @"succeeded" : @"failed");
#endif
  return result;
}

@interface WebView (GearsSwizzledMethods)
- (void)updateFromDraggingInfo:(id <NSDraggingInfo>)draggingInfo;
- (NSDragOperation)swizzledDraggingEntered:(id <NSDraggingInfo>)draggingInfo;
- (NSDragOperation)swizzledDraggingUpdated:(id <NSDraggingInfo>)draggingInfo;
- (NSDragOperation)swizzledDraggingExited:(id <NSDraggingInfo>)draggingInfo;
- (BOOL)swizzledPerformDragOperation:(id <NSDraggingInfo>)draggingInfo;
@end

@implementation WebView (GearsSwizzledMethods)

- (void)updateFromDraggingInfo:(id <NSDraggingInfo>)draggingInfo {
  NSInteger seqno = [draggingInfo draggingSequenceNumber];
  if (g_last_seen_dragging_sequence_number == seqno) {
    return;
  }
  g_last_seen_dragging_sequence_number = seqno;
  if (!g_file_drag_and_drop_meta_data) {
    g_file_drag_and_drop_meta_data = new FileDragAndDropMetaData;
  }
  g_file_drag_and_drop_meta_data->Reset();

  // In Safari, arbitrary web pages can put on the pasteboard during an ondrag
  // event, simply by calling window.event.dataTransfer.setData('URL',
  // 'file:///some/file'). If we did not distinguish such drag sources, then
  // a malicious web page could access the local file system, with the help of
  // Gears drag and drop, by tricking a user to drag one image on the page and
  // dropping it on another image on the same page.
  // To prevent this, we require that the source of the drag be from a
  // separate application (i.e. it is in a separate process' address
  // space), and hence [draggingInfo draggingSource] will return nil rather
  // than a pointer to something in the address space of this Safari process
  // (or more specifically, a pointer to something caused by a web page viewed
  // in Safari). If Safari ever went to a multi-process architecture, then we
  // would have to revisit this guard.
  BOOL is_drag_from_another_application =
      ([draggingInfo draggingSource] == nil);

  if (is_drag_from_another_application) {
    std::vector<std::string16> filenames;
    NSPasteboard *pboard = [draggingInfo draggingPasteboard];
    if ([[pboard types] containsObject:NSFilenamesPboardType]) {
      NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
      NSEnumerator *enumerator = [files objectEnumerator];
      NSString *ns_string;
      while ((ns_string = [enumerator nextObject])) {
        std::string16 std_string;
        ResolveAlias(ns_string, &std_string);
        filenames.push_back(std_string);
      }
    }
    g_file_drag_and_drop_meta_data->SetFilenames(filenames);
  }
}

- (NSDragOperation)swizzledDraggingEntered:(id <NSDraggingInfo>)draggingInfo {
  [self updateFromDraggingInfo:draggingInfo];
  g_is_in_a_drag_operation = true;
  g_drag_operation_has_been_set = false;
  NSDragOperation result = [self swizzledDraggingEntered:draggingInfo];
  g_is_in_a_drag_operation = false;
  return g_drag_operation_has_been_set ? g_drag_operation : result;
}

- (NSDragOperation)swizzledDraggingUpdated:(id <NSDraggingInfo>)draggingInfo {
  [self updateFromDraggingInfo:draggingInfo];
  g_is_in_a_drag_operation = true;
  g_drag_operation_has_been_set = false;
  NSDragOperation result = [self swizzledDraggingUpdated:draggingInfo];
  g_is_in_a_drag_operation = false;
  return g_drag_operation_has_been_set ? g_drag_operation : result;
}

- (NSDragOperation)swizzledDraggingExited:(id <NSDraggingInfo>)draggingInfo {
  [self updateFromDraggingInfo:draggingInfo];
  g_is_in_a_drag_operation = true;
  g_drag_operation_has_been_set = false;
  NSDragOperation result = [self swizzledDraggingExited:draggingInfo];
  g_is_in_a_drag_operation = false;
  return g_drag_operation_has_been_set ? g_drag_operation : result;
}

- (BOOL)swizzledPerformDragOperation:(id <NSDraggingInfo>)draggingInfo {
  [self updateFromDraggingInfo:draggingInfo];
  g_is_in_a_drop_operation = true;
  BOOL result = [self swizzledPerformDragOperation:draggingInfo];
  g_is_in_a_drop_operation = false;
  return result;
}

@end

bool SwizzleWebViewMethods() {
  SEL selectors[] = {
    @selector(draggingEntered:),
    @selector(swizzledDraggingEntered:),
    @selector(draggingUpdated:),
    @selector(swizzledDraggingUpdated:),
    @selector(draggingExited:),
    @selector(swizzledDraggingExited:),
    @selector(performDragOperation:),
    @selector(swizzledPerformDragOperation:),
  };
  for (unsigned int i = 0; i < ARRAYSIZE(selectors); i += 2) {
    if (!MethodSwizzle([WebView class], selectors[i], selectors[i + 1])) {
      return false;
    }
  }
  return true;
}

bool AddFileDragAndDropData(ModuleEnvironment *module_environment,
                            JsObject *data_out,
                            std::string16 *error_out) {
  return g_file_drag_and_drop_meta_data->ToJsObject(
      module_environment, IsInADropOperation(), data_out, error_out);
};

void AcceptDrag(ModuleEnvironment *module_environment,
                JsObject *event,
                bool acceptance,
                std::string16 *error_out) {
  DragAndDropCursorType cursor_type = acceptance
      ? DRAG_AND_DROP_CURSOR_COPY
      : DRAG_AND_DROP_CURSOR_NONE;
  SetDragCursor(module_environment, event, cursor_type, error_out);
  if (error_out->empty()) {
    event->SetPropertyBool(STRING16(L"returnValue"), false);
  }
}

bool GetDragData(ModuleEnvironment *module_environment,
                 JsObject *event_as_js_object,
                 JsObject *data_out,
                 std::string16 *error_out) {
  // We ignore event_as_js_object, since Gears (being restricted to the NPAPI
  // interface) cannot distinguish between a drag and drop event genuinely
  // inside dispatch, and an event that has had dispatchEvent called on,
  // outside of a user drag and drop action.
  // Instead, we rely on our IsInAXxxxOperation tests to tell whether or not
  // we are called during a user drag and drop action.
  if (!IsInADragOperation() && !IsInADropOperation()) {
    *error_out = STRING16(L"The drag-and-drop event is invalid.");
    return false;
  }
  return AddFileDragAndDropData(module_environment, data_out, error_out);
}

void SetDragCursor(ModuleEnvironment *module_environment,
                   JsObject *event,
                   DragAndDropCursorType cursor_type,
                   std::string16 *error_out) {
  // As per GetDragData below, we rely on our IsInAXxxxOperation tests to tell
  // whether or not we are called during a user drag and drop action.
  if (!IsInADragOperation() && !IsInADropOperation()) {
    *error_out = STRING16(L"The drag-and-drop event is invalid.");
    return;
  }
  if (!IsInADropOperation()) {
    if (cursor_type == DRAG_AND_DROP_CURSOR_COPY) {
      g_drag_operation_has_been_set = true;
      g_drag_operation = NSDragOperationCopy;
    } else if (cursor_type == DRAG_AND_DROP_CURSOR_NONE) {
      g_drag_operation_has_been_set = true;
      g_drag_operation = NSDragOperationNone;
    } else {
      assert(cursor_type == DRAG_AND_DROP_CURSOR_INVALID);
    }
  }
}
