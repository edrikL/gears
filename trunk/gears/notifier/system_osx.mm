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
  // The notification API has not been finalized for official builds.
#else
#import "gears/notifier/system.h"
#import <Cocoa/Cocoa.h>
#import <string>
#import "third_party/glint/include/rectangle.h"

std::string System::GetResourcePath() {
  return [[[NSBundle mainBundle] resourcePath] fileSystemRepresentation];
}

void System::GetMainScreenBounds(glint::Rectangle *bounds) {
  assert(bounds);
  // Reset - makes rectangle 'empty'.
  bounds->Reset();

  NSArray *screens = [NSScreen screens];
  if ([screens count] == 0)
    return;
  // Get the screen with main menu.Should work even if screens is empty
  NSScreen *screen = [screens objectAtIndex:0];

  NSRect full_bounds = [screen frame];
  NSRect work_bounds = [screen visibleFrame];

  // We need to make an adjustment here. If we returned the work area as it is,
  // using visibleFrame, the caller will think the dock is on the top and the
  // menubar is at the bottom, because the caller uses top-left origin. So we
  // adjust the origin to reverse the dock and menubar constraints.
  int menubar_size = NSMaxY(full_bounds) - NSMaxY(work_bounds);
#ifdef DEBUG
  // only used in the assert below
  int dock_size = NSMinY(work_bounds) - NSMinY(full_bounds);
#endif

  work_bounds.origin.y = NSMinY(full_bounds) + menubar_size;

  // Both sizes have been switched; compare the asserts to the statements above.

  assert(NSMinY(work_bounds) - NSMinY(full_bounds) == menubar_size);
  assert(NSMaxY(full_bounds) - NSMaxY(work_bounds) == dock_size);

  bounds->set_left(static_cast<int>(NSMinX(work_bounds)));
  bounds->set_top(static_cast<int>(NSMinY(work_bounds)));
  bounds->set_right(static_cast<int>(NSMaxX(work_bounds)));
  bounds->set_bottom(static_cast<int>(NSMaxY(work_bounds)));
}

double System::GetSystemFontScaleFactor() {
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  id val = [defaults objectForKey:@"AppleDisplayScaleFactor"];
  if (val == nil) {
    return 1.0;
  }

  return [val doubleValue];
}

#endif  // OFFICIAL_BUILD


