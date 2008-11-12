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
#import <vector>

#import "gears/desktop/drag_and_drop_utils_osx.h"

#import "gears/base/common/common.h"
#import "gears/base/safari/nsstring_utils.h"
#import "gears/desktop/file_dialog.h"

static std::vector<std::string16> g_dragging_pasteboard_filenames_;
static bool g_is_in_a_drag_operation_ = false;
static bool g_is_in_a_drop_operation_ = false;

bool IsInADragOperation() {
  return g_is_in_a_drag_operation_;
}

bool IsInADropOperation() {
  return g_is_in_a_drop_operation_;
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
- (NSDragOperation)swizzledDraggingEntered:(id <NSDraggingInfo>)draggingInfo;
- (NSDragOperation)swizzledDraggingUpdated:(id <NSDraggingInfo>)draggingInfo;
- (NSDragOperation)swizzledDraggingExited:(id <NSDraggingInfo>)draggingInfo;
- (BOOL)swizzledPerformDragOperation:(id <NSDraggingInfo>)draggingInfo;
@end

@implementation WebView (GearsSwizzledMethods)

- (NSDragOperation)swizzledDraggingEntered:(id <NSDraggingInfo>)draggingInfo {
  g_dragging_pasteboard_filenames_.clear();

  NSPasteboard *pboard = [draggingInfo draggingPasteboard];
  if ([[pboard types] containsObject:NSFilenamesPboardType]) {
    NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
    NSEnumerator *enumerator = [files objectEnumerator];
    while (NSString *ns_string = [enumerator nextObject]) {
      std::string16 std_string;
      [ns_string string16:&std_string];
      g_dragging_pasteboard_filenames_.push_back(std_string);
    }
  }

  return [self swizzledDraggingEntered:draggingInfo];
}

- (NSDragOperation)swizzledDraggingUpdated:(id <NSDraggingInfo>)draggingInfo {
  g_is_in_a_drag_operation_ = true;
  NSDragOperation result = [self swizzledDraggingUpdated:draggingInfo];
  g_is_in_a_drag_operation_ = false;
  return result;
}

- (NSDragOperation)swizzledDraggingExited:(id <NSDraggingInfo>)draggingInfo {
  g_dragging_pasteboard_filenames_.clear();
  return [self swizzledDraggingExited:draggingInfo];
}

- (BOOL)swizzledPerformDragOperation:(id <NSDraggingInfo>)draggingInfo {
  g_is_in_a_drop_operation_ = true;
  BOOL result = [self swizzledPerformDragOperation:draggingInfo];
  g_is_in_a_drop_operation_ = false;
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

bool GetDroppedFiles(ModuleEnvironment *module_environment,
                     JsArray *files_out,
                     bool reset) {
  std::string16 ignored;
  bool result = FileDialog::FilesToJsObjectArray(
      g_dragging_pasteboard_filenames_,
      module_environment,
      files_out,
      &ignored);
  if (reset) {
    g_dragging_pasteboard_filenames_.clear();
  }
  return result;
};
