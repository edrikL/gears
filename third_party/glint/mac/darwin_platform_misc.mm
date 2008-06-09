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

// Implementation of miscellaneous parts of Platform interface for Darwin

#import <assert.h>
#import <Cocoa/Cocoa.h>

#import "glint/include/root_ui.h"
#import "glint/mac/darwin_platform.h"
#import "glint/mac/glint_work_item_dispatcher.h"

using namespace glint;

namespace glint_darwin {

DarwinPlatform::DarwinPlatform() {
  // In case there's no autorelease pool above us on the stack, alloc one here
  // This can happen during unittests
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

  all_bitmaps_ = [[NSMutableSet alloc] init];
  all_fonts_ = [[NSCountedSet alloc] init];
  all_windows_ = [[NSMutableSet alloc] init];
  work_items_for_window_ = [[NSMutableDictionary alloc] init];

  // Cache the weights for normal and bold fonts
  float system_font_size =
      [NSFont systemFontSizeForControlSize:NSRegularControlSize];
  NSFont* system_font = [NSFont systemFontOfSize:system_font_size];
  NSFont* bold_system_font = [NSFont boldSystemFontOfSize:system_font_size];

  NSFontManager* sharedManager = [NSFontManager sharedFontManager];
  normal_weight_ = [sharedManager weightOfFont:system_font];
  bold_weight_ = [sharedManager weightOfFont:bold_system_font];

  [pool release];
}

bool DarwinPlatform::GetResourceByName(const char* name,
                                       void** data,
                                       int* size) {
  if (!data || !size || !name) {
    return false;
  }

  NSString* resource_name = [NSString stringWithUTF8String:name];
  NSString* path = [[NSBundle mainBundle] pathForResource:resource_name
                                                   ofType:nil];

  NSData* content = [NSData dataWithContentsOfFile:path];

  if ([content length] == 0) {
    // File was missing or empty.
    return false;
  }

  *size = [content length];
  *data = malloc(*size);

  if (!*data) {
    return false;
  }
  [content getBytes:*data length:*size];

  return true;
}

void DarwinPlatform::RunMessageLoop() {
  [NSApp run];
}

// This will schedule the work item on the same thread, to be run on the
// thread's run loop.
bool DarwinPlatform::PostWorkItem(RootUI *ui, WorkItem *work_item) {
  GlintWorkItemDispatcher* dispatcher =
      [GlintWorkItemDispatcher dispatcherForWorkItem:work_item withUI:ui];

  if (ui != NULL) {
    // This work item is associated with a window. We keep a record of it,
    // so we can cancel it if the window goes away before the item is run
    GlintWindow* glint_window = ValidateWindow(
        reinterpret_cast<PlatformWindow*>(ui->GetPlatformWindow()));

    assert(glint_window);

    NSMutableSet* work_items =
        [work_items_for_window_ objectForKey:glint_window];
    if (!work_items) {
      work_items = [[[NSMutableSet alloc] init] autorelease];
      [work_items_for_window_ setObject:work_items forKey:glint_window];
    }

    assert(work_items && ![work_items containsObject:dispatcher]);
    [work_items addObject:dispatcher];
  }

  // Schedule the work item
  NSRunLoop *run_loop = [NSRunLoop currentRunLoop];
  [run_loop performSelector:@selector(dispatch)
                     target:dispatcher
                   argument:nil
                      order:0
                      modes:[NSArray arrayWithObject:NSDefaultRunLoopMode]];

  return true;
}

// This will actually run the work item scheduled above
void DarwinPlatform::RunWorkItem(RootUI* ui,
                                 WorkItem* work_item,
                                 GlintWorkItemDispatcher* dispatcher) {
  if (ui != NULL) {
    // Remove this work item from the list we maintain
    GlintWindow* glint_window = ValidateWindow(
        reinterpret_cast<PlatformWindow*>(ui->GetPlatformWindow()));
    NSMutableSet* work_items =
        [work_items_for_window_ objectForKey:glint_window];

    assert(work_items && [work_items containsObject:dispatcher]);
    [work_items removeObject:dispatcher];

    // Tell Glint about this workitem
    Message message;

    message.code = GL_MSG_WORK_ITEM;
    message.ui = ui;
    message.work_item = work_item;

    ui->HandleMessage(message);
  } else {
    if (work_item != NULL) {
      // ui-less workitem; we'll just run it ourselves
      work_item->Run();
      delete work_item;
    }
  }
}

bool DarwinPlatform::StartMouseCapture(RootUI *ui) {
  // Nothing to do - this is only used between mouseUp and mouseDown on Windows
  // to track dragging of the mouse, which we get for free (through
  // mouseDragged) on OSX.
  return true;
}

void DarwinPlatform::EndMouseCapture() {
  // Nothing to do
}

bool DarwinPlatform::SetCursor(Cursors cursor_type) {
  // Unhide the cursor because we might've previously hidden it
  [NSCursor unhide];

  switch (cursor_type) {
    case CURSOR_NONE:
      [NSCursor hide];

    case CURSOR_POINTER:
    case CURSOR_VERTICAL_TEXT:
    case CURSOR_CELL:
    case CURSOR_CONTEXT_MENU:
    case CURSOR_NO_DROP:
    case CURSOR_NOT_ALLOWED:
    case CURSOR_PROGRESS:
    case CURSOR_ALIAS:
    case CURSOR_ZOOMIN:
    case CURSOR_ZOOMOUT:
    case CURSOR_COPY:
    case CURSOR_MOVE:
    case CURSOR_NORTH_EAST_RESIZE:
    case CURSOR_NORTH_WEST_RESIZE:
    case CURSOR_SOUTH_EAST_RESIZE:
    case CURSOR_SOUTH_WEST_RESIZE:
    case CURSOR_NORTH_EAST_SOUTH_WEST_RESIZE:
    case CURSOR_NORTH_WEST_SOUTH_EAST_RESIZE:
    case CURSOR_WAIT:
    case CURSOR_HELP:
      // This is our default cursor for all cases that don't have a
      // system cursor on OSX.
      [[NSCursor arrowCursor] set];
      break;

    case CURSOR_CROSS:
      [[NSCursor crosshairCursor] set];
      break;

    case CURSOR_HAND:
      [[NSCursor openHandCursor] set];
      break;

    case CURSOR_IBEAM:
      [[NSCursor IBeamCursor] set];
      break;

    case CURSOR_EAST_RESIZE:
      [[NSCursor resizeLeftCursor] set];
      break;

    case CURSOR_WEST_RESIZE:
      [[NSCursor resizeRightCursor] set];
      break;

    case CURSOR_EAST_WEST_RESIZE:
    case CURSOR_COLUMN_RESIZE:
      [[NSCursor resizeLeftRightCursor] set];
      break;

    case CURSOR_NORTH_RESIZE:
      [[NSCursor resizeUpCursor] set];
      break;

    case CURSOR_SOUTH_RESIZE:
      [[NSCursor resizeDownCursor] set];
      break;

    case CURSOR_NORTH_SOUTH_RESIZE:
    case CURSOR_ROW_RESIZE:
      [[NSCursor resizeUpDownCursor] set];
      break;
  }
  return true;
}

bool DarwinPlatform::SetIcon(PlatformWindow *window, const char* icon_name) {
  // TODO(pankaj): Figure out what is appropriate here.

  return true;
}

void DarwinPlatform::TraceImplementation(const char *format, va_list args) {
  vfprintf(stderr, format, args);
}

void DarwinPlatform::CrashWithMessage(const char* format, ...) {
  // Log the message
  va_list args;
  va_start(args, format);
  TraceImplementation(format, args);
  va_end(args);
  // Crash now
  // TODO(pankaj): Need to hook this up with Breakpad.
  abort();
}

AutoRestoreGraphicsContext::AutoRestoreGraphicsContext(
     NSBitmapImageRep* ns_bitmap) {
  assert(ns_bitmap);

  // Tiger/PPC seems to have a bug; starting from an ARGB NSBitmapImageRep
  // +[NSGraphicsContext graphicsContextWithBitmapImageRep] actually returns
  // an RGBA context. We get around this by going through Quartz ourselves.

  CMProfileRef profile;
  CMGetDefaultProfileBySpace(cmSRGBData, &profile);
  CGColorSpaceRef color_space =
      CGColorSpaceCreateWithPlatformColorSpace(profile);
  CMCloseProfile(profile);

  CGBitmapInfo bitmap_info;

  // We really want non-premultiplied data, but kCGImageAlpha{First|Last} are
  // not supported with CGBitmapContextCreate. Surprisingly, the premultiplied
  // versions seem to work fine too. TODO(pankaj): Understand why premultiplied
  // contexts work, and write more unittests to potentially find situations
  // where they won't work.
#if defined(__BIG_ENDIAN__)
  bitmap_info = kCGImageAlphaPremultipliedFirst;
#else
  bitmap_info = kCGImageAlphaPremultipliedLast;
#endif

  CGContextRef cg_context(
      CGBitmapContextCreate([ns_bitmap bitmapData],
                            [ns_bitmap pixelsWide],
                            [ns_bitmap pixelsHigh],
                            [ns_bitmap bitsPerSample],
                            [ns_bitmap bytesPerRow],
                            color_space,
                            bitmap_info));
  assert(cg_context != NULL);

  NSGraphicsContext* ctx =
      [NSGraphicsContext graphicsContextWithGraphicsPort:cg_context
                                                 flipped:NO];
  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext:ctx];
  CFRelease(cg_context);
  CFRelease(color_space);
}

AutoRestoreGraphicsContext::~AutoRestoreGraphicsContext() {
  [NSGraphicsContext restoreGraphicsState];
}

}  // namespace glint_darwin

namespace glint {
Platform* Platform::Create() {
  return new glint_darwin::DarwinPlatform();
}
}  // namespace glint


